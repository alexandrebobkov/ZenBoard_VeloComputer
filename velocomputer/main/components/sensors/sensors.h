#pragma once

#include <stdint.h>
#include <stdbool.h>

// Sensor data structure
typedef struct {
    float speed;         // km/h
    uint16_t cadence;    // RPM
    uint64_t timestamp;  // nanoseconds

    // Optional sensor data
    struct {
        float temperature; // °C
        float humidity;    // %
        uint16_t heart_rate; // BPM
        float power;       // Watts
    } optional;
} sensor_data_t;

// Initialize sensor system
void sensors_init(void);

// Get current sensor data
bool sensors_get_data(sensor_data_t* data);

// Set wheel circumference for speed calculation (in meters)
void sensors_set_wheel_circumference(float circumference);

// Set cadence sensor type (magnet count)
void sensors_set_cadence_magnets(uint8_t magnets);

// Enable/disable optional sensors
void sensors_enable_temperature(bool enable);
void sensors_enable_humidity(bool enable);
void sensors_enable_heart_rate(bool enable);
void sensors_enable_power(bool enable);
