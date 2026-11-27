idf_component_register(SRCS "tuner_display.c" "tuner_screen.c"
                       INCLUDE_DIRS "include"
                       REQUIRES pitch_analysis oled_display
                       PRIV_REQUIRES esp_timer)
