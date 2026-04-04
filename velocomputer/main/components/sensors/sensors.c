#include "sensors.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_driver_pcnt.h"
#include <sys/time.h>
#include <string.h>

static const char *TAG = "sensors";

// Configuration
static float wheel_circumference = 2.105; // Default: 700x23c road tire (~2.1m)
static uint8_t cadence_magnets = 1;      // Default: 1 magnet
static bool temp_enabled = false;
static bool humid_enabled = false;
static bool hr_enabled = false;
static bool power_enabled = false;

// Simulated sensor data
static sensor_data_t current_data = {0};

// Pulse counter handles for speed and cadence
static pcnt_unit_handle_t speed_unit = NULL;
static pcnt_unit_handle_t cadence_unit = NULL;
static uint64_t last_speed_time = 0;
static uint64_t last_cadence_time = 0;

static bool IRAM_ATTR speed_isr_handler(pcnt_unit_handle_t unit, const pcnt_watch_event_t *event, void *user_data) {
    // Handle speed sensor interrupt
    if (event->watch_point_id == 0) {
        // Threshold event - we have a complete revolution
        struct timeval tv;
        gettimeofday(&tv, NULL);
        uint64_t now = (uint64_t)tv.tv_sec * 1000000000 + tv.tv_usec * 1000;

        if (last_speed_time > 0) {
            // Calculate speed in km/h
            float time_diff_s = (now - last_speed_time) / 1000000000.0;
            if (time_diff_s > 0) {
                current_data.speed = (wheel_circumference / 1000.0) / time_diff_s * 3.6; // km/h
            }
        }

        last_speed_time = now;
        current_data.timestamp = now;
    }

    return false; // Don't wake higher priority task
}

static bool IRAM_ATTR cadence_isr_handler(pcnt_unit_handle_t unit, const pcnt_watch_event_t *event, void *user_data) {
    // Handle cadence sensor interrupt
    if (event->watch_point_id == 0) {
        // Threshold event - we have cadence_magnets pulses
        struct timeval tv;
        gettimeofday(&tv, NULL);
        uint64_t now = (uint64_t)tv.tv_sec * 1000000000 + tv.tv_usec * 1000;

        static uint64_t last_cadence_revolution = 0;
        static uint16_t revolution_count = 0;

        revolution_count++;

        if (last_cadence_revolution > 0 && revolution_count >= cadence_magnets) {
            float time_diff_min = (now - last_cadence_revolution) / 60000000000.0;
            if (time_diff_min > 0) {
                current_data.cadence = (float)revolution_count / time_diff_min;
            }
            revolution_count = 0;
            last_cadence_revolution = now;
        } else if (last_cadence_revolution == 0) {
            last_cadence_revolution = now;
        }

        current_data.timestamp = now;
    }

    return false; // Don't wake higher priority task
}

void sensors_init(void) {
    ESP_LOGI(TAG, "Initializing sensor system");

    // Create speed unit
    pcnt_unit_config_t speed_unit_config = {
        .low_lim = -1000,
        .high_lim = 1000,
        .accum_count_thres = 1,
    };
    pcnt_new_unit(&speed_unit_config, &speed_unit);

    // Create speed channel
    pcnt_chan_config_t speed_chan_config = {
        .edge_gpio_num = GPIO_NUM_4,  // Speed sensor GPIO
        .level_gpio_num = PCNT_GPIO_NOT_USED,
    };
    pcnt_chan_handle_t speed_chan = NULL;
    pcnt_new_channel(speed_unit, &speed_chan_config, &speed_chan);

    // Set speed channel events
    pcnt_chan_set_edge_action(speed_chan, PCNT_CHANNEL_EDGE_ACTION_INC, PCNT_CHANNEL_EDGE_ACTION_DIS);
    pcnt_chan_set_level_action(speed_chan, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_KEEP);

    // Add watch point for speed
    pcnt_unit_add_watch_point(speed_unit, 1, 0);

    // Register speed ISR
    pcnt_unit_register_event_callback(speed_unit, speed_isr_handler, NULL);

    // Create cadence unit
    pcnt_unit_config_t cadence_unit_config = {
        .low_lim = -1000,
        .high_lim = 1000,
        .accum_count_thres = 1,
    };
    pcnt_new_unit(&cadence_unit_config, &cadence_unit);

    // Create cadence channel
    pcnt_chan_config_t cadence_chan_config = {
        .edge_gpio_num = GPIO_NUM_5,  // Cadence sensor GPIO
        .level_gpio_num = PCNT_GPIO_NOT_USED,
    };
    pcnt_chan_handle_t cadence_chan = NULL;
    pcnt_new_channel(cadence_unit, &cadence_chan_config, &cadence_chan);

    // Set cadence channel events
    pcnt_chan_set_edge_action(cadence_chan, PCNT_CHANNEL_EDGE_ACTION_INC, PCNT_CHANNEL_EDGE_ACTION_DIS);
    pcnt_chan_set_level_action(cadence_chan, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_KEEP);

    // Add watch point for cadence
    pcnt_unit_add_watch_point(cadence_unit, 1, 0);

    // Register cadence ISR
    pcnt_unit_register_event_callback(cadence_unit, cadence_isr_handler, NULL);

    // Enable both units
    pcnt_unit_enable(speed_unit);
    pcnt_unit_start(speed_unit);
    pcnt_unit_enable(cadence_unit);
    pcnt_unit_start(cadence_unit);

    ESP_LOGI(TAG, "Speed and cadence sensors initialized");
    ESP_LOGI(TAG, "Wheel circumference: %.3fm, Cadence magnets: %u",
             wheel_circumference, cadence_magnets);
}

bool sensors_get_data(sensor_data_t* data) {
    if (!data) return false;

    // Get current sensor readings
    memcpy(data, &current_data, sizeof(sensor_data_t));

    // Simulate some optional sensor data if enabled
    if (temp_enabled) {
        data->optional.temperature = 22.5; // Simulated temperature
    }

    if (humid_enabled) {
        data->optional.humidity = 45.0; // Simulated humidity
    }

    if (hr_enabled) {
        data->optional.heart_rate = 120; // Simulated heart rate
    }

    if (power_enabled) {
        data->optional.power = data->speed * 3.0; // Rough power estimate
    }

    return true;
}

void sensors_set_wheel_circumference(float circumference) {
    if (circumference > 0.1 && circumference < 5.0) {
        wheel_circumference = circumference;
        ESP_LOGI(TAG, "Wheel circumference set to %.3fm", circumference);
    }
}

void sensors_set_cadence_magnets(uint8_t magnets) {
    if (magnets >= 1 && magnets <= 8) {
        cadence_magnets = magnets;
        ESP_LOGI(TAG, "Cadence magnets set to %u", magnets);
    }
}

void sensors_enable_temperature(bool enable) {
    temp_enabled = enable;
    ESP_LOGI(TAG, "Temperature sensor %s", enable ? "enabled" : "disabled");
}

void sensors_enable_humidity(bool enable) {
    humid_enabled = enable;
    ESP_LOGI(TAG, "Humidity sensor %s", enable ? "enabled" : "disabled");
}

void sensors_enable_heart_rate(bool enable) {
    hr_enabled = enable;
    ESP_LOGI(TAG, "Heart rate sensor %s", enable ? "enabled" : "disabled");
}

void sensors_enable_power(bool enable) {
    power_enabled = enable;
    ESP_LOGI(TAG, "Power sensor %s", enable ? "enabled" : "disabled");
}
