#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#include "config.h"
#include "sensors.h"
#include "display.h"

static const char *TAG = "velocomputer";

void app_main(void)
{
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "  Velocomputer — sensor debug mode");
    ESP_LOGI(TAG, "========================================");

    // Only bring up what we need for sensor debugging.
    // GPS, storage, and network are skipped for now to isolate the crash.
    config_init();
    sensors_init();

    ESP_LOGI(TAG, "Setup complete — watching speed and cadence sensors.");
    ESP_LOGI(TAG, "Briefly short GPIO4→GND for speed, GPIO5→GND for cadence.");

    // Nothing else needed here — sensors_init() already starts the
    // sensor_debug_task that prints speed/cadence every second.
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}