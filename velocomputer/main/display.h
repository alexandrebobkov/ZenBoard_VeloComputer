#pragma once

#include <stdbool.h>

// Initialize the SSD1306 display (I2C)
void display_init(void);

// Show speed as large text on screen
void display_show_speed(float speed_kmh);

// Clear the display
void display_clear(void);

// Start the display update task
void display_start_task(void);