#include "network.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "network";
static bool wifi_connected = false;
static wifi_config_t current_config = {0};
static const char *NVS_NAMESPACE = "velo_network";

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = false;
        ESP_LOGI(TAG, "WiFi disconnected");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        wifi_connected = true;
    }
}

void network_init(void) {
    ESP_LOGI(TAG, "Initializing network system");

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Load saved configuration
    network_load_wifi_config(&current_config);

    // Initialize TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Initialize WiFi
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    ESP_LOGI(TAG, "Network system initialized");
}

bool network_connect_wifi(const char* ssid, const char* password) {
    if (!ssid) return false;

    // Store in current config for later use
    memset(&current_config, 0, sizeof(current_config));
    strncpy(current_config.ssid, ssid, sizeof(current_config.ssid) - 1);
    if (password) {
        strncpy(current_config.password, password, sizeof(current_config.password) - 1);
    }

    // Set up ESP-IDF wifi config
    wifi_config_t esp_wifi_config = {0};
    strncpy((char*)esp_wifi_config.sta.ssid, ssid, sizeof(esp_wifi_config.sta.ssid) - 1);
    if (password) {
        strncpy((char*)esp_wifi_config.sta.password, password, sizeof(esp_wifi_config.sta.password) - 1);
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &esp_wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Connecting to WiFi: %s", ssid);

    // Wait for connection
    for (int i = 0; i < 20; i++) {
        if (wifi_connected) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    ESP_LOGW(TAG, "WiFi connection timed out");
    return false;
}

void network_disconnect_wifi(void) {
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    ESP_ERROR_CHECK(esp_wifi_stop());
    wifi_connected = false;
    ESP_LOGI(TAG, "Disconnected from WiFi");
}

bool network_is_connected(void) {
    return wifi_connected;
}

bool network_upload_telemetry(const char* ride_id, const char* telemetry_data, size_t data_length) {
    if (!wifi_connected || !ride_id || !telemetry_data || data_length == 0) {
        return false;
    }

    char url[256];
    snprintf(url, sizeof(url), "http://%s:%u/write?db=%s",
             current_config.influxdb_url,
             current_config.influxdb_port,
             current_config.influxdb_db);

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_POST,
        .auth_type = current_config.influxdb_user[0] ? HTTP_AUTH_TYPE_BASIC : HTTP_AUTH_TYPE_NONE,
        .username = current_config.influxdb_user,
        .password = current_config.influxdb_password,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return false;
    }

    esp_http_client_set_post_field(client, telemetry_data, data_length);
    esp_http_client_set_header(client, "Content-Type", "text/plain");

    esp_err_t err = esp_http_client_perform(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return false;
    }

    int status_code = esp_http_client_get_status_code(client);
    if (status_code != 204) { // InfluxDB returns 204 on success
        ESP_LOGW(TAG, "Unexpected HTTP status: %d", status_code);
    } else {
        ESP_LOGI(TAG, "Successfully uploaded telemetry for ride: %s", ride_id);
    }

    esp_http_client_cleanup(client);
    return (status_code == 204);
}

bool network_upload_ride_file(const char* ride_id) {
    // This would read the telemetry.lp file from SD card and upload it
    // Implementation would be similar to network_upload_telemetry but reading from file
    ESP_LOGI(TAG, "Ride file upload for %s not yet implemented", ride_id);
    return false;
}

void network_configure_influxdb(const char* url, uint16_t port,
                                const char* db, const char* user, const char* password) {
    if (url) {
        strncpy(current_config.influxdb_url, url, sizeof(current_config.influxdb_url) - 1);
    }
    if (port > 0) {
        current_config.influxdb_port = port;
    }
    if (db) {
        strncpy(current_config.influxdb_db, db, sizeof(current_config.influxdb_db) - 1);
    }
    if (user) {
        strncpy(current_config.influxdb_user, user, sizeof(current_config.influxdb_user) - 1);
    }
    if (password) {
        strncpy(current_config.influxdb_password, password, sizeof(current_config.influxdb_password) - 1);
    }

    // Save configuration
    network_save_wifi_config(&current_config);
}

bool network_save_wifi_config(const wifi_config_t* config) {
    if (!config) return false;

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS namespace: %s", esp_err_to_name(err));
        return false;
    }

    // Save WiFi credentials
    err = nvs_set_str(nvs_handle, "wifi_ssid", (const char*)config->ssid);
    if (err != ESP_OK) goto cleanup;

    err = nvs_set_str(nvs_handle, "wifi_pass", (const char*)config->password);
    if (err != ESP_OK) goto cleanup;

    // Save InfluxDB configuration
    err = nvs_set_str(nvs_handle, "influx_url", config->influxdb_url);
    if (err != ESP_OK) goto cleanup;

    err = nvs_set_u16(nvs_handle, "influx_port", config->influxdb_port);
    if (err != ESP_OK) goto cleanup;

    err = nvs_set_str(nvs_handle, "influx_db", config->influxdb_db);
    if (err != ESP_OK) goto cleanup;

    err = nvs_set_str(nvs_handle, "influx_user", config->influxdb_user);
    if (err != ESP_OK) goto cleanup;

    err = nvs_set_str(nvs_handle, "influx_pass", config->influxdb_password);
    if (err != ESP_OK) goto cleanup;

    // Commit changes
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit configuration: %s", esp_err_to_name(err));
        goto cleanup;
    }

    ESP_LOGI(TAG, "Network configuration saved");
    nvs_close(nvs_handle);
    return true;

cleanup:
    nvs_close(nvs_handle);
    ESP_LOGE(TAG, "Failed to save network configuration: %s", esp_err_to_name(err));
    return false;
}

bool network_load_wifi_config(wifi_config_t* config) {
    if (!config) return false;

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to open NVS namespace, using defaults: %s", esp_err_to_name(err));
        // Set defaults
        strncpy(config->ssid, "", sizeof(config->ssid));
        strncpy(config->password, "", sizeof(config->password));
        strncpy(config->influxdb_url, "localhost", sizeof(config->influxdb_url));
        config->influxdb_port = 8086;
        strncpy(config->influxdb_db, "bike_data", sizeof(config->influxdb_db));
        strncpy(config->influxdb_user, "", sizeof(config->influxdb_user));
        strncpy(config->influxdb_password, "", sizeof(config->influxdb_password));
        return false;
    }

    size_t size = sizeof(config->ssid);
    err = nvs_get_str(nvs_handle, "wifi_ssid", config->ssid, &size);
    if (err != ESP_OK) {
        config->ssid[0] = '\0';
    }

    size = sizeof(config->password);
    err = nvs_get_str(nvs_handle, "wifi_pass", config->password, &size);
    if (err != ESP_OK) {
        config->password[0] = '\0';
    }

    size = sizeof(config->influxdb_url);
    err = nvs_get_str(nvs_handle, "influx_url", config->influxdb_url, &size);
    if (err != ESP_OK) {
        strncpy(config->influxdb_url, "localhost", sizeof(config->influxdb_url));
    }

    uint16_t port;
    err = nvs_get_u16(nvs_handle, "influx_port", &port);
    if (err != ESP_OK) {
        config->influxdb_port = 8086;
    } else {
        config->influxdb_port = port;
    }

    size = sizeof(config->influxdb_db);
    err = nvs_get_str(nvs_handle, "influx_db", config->influxdb_db, &size);
    if (err != ESP_OK) {
        strncpy(config->influxdb_db, "bike_data", sizeof(config->influxdb_db));
    }

    size = sizeof(config->influxdb_user);
    err = nvs_get_str(nvs_handle, "influx_user", config->influxdb_user, &size);
    if (err != ESP_OK) {
        config->influxdb_user[0] = '\0';
    }

    size = sizeof(config->influxdb_password);
    err = nvs_get_str(nvs_handle, "influx_pass", config->influxdb_password, &size);
    if (err != ESP_OK) {
        config->influxdb_password[0] = '\0';
    }

    nvs_close(nvs_handle);
    ESP_LOGI(TAG, "Network configuration loaded");
    return true;
}
