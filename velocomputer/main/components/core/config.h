#pragma once

#include <stdbool.h>

#define DEVICE_ID_LENGTH 32
#define DEFAULT_BIKE_LENGTH 32
#define DEFAULT_RIDER_LENGTH 32

typedef struct {
    char device_id[DEVICE_ID_LENGTH];
    char default_bicycle[DEFAULT_BIKE_LENGTH];
    char default_rider[DEFAULT_RIDER_LENGTH];
    bool enable_temperature;
    bool enable_humidity;
    bool enable_altitude;
    bool enable_heart_rate;
    bool enable_power;
} system_config_t;

// Initialize configuration system
void config_init(void);

// Load configuration from storage
bool config_load(system_config_t* config);

// Save configuration to storage
bool config_save(const system_config_t* config);

// Get default configuration
void config_get_defaults(system_config_t* config);
