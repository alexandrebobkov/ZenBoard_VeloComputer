#include "telemetry.h"
#include "config.h"
#include "esp_log.h"
#include "esp_random.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "telemetry";
static bike_telemetry_t current_metadata;

static void generate_ride_id(char *buffer, size_t length) {
    // Simple UUID-like generation
    for (int i = 0; i < length - 1; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            buffer[i] = '-';
        } else {
            static const char chars[] = "0123456789abcdef";
            buffer[i] = chars[esp_random() % 16];
        }
    }
    buffer[length - 1] = '\0';
}

void telemetry_init(void) {
    ESP_LOGI(TAG, "Initializing telemetry system");

    // Load default metadata from config
    system_config_t config;
    config_load(&config);

    telemetry_set_metadata(
        config.default_bicycle,
        config.default_rider,
        "",  // No default ride type
        ""   // Will generate new ride ID
    );
}

bike_telemetry_t* telemetry_create_point(void) {
    static bike_telemetry_t point;

    // Copy metadata
    memcpy(&point, &current_metadata, sizeof(bike_telemetry_t));

    // Set current timestamp
    struct timeval tv;
    gettimeofday(&tv, NULL);
    point.timestamp = (uint64_t)tv.tv_sec * 1000000000 + tv.tv_usec * 1000;

    // Initialize optional fields to 0 (not available)
    memset(&point.optional, 0, sizeof(point.optional));

    return &point;
}

int telemetry_to_line_protocol(bike_telemetry_t* data, char* buffer, size_t buffer_size) {
    if (!data || !buffer || buffer_size < 100) {
        return -1;
    }

    int len = 0;

    // Start with measurement name and tags
    len = snprintf(buffer, buffer_size,
                  "bike_telemetry,device=%s,bicycle=%s,rider=%s,ride_type=%s,ride_id=%s ",
                  strlen(data->device_id) > 0 ? data->device_id : "unknown",
                  strlen(data->bicycle) > 0 ? data->bicycle : "unknown",
                  strlen(data->rider) > 0 ? data->rider : "unknown",
                  strlen(data->ride_type) > 0 ? data->ride_type : "unknown",
                  strlen(data->ride_id) > 0 ? data->ride_id : "0");

    if (len >= buffer_size) return len;

    // Add core fields
    int added = snprintf(buffer + len, buffer_size - len,
                        "speed=%.1f,cadence=%u,lat=%.6f,lon=%.6f,gps_quality=%u,battery=%u,duration=%us",
                        data->speed,
                        data->cadence,
                        data->latitude,
                        data->longitude,
                        data->gps_quality,
                        data->battery_level,
                        data->ride_duration);
    len += added;

    if (len >= buffer_size) return len;

    // Add optional fields if available
    if (data->optional.temperature != 0) {
        added = snprintf(buffer + len, buffer_size - len, ",temperature=%.1f", data->optional.temperature);
        len += added;
    }

    if (len >= buffer_size) return len;

    if (data->optional.humidity != 0) {
        added = snprintf(buffer + len, buffer_size - len, ",humidity=%.1f", data->optional.humidity);
        len += added;
    }

    if (len >= buffer_size) return len;

    if (data->optional.altitude != 0) {
        added = snprintf(buffer + len, buffer_size - len, ",altitude=%.1f", data->optional.altitude);
        len += added;
    }

    if (len >= buffer_size) return len;

    if (data->optional.heart_rate != 0) {
        added = snprintf(buffer + len, buffer_size - len, ",heart_rate=%u", data->optional.heart_rate);
        len += added;
    }

    if (len >= buffer_size) return len;

    if (data->optional.power != 0) {
        added = snprintf(buffer + len, buffer_size - len, ",power=%.1f", data->optional.power);
        len += added;
    }

    if (len >= buffer_size) return len;

    // Add timestamp
    added = snprintf(buffer + len, buffer_size - len, " %llu\n", data->timestamp);
    len += added;

    return len;
}

void telemetry_set_metadata(const char* bicycle, const char* rider, const char* ride_type, const char* ride_id) {
    // Copy device ID from config
    system_config_t config;
    config_load(&config);
    strncpy(current_metadata.device_id, config.device_id, sizeof(current_metadata.device_id) - 1);
    current_metadata.device_id[sizeof(current_metadata.device_id) - 1] = '\0';

    // Set bicycle name
    if (bicycle && strlen(bicycle) > 0) {
        strncpy(current_metadata.bicycle, bicycle, sizeof(current_metadata.bicycle) - 1);
    } else {
        strncpy(current_metadata.bicycle, config.default_bicycle, sizeof(current_metadata.bicycle) - 1);
    }
    current_metadata.bicycle[sizeof(current_metadata.bicycle) - 1] = '\0';

    // Set rider name
    if (rider && strlen(rider) > 0) {
        strncpy(current_metadata.rider, rider, sizeof(current_metadata.rider) - 1);
    } else {
        strncpy(current_metadata.rider, config.default_rider, sizeof(current_metadata.rider) - 1);
    }
    current_metadata.rider[sizeof(current_metadata.rider) - 1] = '\0';

    // Set ride type
    if (ride_type && strlen(ride_type) > 0) {
        strncpy(current_metadata.ride_type, ride_type, sizeof(current_metadata.ride_type) - 1);
    } else {
        strncpy(current_metadata.ride_type, "unknown", sizeof(current_metadata.ride_type) - 1);
    }
    current_metadata.ride_type[sizeof(current_metadata.ride_type) - 1] = '\0';

    // Set ride ID (generate if not provided)
    if (ride_id && strlen(ride_id) > 0) {
        strncpy(current_metadata.ride_id, ride_id, sizeof(current_metadata.ride_id) - 1);
    } else {
        generate_ride_id(current_metadata.ride_id, sizeof(current_metadata.ride_id));
    }
    current_metadata.ride_id[sizeof(current_metadata.ride_id) - 1] = '\0';

    ESP_LOGI(TAG, "Telemetry metadata set: bike=%s, rider=%s, type=%s, ride_id=%s",
             current_metadata.bicycle, current_metadata.rider,
             current_metadata.ride_type, current_metadata.ride_id);
}
