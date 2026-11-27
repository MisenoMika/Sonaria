#ifndef SONARIA_PRACTICE_DISPLAY_H
#define SONARIA_PRACTICE_DISPLAY_H
#include "practice_session.h"
#include "pitch_tracker.h"
#include "sweep_demo.h"
#include "oled_display.h"
void practice_live_screen_render(oled_frame_t *frame, const tuner_result_t *result);
void practice_screen_render(oled_frame_t *frame, const practice_session_t *session);
void practice_demo_screen_render(oled_frame_t *frame, const sweep_demo_t *demo);
void practice_display_publish_demo(const sweep_demo_t *demo, bool debug);
void practice_display_start(void);
void practice_display_publish(const practice_session_t *session, bool debug);
void practice_display_publish_live(const tuner_result_t *result, bool debug);
#endif
