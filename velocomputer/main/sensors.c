#include "sensors.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <sys/time.h>
#include <string.h>

static const char *TAG = "sensors";

#define SPEED_GPIO    GPIO_NUM_4
#define CADENCE_GPIO  GPIO_NUM_5

#define DEBOUNCE_SPEED_MS    20
#define DEBOUNCE_CADENCE_MS  50

static float   wheel_circumference = 2.105f; // metres, 700x23c
static uint8_t cadence_magnets     = 1;

static volatile uint64_t speed_last_ns    = 0;
static volatile uint64_t cadence_last_ns  = 0;
static volatile float    speed_kmh        = 0.0f;
static volatile float    cadence_rpm      = 0.0f;

// ── Helpers ────────────────────────────────────────────────────────────────

static inline uint64_t IRAM_ATTR now_ns(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000000ULL + (uint64_t)tv.tv_usec * 1000ULL;
}

// ── ISR handlers ───────────────────────────────────────────────────────────

static void IRAM_ATTR speed_isr(void *arg) {
    uint64_t now   = now_ns();
    uint64_t dt_ns = now - speed_last_ns;

    if (speed_last_ns == 0 || dt_ns < (DEBOUNCE_SPEED_MS * 1000000ULL)) return;

    // km/h = circumference(m) / dt(s) * 3.6
    speed_kmh     = wheel_circumference / (dt_ns / 1e9f) * 3.6f;
    speed_last_ns = now;
}

static void IRAM_ATTR cadence_isr(void *arg) {
    uint64_t now   = now_ns();
    uint64_t dt_ns = now - cadence_last_ns;

    if (cadence_last_ns == 0 || dt_ns < (DEBOUNCE_CADENCE_MS * 1000000ULL)) return;

    // RPM = 60 / (seconds between pulses * magnet count)
    cadence_rpm     = 60.0f / ((dt_ns / 1e9f) * cadence_magnets);
    cadence_last_ns = now;
}

// ── Stale detection ────────────────────────────────────────────────────────
// Zero out readings if no pulse received for 3 seconds (wheel stopped).

static void zero_stale_sensors(void) {
    uint64_t now = now_ns();

    if (speed_last_ns > 0 && (now - speed_last_ns) > 3000000000ULL) {
        speed_kmh     = 0.0f;
        speed_last_ns = 0;
    }
    if (cadence_last_ns > 0 && (now - cadence_last_ns) > 3000000000ULL) {
        cadence_rpm     = 0.0f;
        cadence_last_ns = 0;
    }
}

// ── Debug print task ───────────────────────────────────────────────────────

static void sensor_debug_task(void *arg) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        zero_stale_sensors();
        ESP_LOGI(TAG, "Speed: %.1f km/h | Cadence: %.0f RPM", speed_kmh, cadence_rpm);
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

    xTaskCreate(sensor_debug_task, "sensor_dbg", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "Sensors ready — trigger the reed switches to test");
}

bool sensors_get_data(sensor_data_t *data) {
    if (!data) return false;
    zero_stale_sensors();
    data->speed     = speed_kmh;
    data->cadence   = (uint16_t)cadence_rpm;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    data->timestamp = (uint64_t)tv.tv_sec * 1000000000ULL + (uint64_t)tv.tv_usec * 1000ULL;
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