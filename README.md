# ZenBoard VeloComputer

Bicycle computer powered by ESP32-C3 microcontroller to measure distance, speed and cadence telemetry.

The Bucycle Velocomputer is a compact, ESP32‑C3‑powered bicycle computer designed to capture and analyze real‑time ride telemetry. It measures distance, speed, and cadence using dedicated reed switch sensors: speed on GPIO4 and cadence on GPIO5, providing accurate mechanical pulse detection. GPS tracking enables route recording with automatic GPX file generation, while all ride data is logged to an SD card using the FATFS filesystem for easy retrieval. The system also supports transmission of telemetry over Wi‑Fi, allowing live data streaming or synchronization with external dashboards. Configuration parameters are managed through a persistent storage layer, ensuring user settings remain intact across power cycles.

## Features

- **Speed measurement** via reed switch sensor on GPIO4
- **Cadence measurement** via reed switch sensor on GPIO5
- **GPS tracking** with GPX file generation
- **Data logging** to SD card (FATFS filesystem)
- **Telemetry transmission** over WiFi
- **Configuration management** with persistent storage

## Hardware Requirements

- ESP32-C3 development board
- Reed switch sensors (x2) with pull-up resistors
- SD card module (SPI/MMC interface)
- GPS module (UART)
- OLED/LCD display (optional, via I2C/SPI)

## GPIO Pin Assignments

| Component | GPIO | Notes |
|-----------|------|-------|
| Speed sensor (reed switch) | GPIO4 | PCNT alternative: uses GPIO interrupt |
| Cadence sensor (reed switch) | GPIO5 | PCNT alternative: uses GPIO interrupt |
| Display SDA | GPIO7 | I2C Data GPIO |
| Display SCL | GPIO10 | I2C Clock GPIO |
| SD Card CS | GPIO13 | SPI chip select |
| SD Card CLK | GPI14 | SPI clock |
| SD Card MOSI | GPIO15 | SPI data out |
| SD Card MISO | GPIO2 | SPI data in |
| GPS TX | GPIO10 | UART Rx |
| GPS RX | GPIO11 | UART Tx |

## Project Structure

```raw
velocomputer/
├── main/
│   ├── main.c        # Application entry point
│   ├── sensors.c/h   # Speed/cadence sensors (GPIO interrupt-based)
│   ├── config.c/h    # Configuration management
│   ├── telemetry.c/h # Data transmission
│   ├── gps.c/h       # GPS handling
│   ├── storage.c/h   # Data logging to SD card
│   ├── network.c/h   # WiFi/Network handling
│   └── gpx.c/h       # GPX file generation
├── CMakeLists.txt
└── sdkconfig.defaults
```

## Build Instructions

### Prerequisites

ESP-IDF v5.5.1 installed at `/opt/esp/esp-idf/v5.5.1`

### Setup

```bash
# Initialize ESP-IDF environment
source /opt/esp/esp-idf/v5.5.1/export.sh

# Set target to ESP32-C3
idf.py set-target esp32-c3

# Configure (optional)
idf.py menuconfig
```

### Build and Flash

```bash
# Clean build
rm -rf build && idf.py build

# Flash and monitor
idf.py flash monitor
```

## License

MIT
