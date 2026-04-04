#pragma once

#include <stdbool.h>
#include "core/telemetry.h"

// Generate GPX file from telemetry data
bool gpx_generate_from_telemetry(const char* input_file, const char* output_file);

// Add telemetry point to GPX builder
void gpx_add_point(bike_telemetry_t* data);

// Start new GPX file
bool gpx_start_file(const char* filename, const char* ride_id,
                   const char* bicycle, const char* rider);

// Finalize and close GPX file
bool gpx_finalize_file(void);
