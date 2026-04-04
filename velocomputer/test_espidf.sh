#!/bin/bash

echo "Testing ESP-IDF Setup"
echo "====================="
echo ""

# Check if idf.py exists
if command -v idf.py &> /dev/null; then
    echo "✓ idf.py found in PATH"

    # Check ESP-IDF version
    IDF_VERSION=$(idf.py --version 2>/dev/null | head -1)
    echo "ESP-IDF Version: $IDF_VERSION"

    # Check if version is compatible
    if [[ $IDF_VERSION == *"v5.4"* ]]; then
        echo "✓ Correct version (v5.4) detected"
    elif [[ $IDF_VERSION == *"v5.1"* ]]; then
        echo "⚠ Version v5.1 detected - project designed for v5.4"
        echo "   Some adjustments may be needed"
    else
        echo "⚠ Unsupported version detected: $IDF_VERSION"
        echo "   Project designed for ESP-IDF v5.4"
    fi
else
    echo "✗ idf.py not found in PATH"
    echo ""
    echo "Please set up ESP-IDF first:"
    echo "1. Install ESP-IDF v5.4"
    echo "2. Source the environment:"
    echo "   . ~/esp/esp-idf/export.sh"
    exit 1
fi

# Check Python version
PYTHON_VERSION=$(python --version 2>/dev/null | cut -d' ' -f2)
if [ -z "$PYTHON_VERSION" ]; then
    PYTHON_VERSION=$(python3 --version 2>/dev/null | cut -d' ' -f2)
fi

echo "Python Version: $PYTHON_VERSION"

if [[ $PYTHON_VERSION =~ ^3\.(8|9|1[0-9]) ]]; then
    echo "✓ Python version is compatible"
else
    echo "⚠ Python version may not be compatible"
    echo "   ESP-IDF v5.4 requires Python 3.8+"
fi

# Check CMake
CMAKE_VERSION=$(cmake --version 2>/dev/null | head -1)
echo "CMake: $CMAKE_VERSION"

# Check Ninja
if command -v ninja &> /dev/null; then
    echo "✓ Ninja build system installed"
else
    echo "⚠ Ninja not found - may need to install"
fi

# Check Git
GIT_VERSION=$(git --version 2>/dev/null)
echo "Git: $GIT_VERSION"

echo ""
echo "====================="
echo "Setup verification complete"
echo ""

if command -v idf.py &> /dev/null; then
    echo "You can now build the project:"
    echo "cd /home/alex/Gitea/ZenBoard_Velocomputer/velocomputer"
    echo "idf.py set-target esp32c3"
    echo "idf.py build"
else
    echo "Please set up ESP-IDF first before building"
fi
