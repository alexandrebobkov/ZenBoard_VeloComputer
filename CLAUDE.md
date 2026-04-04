# Velocomputer Project Guidelines

## Environment
- **ESP-IDF Version**: v5.5.1 (use `/opt/esp/esp-idf/v5.5.1/tools/idf.py`)
- **Target**: ESP32-C3 (does NOT have PCNT peripheral!)
- **Build command**: `idf.py build`

Command used to initialize esp-idf v5.5.1 environment:
'source /opt/esp/esp-idf/v5.5.1/export.sh'


## Hardware Configuration

### GPIO Assignments
| Component | GPIO | Notes |
|-----------|------|-------|
| Speed sensor (reed switch) | GPIO4 | PCNT alternative: use GPIO interrupt |
| Cadence sensor (reed switch) | GPIO5 | PCNT alternative: use GPIO interrupt |
| SD Card | (see sdmmc config) | FATFS filesystem |

### Important Hardware Limitations
- **ESP32-C3 does NOT support PCNT (Pulse Counter) peripheral**
- Use GPIO interrupts + timer for pulse counting instead
- Do NOT add `esp_driver_pcnt` to CMakeLists.txt - it won't work!

## Code Style
- Use GPIO interrupt handlers with `portSET_INTERRUPT_MASK_FROM_ISR()` for ESP32-C3
- Use `taskENTER_CRITICAL(NULL)` / `taskEXIT_CRITICAL(NULL)` for main thread critical sections
- Prefer `IRAM_ATTR` for ISR handlers

## Code Organization
- Groups of related variables must be stores using struct
- Each feature/module must be implemented in its own `.c` and `.h` file pair (e.g., `sensors.h` and `sensors.c`)
- Header files contain public function declaration and types
- Source files contain implementations and private helpers (static functions)

## Project Structure
```
velocomputer/
├── main/
│   ├── main.c        # Application entry point
│   ├── sensors.c     # Speed/cadence sensors (GPIO interrupt-based)
│   ├── config.c      # Configuration management
│   ├── telemetry.c   # Data transmission
│   ├── gps.c         # GPS handling
│   ├── storage.c     # Data logging to SD card
│   ├── network.c     # WiFi/Network handling
│   └── gpx.c         # GPX file generation
├── CMakeLists.txt
└── sdkconfig.defaults
```

## Common Tasks
- Initialize esp-idf v5.5.1 environment: `source /opt/esp/esp-idf/v5.5.1/export.sh`
- Set micro-controller: `idf.py set-target esp32-c3`
- Clean build: `rm -rf build && idf.py build`
- Menuconfig: `idf.py menuconfig`
- Flash: `idf.py flash monitor`
- Git add and commit: `git add . && git commit -m '.'`
- Git push: `git push`
