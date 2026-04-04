#include "config.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_mac.h"      /* esp_read_mac() – not re-exported by esp_system.h in v5.x */
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdio.h>

static const char *TAG           = "config";
static const char *NVS_NAMESPACE = "velo_config";

/* ------------------------------------------------------------------ */
void config_init(void)
{
    ESP_LOGI(TAG, "Initializing configuration system");

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
}

/* ------------------------------------------------------------------ */
bool config_load(system_config_t *config)
{
    if (!config) return false;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open failed (%s) – using defaults", esp_err_to_name(err));
        config_get_defaults(config);
        return false;
    }

    size_t sz = sizeof(config->device_id);
    err = nvs_get_str(handle, "device_id", config->device_id, &sz);
    if (err != ESP_OK) {
        config_get_defaults(config);
        nvs_close(handle);
        return false;
    }

    sz = sizeof(config->default_bicycle);
    nvs_get_str(handle, "bike",  config->default_bicycle, &sz);

    sz = sizeof(config->default_rider);
    nvs_get_str(handle, "rider", config->default_rider,   &sz);

    uint8_t v = 0;
    nvs_get_u8(handle, "temp_en",  &v); config->enable_temperature = (bool)v;
    nvs_get_u8(handle, "humid_en", &v); config->enable_humidity    = (bool)v;
    nvs_get_u8(handle, "alt_en",   &v); config->enable_altitude    = (bool)v;
    nvs_get_u8(handle, "hr_en",    &v); config->enable_heart_rate  = (bool)v;
    nvs_get_u8(handle, "power_en", &v); config->enable_power       = (bool)v;

    nvs_close(handle);
    ESP_LOGI(TAG, "Configuration loaded: device=%s bike=%s rider=%s",
             config->device_id, config->default_bicycle, config->default_rider);
    return true;
}

/* ------------------------------------------------------------------ */
bool config_save(const system_config_t *config)
{
    if (!config) return false;

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open failed: %s", esp_err_to_name(err));
        return false;
    }

#define CHK(x) do { err = (x); if (err != ESP_OK) goto cleanup; } while (0)

    CHK(nvs_set_str(handle, "device_id", config->device_id));
    CHK(nvs_set_str(handle, "bike",      config->default_bicycle));
    CHK(nvs_set_str(handle, "rider",     config->default_rider));
    CHK(nvs_set_u8 (handle, "temp_en",   config->enable_temperature));
    CHK(nvs_set_u8 (handle, "humid_en",  config->enable_humidity));
    CHK(nvs_set_u8 (handle, "alt_en",    config->enable_altitude));
    CHK(nvs_set_u8 (handle, "hr_en",     config->enable_heart_rate));
    CHK(nvs_set_u8 (handle, "power_en",  config->enable_power));
    CHK(nvs_commit(handle));

#undef CHK

    ESP_LOGI(TAG, "Configuration saved");
    nvs_close(handle);
    return true;

cleanup:
    nvs_close(handle);
    ESP_LOGE(TAG, "config_save failed: %s", esp_err_to_name(err));
    return false;
}

/* ------------------------------------------------------------------ */
void config_get_defaults(system_config_t *config)
{
    if (!config) return;

    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(config->device_id, sizeof(config->device_id),
             "esp32c3-%02x%02x%02x", mac[3], mac[4], mac[5]);

    strncpy(config->default_bicycle, "MyBike", sizeof(config->default_bicycle) - 1);
    config->default_bicycle[sizeof(config->default_bicycle) - 1] = '\0';

    strncpy(config->default_rider, "Rider1", sizeof(config->default_rider) - 1);
    config->default_rider[sizeof(config->default_rider) - 1] = '\0';

    config->enable_temperature = false;
    config->enable_humidity    = false;
    config->enable_altitude    = false;
    config->enable_heart_rate  = false;
    config->enable_power       = false;

    ESP_LOGI(TAG, "Using default configuration (device=%s)", config->device_id);
}
