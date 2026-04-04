#pragma once

#include <stdbool.h>
#include "telemetry.h"   /* flat include – no components/core/ prefix */

/* Open a new GPX file and write the XML header + track start */
bool gpx_start_file(const char *filename, const char *ride_id,
                    const char *bicycle,  const char *rider);

/* Append one track point from a telemetry record */
void gpx_add_point(const bike_telemetry_t *data);

/* Close the track segment and the file */
bool gpx_finalize_file(void);

/* (Post-ride) Parse telemetry.lp and emit a GPX file – not yet implemented */
bool gpx_generate_from_telemetry(const char *input_lp_file,
                                 const char *output_gpx_file);
