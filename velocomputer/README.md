# Velocomputer - ESP32-C3 Bicycle Computer

A modular bicycle computer firmware for ESP32-C3 using ESP-IDF v5.4 that tracks speed, cadence, GPS location, and uploads data to InfluxDB.

## Features

- **Speed & Cadence Measurement**: Using pulse counters for accurate readings
- **GPS Tracking**: UBlox/MTK GPS module support
- **Data Logging**: Stores ride data in InfluxDB line protocol format on microSD card
- **WiFi Upload**: Automatically uploads ride data to InfluxDB
- **GPX Export**: Generates GPX files compatible with Strava and other platforms
- **Multi-Bicycle Support**: Track multiple bicycles with different configurations
- **Multi-Rider Support**: Support for multiple riders using the same device
- **Ride Categorization**: Classify rides as commute, workout, race, or leisure
- **Modular Design**: Easy to add/remove features

## Hardware Requirements

- ESP32-C3 development board
- microSD card module
- GPS module (UBlox NEO-6M/7M/8M or similar)
- Speed sensor (reed switch or hall effect)
- Cadence sensor (reed switch or hall effect)
- Optional: Temperature/humidity sensor (BME280, DHT22, etc.)
- Optional: Heart rate monitor (ANT+/Bluetooth)
- Optional: Power meter

## Software Architecture

```
velocomputer/
├── main/
│   ├── components/
│   │   ├── core/          # Core telemetry and configuration
│   │   ├── gps/           # GPS module interface
│   │   ├── sensors/       # Speed, cadence, and optional sensors
│   │   ├── storage/       # SD card storage and file management
│   │   ├── network/       # WiFi and InfluxDB upload
│   │   └── gpx/           # GPX file generation
│   └── main.c             # Main application
├── CMakeLists.txt
├── sdkconfig.defaults
└── README.md
```

## Data Format

### InfluxDB Line Protocol

Ride data is stored in InfluxDB line protocol format:

```
bike_telemetry,device=esp32-c3-abc,bicycle=TrekDomane,rider=Alex,ride_type=workout,ride_id=a1b2c3d4-... \
    speed=25.3,cadence=87,lat=40.7128,lon=-74.0060,gps_quality=8,battery=85,duration=1234 \
    1640995200000000000
```

### File Structure on SD Card

```
/sdcard/rides/
├── {ride_id}/
│   ├── metadata.json      # Ride metadata in JSON
│   ├── telemetry.lp       # Telemetry data in line protocol
│   └── ride.gpx           # GPX file for Strava (generated)
```

## Configuration

The system uses NVS (Non-Volatile Storage) to store configuration:

- **Device Configuration**: Device ID, default bicycle, default rider
- **WiFi Configuration**: SSID, password
- **InfluxDB Configuration**: URL, port, database, credentials
- **Sensor Configuration**: Wheel circumference, cadence magnets, enabled optional sensors

## Building and Flashing

1. Set up ESP-IDF v5.4 development environment
2. Connect your ESP32-C3 board
3. Configure the project:

```bash
idf.py set-target esp32c3
idf.py menuconfig
```

4. Build and flash:

```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Usage

1. **Initial Setup**:
   - Configure WiFi credentials
   - Set up InfluxDB connection details
   - Configure bicycle settings (wheel size, etc.)

2. **Starting a Ride**:
   - Power on the device
   - Select bicycle and rider (if different from defaults)
   - Select ride type
   - Press start to begin recording

3. **During Ride**:
   - Data is automatically logged to SD card
   - Real-time display of speed, cadence, distance

4. **After Ride**:
   - Device automatically finalizes ride data
   - When WiFi is available, data uploads to InfluxDB
   - GPX file is generated for Strava upload

## Customization

### Adding New Sensors

1. Add sensor reading to `sensor_data_t` in `components/sensors/sensors.h`
2. Implement sensor driver in `sensors.c`
3. Add configuration option in `config.h`
4. Update telemetry structure in `core/telemetry.h`
5. Add to line protocol conversion in `telemetry.c`

### Modifying Data Format

Edit the `telemetry_to_line_protocol()` function to change the InfluxDB format.

## Future Enhancements

- ANT+ sensor support
- Bluetooth sensor support
- OTA firmware updates
- Mobile app for configuration and ride start/stop
- Real-time display (e-ink or OLED)
- Power management for long rides
- Automatic ride detection

## License

MIT License - see LICENSE file for details.

## Contributing

Pull requests are welcome! Please follow the existing code style and add tests for new features.
