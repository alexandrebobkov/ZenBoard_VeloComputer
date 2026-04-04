#!/bin/bash

echo "Verifying Velocomputer Project Structure..."
echo "=========================================="
echo ""

# Check if we're in the right directory
if [ ! -f "CMakeLists.txt" ]; then
    echo "ERROR: Please run this script from the project root directory"
    exit 1
fi

# Check main files
echo "Checking main project files..."
files=("CMakeLists.txt" "sdkconfig.defaults" "README.md" "main/CMakeLists.txt" "main/main.c")
missing=()

for file in "${files[@]}"; do
    if [ ! -f "$file" ]; then
        missing+=("$file")
    fi
done

if [ ${#missing[@]} -gt 0 ]; then
    echo "ERROR: Missing files:"
    for file in "${missing[@]}"; do
        echo "  - $file"
    done
    exit 1
else
    echo "✓ All main files present"
fi

# Check components
echo ""
echo "Checking component structure..."
components=("core" "gps" "sensors" "storage" "network" "gpx")
missing_components=()

for comp in "${components[@]}"; do
    if [ ! -d "main/components/$comp" ]; then
        missing_components+=("$comp")
    else
        # Check for required files in each component
        if [ ! -f "main/components/$comp/CMakeLists.txt" ]; then
            echo "WARNING: Missing CMakeLists.txt in $comp component"
        fi

        # Check for header file
        if [ ! -f "main/components/$comp/${comp}.h" ]; then
            echo "WARNING: Missing ${comp}.h header file"
        fi

        # Check for source file
        if [ ! -f "main/components/$comp/${comp}.c" ]; then
            echo "WARNING: Missing ${comp}.c source file"
        fi
    fi
done

if [ ${#missing_components[@]} -gt 0 ]; then
    echo "ERROR: Missing components:"
    for comp in "${missing_components[@]}"; do
        echo "  - $comp"
    done
    exit 1
else
    echo "✓ All components present"
fi

# Check CMakeLists.txt files
echo ""
echo "Checking CMakeLists.txt files..."
cmake_files=(
    "CMakeLists.txt"
    "main/CMakeLists.txt"
    "main/components/core/CMakeLists.txt"
    "main/components/gps/CMakeLists.txt"
    "main/components/sensors/CMakeLists.txt"
    "main/components/storage/CMakeLists.txt"
    "main/components/network/CMakeLists.txt"
    "main/components/gpx/CMakeLists.txt"
)

missing_cmake=()
for file in "${cmake_files[@]}"; do
    if [ ! -f "$file" ]; then
        missing_cmake+=("$file")
    fi
done

if [ ${#missing_cmake[@]} -gt 0 ]; then
    echo "ERROR: Missing CMakeLists.txt files:"
    for file in "${missing_cmake[@]}"; do
        echo "  - $file"
    done
    exit 1
else
    echo "✓ All CMakeLists.txt files present"
fi

# Check header files
echo ""
echo "Checking header files..."
header_files=(
    "main/components/core/telemetry.h"
    "main/components/core/config.h"
    "main/components/gps/gps.h"
    "main/components/sensors/sensors.h"
    "main/components/storage/storage.h"
    "main/components/network/network.h"
    "main/components/gpx/gpx.h"
)

missing_headers=()
for file in "${header_files[@]}"; do
    if [ ! -f "$file" ]; then
        missing_headers+=("$file")
    fi
done

if [ ${#missing_headers[@]} -gt 0 ]; then
    echo "ERROR: Missing header files:"
    for file in "${missing_headers[@]}"; do
        echo "  - $file"
    done
    exit 1
else
    echo "✓ All header files present"
fi

# Check source files
echo ""
echo "Checking source files..."
source_files=(
    "main/components/core/telemetry.c"
    "main/components/core/config.c"
    "main/components/gps/gps.c"
    "main/components/sensors/sensors.c"
    "main/components/storage/storage.c"
    "main/components/network/network.c"
    "main/components/gpx/gpx.c"
)

missing_sources=()
for file in "${source_files[@]}"; do
    if [ ! -f "$file" ]; then
        missing_sources+=("$file")
    fi
done

if [ ${#missing_sources[@]} -gt 0 ]; then
    echo "ERROR: Missing source files:"
    for file in "${missing_sources[@]}"; do
        echo "  - $file"
    done
    exit 1
else
    echo "✓ All source files present"
fi

echo ""
echo "=========================================="
echo "✓ Project structure verification PASSED"
echo ""
echo "Project is ready for ESP-IDF v5.4"
echo ""
echo "To build this project:"
echo "1. Source the ESP-IDF environment:"
echo "   . $HOME/esp/esp-idf/export.sh"
echo ""
echo "2. Navigate to this directory:"
echo "   cd /home/alex/Gitea/ZenBoard_Velocomputer/velocomputer"
echo ""
echo "3. Build the project:"
echo "   idf.py set-target esp32c3"
echo "   idf.py build"
echo ""
echo "4. Flash to your device:"
echo "   idf.py -p /dev/ttyUSB0 flash monitor"
echo ""
