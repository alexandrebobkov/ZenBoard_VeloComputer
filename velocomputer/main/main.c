#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

// Component headers
#include "components/core/telemetry.h"
#include "components/core/config.h"
#include "components/gps/gps.h"
#include "components/sensors/sensors.h"
#include "components/storage/storage.h"
#include "components/network/network.h"

static const char *TAG = "velocomputer";

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Velocomputer...");

    // Initialize configuration
    config_init();

    // Initialize components
    telemetry_init();
    gps_init();
    sensors_init();
    storage_init();
    network_init();

    ESP_LOGI(TAG, "All components initialized");

    // Main application loop would go here
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
