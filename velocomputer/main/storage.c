#include "storage.h"
#include "telemetry.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdspi_host.h"
#include "driver/spi_master.h"
#include "driver/sdmmc_host.h"
#include "cJSON.h"
#include <sys/stat.h>
#include <sys/unistd.h>
#include <dirent.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const char *TAG = "storage";

/* ---------- SD SPI pin mapping ------------------------------------ */
#define SD_MISO  GPIO_NUM_2
#define SD_MOSI  GPIO_NUM_15
#define SD_CLK   GPIO_NUM_14
#define SD_CS    GPIO_NUM_13
#define SD_HOST  SPI2_HOST          /* HSPI */

/* ---------- State ------------------------------------------------- */
static bool   s_mounted            = false;
static char   s_ride_path[80]      = {0};
static FILE  *s_telemetry_file     = NULL;

/* ------------------------------------------------------------------ */
static void mount_sd_card(void)
{
    if (s_mounted) return;

    ESP_LOGI(TAG, "Mounting SD card (SPI)");

    /* 1. Initialise the SPI bus */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = SD_MOSI,
        .miso_io_num     = SD_MISO,
        .sclk_io_num     = SD_CLK,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = 4096,
    };
    esp_err_t ret = spi_bus_initialize(SD_HOST, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return;
    }

    /* 2. Describe the SD device on that bus */
    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs   = SD_CS;
    slot_cfg.host_id   = SD_HOST;

    /* 3. VFS-FAT mount config */
    esp_vfs_fat_mount_config_t mount_cfg = {
        .format_if_mount_failed = true,
        .max_files              = 5,
        .allocation_unit_size   = 16 * 1024,
    };

    /* 4. Host config (SPI mode) */
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_HOST;

    sdmmc_card_t *card = NULL;
    ret = esp_vfs_fat_sdspi_mount("/sdcard", &host, &slot_cfg, &mount_cfg, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed: %s", esp_err_to_name(ret));
        spi_bus_free(SD_HOST);
        return;
    }

    s_mounted = true;
    ESP_LOGI(TAG, "SD card mounted – %lluMB",
             (unsigned long long)card->csd.capacity
                 * card->csd.sector_size / (1024ULL * 1024ULL));
    sdmmc_card_print_info(stdout, card);
}

/* ------------------------------------------------------------------ */
void storage_init(void)
{
    ESP_LOGI(TAG, "Initialising storage");
    mount_sd_card();

    if (!s_mounted) {
        ESP_LOGW(TAG, "SD card not available – data will not be saved");
        return;
    }

    /* Ensure /sdcard/rides/ exists */
    struct stat st = {0};
    if (stat("/sdcard/rides", &st) != 0) {
        if (mkdir("/sdcard/rides", 0777) != 0) {
            ESP_LOGE(TAG, "Failed to create /sdcard/rides/");
        }
    }
}

/* ------------------------------------------------------------------ */
bool storage_is_available(void)
{
    return s_mounted;
}

/* ------------------------------------------------------------------ */
bool storage_create_ride_directory(const char *ride_id)
{
    if (!s_mounted || !ride_id) return false;

    snprintf(s_ride_path, sizeof(s_ride_path), "/sdcard/rides/%s", ride_id);

    if (mkdir(s_ride_path, 0777) != 0) {
        ESP_LOGE(TAG, "mkdir failed: %s", s_ride_path);
        return false;
    }
    ESP_LOGI(TAG, "Ride directory: %s", s_ride_path);

    /* Open telemetry line-protocol file */
    char tp_path[96];
    snprintf(tp_path, sizeof(tp_path), "%s/telemetry.lp", s_ride_path);
    s_telemetry_file = fopen(tp_path, "w");
    if (!s_telemetry_file) {
        ESP_LOGE(TAG, "Cannot open %s", tp_path);
        return false;
    }

    fprintf(s_telemetry_file,
            "# Velocomputer – InfluxDB Line Protocol\n"
            "# ride_id: %s\n", ride_id);
    fflush(s_telemetry_file);
    return true;
}

/* ------------------------------------------------------------------ */
bool storage_write_telemetry(bike_telemetry_t *data)
{
    if (!s_mounted || !data || !s_telemetry_file) return false;

    char line[320];
    int n = telemetry_to_line_protocol(data, line, sizeof(line));
    if (n <= 0) {
        ESP_LOGE(TAG, "Line-protocol serialisation failed");
        return false;
    }

    if (fwrite(line, 1, (size_t)n, s_telemetry_file) != (size_t)n) {
        ESP_LOGE(TAG, "Write error");
        return false;
    }

    fflush(s_telemetry_file);
    return true;
}

/* ------------------------------------------------------------------ */
bool storage_write_metadata(const char *ride_id, const char *bicycle,
                            const char *rider,   const char *ride_type,
                            uint64_t start_time_ns)
{
    if (!s_mounted || !ride_id) return false;

    char path[96];
    snprintf(path, sizeof(path), "%s/metadata.json", s_ride_path);

    cJSON *root = cJSON_CreateObject();
    if (!root) return false;

    cJSON_AddStringToObject(root, "ride_id",    ride_id);
    cJSON_AddStringToObject(root, "bicycle",    bicycle   ? bicycle   : "unknown");
    cJSON_AddStringToObject(root, "rider",      rider     ? rider     : "unknown");
    cJSON_AddStringToObject(root, "ride_type",  ride_type ? ride_type : "unknown");

    time_t sec = (time_t)(start_time_ns / 1000000000ULL);
    struct tm *ti = gmtime(&sec);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", ti);
    cJSON_AddStringToObject(root, "start_time", ts);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) return false;

    FILE *f = fopen(path, "w");
    bool ok = false;
    if (f) {
        ok = (fwrite(json, 1, strlen(json), f) == strlen(json));
        fclose(f);
    }
    free(json);
    return ok;
}

/* ------------------------------------------------------------------ */
bool storage_finalize_ride(const char *ride_id, uint64_t end_time_ns,
                           float distance_km, float max_speed, float avg_speed)
{
    if (!s_mounted || !ride_id) return false;

    if (s_telemetry_file) {
        fclose(s_telemetry_file);
        s_telemetry_file = NULL;
    }

    /* Read existing metadata.json, add summary, write back */
    char path[96];
    snprintf(path, sizeof(path), "%s/metadata.json", s_ride_path);

    FILE *f = fopen(path, "r");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return false; }

    fread(buf, 1, (size_t)sz, f);
    buf[sz] = '\0';
    fclose(f);

    cJSON *root = cJSON_Parse(buf);
    free(buf);
    if (!root) return false;

    time_t sec = (time_t)(end_time_ns / 1000000000ULL);
    struct tm *ti = gmtime(&sec);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", ti);
    cJSON_AddStringToObject(root, "end_time",    ts);
    cJSON_AddNumberToObject(root, "distance_km", (double)distance_km);
    cJSON_AddNumberToObject(root, "max_speed",   (double)max_speed);
    cJSON_AddNumberToObject(root, "avg_speed",   (double)avg_speed);

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!json) return false;

    f = fopen(path, "w");
    bool ok = false;
    if (f) {
        ok = (fwrite(json, 1, strlen(json), f) == strlen(json));
        fclose(f);
    }
    free(json);

    if (ok) ESP_LOGI(TAG, "Ride finalised: %s (%.2f km)", ride_id, (double)distance_km);
    return ok;
}

/* ------------------------------------------------------------------ */
bool storage_generate_gpx(const char *ride_id)
{
    /*
     * TODO: open telemetry.lp, parse line-protocol records, and call
     * gpx_start_file / gpx_add_point / gpx_finalize_file.
     */
    ESP_LOGI(TAG, "GPX generation for %s not yet implemented", ride_id);
    return false;
}

/* ------------------------------------------------------------------ */
int storage_get_ride_count(void)
{
    if (!s_mounted) return 0;

    DIR *dir = opendir("/sdcard/rides");
    if (!dir) return 0;

    int count = 0;
    struct dirent *e;
    while ((e = readdir(dir)) != NULL) {
        if (e->d_type == DT_DIR
            && e->d_name[0] != '.')   /* skip . and .. */
        {
            count++;
        }
    }
    closedir(dir);
    return count;
}

/* ------------------------------------------------------------------ */
bool storage_get_ride_id(int index, char *ride_id, size_t buf_sz)
{
    if (!s_mounted || index < 0 || !ride_id || buf_sz < 2) return false;

    DIR *dir = opendir("/sdcard/rides");
    if (!dir) return false;

    struct dirent *e;
    int cur = 0;
    bool found = false;

    while ((e = readdir(dir)) != NULL) {
        if (e->d_type == DT_DIR && e->d_name[0] != '.') {
            if (cur == index) {
                strncpy(ride_id, e->d_name, buf_sz - 1);
                ride_id[buf_sz - 1] = '\0';
                found = true;
                break;
            }
            cur++;
        }
    }
    closedir(dir);
    return found;
}
