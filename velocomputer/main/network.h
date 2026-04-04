#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * NOTE: we deliberately avoid the name wifi_config_t because ESP-IDF
 * already defines that type in esp_wifi_types.h.
 */
typedef struct {
    char     ssid[32];
    char     password[64];
    char     influxdb_url[128];
    char     influxdb_db[32];
    uint16_t influxdb_port;
    char     influxdb_user[32];
    char     influxdb_password[64];
} velo_wifi_config_t;

/* Initialise TCP/IP stack and WiFi driver (does not connect yet) */
void network_init(void);

/* Connect to the given SSID; blocks up to ~10 s. Returns true on success. */
bool network_connect_wifi(const char *ssid, const char *password);

/* Disconnect and stop WiFi */
void network_disconnect_wifi(void);

/* True when an IP address has been obtained */
bool network_is_connected(void);

/* POST telemetry_data (line-protocol) to the configured InfluxDB */
bool network_upload_telemetry(const char *ride_id,
                              const char *telemetry_data,
                              size_t data_length);

/* Read telemetry.lp from SD and upload it (used after a ride) */
bool network_upload_ride_file(const char *ride_id);

/* Upload a GPX file from SD to InfluxDB-adjacent storage (stub) */
bool network_upload_gpx_file(const char *ride_id);

/* Override InfluxDB connection parameters and persist them */
void network_configure_influxdb(const char *url, uint16_t port,
                                const char *db,
                                const char *user, const char *password);

/* Persist / restore velo_wifi_config_t to/from NVS */
bool network_save_config(const velo_wifi_config_t *config);
bool network_load_config(velo_wifi_config_t *config);
