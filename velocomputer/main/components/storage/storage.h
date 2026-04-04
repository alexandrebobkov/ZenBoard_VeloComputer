#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "telemetry.h"

/* Initialise storage system (mounts SD card via SPI) */
void storage_init(void);

/* Returns true when the SD card is mounted and ready */
bool storage_is_available(void);

/* Create /sdcard/rides/<ride_id>/ and open telemetry.lp for writing */
bool storage_create_ride_directory(const char *ride_id);

/* Append one telemetry point (line-protocol format) to the open ride file */
bool storage_write_telemetry(bike_telemetry_t *data);

/* Write metadata.json for the current ride */
bool storage_write_metadata(const char *ride_id, const char *bicycle,
                            const char *rider,   const char *ride_type,
                            uint64_t start_time_ns);

/* Close the telemetry file and update metadata.json with summary stats */
bool storage_finalize_ride(const char *ride_id, uint64_t end_time_ns,
                           float distance_km, float max_speed, float avg_speed);

/* Generate a GPX file from telemetry.lp (post-ride) */
bool storage_generate_gpx(const char *ride_id);

/* Number of completed rides on the SD card */
int  storage_get_ride_count(void);

/* Copy the ride_id string for the ride at position index into ride_id[buf_sz] */
bool storage_get_ride_id(int index, char *ride_id, size_t buf_sz);
