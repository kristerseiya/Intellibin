# set(COMPONENT_SRCS
#     "lcd1602_i2c.c"
#     "lcd1602_lib.c"
# )
#
# register_component()

idf_component_register(SRCS "uart_events_example_main.c"
                            "lcd.c"
                      INCLUDE_DIRS ".")
