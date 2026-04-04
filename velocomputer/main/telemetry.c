#include "telemetry.h"
#include "config.h"
#include "esp_log.h"
#include "esp_random.h"
#include <inttypes.h>   /* PRIu32, PRIu64, etc. */
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

static const char      *TAG              = "telemetry";
static bike_telemetry_t current_metadata = {0};

/* ------------------------------------------------------------------ */
static void generate_ride_id(char *buf, size_t len)
{
    /* Simple hex UUID: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx */
    static const char hex[] = "0123456789abcdef";
    const int dashes[] = {8, 13, 18, 23};
    int d = 0;

    for (size_t i = 0; i < len - 1; i++) {
        if (d < 4 && i == (size_t)dashes[d]) {
            buf[i] = '-';
            d++;
        } else {
            buf[i] = hex[esp_random() % 16];
        }
    }
    buf[len - 1] = '\0';
}

/* ------------------------------------------------------------------ */
void telemetry_init(void)
{
    ESP_LOGI(TAG, "Initialising telemetry");

    system_config_t cfg;
    config_load(&cfg);

    telemetry_set_metadata(cfg.default_bicycle, cfg.default_rider, "", "");
}

/* ------------------------------------------------------------------ */
bike_telemetry_t *telemetry_create_point(void)
{
    static bike_telemetry_t point;

    memcpy(&point, &current_metadata, sizeof(bike_telemetry_t));

    struct timeval tv;
    gettimeofday(&tv, NULL);
    point.timestamp = (uint64_t)tv.tv_sec * 1000000000ULL
                    + (uint64_t)tv.tv_usec * 1000ULL;

    /* caller fills in sensor values after this */
    return &point;
}

/* ------------------------------------------------------------------ */
int telemetry_to_line_protocol(const bike_telemetry_t *data,
                               char *buf, size_t buf_size)
{
    if (!data || !buf || buf_size < 128) return -1;

    /* Tags */
    int len = snprintf(buf, buf_size,
        "bike_telemetry,"
        "device=%s,"
        "bicycle=%s,"
        "rider=%s,"
        "ride_type=%s,"
        "ride_id=%s ",
        data->device_id[0]  ? data->device_id  : "unknown",
        data->bicycle[0]    ? data->bicycle     : "unknown",
        data->rider[0]      ? data->rider       : "unknown",
        data->ride_type[0]  ? data->ride_type   : "unknown",
        data->ride_id[0]    ? data->ride_id     : "0");

    if (len < 0 || (size_t)len >= buf_size) return len;

    /* Core fields */
    int n = snprintf(buf + len, buf_size - (size_t)len,
        "speed=%.2f,"
        "cadence=%u,"
        "lat=%.6f,"
        "lon=%.6f,"
        "gps_sats=%u,"
        "battery=%u,"
        "duration=%" PRIu32,
        (double)data->speed,
        data->cadence,
        data->latitude,
        data->longitude,
        data->gps_quality,
        data->battery_level,
        data->ride_duration);
    if (n < 0) return -1;
    len += n;

    /* Optional fields – only emit when non-zero */
#define APPEND_F(field, fmt, val) \
    if ((size_t)len < buf_size && (val) != 0) { \
        n = snprintf(buf + len, buf_size - (size_t)len, "," field "=" fmt, val); \
        if (n > 0) len += n; \
    }

    APPEND_F("temperature", "%.2f",  (double)data->optional.temperature)
    APPEND_F("humidity",    "%.2f",  (double)data->optional.humidity)
    APPEND_F("altitude",    "%.2f",  (double)data->optional.altitude)
    APPEND_F("heart_rate",  "%u",    data->optional.heart_rate)
    APPEND_F("power",       "%.2f",  (double)data->optional.power)

#undef APPEND_F

    /* Timestamp */
    if ((size_t)len < buf_size) {
        n = snprintf(buf + len, buf_size - (size_t)len,
                     " %" PRIu64 "\n", data->timestamp);
        if (n > 0) len += n;
    }

    return len;
}

/* ------------------------------------------------------------------ */
void telemetry_set_metadata(const char *bicycle, const char *rider,
                            const char *ride_type, const char *ride_id)
{
    system_config_t cfg;
    config_load(&cfg);

    /* device_id always comes from config */
    strncpy(current_metadata.device_id, cfg.device_id,
            sizeof(current_metadata.device_id) - 1);
    current_metadata.device_id[sizeof(current_metadata.device_id) - 1] = '\0';

#define SET_FIELD(dst, src, fallback) \
    do { \
        const char *_s = ((src) && (src)[0]) ? (src) : (fallback); \
        strncpy((dst), _s, sizeof(dst) - 1); \
        (dst)[sizeof(dst) - 1] = '\0'; \
    } while (0)

    SET_FIELD(current_metadata.bicycle,   bicycle,    cfg.default_bicycle);
    SET_FIELD(current_metadata.rider,     rider,      cfg.default_rider);
    SET_FIELD(current_metadata.ride_type, ride_type,  "unknown");

#undef SET_FIELD

    if (ride_id && ride_id[0]) {
        strncpy(current_metadata.ride_id, ride_id,
                sizeof(current_metadata.ride_id) - 1);
        current_metadata.ride_id[sizeof(current_metadata.ride_id) - 1] = '\0';
    } else {
        generate_ride_id(current_metadata.ride_id, sizeof(current_metadata.ride_id));
    }

    ESP_LOGI(TAG, "Metadata: bike=%s rider=%s type=%s ride_id=%s",
             current_metadata.bicycle, current_metadata.rider,
             current_metadata.ride_type, current_metadata.ride_id);
}
