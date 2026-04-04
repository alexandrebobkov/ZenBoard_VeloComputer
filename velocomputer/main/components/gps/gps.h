#pragma once

#include <stdint.h>
#include <stdbool.h>

// GPS data structure
typedef struct {
    double latitude;
    double longitude;
    float altitude;
    float speed;      // km/h
    float heading;    // degrees
    uint8_t satellites;
    bool valid;
    uint64_t timestamp; // nanoseconds
} gps_data_t;

// Initialize GPS module
void gps_init(void);

// Get current GPS data (non-blocking)
bool gps_get_data(gps_data_t* data);

// Start GPS acquisition
void gps_start(void);

// Stop GPS to save power
void gps_stop(void);
