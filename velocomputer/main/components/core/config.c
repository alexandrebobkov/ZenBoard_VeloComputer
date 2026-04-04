#include "config.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "config";
static const char *NVS_NAMESPACE = "velo_config";

void config_init(void) {
    ESP_LOGI(TAG, "Initializing configuration system");

    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

bool config_load(system_config_t* config) {
    if (!config) return false;

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to open NVS namespace, using defaults: %s", esp_err_to_name(err));
        config_get_defaults(config);
        return false;
    }

    // Read configuration values
    size_t size = sizeof(config->device_id);
    err = nvs_get_str(nvs_handle, "device_id", config->device_id, &size);
    if (err != ESP_OK) {
        config_get_defaults(config);
        nvs_close(nvs_handle);
        return false;
    }

    size = sizeof(config->default_bicycle);
    nvs_get_str(nvs_handle, "bike", config->default_bicycle, &size);

    size = sizeof(config->default_rider);
    nvs_get_str(nvs_handle, "rider", config->default_rider, &size);

    // Read boolean flags
    nvs_get_u8(nvs_handle, "temp_en", (uint8_t*)&config->enable_temperature);
    nvs_get_u8(nvs_handle, "humid_en", (uint8_t*)&config->enable_humidity);
    nvs_get_u8(nvs_handle, "alt_en", (uint8_t*)&config->enable_altitude);
    nvs_get_u8(nvs_handle, "hr_en", (uint8_t*)&config->enable_heart_rate);
    nvs_get_u8(nvs_handle, "power_en", (uint8_t*)&config->enable_power);

    nvs_close(nvs_handle);

    ESP_LOGI(TAG, "Configuration loaded successfully");
    return true;
}

bool config_save(const system_config_t* config) {
    if (!config) return false;

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        return false;
    }

    // Write configuration values
    err = nvs_set_str(nvs_handle, "device_id", config->device_id);
    if (err != ESP_OK) goto cleanup;

    err = nvs_set_str(nvs_handle, "bike", config->default_bicycle);
    if (err != ESP_OK) goto cleanup;

    err = nvs_set_str(nvs_handle, "rider", config->default_rider);
    if (err != ESP_OK) goto cleanup;

    // Write boolean flags
    err = nvs_set_u8(nvs_handle, "temp_en", config->enable_temperature);
    if (err != ESP_OK) goto cleanup;

    err = nvs_set_u8(nvs_handle, "humid_en", config->enable_humidity);
    if (err != ESP_OK) goto cleanup;

    err = nvs_set_u8(nvs_handle, "alt_en", config->enable_altitude);
    if (err != ESP_OK) goto cleanup;

    err = nvs_set_u8(nvs_handle, "hr_en", config->enable_heart_rate);
    if (err != ESP_OK) goto cleanup;

    err = nvs_set_u8(nvs_handle, "power_en", config->enable_power);
    if (err != ESP_OK) goto cleanup;

    // Commit changes
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit configuration: %s", esp_err_to_name(err));
        goto cleanup;
    }

    ESP_LOGI(TAG, "Configuration saved successfully");
    nvs_close(nvs_handle);
    return true;

cleanup:
    nvs_close(nvs_handle);
    ESP_LOGE(TAG, "Failed to save configuration: %s", esp_err_to_name(err));
    return false;
}

void config_get_defaults(system_config_t* config) {
    if (!config) return;

    // Generate a default device ID based on MAC address
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(config->device_id, sizeof(config->device_id),
             "esp32-c3-%02x%02x%02x", mac[3], mac[4], mac[5]);

    strncpy(config->default_bicycle, "MyBike", sizeof(config->default_bicycle));
    strncpy(config->default_rider, "Rider1", sizeof(config->default_rider));

    // Enable basic sensors by default
    config->enable_temperature = false;
    config->enable_humidity = false;
    config->enable_altitude = false;
    config->enable_heart_rate = false;
    config->enable_power = false;

    ESP_LOGI(TAG, "Using default configuration");
}
