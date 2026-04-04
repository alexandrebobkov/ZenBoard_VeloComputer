#include "network.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG           = "network";
static const char *NVS_NAMESPACE = "velo_net";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_MAX_RETRY     5

static EventGroupHandle_t   s_wifi_events = NULL;
static bool                 s_connected   = false;
static int                  s_retries     = 0;
static velo_wifi_config_t   s_cfg         = {0};

/* ------------------------------------------------------------------ */
static void wifi_event_handler(void *arg, esp_event_base_t base,
                               int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();

    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_connected = false;
        if (s_retries < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retries++;
            ESP_LOGI(TAG, "WiFi retry %d/%d", s_retries, WIFI_MAX_RETRY);
        } else {
            xEventGroupSetBits(s_wifi_events, WIFI_FAIL_BIT);
            ESP_LOGW(TAG, "WiFi connection failed after %d attempts", WIFI_MAX_RETRY);
        }

    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&ev->ip_info.ip));
        s_retries   = 0;
        s_connected = true;
        xEventGroupSetBits(s_wifi_events, WIFI_CONNECTED_BIT);
    }
}

/* ------------------------------------------------------------------ */
void network_init(void)
{
    ESP_LOGI(TAG, "Initialising network");

    /* Load saved settings (fills defaults if not found) */
    network_load_config(&s_cfg);

    /* TCP/IP stack */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    /* WiFi driver */
    wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init_cfg));

    /* Event handlers */
    s_wifi_events = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Network initialised");
}

/* ------------------------------------------------------------------ */
bool network_connect_wifi(const char *ssid, const char *password)
{
    if (!ssid || ssid[0] == '\0') return false;

    /* Update in-memory config */
    strncpy(s_cfg.ssid,     ssid,     sizeof(s_cfg.ssid)     - 1);
    strncpy(s_cfg.password, password ? password : "",
            sizeof(s_cfg.password) - 1);

    wifi_config_t esp_cfg = {0};
    strncpy((char *)esp_cfg.sta.ssid,     ssid,
            sizeof(esp_cfg.sta.ssid)     - 1);
    strncpy((char *)esp_cfg.sta.password, password ? password : "",
            sizeof(esp_cfg.sta.password) - 1);
    esp_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    s_retries = 0;
    xEventGroupClearBits(s_wifi_events, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &esp_cfg));
    esp_wifi_connect();

    ESP_LOGI(TAG, "Connecting to \"%s\"…", ssid);

    /* Block until connected or permanently failed */
    EventBits_t bits = xEventGroupWaitBits(
        s_wifi_events,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE, pdFALSE,
        pdMS_TO_TICKS(15000));

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to \"%s\"", ssid);
        return true;
    }

    ESP_LOGW(TAG, "Failed to connect to \"%s\"", ssid);
    return false;
}

/* ------------------------------------------------------------------ */
void network_disconnect_wifi(void)
{
    esp_wifi_disconnect();
    s_connected = false;
    ESP_LOGI(TAG, "WiFi disconnected");
}

/* ------------------------------------------------------------------ */
bool network_is_connected(void)
{
    return s_connected;
}

/* ------------------------------------------------------------------ */
bool network_upload_telemetry(const char *ride_id,
                              const char *telemetry_data,
                              size_t data_length)
{
    if (!s_connected || !ride_id || !telemetry_data || data_length == 0)
        return false;

    char url[256];
    snprintf(url, sizeof(url), "http://%s:%u/write?db=%s&precision=ns",
             s_cfg.influxdb_url,
             s_cfg.influxdb_port,
             s_cfg.influxdb_db);

    esp_http_client_config_t http_cfg = {
        .url       = url,
        .method    = HTTP_METHOD_POST,
        .auth_type = s_cfg.influxdb_user[0]
                         ? HTTP_AUTH_TYPE_BASIC
                         : HTTP_AUTH_TYPE_NONE,
        .username  = s_cfg.influxdb_user,
        .password  = s_cfg.influxdb_password,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&http_cfg);
    if (!client) {
        ESP_LOGE(TAG, "HTTP client init failed");
        return false;
    }

    esp_http_client_set_post_field(client, telemetry_data, (int)data_length);
    esp_http_client_set_header(client, "Content-Type", "text/plain; charset=utf-8");

    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "HTTP POST error: %s", esp_err_to_name(err));
        return false;
    }

    /* InfluxDB v1 returns 204 No Content on success */
    if (status != 204) {
        ESP_LOGW(TAG, "InfluxDB returned HTTP %d (expected 204)", status);
        return false;
    }

    ESP_LOGI(TAG, "Telemetry uploaded for ride %s (%u bytes)", ride_id,
             (unsigned)data_length);
    return true;
}

/* ------------------------------------------------------------------ */
bool network_upload_ride_file(const char *ride_id)
{
    if (!s_connected || !ride_id) return false;

    char path[96];
    snprintf(path, sizeof(path), "/sdcard/rides/%s/telemetry.lp", ride_id);

    FILE *f = fopen(path, "r");
    if (!f) {
        ESP_LOGE(TAG, "Cannot open %s", path);
        return false;
    }

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz <= 0) { fclose(f); return false; }

    char *buf = malloc((size_t)sz);
    if (!buf) { fclose(f); return false; }

    size_t n = fread(buf, 1, (size_t)sz, f);
    fclose(f);

    bool ok = network_upload_telemetry(ride_id, buf, n);
    free(buf);
    return ok;
}

/* ------------------------------------------------------------------ */
bool network_upload_gpx_file(const char *ride_id)
{
    /*
     * TODO: implement a multipart/form-data or PUT upload to a
     * Strava-compatible endpoint.  For now this is a stub.
     */
    ESP_LOGI(TAG, "GPX upload for %s not yet implemented", ride_id);
    return false;
}

/* ------------------------------------------------------------------ */
void network_configure_influxdb(const char *url, uint16_t port,
                                const char *db,
                                const char *user, const char *password)
{
    if (url)      strncpy(s_cfg.influxdb_url,      url,      sizeof(s_cfg.influxdb_url)      - 1);
    if (port > 0) s_cfg.influxdb_port = port;
    if (db)       strncpy(s_cfg.influxdb_db,       db,       sizeof(s_cfg.influxdb_db)       - 1);
    if (user)     strncpy(s_cfg.influxdb_user,     user,     sizeof(s_cfg.influxdb_user)     - 1);
    if (password) strncpy(s_cfg.influxdb_password, password, sizeof(s_cfg.influxdb_password) - 1);

    network_save_config(&s_cfg);
}

/* ------------------------------------------------------------------ */
bool network_save_config(const velo_wifi_config_t *config)
{
    if (!config) return false;

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS open: %s", esp_err_to_name(err));
        return false;
    }

#define CHK(x) do { err = (x); if (err != ESP_OK) goto cleanup; } while (0)

    CHK(nvs_set_str(h, "ssid",         config->ssid));
    CHK(nvs_set_str(h, "pass",         config->password));
    CHK(nvs_set_str(h, "influx_url",   config->influxdb_url));
    CHK(nvs_set_u16(h, "influx_port",  config->influxdb_port));
    CHK(nvs_set_str(h, "influx_db",    config->influxdb_db));
    CHK(nvs_set_str(h, "influx_user",  config->influxdb_user));
    CHK(nvs_set_str(h, "influx_pass",  config->influxdb_password));
    CHK(nvs_commit(h));

#undef CHK

    ESP_LOGI(TAG, "Network config saved");
    nvs_close(h);
    return true;

cleanup:
    nvs_close(h);
    ESP_LOGE(TAG, "network_save_config failed: %s", esp_err_to_name(err));
    return false;
}

/* ------------------------------------------------------------------ */
bool network_load_config(velo_wifi_config_t *config)
{
    if (!config) return false;

    /* Defaults */
    memset(config, 0, sizeof(*config));
    strncpy(config->influxdb_url, "192.168.1.100", sizeof(config->influxdb_url) - 1);
    config->influxdb_port = 8086;
    strncpy(config->influxdb_db, "bike_data", sizeof(config->influxdb_db) - 1);

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS open failed, using defaults: %s", esp_err_to_name(err));
        return false;
    }

#define LOAD_STR(key, dst) \
    do { size_t sz = sizeof(dst); nvs_get_str(h, key, dst, &sz); } while (0)

    LOAD_STR("ssid",        config->ssid);
    LOAD_STR("pass",        config->password);
    LOAD_STR("influx_url",  config->influxdb_url);
    LOAD_STR("influx_db",   config->influxdb_db);
    LOAD_STR("influx_user", config->influxdb_user);
    LOAD_STR("influx_pass", config->influxdb_password);

#undef LOAD_STR

    uint16_t port = 0;
    if (nvs_get_u16(h, "influx_port", &port) == ESP_OK) config->influxdb_port = port;

    nvs_close(h);
    ESP_LOGI(TAG, "Network config loaded (influx=%s:%u db=%s)",
             config->influxdb_url, config->influxdb_port, config->influxdb_db);
    return true;
}
