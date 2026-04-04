#include "storage.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_defs.h"
#include "cJSON.h"
#include <sys/stat.h>
#include <sys/unistd.h>
#include <dirent.h>
#include <time.h>

static const char *TAG = "storage";
static bool sd_card_mounted = false;
static char current_ride_path[64] = "/sdcard/rides/current";
static FILE* current_telemetry_file = NULL;

static void mount_sd_card() {
    if (sd_card_mounted) return;

    ESP_LOGI(TAG, "Initializing SD card");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
    slot_config.gpio_miso = GPIO_NUM_2;
    slot_config.gpio_mosi = GPIO_NUM_15;
    slot_config.gpio_sck = GPIO_NUM_14;
    slot_config.gpio_cs = GPIO_NUM_13;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set format_if_mount_failed = true.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        }
        return;
    }

    sd_card_mounted = true;
    ESP_LOGI(TAG, "SD card mounted successfully");
    ESP_LOGI(TAG, "SDMMC card info: %s", card ? "valid" : "invalid");
    ESP_LOGI(TAG, "Size: %lluMB", card ? (card->csd.capacity / (1024 * 1024)) : 0);
}

void storage_init(void) {
    ESP_LOGI(TAG, "Initializing storage system");
    mount_sd_card();

    if (sd_card_mounted) {
        // Create rides directory if it doesn't exist
        struct stat st = {0};
        if (stat("/sdcard/rides", &st) == -1) {
            mkdir("/sdcard/rides", 0777);
            ESP_LOGI(TAG, "Created rides directory");
        }
    } else {
        ESP_LOGW(TAG, "SD card not available");
    }
}

bool storage_is_available(void) {
    return sd_card_mounted;
}

bool storage_create_ride_directory(const char* ride_id) {
    if (!sd_card_mounted || !ride_id) return false;

    char path[64];
    snprintf(path, sizeof(path), "/sdcard/rides/%s", ride_id);

    if (mkdir(path, 0777) != 0) {
        ESP_LOGE(TAG, "Failed to create ride directory: %s", path);
        return false;
    }

    snprintf(current_ride_path, sizeof(current_ride_path), "/sdcard/rides/%s", ride_id);
    ESP_LOGI(TAG, "Created ride directory: %s", current_ride_path);

    // Open telemetry file
    char telemetry_path[64];
    snprintf(telemetry_path, sizeof(telemetry_path), "%s/telemetry.lp", current_ride_path);
    current_telemetry_file = fopen(telemetry_path, "w");

    if (!current_telemetry_file) {
        ESP_LOGE(TAG, "Failed to open telemetry file");
        return false;
    }

    // Write line protocol header
    fprintf(current_telemetry_file, "# Velocomputer Ride Data - InfluxDB Line Protocol\n");
    fflush(current_telemetry_file);

    return true;
}

bool storage_write_telemetry(bike_telemetry_t* data) {
    if (!sd_card_mounted || !data || !current_telemetry_file) {
        return false;
    }

    char line_protocol[256];
    int len = telemetry_to_line_protocol(data, line_protocol, sizeof(line_protocol));

    if (len <= 0) {
        ESP_LOGE(TAG, "Failed to convert telemetry to line protocol");
        return false;
    }

    if (fwrite(line_protocol, 1, len, current_telemetry_file) != len) {
        ESP_LOGE(TAG, "Failed to write telemetry data");
        return false;
    }

    fflush(current_telemetry_file);
    return true;
}

bool storage_write_metadata(const char* ride_id, const char* bicycle,
                            const char* rider, const char* ride_type,
                            uint64_t start_time) {
    if (!sd_card_mounted || !ride_id) return false;

    char metadata_path[64];
    snprintf(metadata_path, sizeof(metadata_path), "%s/metadata.json", current_ride_path);

    cJSON *root = cJSON_CreateObject();
    if (!root) return false;

    cJSON_AddStringToObject(root, "ride_id", ride_id);
    cJSON_AddStringToObject(root, "bicycle", bicycle ? bicycle : "unknown");
    cJSON_AddStringToObject(root, "rider", rider ? rider : "unknown");
    cJSON_AddStringToObject(root, "ride_type", ride_type ? ride_type : "unknown");

    // Convert timestamp to ISO 8601
    time_t sec = start_time / 1000000000;
    struct tm *timeinfo = localtime(&sec);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%SZ", timeinfo);
    cJSON_AddStringToObject(root, "start_time", time_str);

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);

    if (!json_str) return false;

    FILE *f = fopen(metadata_path, "w");
    if (!f) {
        free(json_str);
        return false;
    }

    bool result = (fwrite(json_str, 1, strlen(json_str), f) == strlen(json_str));
    fclose(f);
    free(json_str);

    return result;
}

bool storage_finalize_ride(const char* ride_id, uint64_t end_time,
                          float distance_km, float max_speed, float avg_speed) {
    if (!sd_card_mounted || !ride_id || !current_telemetry_file) {
        return false;
    }

    // Close telemetry file
    fclose(current_telemetry_file);
    current_telemetry_file = NULL;

    // Update metadata with ride summary
    char metadata_path[64];
    snprintf(metadata_path, sizeof(metadata_path), "%s/metadata.json", current_ride_path);

    FILE *f = fopen(metadata_path, "r+");
    if (!f) return false;

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(f);
        return false;
    }

    size_t read = fread(buffer, 1, size, f);
    buffer[read] = '\0';

    cJSON *root = cJSON_Parse(buffer);
    free(buffer);

    if (!root) {
        fclose(f);
        return false;
    }

    // Convert end time to ISO 8601
    time_t sec = end_time / 1000000000;
    struct tm *timeinfo = localtime(&sec);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%SZ", timeinfo);
    cJSON_AddStringToObject(root, "end_time", time_str);

    cJSON_AddNumberToObject(root, "distance_km", distance_km);
    cJSON_AddNumberToObject(root, "max_speed", max_speed);
    cJSON_AddNumberToObject(root, "avg_speed", avg_speed);

    char *json_str = cJSON_Print(root);
    cJSON_Delete(root);

    if (!json_str) {
        fclose(f);
        return false;
    }

    fseek(f, 0, SEEK_SET);
    ftruncate(fileno(f), 0);
    bool result = (fwrite(json_str, 1, strlen(json_str), f) == strlen(json_str));
    fclose(f);
    free(json_str);

    if (result) {
        ESP_LOGI(TAG, "Ride finalized: %s", ride_id);
    }

    return result;
}

bool storage_generate_gpx(const char* ride_id) {
    // This would read the telemetry.lp file and convert to GPX
    // Implementation would parse line protocol and generate GPX XML
    ESP_LOGI(TAG, "GPX generation for ride %s not yet implemented", ride_id);
    return false;
}

int storage_get_ride_count(void) {
    if (!sd_card_mounted) return 0;

    DIR *dir = opendir("/sdcard/rides");
    if (!dir) return 0;

    int count = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            count++;
        }
    }

    closedir(dir);
    return count;
}

bool storage_get_ride_id(int index, char* ride_id, size_t buffer_size) {
    if (!sd_card_mounted || index < 0 || !ride_id || buffer_size < 1) return false;

    DIR *dir = opendir("/sdcard/rides");
    if (!dir) return false;

    struct dirent *entry;
    int current = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            if (current == index) {
                strncpy(ride_id, entry->d_name, buffer_size - 1);
                ride_id[buffer_size - 1] = '\0';
                closedir(dir);
                return true;
            }
            current++;
        }
    }

    closedir(dir);
    return false;
}
