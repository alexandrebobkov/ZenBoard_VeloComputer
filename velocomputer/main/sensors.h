#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    float    speed;       /* km/h  */
    uint16_t cadence;     /* RPM   */
    uint64_t timestamp;   /* ns    */

    struct {
        float    temperature; /* °C  */
        float    humidity;    /* %   */
        uint16_t heart_rate;  /* BPM */
        float    power;       /* W   */
    } optional;
} sensor_data_t;

/* Initialise PCNT units for speed & cadence, plus any I²C sensors */
void sensors_init(void);

/* Copy latest readings into *data; returns false on error */
bool sensors_get_data(sensor_data_t *data);

/* Wheel circumference in metres (e.g. 2.105 for 700×23c) */
void sensors_set_wheel_circumference(float circumference_m);

/* Number of magnets on the crank (typically 1) */
void sensors_set_cadence_magnets(uint8_t count);

/* Optional sensor enable/disable */
void sensors_enable_temperature(bool enable);
void sensors_enable_humidity(bool enable);
void sensors_enable_heart_rate(bool enable);
void sensors_enable_power(bool enable);
