# set(COMPONENT_SRCS
#     "lcd1602_i2c.c"
#     "lcd1602_lib.c"
# )
#
# register_component()

idf_component_register(SRCS "app_main.c"
                            "lcd.c"
                            "motor.c"
                            "thinkspeak.c"
                            "uart.c"
                            "vl53l0x.c"
                      INCLUDE_DIRS "include")
