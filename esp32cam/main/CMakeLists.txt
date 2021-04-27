# set(COMPONENT_SRCS "take_picture.c" "station_example_main.c")
#
#
# register_component()
idf_component_register(SRCS "take_picture.c"
                            "wifi.c"
                            "http_request.c"
                            "app_main.c"
                            "uart.c"
                            "vl53l0x.c"
                       INCLUDE_DIRS "include")

# target_compile_definitions(${COMPONENT_TARGET} BOARD_ESP32CAM_AITHINKER=1)
# add_compile_definitions(-DBOARD_ESP32CAM_AITHINKER)
