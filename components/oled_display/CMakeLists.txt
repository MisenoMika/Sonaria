idf_component_register(SRCS "oled_display.c" "oled_graphics.c"
                       INCLUDE_DIRS "include"
                       PRIV_REQUIRES esp_driver_spi esp_driver_gpio)
