idf_component_register(SRCS "practice_screen.c" "practice_display.c" INCLUDE_DIRS "include"
                       REQUIRES practice oled_display PRIV_REQUIRES esp_timer)
