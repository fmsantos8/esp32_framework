cmake_minimum_required(VERSION 3.22)

set(EXTRA_COMPONENT_DIRS "hal/ports/esp32-wroom-32" "bsp" "memory")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32_framework)
