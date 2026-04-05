# ZenBoard VeloComputer

Bicycle computer powered by ESP32-C3 microcontroller to measure distance, speed and cadence telemetry.

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
| Speed sensor (reed switch) | GPIO4 | External interrupt |
| Cadence sensor (reed switch) | GPIO5 | External interrupt |
| SD Card CS | GPIO6 (13) | SPI chip select |
| SD Card CLK | GPIO7 (14) | SPI clock |
| SD Card MOSI | GPIO8 (15) | SPI data out |
| SD Card MISO | GPIO9 (2) | SPI data in |
| GPS TX | GPIO10 | UART RX |
| GPS RX | GPIO11 | UART TX |

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