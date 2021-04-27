cmake_minimum_required(VERSION 3.9)
set(CXX_STANDARD 11)

set(EXTRA_COMPONENT_DIRS ../components/esp32-vl53l0x)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

add_compile_options(-Wno-switch)
# list(APPEND EXTRA_COMPONENT_DIRS ../..)
project(feather)
