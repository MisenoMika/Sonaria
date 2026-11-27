#ifndef SONARIA_TUNER_DISPLAY_H
#define SONARIA_TUNER_DISPLAY_H
#include "pitch_tracker.h"
#include <stdint.h>
void tuner_display_start(void);
void tuner_display_publish(const tuner_result_t *result, int64_t compute_us);
#endif
