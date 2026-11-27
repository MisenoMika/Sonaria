#ifndef SONARIA_TUNER_SCREEN_H
#define SONARIA_TUNER_SCREEN_H
#include "oled_display.h"
#include "pitch_tracker.h"
void tuner_render(oled_frame_t *frame, const tuner_result_t *result);
#endif
