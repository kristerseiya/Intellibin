cmake_minimum_required(VERSION 3.9)
set(CXX_STANDARD 11)

set(EXTRA_COMPONENT_DIRS ~/Projects/SeniorDesign/ESP32_VL53L0X)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

add_compile_options(-Wno-switch)
# list(APPEND EXTRA_COMPONENT_DIRS ../..)
project(feather)
