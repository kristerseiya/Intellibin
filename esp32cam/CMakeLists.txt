cmake_minimum_required(VERSION 3.5)

set(EXTRA_COMPONENT_DIRS ../components/esp32-camera ../components/esp32-vl53l0x)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(take_pic)
