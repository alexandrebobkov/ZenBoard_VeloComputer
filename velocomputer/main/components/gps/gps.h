#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    double   latitude;    /* decimal degrees */
    double   longitude;
    float    altitude;    /* metres above MSL */
    float    speed;       /* km/h */
    float    heading;     /* degrees true */
    uint8_t  satellites;
    bool     valid;
    uint64_t timestamp;   /* nanoseconds since epoch */
} gps_data_t;

/* Initialise UART and GPS module */
void gps_init(void);

/* Non-blocking: copy current fix into *data.
   Returns true when fix is valid. */
bool gps_get_data(gps_data_t *data);

/* Start acquisition (blocks until first fix or timeout) */
void gps_start(void);

/* Put GPS module to sleep */
void gps_stop(void);
