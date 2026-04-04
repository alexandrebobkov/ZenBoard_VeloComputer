# Potential Build Issues and Solutions

## Fixed Issues

### 1. Incorrect Include Paths in main.c
**Problem**: main.c was including `"core/telemetry.h"` instead of `"components/core/telemetry.h"`

**Solution**: Fixed in commit - updated all include paths to use `"components/..."` prefix

### 2. Missing Component Dependencies in sdkconfig.defaults
**Problem**: Several required components weren't enabled by default

**Solution**: Added to sdkconfig.defaults:
- `CONFIG_CJSON_ENABLED=y` - for JSON parsing
- `CONFIG_SPI_FLASH=y` - for SPI support
- `CONFIG_SDMMC_HOST=y` - for SD card
- `CONFIG_FATFS_ENABLED=y` - for FAT filesystem
- `CONFIG_PCNT_SUPPORTED=y` - for pulse counters
- `CONFIG_PCNT=y` - for pulse counters
- `CONFIG_ESP_HTTP_CLIENT_ENABLED=y` - for HTTP client
- `CONFIG_NVS_FLASH=y` - for NVS storage

## Potential Issues to Watch For

### 1. ESP-IDF Version Compatibility
**Issue**: Project designed for v5.4 but you're using v5.5.1

**Solution**: Should work fine, but if you encounter issues:
- Check API changes in ESP-IDF v5.5.1 release notes
- Use `idf.py --version` to confirm your version
- Most APIs are backward compatible

### 2. Missing Python Dependencies
**Symptoms**: Errors about missing Python modules during build

**Solution**:
```bash
cd ~/esp/esp-idf-v5.5.1
. export.sh
python -m pip install --upgrade pip
python -m pip install -r requirements.txt
```

### 3. CMake Cache Issues
**Symptoms**: Strange build errors after changing configurations

**Solution**:
```bash
idf.py fullclean
rm -rf build/ sdkconfig
idf.py build
```

### 4. Serial Port Permissions
**Symptoms**: `Permission denied` when trying to flash

**Solution**:
```bash
sudo usermod -a -G dialout $USER
sudo usermod -a -G plugdev $USER
# Log out and log back in
```

### 5. Missing Git Submodules
**Symptoms**: Errors about missing tools during build

**Solution**:
```bash
cd ~/esp/esp-idf-v5.5.1
git submodule update --init --recursive
```

### 6. Insufficient Memory for Build
**Symptoms**: Build fails with memory errors

**Solution**:
- Close other applications
- Create a swap file if on Linux:
```bash
sudo fallocate -l 4G /swapfile
sudo chmod 600 /swapfile
sudo mkswap /swapfile
sudo swapon /swapfile
```

### 7. Toolchain Issues
**Symptoms**: Errors about missing compiler or tools

**Solution**:
```bash
cd ~/esp/esp-idf-v5.5.1
./install.sh
```

## Component-Specific Issues

### Storage Component
**Potential Issues**:
1. SD card not detected
2. FATFS initialization fails
3. File write permissions

**Solutions**:
1. Check wiring (MISO, MOSI, CLK, CS pins)
2. Verify SD card is formatted FAT32
3. Check `storage.c` for correct SPI configuration
4. Enable debug logging in menuconfig

### Network Component
**Potential Issues**:
1. WiFi connection fails
2. HTTP client errors
3. SSL/TLS issues

**Solutions**:
1. Verify WiFi credentials in menuconfig
2. Check router compatibility (2.4GHz vs 5GHz)
3. Disable SSL verification for testing (not recommended for production)
4. Check firewall settings

### Sensors Component
**Potential Issues**:
1. Pulse counter not working
2. GPIO conflicts
3. Interrupt issues

**Solutions**:
1. Verify GPIO pins in `sensors.c`
2. Check pulse counter configuration
3. Test with simple GPIO input first
4. Use logic analyzer to verify sensor signals

### GPS Component
**Potential Issues**:
1. UART communication fails
2. No GPS fix
3. Data parsing errors

**Solutions**:
1. Check UART configuration (baud rate, pins)
2. Test GPS module separately
3. Verify antenna connection
4. Test outdoors for better signal

## Debugging Tips

### Enable Verbose Logging
```bash
idf.py build -v
```

### Check Component Logs
```bash
idf.py monitor
```

### Increase Log Level
In `menuconfig`:
- Go to `Component config` → `Log output`
- Set `Default log verbosity` to `Debug` or `Verbose`

### Check Specific Component
```bash
# For example, to check storage issues:
idf.py monitor | grep storage
```

## Common Error Messages and Fixes

### "Component not found"
**Cause**: Missing component registration or typo in CMakeLists.txt

**Fix**:
1. Check component CMakeLists.txt
2. Verify component is in EXTRA_COMPONENT_DIRS
3. Check for typos in component names

### "Undefined reference"
**Cause**: Missing source file or library

**Fix**:
1. Check all source files are listed in CMakeLists.txt
2. Verify REQUIRES lists all needed components
3. Check for typos in function names

### "Cannot open include file"
**Cause**: Incorrect include path or missing header

**Fix**:
1. Verify include paths in CMakeLists.txt
2. Check header file exists
3. Use correct relative paths

### "Partition table not found"
**Cause**: Missing or incorrect partition table

**Fix**:
1. Use default partition table
2. Verify partition.csv exists
3. Check flash size in menuconfig

## Testing Without Hardware

You can test the build system without actual hardware:

```bash
# Build only (no flash)
idf.py build

# Run size analysis
idf.py size

# Check component dependencies
idf.py size-components
```

## Getting Help

If you encounter issues not listed here:

1. **Check ESP-IDF Documentation**: https://docs.espressif.com/projects/esp-idf/
2. **Search GitHub Issues**: https://github.com/espressif/esp-idf/issues
3. **ESP32 Forum**: https://www.esp32.com/
4. **Espressif Support**: https://www.espressif.com/en/support

## Next Steps

After fixing any build issues:

1. ✅ Verify project structure
2. ✅ Fix known issues (done)
3. ⏳ Set up ESP-IDF v5.5.1
4. ⏳ Build the project
5. ⏳ Test on hardware
6. ⏳ Customize for your specific sensors

The project should now build successfully with ESP-IDF v5.5.1!
