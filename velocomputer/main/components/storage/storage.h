#pragma once

#include "telemetry.h"
#include "telemetry.h"
#include "telemetry.h"

// Initialize storage system (SD card)
void storage_init(void);

// Check if storage is available
bool storage_is_available(void);

// Create new ride directory
bool storage_create_ride_directory(const char* ride_id);

// Write telemetry data to current ride file
bool storage_write_telemetry(bike_telemetry_t* data);

// Write metadata JSON for current ride
bool storage_write_metadata(const char* ride_id, const char* bicycle,
                            const char* rider, const char* ride_type,
                            uint64_t start_time);

// Finalize current ride (close files, etc.)
bool storage_finalize_ride(const char* ride_id, uint64_t end_time,
                          float distance_km, float max_speed, float avg_speed);

// Generate GPX file from ride data
bool storage_generate_gpx(const char* ride_id);

// List available rides
// Get ride count
int storage_get_ride_count(void);

// Get ride ID by index
bool storage_get_ride_id(int index, char* ride_id, size_t buffer_size);
