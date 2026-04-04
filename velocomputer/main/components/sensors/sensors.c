#include "sensors.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/pcnt.h"

static const char *TAG = "sensors";

// Configuration
static float wheel_circumference = 2.105; // Default: 700x23c road tire (~2.1m)
static uint8_t cadence_magnets = 1;      // Default: 1 magnet
static bool temp_enabled = false;
static bool humid_enabled = false;
static bool hr_enabled = false;
static bool power_enabled = false;

// Simulated sensor data
static sensor_data_t current_data = {0};

// Pulse counter for speed sensor
static pcnt_unit_t speed_unit = PCNT_UNIT_0;
static int16_t speed_pulse_count = 0;
static uint64_t last_speed_time = 0;

// Pulse counter for cadence sensor
static pcnt_unit_t cadence_unit = PCNT_UNIT_1;
static int16_t cadence_pulse_count = 0;
static uint64_t last_cadence_time = 0;

static void IRAM_ATTR speed_isr_handler(void* arg) {
    // Handle speed sensor interrupt
    uint32_t status;
    pcnt_get_event_status(speed_unit, &status);

    if (status & PCNT_EVT_THRES_1) {
        // Threshold event - we have a complete revolution
        struct timeval tv;
        gettimeofday(&tv, NULL);
        uint64_t now = (uint64_t)tv.tv_sec * 1000000000 + tv.tv_usec * 1000;

        if (last_speed_time > 0) {
            // Calculate speed in km/h
            float time_diff_s = (now - last_speed_time) / 1000000000.0;
            if (time_diff_s > 0) {
                current_data.speed = (wheel_circumference / 1000.0) / time_diff_s * 3.6; // km/h
            }
        }

        last_speed_time = now;
        current_data.timestamp = now;
    }

    pcnt_clear_event_status(speed_unit, status);
}

static void IRAM_ATTR cadence_isr_handler(void* arg) {
    // Handle cadence sensor interrupt
    uint32_t status;
    pcnt_get_event_status(cadence_unit, &status);

    if (status & PCNT_EVT_THRES_1) {
        // Threshold event - we have cadence_magnets pulses
        struct timeval tv;
        gettimeofday(&tv, NULL);
        uint64_t now = (uint64_t)tv.tv_sec * 1000000000 + tv.tv_usec * 1000;

        static uint64_t last_cadence_revolution = 0;
        static uint16_t revolution_count = 0;

        revolution_count++;

        if (last_cadence_revolution > 0 && revolution_count >= cadence_magnets) {
            float time_diff_min = (now - last_cadence_revolution) / 60000000000.0;
            if (time_diff_min > 0) {
                current_data.cadence = (float)revolution_count / time_diff_min;
            }
            revolution_count = 0;
            last_cadence_revolution = now;
        } else if (last_cadence_revolution == 0) {
            last_cadence_revolution = now;
        }

        current_data.timestamp = now;
    }

    pcnt_clear_event_status(cadence_unit, status);
}

void sensors_init(void) {
    ESP_LOGI(TAG, "Initializing sensor system");

    // Initialize pulse counters for speed and cadence
    pcnt_config_t pcnt_config = {
        .pulse_gpio_num = GPIO_NUM_4, // Speed sensor GPIO
        .ctrl_gpio_num = PCNT_PIN_NOT_USED,
        .lctrl_mode = PCNT_MODE_REVERSE,
        .hctrl_mode = PCNT_MODE_KEEP,
        .pos_mode = PCNT_COUNT_INC,
        .neg_mode = PCNT_COUNT_DIS,
        .counter_h_lim = 1000,
        .counter_l_lim = -1000,
        .unit = speed_unit,
        .channel = PCNT_CHANNEL_0,
    };

    pcnt_unit_config(&pcnt_config);
    pcnt_set_event_value(speed_unit, PCNT_EVT_THRES_1, 1); // Trigger on each pulse
    pcnt_event_enable(speed_unit, PCNT_EVT_THRES_1);
    pcnt_isr_register(speed_isr_handler, NULL, 0, NULL);
    pcnt_intr_enable(speed_unit);
    pcnt_counter_clear(speed_unit);
    pcnt_counter_pause(speed_unit);
    pcnt_counter_resume(speed_unit);

    // Cadence sensor setup
    pcnt_config.pulse_gpio_num = GPIO_NUM_5; // Cadence sensor GPIO
    pcnt_config.unit = cadence_unit;
    pcnt_config.channel = PCNT_CHANNEL_0;
    pcnt_unit_config(&pcnt_config);
    pcnt_set_event_value(cadence_unit, PCNT_EVT_THRES_1, cadence_magnets);
    pcnt_event_enable(cadence_unit, PCNT_EVT_THRES_1);
    pcnt_isr_register(cadence_isr_handler, NULL, 0, NULL);
    pcnt_intr_enable(cadence_unit);
    pcnt_counter_clear(cadence_unit);
    pcnt_counter_pause(cadence_unit);
    pcnt_counter_resume(cadence_unit);

    ESP_LOGI(TAG, "Speed and cadence sensors initialized");
    ESP_LOGI(TAG, "Wheel circumference: %.3fm, Cadence magnets: %u",
             wheel_circumference, cadence_magnets);
}

bool sensors_get_data(sensor_data_t* data) {
    if (!data) return false;

    // Get current sensor readings
    memcpy(data, &current_data, sizeof(sensor_data_t));

    // Simulate some optional sensor data if enabled
    if (temp_enabled) {
        data->optional.temperature = 22.5; // Simulated temperature
    }

    if (humid_enabled) {
        data->optional.humidity = 45.0; // Simulated humidity
    }

    if (hr_enabled) {
        data->optional.heart_rate = 120; // Simulated heart rate
    }

    if (power_enabled) {
        data->optional.power = data->speed * 3.0; // Rough power estimate
    }

    return true;
}

void sensors_set_wheel_circumference(float circumference) {
    if (circumference > 0.1 && circumference < 5.0) {
        wheel_circumference = circumference;
        ESP_LOGI(TAG, "Wheel circumference set to %.3fm", circumference);
    }
}

void sensors_set_cadence_magnets(uint8_t magnets) {
    if (magnets >= 1 && magnets <= 8) {
        cadence_magnets = magnets;
        ESP_LOGI(TAG, "Cadence magnets set to %u", magnets);
    }
}

void sensors_enable_temperature(bool enable) {
    temp_enabled = enable;
    ESP_LOGI(TAG, "Temperature sensor %s", enable ? "enabled" : "disabled");
}

void sensors_enable_humidity(bool enable) {
    humid_enabled = enable;
    ESP_LOGI(TAG, "Humidity sensor %s", enable ? "enabled" : "disabled");
}

void sensors_enable_heart_rate(bool enable) {
    hr_enabled = enable;
    ESP_LOGI(TAG, "Heart rate sensor %s", enable ? "enabled" : "disabled");
}

void sensors_enable_power(bool enable) {
    power_enabled = enable;
    ESP_LOGI(TAG, "Power sensor %s", enable ? "enabled" : "disabled");
}
