#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "core/telemetry.h"

// WiFi configuration
typedef struct {
    char ssid[32];
    char password[64];
    char influxdb_url[128];
    char influxdb_db[32];
    uint16_t influxdb_port;
    char influxdb_user[32];
    char influxdb_password[64];
} velo_wifi_config_t;

// Initialize network system
void network_init(void);

// Connect to WiFi
bool network_connect_wifi(const char* ssid, const char* password);

// Disconnect from WiFi
void network_disconnect_wifi(void);

// Check WiFi connection status
bool network_is_connected(void);

// Upload telemetry data to InfluxDB
bool network_upload_telemetry(const char* ride_id, const char* telemetry_data, size_t data_length);

// Upload ride file to InfluxDB
bool network_upload_ride_file(const char* ride_id);

// Configure InfluxDB settings
void network_configure_influxdb(const char* url, uint16_t port,
                                const char* db, const char* user, const char* password);

// Save WiFi configuration
bool network_save_wifi_config(const wifi_config_t* config);

// Load WiFi configuration
bool network_load_wifi_config(wifi_config_t* config);

// Update all function signatures to use velo_wifi_config_t*
bool network_save_wifi_config(const velo_wifi_config_t* config);
bool network_load_wifi_config(velo_wifi_config_t* config);
