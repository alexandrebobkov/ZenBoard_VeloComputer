#include "sensors.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
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

/* ---------- PCNT handles ------------------------------------------ */
static pcnt_unit_handle_t s_speed_unit   = NULL;
static pcnt_unit_handle_t s_cadence_unit = NULL;

/* Timestamps (ns) of last pulse, used to compute interval */
static volatile uint64_t s_last_speed_ns   = 0;
static volatile uint64_t s_last_cadence_ns = 0;

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
static bool IRAM_ATTR speed_on_reach(pcnt_unit_handle_t unit,
                                     const pcnt_watch_event_data_t *edata,
                                     void *user_ctx)
{
    (void)unit; (void)edata; (void)user_ctx;

    uint64_t now = now_ns();

    if (s_last_speed_ns > 0) {
        float dt_s = (float)(now - s_last_speed_ns) / 1e9f;
        if (dt_s > 0.0f) {
            /* speed (km/h) = circumference(m) / dt(s)  * 3.6 */
            s_current.speed = (s_wheel_circumference / dt_s) * 3.6f;
        }
    }

    s_last_speed_ns  = now;
    s_current.timestamp = now;

    return false; /* no high-priority task wake-up needed */
}

/* ------------------------------------------------------------------ */
/*  Cadence ISR – called every time crank completes one revolution    */
/* ------------------------------------------------------------------ */
static bool IRAM_ATTR cadence_on_reach(pcnt_unit_handle_t unit,
                                       const pcnt_watch_event_data_t *edata,
                                       void *user_ctx)
{
    (void)unit; (void)edata; (void)user_ctx;

    uint64_t now = now_ns();

    if (s_last_cadence_ns > 0) {
        float dt_min = (float)(now - s_last_cadence_ns) / 60e9f;
        if (dt_min > 0.0f) {
            s_current.cadence = (uint16_t)(1.0f / dt_min);
        }
    }

    s_last_cadence_ns   = now;
    s_current.timestamp = now;

    return false;
}

/* ------------------------------------------------------------------ */
static pcnt_unit_handle_t create_pcnt_unit(gpio_num_t gpio,
                                           pcnt_watch_cb_t cb,
                                           int watch_point)
{
    /* Unit config: count rising edges, limit ±10000 */
    pcnt_unit_config_t unit_cfg = {
        .low_limit  = -10000,
        .high_limit =  10000,
    };
    pcnt_unit_handle_t unit = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_cfg, &unit));

    /* Channel: count rising edges on the reed-switch GPIO */
    pcnt_chan_config_t chan_cfg = {
        .edge_gpio_num  = gpio,
        .level_gpio_num = PCNT_GPIO_NOT_USED,
    };
    pcnt_channel_handle_t chan = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(unit, &chan_cfg, &chan));

    /* Increment on rising edge, ignore falling edge */
    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        chan,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,  /* rising  */
        PCNT_CHANNEL_EDGE_ACTION_HOLD));    /* falling */

    /* Watch point fires the ISR every `watch_point` pulses */
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(unit, watch_point));

    /* Register ISR callback via the v5.x callbacks struct */
    pcnt_event_callbacks_t cbs = {
        .on_reach = cb,
    };
    ESP_ERROR_CHECK(pcnt_unit_register_event_callbacks(unit, &cbs, NULL));

    /* Enable and start counting */
    ESP_ERROR_CHECK(pcnt_unit_enable(unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(unit));
    ESP_ERROR_CHECK(pcnt_unit_start(unit));

    return unit;
}

/* ------------------------------------------------------------------ */
void sensors_init(void)
{
    ESP_LOGI(TAG, "Initialising sensors (wheel circ=%.3f m, magnets=%u)",
             s_wheel_circumference, s_cadence_magnets);

    /*
     * Speed: fire ISR on every wheel revolution (1 magnet → watch_point = 1).
     * Cadence: fire ISR every s_cadence_magnets pulses.
     */
    s_speed_unit   = create_pcnt_unit(SPEED_GPIO,   speed_on_reach,   1);
    s_cadence_unit = create_pcnt_unit(CADENCE_GPIO, cadence_on_reach,
                                      s_cadence_magnets);

    ESP_LOGI(TAG, "Speed sensor on GPIO %d, cadence on GPIO %d",
             SPEED_GPIO, CADENCE_GPIO);
}

/* ------------------------------------------------------------------ */
bool sensors_get_data(sensor_data_t *data)
{
    if (!data) return false;

    /* Speed goes to zero after 3 s without a pulse */
    uint64_t now = now_ns();
    if (s_last_speed_ns > 0 && (now - s_last_speed_ns) > 3000000000ULL) {
        s_current.speed = 0.0f;
    }

    memcpy(data, &s_current, sizeof(sensor_data_t));

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
