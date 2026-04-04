#include "gps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>   /* memcpy */
#include <sys/time.h>

static const char *TAG = "gps";

/*
 * Stub implementation.
 * Replace the body of gps_start() with real UART / NMEA parsing
 * for your GPS module (UBlox, MTK, Quectel, …).
 */

static gps_data_t s_current = {0};
static bool       s_active  = false;

/* ------------------------------------------------------------------ */
void gps_init(void)
{
    ESP_LOGI(TAG, "Initialising GPS (stub)");
    memset(&s_current, 0, sizeof(s_current));
    s_current.valid = false;
    /*
     * TODO: configure UART, set baud rate, send module init commands.
     * Example for UBlox on UART1:
     *   uart_config_t uart_cfg = { .baud_rate = 9600, ... };
     *   uart_param_config(UART_NUM_1, &uart_cfg);
     *   uart_set_pin(UART_NUM_1, TX_PIN, RX_PIN, -1, -1);
     *   uart_driver_install(UART_NUM_1, 1024, 0, 0, NULL, 0);
     */
}

/* ------------------------------------------------------------------ */
bool gps_get_data(gps_data_t *data)
{
    if (!data || !s_active) return false;

    /* Attach a fresh nanosecond timestamp to whichever fix we have */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    s_current.timestamp = (uint64_t)tv.tv_sec * 1000000000ULL
                        + (uint64_t)tv.tv_usec * 1000ULL;

    memcpy(data, &s_current, sizeof(gps_data_t));
    return s_current.valid;
}

/* ------------------------------------------------------------------ */
void gps_start(void)
{
    if (s_active) return;

    ESP_LOGI(TAG, "Starting GPS acquisition");
    s_active = true;

    /*
     * TODO: replace the simulated fix below with a real NMEA read loop.
     *
     * Stub: pretend we get a fix after 5 s (Ottawa, ON).
     */
    vTaskDelay(pdMS_TO_TICKS(5000));

    s_current.latitude   = 45.4215;
    s_current.longitude  = -75.6972;
    s_current.altitude   = 70.0f;
    s_current.speed      = 0.0f;
    s_current.heading    = 0.0f;
    s_current.satellites = 8;
    s_current.valid      = true;

    ESP_LOGI(TAG, "GPS fix acquired (stub)");
}

/* ------------------------------------------------------------------ */
void gps_stop(void)
{
    if (!s_active) return;

    ESP_LOGI(TAG, "Stopping GPS");
    s_active        = false;
    s_current.valid = false;
    /* TODO: send sleep command to GPS module */
}
