#ifndef SONARIA_PRACTICE_ENGINE_H
#define SONARIA_PRACTICE_ENGINE_H
#include "note_event_detector.h"
typedef struct
{
    practice_session_t session;
    note_event_detector_t events;
    pitch_tracker_t tracker;
    float frame[PITCH_FRAME_SIZE];
    size_t filled;
    tuner_result_t pitch;
} practice_engine_t;
void practice_engine_reset_audio(practice_engine_t *engine);
// Process continuous pitch only; do not update the score or articulation state.
bool practice_engine_live_sample(practice_engine_t *engine, float sample);
// Returns true at each processed audio hop. has_event is true only if a judged event exists.
bool practice_engine_sample(practice_engine_t *engine, float sample, bool *has_event,
                            practice_result_t *result);
#endif
