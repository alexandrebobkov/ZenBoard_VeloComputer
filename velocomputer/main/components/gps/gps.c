#include "gps.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "gps";

// This is a stub implementation
// In a real implementation, you would interface with a GPS module
// like UBlox, MTK, etc. over UART

static gps_data_t current_gps_data = {0};
static bool gps_active = false;

void gps_init(void) {
    ESP_LOGI(TAG, "Initializing GPS module (stub)");
    // Initialize UART, configure GPS module, etc.
    // For now, we'll just set some dummy data
    current_gps_data.latitude = 0.0;
    current_gps_data.longitude = 0.0;
    current_gps_data.valid = false;
}

bool gps_get_data(gps_data_t* data) {
    if (!data || !gps_active) return false;

    // In real implementation, read from GPS module
    // For stub, return current data
    memcpy(data, &current_gps_data, sizeof(gps_data_t));
    return data->valid;
}

void gps_start(void) {
    if (gps_active) return;

    ESP_LOGI(TAG, "Starting GPS acquisition");
    gps_active = true;

    // In real implementation, start GPS module
    // For stub, simulate getting a fix after 5 seconds
    vTaskDelay(pdMS_TO_TICKS(5000));
    current_gps_data.latitude = 40.7128;  // New York
    current_gps_data.longitude = -74.0060;
    current_gps_data.altitude = 10.0;
    current_gps_data.satellites = 8;
    current_gps_data.valid = true;
    current_gps_data.speed = 0.0;
    current_gps_data.heading = 0.0;
}

void gps_stop(void) {
    if (!gps_active) return;

    ESP_LOGI(TAG, "Stopping GPS module");
    gps_active = false;
    // In real implementation, put GPS to sleep
}
