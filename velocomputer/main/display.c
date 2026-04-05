#include "display.h"
#include "sensors.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "display";

// ── I2C / SSD1306 config ───────────────────────────────────────────────────
#define I2C_PORT        I2C_NUM_0
#define SDA_GPIO        GPIO_NUM_7
#define SCL_GPIO        GPIO_NUM_10
#define I2C_FREQ_HZ     400000
#define OLED_ADDR       0x3C

#define OLED_WIDTH      128
#define OLED_HEIGHT     32
#define OLED_PAGES      (OLED_HEIGHT / 8)   // 4 pages for 32px height

// ── Framebuffer ────────────────────────────────────────────────────────────
static uint8_t framebuf[OLED_PAGES][OLED_WIDTH];

// ── 6×8 font (printable ASCII starting at 0x20) ────────────────────────────
// Each character is 6 bytes wide, 8 bits tall. Last column is always 0 (gap).
static const uint8_t font6x8[][6] = {
    {0x00,0x00,0x00,0x00,0x00,0x00}, // ' '
    {0x00,0x00,0x5F,0x00,0x00,0x00}, // '!'
    {0x00,0x07,0x00,0x07,0x00,0x00}, // '"'
    {0x14,0x7F,0x14,0x7F,0x14,0x00}, // '#'
    {0x24,0x2A,0x7F,0x2A,0x12,0x00}, // '$'
    {0x23,0x13,0x08,0x64,0x62,0x00}, // '%'
    {0x36,0x49,0x55,0x22,0x50,0x00}, // '&'
    {0x00,0x05,0x03,0x00,0x00,0x00}, // '''
    {0x00,0x1C,0x22,0x41,0x00,0x00}, // '('
    {0x00,0x41,0x22,0x1C,0x00,0x00}, // ')'
    {0x08,0x2A,0x1C,0x2A,0x08,0x00}, // '*'
    {0x08,0x08,0x3E,0x08,0x08,0x00}, // '+'
    {0x00,0x50,0x30,0x00,0x00,0x00}, // ','
    {0x08,0x08,0x08,0x08,0x08,0x00}, // '-'
    {0x00,0x60,0x60,0x00,0x00,0x00}, // '.'
    {0x20,0x10,0x08,0x04,0x02,0x00}, // '/'
    {0x3E,0x51,0x49,0x45,0x3E,0x00}, // '0'
    {0x00,0x42,0x7F,0x40,0x00,0x00}, // '1'
    {0x42,0x61,0x51,0x49,0x46,0x00}, // '2'
    {0x21,0x41,0x45,0x4B,0x31,0x00}, // '3'
    {0x18,0x14,0x12,0x7F,0x10,0x00}, // '4'
    {0x27,0x45,0x45,0x45,0x39,0x00}, // '5'
    {0x3C,0x4A,0x49,0x49,0x30,0x00}, // '6'
    {0x01,0x71,0x09,0x05,0x03,0x00}, // '7'
    {0x36,0x49,0x49,0x49,0x36,0x00}, // '8'
    {0x06,0x49,0x49,0x29,0x1E,0x00}, // '9'
    {0x00,0x36,0x36,0x00,0x00,0x00}, // ':'
    {0x00,0x56,0x36,0x00,0x00,0x00}, // ';'
    {0x08,0x14,0x22,0x41,0x00,0x00}, // '<'
    {0x14,0x14,0x14,0x14,0x14,0x00}, // '='
    {0x41,0x22,0x14,0x08,0x00,0x00}, // '>'
    {0x02,0x01,0x51,0x09,0x06,0x00}, // '?'
    {0x32,0x49,0x79,0x41,0x3E,0x00}, // '@'
    {0x7E,0x09,0x09,0x09,0x7E,0x00}, // 'A'
    {0x7F,0x49,0x49,0x49,0x36,0x00}, // 'B'
    {0x3E,0x41,0x41,0x41,0x22,0x00}, // 'C'
    {0x7F,0x41,0x41,0x22,0x1C,0x00}, // 'D'
    {0x7F,0x49,0x49,0x49,0x41,0x00}, // 'E'
    {0x7F,0x09,0x09,0x09,0x01,0x00}, // 'F'
    {0x3E,0x41,0x49,0x49,0x7A,0x00}, // 'G'
    {0x7F,0x08,0x08,0x08,0x7F,0x00}, // 'H'
    {0x00,0x41,0x7F,0x41,0x00,0x00}, // 'I'
    {0x20,0x40,0x41,0x3F,0x01,0x00}, // 'J'
    {0x7F,0x08,0x14,0x22,0x41,0x00}, // 'K'
    {0x7F,0x40,0x40,0x40,0x40,0x00}, // 'L'
    {0x7F,0x02,0x04,0x02,0x7F,0x00}, // 'M'
    {0x7F,0x04,0x08,0x10,0x7F,0x00}, // 'N'
    {0x3E,0x41,0x41,0x41,0x3E,0x00}, // 'O'
    {0x7F,0x09,0x09,0x09,0x06,0x00}, // 'P'
    {0x3E,0x41,0x51,0x21,0x5E,0x00}, // 'Q'
    {0x7F,0x09,0x19,0x29,0x46,0x00}, // 'R'
    {0x46,0x49,0x49,0x49,0x31,0x00}, // 'S'
    {0x01,0x01,0x7F,0x01,0x01,0x00}, // 'T'
    {0x3F,0x40,0x40,0x40,0x3F,0x00}, // 'U'
    {0x1F,0x20,0x40,0x20,0x1F,0x00}, // 'V'
    {0x3F,0x40,0x38,0x40,0x3F,0x00}, // 'W'
    {0x63,0x14,0x08,0x14,0x63,0x00}, // 'X'
    {0x07,0x08,0x70,0x08,0x07,0x00}, // 'Y'
    {0x61,0x51,0x49,0x45,0x43,0x00}, // 'Z'
};

// ── Low-level I2C helpers ──────────────────────────────────────────────────

static esp_err_t oled_write_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd}; // 0x00 = command mode
    i2c_cmd_handle_t h = i2c_cmd_link_create();
    i2c_master_start(h);
    i2c_master_write_byte(h, (OLED_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(h, buf, sizeof(buf), true);
    i2c_master_stop(h);
    esp_err_t err = i2c_master_cmd_begin(I2C_PORT, h, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(h);
    return err;
}

static esp_err_t oled_write_data(const uint8_t *data, size_t len) {
    i2c_cmd_handle_t h = i2c_cmd_link_create();
    i2c_master_start(h);
    i2c_master_write_byte(h, (OLED_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(h, 0x40, true); // 0x40 = data mode
    i2c_master_write(h, data, len, true);
    i2c_master_stop(h);
    esp_err_t err = i2c_master_cmd_begin(I2C_PORT, h, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(h);
    return err;
}

// ── SSD1306 initialisation sequence ───────────────────────────────────────

static void oled_init_sequence(void) {
    const uint8_t cmds[] = {
        0xAE,       // display off
        0xD5, 0x80, // clock divide / oscillator
        0xA8, 0x1F, // multiplex ratio (32-1 = 31 for 128x32)
        0xD3, 0x00, // display offset
        0x40,       // start line 0
        0x8D, 0x14, // charge pump on
        0x20, 0x00, // horizontal addressing mode
        0xA1,       // segment remap (col 127 = SEG0)
        0xC8,       // COM scan direction reversed
        0xDA, 0x02, // COM pins config for 128x32
        0x81, 0x8F, // contrast
        0xD9, 0xF1, // pre-charge period
        0xDB, 0x40, // VCOMH deselect level
        0xA4,       // entire display on (follow RAM)
        0xA6,       // normal display (not inverted)
        0xAF,       // display on
    };
    for (size_t i = 0; i < sizeof(cmds); i++) {
        oled_write_cmd(cmds[i]);
    }
}

// ── Framebuffer → display ──────────────────────────────────────────────────

static void oled_flush(void) {
    // Set column and page range to full display
    oled_write_cmd(0x21); oled_write_cmd(0); oled_write_cmd(127); // columns 0-127
    oled_write_cmd(0x22); oled_write_cmd(0); oled_write_cmd(3);   // pages 0-3

    // Send all 4 pages in one shot
    oled_write_data((uint8_t *)framebuf, sizeof(framebuf));
}

// ── Drawing primitives ─────────────────────────────────────────────────────

static void fb_clear(void) {
    memset(framebuf, 0, sizeof(framebuf));
}

// Draw one character from font6x8, scaled 2× (12×16 px per char).
// x, y are in pixels; y must be page-aligned (0, 8, 16, 24).
static void fb_draw_char_2x(int x, int y, char c) {
    if (c < 0x20 || c > 0x5A) c = ' ';
    const uint8_t *glyph = font6x8[c - 0x20];
    int page = y / 8;

    for (int col = 0; col < 5; col++) {
        uint8_t bits = glyph[col];

        // Scale each source column into 2 destination columns
        for (int dx = 0; dx < 2; dx++) {
            int px = x + col * 2 + dx;
            if (px >= OLED_WIDTH) break;

            // Scale 8-bit column vertically into two 8-bit pages
            uint8_t lo = 0, hi = 0;
            for (int bit = 0; bit < 8; bit++) {
                if (bits & (1 << bit)) {
                    // Each source bit → 2 dest bits
                    lo |= (3 << (bit * 2)) & 0xFF;        // lower page bits
                    hi |= ((3 << (bit * 2)) >> 8) & 0xFF; // upper page bits
                }
            }

            if (page < OLED_PAGES)     framebuf[page][px]     |= lo;
            if (page + 1 < OLED_PAGES) framebuf[page + 1][px] |= hi;
        }
    }
}

static void fb_draw_string_2x(int x, int y, const char *str) {
    while (*str) {
        fb_draw_char_2x(x, y, *str++);
        x += 12; // 6 cols * 2x scale
        if (x >= OLED_WIDTH) break;
    }
}

// ── Public API ─────────────────────────────────────────────────────────────

void display_init(void) {
    ESP_LOGI(TAG, "Initializing SSD1306 128x32 on SDA=GPIO%d SCL=GPIO%d",
             SDA_GPIO, SCL_GPIO);

    i2c_config_t cfg = {
        .mode             = I2C_MODE_MASTER,
        .sda_io_num       = SDA_GPIO,
        .scl_io_num       = SCL_GPIO,
        .sda_pullup_en    = GPIO_PULLUP_ENABLE,
        .scl_pullup_en    = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_param_config(I2C_PORT, &cfg));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_PORT, I2C_MODE_MASTER, 0, 0, 0));

    oled_init_sequence();
    fb_clear();
    oled_flush();

    ESP_LOGI(TAG, "Display ready");
}

void display_clear(void) {
    fb_clear();
    oled_flush();
}

void display_show_speed(float speed_kmh) {
    char line1[16], line2[8];

    // Line 1 (top, y=0): speed value large
    if (speed_kmh < 0.5f) {
        snprintf(line1, sizeof(line1), "  0.0");
    } else {
        snprintf(line1, sizeof(line1), "%5.1f", speed_kmh);
    }

    // Line 2 (bottom, y=16): unit label, right-aligned
    snprintf(line2, sizeof(line2), " KM/H");

    fb_clear();
    fb_draw_string_2x(8,  0, line1);  // speed number, 2x scaled (12px tall × 2 = 24px... fits in 32)
    fb_draw_string_2x(8, 16, line2);  // unit on second row
    oled_flush();
}

// ── Display update task ────────────────────────────────────────────────────

static void display_task(void *arg) {
    sensor_data_t data;
    while (1) {
        sensors_get_data(&data);
        display_show_speed(data.speed);
        vTaskDelay(pdMS_TO_TICKS(500)); // refresh at 2 Hz
    }
}

void display_start_task(void) {
    xTaskCreate(display_task, "display", 8192, NULL, 4, NULL);
}