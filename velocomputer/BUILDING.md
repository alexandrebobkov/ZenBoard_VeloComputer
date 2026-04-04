# Building the Velocomputer Project

## Prerequisites

Before building this project, you need to:

1. **Install ESP-IDF v5.4**: Follow the official installation guide
   - https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32c3/get-started/index.html

2. **Set up the environment**:
   ```bash
   # For Linux/macOS
   . $HOME/esp/esp-idf/export.sh
   
   # For Windows (Command Prompt)
   %userprofile%\esp\esp-idf\export.bat
   ```

3. **Install required tools**:
   - Python 3.8+
   - CMake 3.16+
   - Ninja build system
   - Git
   - dfu-util (for flashing)

## Project Setup

1. **Navigate to the project directory**:
   ```bash
   cd /home/alex/Gitea/ZenBoard_Velocomputer/velocomputer
   ```

2. **Set the target**:
   ```bash
   idf.py set-target esp32c3
   ```

3. **Configure the project**:
   ```bash
   idf.py menuconfig
   ```
   
   In menuconfig:
   - Go to "Component config" → "WiFi" and configure your WiFi settings
   - Go to "Component config" → "Storage" and configure SD card settings
   - Save and exit

## Building

```bash
idf.py build
```

## Flashing

Connect your ESP32-C3 board and flash:

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Replace `/dev/ttyUSB0` with your actual serial port.

## Common Issues

### "CMakeLists.txt not found" Error

This typically means:
1. You're not using `idf.py` to build (ESP-IDF requires this)
2. The project structure is incorrect
3. ESP-IDF environment is not sourced

**Solution**:
- Make sure you've run `. $HOME/esp/esp-idf/export.sh`
- Use `idf.py build` instead of `cmake` directly
- Verify the project structure matches the ESP-IDF requirements

### Missing Components

If you get errors about missing components:
```bash
idf.py fullclean
idf.py build
```

### SD Card Issues

If SD card initialization fails:
- Check your wiring (MISO, MOSI, CLK, CS)
- Verify the SD card is formatted as FAT32
- Check the SPI bus configuration in `storage.c`

## Configuration

### WiFi Configuration

Edit `sdkconfig.defaults` or use `idf.py menuconfig` to set:
- WiFi SSID and password
- InfluxDB server URL and credentials

### Sensor Configuration

Modify `components/sensors/sensors.c` to:
- Set the correct GPIO pins for speed and cadence sensors
- Configure wheel circumference for your bicycle
- Set the number of magnets on your cadence sensor

### GPS Configuration

Configure the UART port and baud rate in `components/gps/gps.c` for your specific GPS module.

## Testing

1. **Basic functionality test**:
   ```bash
   idf.py build
   idf.py -p PORT flash monitor
   ```

2. **Check logs**:
   - Verify all components initialize correctly
   - Check that telemetry data is being collected
   - Verify SD card writing works

3. **WiFi test**:
   - Check that WiFi connects successfully
   - Verify InfluxDB upload works (if configured)

## Customization

### Adding New Sensors

1. Add the sensor to `sensor_data_t` in `sensors.h`
2. Implement the driver in `sensors.c`
3. Add configuration in `config.h`
4. Update the telemetry structure
5. Add to line protocol conversion

### Modifying Data Format

Edit `telemetry_to_line_protocol()` in `core/telemetry.c` to change the InfluxDB format.

## Support

For issues with:
- **ESP-IDF**: Check https://docs.espressif.com/projects/esp-idf/
- **CMake**: Check https://cmake.org/documentation/
- **InfluxDB**: Check https://docs.influxdata.com/
- **GPX format**: Check http://www.topografix.com/GPX/1/1/

## Troubleshooting

### Clean Build

If you encounter strange build issues:
```bash
idf.py fullclean
rm -rf build/ sdkconfig
idf.py build
```

### Verbose Output

For detailed build output:
```bash
idf.py build -v
```

### Check Environment

Verify ESP-IDF is properly set up:
```bash
idf.py --version
```

This should show ESP-IDF v5.4.x
