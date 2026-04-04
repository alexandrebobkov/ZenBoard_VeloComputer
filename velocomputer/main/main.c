#include "telemetry.h"
#include "config.h"
#include "gps.h"
#include "sensors.h"
#include "storage.h"
#include "network.h"

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
