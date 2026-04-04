#pragma once

#include <stdint.h>
#include <time.h>

#define DEVICE_ID_LENGTH 32
#define BIKE_NAME_LENGTH 32
#define RIDER_NAME_LENGTH 32
#define RIDE_TYPE_LENGTH 16
#define RIDE_ID_LENGTH 36

typedef struct {
    // Metadata (InfluxDB tags)
    char device_id[DEVICE_ID_LENGTH];
    char bicycle[BIKE_NAME_LENGTH];
    char rider[RIDER_NAME_LENGTH];
    char ride_type[RIDE_TYPE_LENGTH];
    char ride_id[RIDE_ID_LENGTH];

    // Core metrics
    uint64_t timestamp;      // Nanoseconds since epoch
    float speed;             // km/h
    uint16_t cadence;        // RPM
    double latitude;         // GPS coordinates
    double longitude;
    uint8_t gps_quality;     // Number of satellites

    // Optional metrics
    struct {
        float temperature;   // °C, 0 if not available
        float humidity;      // %, 0 if not available
        float altitude;      // meters
        uint16_t heart_rate; // BPM
        float power;         // Watts
    } optional;

    // System metrics
    uint8_t battery_level;   // 0-100%
    uint32_t ride_duration;  // seconds
} bike_telemetry_t;

// Initialize telemetry system
void telemetry_init(void);

// Create new telemetry point
bike_telemetry_t* telemetry_create_point(void);

// Convert telemetry to InfluxDB line protocol
int telemetry_to_line_protocol(bike_telemetry_t* data, char* buffer, size_t buffer_size);

// Set metadata for current ride
void telemetry_set_metadata(const char* bicycle, const char* rider, const char* ride_type, const char* ride_id);
