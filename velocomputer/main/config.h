#pragma once

#include <stdbool.h>

#define DEVICE_ID_LENGTH  32
#define DEFAULT_BIKE_LENGTH   32
#define DEFAULT_RIDER_LENGTH  32

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

/* Initialize configuration system (NVS) */
void config_init(void);

/* Load configuration from NVS; returns false and fills defaults on failure */
bool config_load(system_config_t *config);

/* Persist configuration to NVS */
bool config_save(const system_config_t *config);

/* Fill *config with factory defaults */
void config_get_defaults(system_config_t *config);
