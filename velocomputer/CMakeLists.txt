cmake_minimum_required(VERSION 3.5)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(velocomputer)

# Register component directories
set(EXTRA_COMPONENT_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/main/components/core
    ${CMAKE_CURRENT_SOURCE_DIR}/main/components/gps
    ${CMAKE_CURRENT_SOURCE_DIR}/main/components/sensors
    ${CMAKE_CURRENT_SOURCE_DIR}/main/components/storage
    ${CMAKE_CURRENT_SOURCE_DIR}/main/components/network
    ${CMAKE_CURRENT_SOURCE_DIR}/main/components/gpx)

set(EXTRA_COMPONENT_DIRS ${EXTRA_COMPONENT_DIRS} CACHE INTERNAL "")
