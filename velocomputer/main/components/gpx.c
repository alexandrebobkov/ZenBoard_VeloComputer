#include "gpx.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *TAG = "gpx";

static FILE *s_file   = NULL;
static bool  s_active = false;

/* ------------------------------------------------------------------ */
bool gpx_start_file(const char *filename, const char *ride_id,
                    const char *bicycle,  const char *rider)
{
    if (s_active) {
        ESP_LOGW(TAG, "A GPX file is already open – close it first");
        return false;
    }
    if (!filename) return false;

    s_file = fopen(filename, "w");
    if (!s_file) {
        ESP_LOGE(TAG, "Cannot open %s for writing", filename);
        return false;
    }

    /* XML declaration */
    fprintf(s_file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");

    /* Root element */
    fprintf(s_file,
        "<gpx version=\"1.1\" creator=\"Velocomputer\"\n"
        "  xmlns=\"http://www.topografix.com/GPX/1/1\"\n"
        "  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
        "  xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1"
        " http://www.topografix.com/GPX/1/1/gpx.xsd\">\n");

    /* Metadata block */
    fprintf(s_file, "  <metadata>\n");
    fprintf(s_file, "    <name>%s</name>\n",
            ride_id ? ride_id : "Unknown Ride");
    if (bicycle && bicycle[0])
        fprintf(s_file, "    <desc>Bicycle: %s</desc>\n", bicycle);
    if (rider && rider[0])
        fprintf(s_file, "    <author><name>%s</name></author>\n", rider);
    fprintf(s_file, "  </metadata>\n");

    /* Track */
    fprintf(s_file, "  <trk>\n");
    fprintf(s_file, "    <name>%s</name>\n",
            ride_id ? ride_id : "Unknown Ride");
    fprintf(s_file, "    <trkseg>\n");

    fflush(s_file);
    s_active = true;
    ESP_LOGI(TAG, "GPX file opened: %s", filename);
    return true;
}

/* ------------------------------------------------------------------ */
void gpx_add_point(const bike_telemetry_t *data)
{
    if (!s_active || !s_file || !data) return;

    /* ISO-8601 timestamp from nanosecond epoch */
    time_t sec = (time_t)(data->timestamp / 1000000000ULL);
    struct tm *ti = gmtime(&sec);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", ti);

    /* Track point with lat/lon attributes */
    fprintf(s_file,
            "      <trkpt lat=\"%.6f\" lon=\"%.6f\">\n",
            data->latitude, data->longitude);
    fprintf(s_file, "        <ele>%.1f</ele>\n",
            (double)data->optional.altitude);
    fprintf(s_file, "        <time>%s</time>\n", ts);

    /* Extensions for cycling-specific data */
    fprintf(s_file, "        <extensions>\n");
    fprintf(s_file, "          <speed>%.2f</speed>\n",     (double)data->speed);
    fprintf(s_file, "          <cadence>%u</cadence>\n",   data->cadence);

    if (data->optional.heart_rate > 0)
        fprintf(s_file, "          <hr>%u</hr>\n",
                data->optional.heart_rate);

    if (data->optional.power > 0.0f)
        fprintf(s_file, "          <power>%.1f</power>\n",
                (double)data->optional.power);

    if (data->optional.temperature != 0.0f)
        fprintf(s_file, "          <atemp>%.1f</atemp>\n",
                (double)data->optional.temperature);

    fprintf(s_file, "        </extensions>\n");
    fprintf(s_file, "      </trkpt>\n");

    fflush(s_file);
}

/* ------------------------------------------------------------------ */
bool gpx_finalize_file(void)
{
    if (!s_active || !s_file) return false;

    fprintf(s_file, "    </trkseg>\n");
    fprintf(s_file, "  </trk>\n");
    fprintf(s_file, "</gpx>\n");

    fclose(s_file);
    s_file   = NULL;
    s_active = false;

    ESP_LOGI(TAG, "GPX file finalised");
    return true;
}

/* ------------------------------------------------------------------ */
bool gpx_generate_from_telemetry(const char *input_lp_file,
                                 const char *output_gpx_file)
{
    /*
     * TODO: open input_lp_file, parse each line-protocol record,
     * call gpx_start_file / gpx_add_point / gpx_finalize_file.
     */
    ESP_LOGI(TAG, "gpx_generate_from_telemetry(%s → %s) not yet implemented",
             input_lp_file, output_gpx_file);
    return false;
}
