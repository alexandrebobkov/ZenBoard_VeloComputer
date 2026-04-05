#include "sensors.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include <sys/time.h>
#include <string.h>

static const char *TAG = "sensors";

/* ---------- Configuration ----------------------------------------- */
#define SPEED_GPIO    GPIO_NUM_4   /* reed switch – wheel */
#define CADENCE_GPIO  GPIO_NUM_5   /* reed switch – crank */

static float   s_wheel_circumference = 2.105f; /* 700×23c, metres   */
static uint8_t s_cadence_magnets     = 1;

static bool s_temp_en  = false;
static bool s_humid_en = false;
static bool s_hr_en    = false;
static bool s_power_en = false;

/* ---------- Shared sensor state ------------------------------------ */
static sensor_data_t s_current = {0};

/* Timestamps (ns) of last pulse, used to compute interval */
static volatile uint64_t s_last_speed_ns   = 0;
static volatile uint64_t s_last_cadence_ns = 0;

/* Track if we've seen at least one pulse */
static volatile bool s_speed_initialized   = false;
static volatile bool s_cadence_initialized = false;

/* ------------------------------------------------------------------ */
static uint64_t now_ns(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000000ULL
         + (uint64_t)tv.tv_usec * 1000ULL;
}

/* ------------------------------------------------------------------ */
/*  Speed ISR – called every time the wheel completes one revolution  */
/* ------------------------------------------------------------------ */
static void IRAM_ATTR speed_isr_handler(void *arg)
{
    (void)arg;

    uint64_t now = now_ns();
    UBaseType_t saved_irq_status = portSET_INTERRUPT_MASK_FROM_ISR();

    if (s_last_speed_ns > 0) {
        uint64_t dt_ns = now - s_last_speed_ns;
        float dt_s = (float)dt_ns / 1e9f;
        if (dt_s > 0.0f && dt_s < 10.0f) {  /* Ignore unrealistic gaps */
            /* speed (km/h) = circumference(m) / dt(s) * 3.6 */
            s_current.speed = (s_wheel_circumference / dt_s) * 3.6f;
        }
    }

    s_last_speed_ns  = now;
    s_current.timestamp = now;
    s_speed_initialized = true;

    portCLEAR_INTERRUPT_MASK_FROM_ISR(saved_irq_status);
}

/* ------------------------------------------------------------------ */
/*  Cadence ISR – called every time crank completes one revolution    */
/* ------------------------------------------------------------------ */
static void IRAM_ATTR cadence_isr_handler(void *arg)
{
    (void)arg;

    uint64_t now = now_ns();
    UBaseType_t saved_irq_status = portSET_INTERRUPT_MASK_FROM_ISR();

    if (s_last_cadence_ns > 0) {
        uint64_t dt_ns = now - s_last_cadence_ns;
        float dt_min = (float)dt_ns / 60e9f;
        if (dt_min > 0.0f && dt_min < 10.0f) {  /* Ignore unrealistic gaps */
            s_current.cadence = (uint16_t)(1.0f / dt_min);
        }
    }

    s_last_cadence_ns   = now;
    s_current.timestamp = now;
    s_cadence_initialized = true;

    portCLEAR_INTERRUPT_MASK_FROM_ISR(saved_irq_status);
}

/* ------------------------------------------------------------------ */
static void setup_gpio_interrupt(gpio_num_t gpio, gpio_isr_t isr_handler,
                                  const char *name)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_POSEDGE,
        .pin_bit_mask = (1ULL << gpio),
    };

    ESP_ERROR_CHECK(gpio_config(&io_conf));
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
    ESP_ERROR_CHECK(gpio_isr_handler_add(gpio, isr_handler, NULL));

    ESP_LOGI(TAG, "%s sensor configured on GPIO %d", name, gpio);
}

/* ------------------------------------------------------------------ */
void sensors_init(void)
{
    ESP_LOGI(TAG, "Initialising sensors (wheel circ=%.3f m, magnets=%u)",
             s_wheel_circumference, s_cadence_magnets);

    /*
     * Speed: interrupt on every wheel revolution (1 magnet).
     * Cadence: interrupt on every crank revolution.
     */
    setup_gpio_interrupt(SPEED_GPIO, speed_isr_handler, "Speed");
    setup_gpio_interrupt(CADENCE_GPIO, cadence_isr_handler, "Cadence");

    ESP_LOGI(TAG, "Speed sensor on GPIO %d, cadence on GPIO %d",
             SPEED_GPIO, CADENCE_GPIO);
}

/* ------------------------------------------------------------------ */
bool sensors_get_data(sensor_data_t *data)
{
    if (!data) return false;

    /* Speed goes to zero after 3 s without a pulse */
    uint64_t now = now_ns();

    taskENTER_CRITICAL(NULL);

    if (s_speed_initialized && s_last_speed_ns > 0 &&
        (now - s_last_speed_ns) > 3000000000ULL) {
        s_current.speed = 0.0f;
    }

    /* Cadence goes to zero after 5 s without a pulse */
    if (s_cadence_initialized && s_last_cadence_ns > 0 &&
        (now - s_last_cadence_ns) > 5000000000ULL) {
        s_current.cadence = 0;
    }

    memcpy(data, &s_current, sizeof(sensor_data_t));

    taskEXIT_CRITICAL(NULL);

    /* ---- Simulated optional sensors (replace with real I²C reads) ---- */
    if (s_temp_en)  data->optional.temperature = 22.5f;
    if (s_humid_en) data->optional.humidity    = 45.0f;
    if (s_hr_en)    data->optional.heart_rate  = 135;
    if (s_power_en) data->optional.power       = s_current.speed * 3.5f;

    return true;
}

/* ------------------------------------------------------------------ */
void sensors_set_wheel_circumference(float c)
{
    if (c > 0.1f && c < 5.0f) {
        s_wheel_circumference = c;
        ESP_LOGI(TAG, "Wheel circumference → %.3f m", c);
    }
}

void sensors_set_cadence_magnets(uint8_t n)
{
    if (n >= 1 && n <= 8) {
        s_cadence_magnets = n;
        ESP_LOGI(TAG, "Cadence magnets → %u", n);
    }
}

void sensors_enable_temperature(bool en) { s_temp_en  = en; }
void sensors_enable_humidity   (bool en) { s_humid_en = en; }
void sensors_enable_heart_rate (bool en) { s_hr_en    = en; }
void sensors_enable_power      (bool en) { s_power_en = en; }
