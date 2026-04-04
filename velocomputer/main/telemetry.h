#pragma once

#include <stdint.h>
#include <stddef.h>

/* Guard against re-definition from config.h */
#ifndef DEVICE_ID_LENGTH
#define DEVICE_ID_LENGTH  32
#endif

#define BIKE_NAME_LENGTH   32
#define RIDER_NAME_LENGTH  32
#define RIDE_TYPE_LENGTH   16
#define RIDE_ID_LENGTH     37   /* 36-char UUID + NUL */
#define PCNT_GPIO_NOT_USED 9

typedef struct {
    /* ---- InfluxDB tags (identity) ---- */
    char device_id[DEVICE_ID_LENGTH];
    char bicycle[BIKE_NAME_LENGTH];
    char rider[RIDER_NAME_LENGTH];
    char ride_type[RIDE_TYPE_LENGTH];
    char ride_id[RIDE_ID_LENGTH];

    /* ---- Core metrics ---- */
    uint64_t timestamp;      /* nanoseconds since Unix epoch */
    float    speed;          /* km/h  */
    uint16_t cadence;        /* RPM   */
    double   latitude;
    double   longitude;
    uint8_t  gps_quality;    /* satellite count */

    /* ---- Optional metrics (0 = not available) ---- */
    struct {
        float    temperature; /* °C   */
        float    humidity;    /* %    */
        float    altitude;    /* m    */
        uint16_t heart_rate;  /* BPM  */
        float    power;       /* W    */
    } optional;

    /* ---- System metrics ---- */
    uint8_t  battery_level;  /* 0–100 % */
    uint32_t ride_duration;  /* seconds */
} bike_telemetry_t;

/* Initialise telemetry module; loads default bike/rider from config */
void telemetry_init(void);

/* Return pointer to a statically-allocated, populated telemetry point.
   Caller must copy the struct if it needs to keep it across calls. */
bike_telemetry_t *telemetry_create_point(void);

/* Serialise *data to InfluxDB line-protocol in buffer[buffer_size].
   Returns number of bytes written, or -1 on error. */
int telemetry_to_line_protocol(const bike_telemetry_t *data,
                               char *buffer, size_t buffer_size);

/* Override ride metadata (generates new ride_id when ride_id == NULL or "") */
void telemetry_set_metadata(const char *bicycle, const char *rider,
                            const char *ride_type, const char *ride_id);
