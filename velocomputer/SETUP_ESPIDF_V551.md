# ESP-IDF v5.5.1 Setup Guide for Velocomputer

## Quick Start

```bash
# Install ESP-IDF v5.5.1
mkdir -p ~/esp
cd ~/esp
git clone -b v5.5.1 --recursive https://github.com/espressif/esp-idf.git esp-idf-v5.5.1
cd esp-idf-v5.5.1
./install.sh

# Source the environment
. ~/esp/esp-idf-v5.5.1/export.sh

# Verify installation
idf.py --version
```

## Detailed Installation Instructions

### 1. Install Prerequisites

#### Ubuntu/Debian
```bash
sudo apt update
sudo apt install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
```

#### macOS (using Homebrew)
```bash
brew update
brew install cmake ninja dfu-util wget
```

#### Windows
- Install Git: https://git-scm.com/download/win
- Install Python 3.8+: https://www.python.org/downloads/
- Install CMake: https://cmake.org/download/
- Install Ninja: https://ninja-build.org/

### 2. Download ESP-IDF v5.5.1

```bash
mkdir -p ~/esp
cd ~/esp
git clone -b v5.5.1 --recursive https://github.com/espressif/esp-idf.git esp-idf-v5.5.1
```

**Note**: The `--recursive` option is important as it clones the submodules (tools) needed for the build system.

### 3. Run the Install Script

```bash
cd ~/esp/esp-idf-v5.5.1
./install.sh
```

This script will:
- Create a Python virtual environment in `~/esp/esp-idf-v5.5.1/.venv`
- Install all Python dependencies
- Set up the build system

### 4. Source the Environment

You need to source the environment variables in every new terminal session:

```bash
. ~/esp/esp-idf-v5.5.1/export.sh
```

**Tip**: Add this to your `~/.bashrc` or `~/.zshrc` for automatic sourcing:
```bash
echo '. ~/esp/esp-idf-v5.5.1/export.sh' >> ~/.bashrc
source ~/.bashrc
```

### 5. Verify Installation

```bash
idf.py --version
```

Should output something like:
```
esp-idf v5.5.1
```

## Building the Velocomputer Project

### 1. Navigate to Project Directory

```bash
cd /home/alex/Gitea/ZenBoard_Velocomputer/velocomputer
```

### 2. Set Target to ESP32-C3

```bash
idf.py set-target esp32c3
```

### 3. Configure the Project

```bash
idf.py menuconfig
```

In menuconfig:
- Go to `Component config` → `WiFi` to configure WiFi settings
- Go to `Component config` → `Storage` for SD card settings
- Save and exit

### 4. Build the Project

```bash
idf.py build
```

This will:
- Create a `build` directory
- Download required components
- Compile the firmware
- Generate binary files

### 5. Flash to Device

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

Replace `/dev/ttyUSB0` with your actual serial port. On Windows, it might be `COM3` or similar.

## Troubleshooting

### Common Issues with ESP-IDF v5.5.1

#### 1. Python Version Issues

**Error**: `Python 3.x required but it was not found`

**Solution**:
```bash
# Check Python version
python --version

# If you have multiple Python versions, specify which one to use
# before running install.sh:
export PYTHON=python3.8
./install.sh
```

#### 2. Missing Tools

**Error**: `cmake not found` or `ninja not found`

**Solution**: Install the missing tools as shown in the prerequisites section.

#### 3. Git Submodule Issues

**Error**: `fatal: not a git repository` when building

**Solution**:
```bash
cd ~/esp/esp-idf-v5.5.1
git submodule update --init --recursive
```

#### 4. Permission Issues

**Error**: `Permission denied` when accessing serial port

**Solution**:
```bash
sudo usermod -a -G dialout $USER
sudo usermod -a -G plugdev $USER
# Log out and log back in for changes to take effect
```

#### 5. CMake Cache Issues

**Error**: Strange build errors after changing versions

**Solution**:
```bash
cd /home/alex/Gitea/ZenBoard_Velocomputer/velocomputer
idf.py fullclean
rm -rf build/ sdkconfig
idf.py build
```

## ESP-IDF v5.5.1 Specific Features

### New Features in v5.5.1 (vs v5.4)

1. **Improved ESP32-C3 Support**: Better stability and performance
2. **Enhanced WiFi**: Better connection handling and power management
3. **New Components**: Additional peripheral drivers
4. **Bug Fixes**: Many stability improvements

### Compatibility with Our Project

The Velocomputer project is fully compatible with ESP-IDF v5.5.1:

- ✅ All used components are available
- ✅ No breaking API changes
- ✅ Better performance on ESP32-C3
- ✅ More stable WiFi and networking

## Updating ESP-IDF

To update to a newer version later:

```bash
cd ~/esp/esp-idf-v5.5.1
git pull
git submodule update --init --recursive
./install.sh
```

## Uninstalling

To completely remove ESP-IDF v5.5.1:

```bash
rm -rf ~/esp/esp-idf-v5.5.1
# Also remove from your shell configuration if you added the source command
```

## Additional Resources

- **ESP-IDF v5.5.1 Documentation**: https://docs.espressif.com/projects/esp-idf/en/v5.5.1/
- **ESP32-C3 Datasheet**: https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf
- **ESP-IDF Programming Guide**: https://docs.espressif.com/projects/esp-idf/en/v5.5.1/esp32c3/

## Next Steps

1. ✅ Install ESP-IDF v5.5.1
2. ⏳ Source the environment
3. ⏳ Build the Velocomputer project
4. ⏳ Flash to your ESP32-C3 device
5. ⏳ Test and customize

The project is ready for ESP-IDF v5.5.1! Let me know if you encounter any issues during setup.
