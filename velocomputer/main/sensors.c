#include "sensors.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <string.h>

static const char *TAG = "sensors";

#define SPEED_GPIO    GPIO_NUM_4
#define CADENCE_GPIO  GPIO_NUM_5

#define DEBOUNCE_SPEED_US    20000   // 20 ms
#define DEBOUNCE_CADENCE_US  50000   // 50 ms

static float   wheel_circumference = 2.105f; // metres, 700x23c
static uint8_t cadence_magnets     = 1;

static volatile uint64_t speed_last_us    = 0;
static volatile uint64_t cadence_last_us  = 0;
static volatile float    speed_kmh        = 0.0f;
static volatile float    cadence_rpm      = 0.0f;
static volatile uint32_t speed_pulses     = 0; // raw pulse counter for debug
static volatile uint32_t cadence_pulses   = 0; // raw pulse counter for debug

// ── ISR handlers ───────────────────────────────────────────────────────────
// esp_timer_get_time() returns microseconds and is ISR-safe.

static void IRAM_ATTR speed_isr(void *arg) {
    uint64_t now   = esp_timer_get_time();
    uint64_t dt_us = now - speed_last_us;

    speed_pulses++;

    if (speed_last_us == 0) {
        speed_last_us = now; // first pulse: record timestamp, no speed yet
        return;
    }
    if (dt_us < DEBOUNCE_SPEED_US) return; // debounce

    // km/h = circumference(m) / dt(s) * 3.6
    speed_kmh     = wheel_circumference / (dt_us / 1e6f) * 3.6f;
    speed_last_us = now;
}

static void IRAM_ATTR cadence_isr(void *arg) {
    uint64_t now   = esp_timer_get_time();
    uint64_t dt_us = now - cadence_last_us;

    cadence_pulses++;

    if (cadence_last_us == 0) {
        cadence_last_us = now; // first pulse: record timestamp, no cadence yet
        return;
    }
    if (dt_us < DEBOUNCE_CADENCE_US) return; // debounce

    // RPM = 60 / (seconds between pulses * magnet count)
    cadence_rpm     = 60.0f / ((dt_us / 1e6f) * cadence_magnets);
    cadence_last_us = now;
}

// ── Stale detection ────────────────────────────────────────────────────────
// Zero out readings if no pulse received for 3 seconds (wheel stopped).

static void zero_stale_sensors(void) {
    uint64_t now = esp_timer_get_time();

    if (speed_last_us > 0 && (now - speed_last_us) > 3000000ULL) {
        speed_kmh     = 0.0f;
        speed_last_us = 0;
    }
    if (cadence_last_us > 0 && (now - cadence_last_us) > 3000000ULL) {
        cadence_rpm     = 0.0f;
        cadence_last_us = 0;
    }
}

// ── Debug print task ───────────────────────────────────────────────────────

static void sensor_debug_task(void *arg) {
    uint32_t prev_spd = 0, prev_cad = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        zero_stale_sensors();

        uint32_t spd = speed_pulses;
        uint32_t cad = cadence_pulses;

        ESP_LOGI(TAG, "Speed: %5.1f km/h | Cadence: %3.0f RPM  "
                      "[raw pulses: spd=%lu (+%lu)  cad=%lu (+%lu)]",
                 speed_kmh, cadence_rpm,
                 spd, spd - prev_spd,
                 cad, cad - prev_cad);

        prev_spd = spd;
        prev_cad = cad;
    }
}

// ── Public API ─────────────────────────────────────────────────────────────

void sensors_init(void) {
    ESP_LOGI(TAG, "Initializing sensors (GPIO interrupt mode)");
    ESP_LOGI(TAG, "Speed GPIO: %d | Cadence GPIO: %d", SPEED_GPIO, CADENCE_GPIO);
    ESP_LOGI(TAG, "Wheel circumference: %.3f m | Cadence magnets: %u",
             wheel_circumference, cadence_magnets);

    // Configure both GPIOs with internal pull-up (suits reed switches).
    // Trigger on NEGEDGE: reed switch connects pin to GND when magnet passes.
    gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL << SPEED_GPIO) | (1ULL << CADENCE_GPIO),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_cfg));

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(SPEED_GPIO,   speed_isr,   NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(CADENCE_GPIO, cadence_isr, NULL));

    xTaskCreate(sensor_debug_task, "sensor_dbg", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Sensors ready — trigger the reed switches to test");
}

bool sensors_get_data(sensor_data_t *data) {
    if (!data) return false;
    zero_stale_sensors();
    data->speed     = speed_kmh;
    data->cadence   = (uint16_t)cadence_rpm;
    data->timestamp = esp_timer_get_time() * 1000ULL; // µs → ns
    return true;
}

void sensors_set_wheel_circumference(float circumference) {
    if (circumference > 0.1f && circumference < 5.0f) {
        wheel_circumference = circumference;
        ESP_LOGI(TAG, "Wheel circumference set to %.3f m", circumference);
    }
}

void sensors_set_cadence_magnets(uint8_t magnets) {
    if (magnets >= 1 && magnets <= 8) {
        cadence_magnets = magnets;
        ESP_LOGI(TAG, "Cadence magnets set to %u", magnets);
    }
}

// Stubs so the rest of the codebase still links
void sensors_enable_temperature(bool enable) {}
void sensors_enable_humidity(bool enable)    {}
void sensors_enable_heart_rate(bool enable)  {}
void sensors_enable_power(bool enable)       {}