#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

/* Flat includes – all headers live in the same directory as main.c */
#include "config.h"
#include "telemetry.h"
#include "gps.h"
#include "sensors.h"
#include "storage.h"
#include "network.h"
#include "gpx.h"

static const char *TAG = "main";

/* ------------------------------------------------------------------ */
/*  Ride task                                                          */
/*  Collects sensor + GPS data every second, writes to SD card, and   */
/*  uploads to InfluxDB whenever WiFi is available.                   */
/* ------------------------------------------------------------------ */
static void ride_task(void *arg)
{
    system_config_t cfg;
    config_load(&cfg);

    /* --- Optional sensors from config --- */
    sensors_enable_temperature(cfg.enable_temperature);
    sensors_enable_humidity   (cfg.enable_humidity);
    sensors_enable_heart_rate (cfg.enable_heart_rate);
    sensors_enable_power      (cfg.enable_power);

    /* --- Start GPS, wait for fix --- */
    ESP_LOGI(TAG, "Waiting for GPS fix…");
    gps_start();

    /* --- Initialise telemetry metadata (generates a new ride_id) --- */
    telemetry_set_metadata(cfg.default_bicycle, cfg.default_rider,
                           "cycling", NULL);

    /* Use a throw-away point just to read the generated ride_id */
    bike_telemetry_t *tp = telemetry_create_point();
    char ride_id[RIDE_ID_LENGTH];
    strncpy(ride_id, tp->ride_id, sizeof(ride_id));

    /* --- Create SD card structures --- */
    bool sd_ok = false;
    if (storage_is_available()) {
        sd_ok = storage_create_ride_directory(ride_id);
        if (sd_ok) {
            storage_write_metadata(ride_id,
                                   cfg.default_bicycle,
                                   cfg.default_rider,
                                   "cycling",
                                   tp->timestamp);
        }
    } else {
        ESP_LOGW(TAG, "SD not available – telemetry will not be saved locally");
    }

    /* --- Open GPX file --- */
    bool gpx_ok = false;
    if (sd_ok) {
        char gpx_path[96];
        snprintf(gpx_path, sizeof(gpx_path),
                 "/sdcard/rides/%s/track.gpx", ride_id);
        gpx_ok = gpx_start_file(gpx_path, ride_id,
                                  cfg.default_bicycle, cfg.default_rider);
    }

    /* --- Ride statistics --- */
    uint32_t seconds       = 0;
    float    distance_km   = 0.0f;
    float    max_speed     = 0.0f;
    float    speed_sum     = 0.0f;

    uint64_t start_time_ns = tp->timestamp;

    ESP_LOGI(TAG, "Ride started – ride_id=%s", ride_id);

    /* ----------------------------------------------------------------
     * Main 1-second telemetry loop.
     * In production, tie the loop exit to a "stop ride" GPIO button.
     * ---------------------------------------------------------------- */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        seconds++;

        /* --- Collect sensor readings --- */
        sensor_data_t sens = {0};
        sensors_get_data(&sens);

        gps_data_t gps = {0};
        gps_get_data(&gps);

        /* --- Build telemetry point --- */
        tp = telemetry_create_point();
        tp->speed          = sens.speed;
        tp->cadence        = sens.cadence;
        tp->latitude       = gps.latitude;
        tp->longitude      = gps.longitude;
        tp->gps_quality    = gps.satellites;
        tp->ride_duration  = seconds;
        tp->battery_level  = 85;  /* TODO: read ADC */

        tp->optional.temperature = sens.optional.temperature;
        tp->optional.humidity    = sens.optional.humidity;
        tp->optional.heart_rate  = sens.optional.heart_rate;
        tp->optional.power       = sens.optional.power;
        tp->optional.altitude    = gps.altitude;

        /* --- Update ride statistics --- */
        if (sens.speed > max_speed)  max_speed = sens.speed;
        speed_sum  += sens.speed;
        /* distance += speed(km/h) × Δt(h) = speed / 3600 per second */
        distance_km += sens.speed / 3600.0f;

        /* --- Save to SD card --- */
        if (sd_ok) {
            storage_write_telemetry(tp);
            if (gpx_ok) gpx_add_point(tp);
        }

        /* --- Upload live to InfluxDB when WiFi is up --- */
        if (network_is_connected()) {
            char line[320];
            int  n = telemetry_to_line_protocol(tp, line, sizeof(line));
            if (n > 0) {
                network_upload_telemetry(ride_id, line, (size_t)n);
            }
        }

        /* Log a summary every 30 s */
        if (seconds % 30 == 0) {
            float avg = (seconds > 0) ? speed_sum / (float)seconds : 0.0f;
            //ESP_LOGI(TAG, "[%u s] speed=%.1f km/h dist=%.2f km avg=%.1f km/h gps=%s",
            //         seconds, (double)sens.speed, (double)distance_km,
            //         (double)avg, gps.valid ? "OK" : "searching");
        }
    }

    /* --- Finalise ride (unreachable in current loop, add button logic) --- */
    if (gpx_ok)  gpx_finalize_file();

    uint64_t end_ns = tp ? tp->timestamp : start_time_ns;
    float avg_speed = (seconds > 0) ? speed_sum / (float)seconds : 0.0f;

    if (sd_ok) {
        storage_finalize_ride(ride_id, end_ns, distance_km, max_speed, avg_speed);
    }

    /* Upload the complete ride file if WiFi is available */
    if (network_is_connected() && sd_ok) {
        ESP_LOGI(TAG, "Uploading ride file…");
        network_upload_ride_file(ride_id);
        network_upload_gpx_file(ride_id);
    }

    vTaskDelete(NULL);
}

/* ------------------------------------------------------------------ */
/*  WiFi reconnect task                                                */
/*  Tries to connect to the saved SSID periodically; once connected,  */
/*  any pending ride files are picked up by network_upload_ride_file.  */
/* ------------------------------------------------------------------ */
static void wifi_task(void *arg)
{
    velo_wifi_config_t net_cfg;
    network_load_config(&net_cfg);

    while (1) {
        if (!network_is_connected() && net_cfg.ssid[0] != '\0') {
            ESP_LOGI(TAG, "Attempting WiFi connection to \"%s\"", net_cfg.ssid);
            bool ok = network_connect_wifi(net_cfg.ssid, net_cfg.password);

            if (ok) {
                ESP_LOGI(TAG, "WiFi connected – checking for pending uploads");

                /* Upload any completed rides that are still on the SD card */
                int count = storage_get_ride_count();
                for (int i = 0; i < count; i++) {
                    char rid[RIDE_ID_LENGTH];
                    if (storage_get_ride_id(i, rid, sizeof(rid))) {
                        ESP_LOGI(TAG, "Uploading pending ride: %s", rid);
                        network_upload_ride_file(rid);
                        network_upload_gpx_file(rid);
                    }
                }
            }
        }

        /* Check every 60 s */
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

/* ------------------------------------------------------------------ */
void app_main(void)
{
    ESP_LOGI(TAG, "=== Velocomputer starting ===");

    /* Core config + NVS */
    config_init();

    system_config_t cfg;
    config_load(&cfg);
    ESP_LOGI(TAG, "Device: %s  Bike: %s  Rider: %s",
             cfg.device_id, cfg.default_bicycle, cfg.default_rider);

    /* Hardware */
    telemetry_init();
    sensors_init();
    storage_init();

    /* Network (starts WiFi driver, does not block waiting for connection) */
    network_init();

    ESP_LOGI(TAG, "All components initialised");

    /* Background WiFi / upload task */
    xTaskCreate(wifi_task, "wifi_task",  4096, NULL, 4, NULL);

    /* Foreground ride-data task */
    xTaskCreate(ride_task, "ride_task", 8192, NULL, 5, NULL);
}
