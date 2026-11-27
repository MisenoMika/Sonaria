#ifndef SONARIA_PRACTICE_DISPLAY_H
#define SONARIA_PRACTICE_DISPLAY_H
#include "practice_session.h"
#include "oled_display.h"
void practice_screen_render(oled_frame_t *frame, const practice_session_t *session);
void practice_display_start(void);
void practice_display_publish(const practice_session_t *session, bool debug);
#endif
