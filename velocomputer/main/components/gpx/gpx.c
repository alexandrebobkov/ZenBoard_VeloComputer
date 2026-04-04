#include "gpx.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *TAG = "gpx";
static FILE* gpx_file = NULL;
static bool gpx_active = false;

bool gpx_start_file(const char* filename, const char* ride_id,
                   const char* bicycle, const char* rider) {
    if (gpx_active) {
        ESP_LOGW(TAG, "GPX file already open");
        return false;
    }

    gpx_file = fopen(filename, "w");
    if (!gpx_file) {
        ESP_LOGE(TAG, "Failed to open GPX file: %s", filename);
        return false;
    }

    // Write GPX header
    fprintf(gpx_file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    fprintf(gpx_file, "<gpx version=\"1.1\" creator=\"Velocomputer\"\n");
    fprintf(gpx_file, "    xmlns=\"http://www.topografix.com/GPX/1/1\"\n");
    fprintf(gpx_file, "    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n");
    fprintf(gpx_file, "    xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 "
                      "http://www.topografix.com/GPX/1/1/gpx.xsd\">\n");
    fprintf(gpx_file, "      <trkpt lat=\"%.6f\" lon=\"%.6f\">\n",
            data->latitude, data->longitude);

    gpx_active = true;
    ESP_LOGI(TAG, "Started GPX file: %s", filename);
    return true;
}

void gpx_add_point(bike_telemetry_t* data) {
    if (!gpx_active || !gpx_file || !data) {
        return;
    }

    // Convert timestamp to ISO 8601
    time_t sec = data->timestamp / 1000000000;
    struct tm *timeinfo = gmtime(&sec);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%SZ", timeinfo);

    // Write GPX track point
    fprintf(gpx_file, "      <trkpt lat="%.6f" lon="%.6f">\n",
            data->latitude, data->longitude);
    fprintf(gpx_file, "        <time>%s</time>\n", time_str);
    fprintf(gpx_file, "        <ele>%.1f</ele>\n", data->optional.altitude);

    // Add extensions for additional data
    fprintf(gpx_file, "        <extensions>\n");
    fprintf(gpx_file, "          <speed>%.1f</speed>\n", data->speed);
    fprintf(gpx_file, "          <cadence>%u</cadence>\n", data->cadence);
    if (data->optional.heart_rate > 0) {
        fprintf(gpx_file, "          <hr>%u</hr>\n", data->optional.heart_rate);
    }
    if (data->optional.power > 0) {
        fprintf(gpx_file, "          <power>%.1f</power>\n", data->optional.power);
    }
    fprintf(gpx_file, "        </extensions>\n");

    fprintf(gpx_file, "      </trkpt>\n");
}

bool gpx_finalize_file(void) {
    if (!gpx_active || !gpx_file) {
        return false;
    }

    // Close track segment and file
    fprintf(gpx_file, "    </trkseg>\n");
    fprintf(gpx_file, "  </trk>\n");
    fprintf(gpx_file, "</gpx>\n");

    fclose(gpx_file);
    gpx_file = NULL;
    gpx_active = false;

    ESP_LOGI(TAG, "Finalized GPX file");
    return true;
}

bool gpx_generate_from_telemetry(const char* input_file, const char* output_file) {
    // This would parse the line protocol file and generate GPX
    // For now, it's a stub
    ESP_LOGI(TAG, "GPX generation from %s to %s not yet implemented",
             input_file, output_file);
    return false;
}
