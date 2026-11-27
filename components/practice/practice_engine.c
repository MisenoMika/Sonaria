#include "practice_engine.h"
#include <string.h>
void practice_engine_reset_audio(practice_engine_t *e)
{
    e->filled = 0;
    pitch_tracker_reset(&e->tracker);
    note_event_reset(&e->events);
    e->pitch = (tuner_result_t){
        .pitch = {.status = PITCH_SILENCE}, .raw = {.status = PITCH_SILENCE}, .candidate_midi = -1};
}
static bool pitch_sample(practice_engine_t *e, float sample, bool live)
{
    e->frame[e->filled++] = sample;
    if (e->filled != PITCH_FRAME_SIZE)
        return false;
    const size_t hop = live ? PITCH_LIVE_HOP_SIZE : PITCH_HOP_SIZE;
    e->pitch = live ? pitch_track_live(&e->tracker, e->frame) : pitch_track(&e->tracker, e->frame);
    memmove(e->frame, e->frame + hop,
            (PITCH_FRAME_SIZE - hop) * sizeof(float));
    e->filled = PITCH_FRAME_SIZE - hop;
    return true;
}

bool practice_engine_live_sample(practice_engine_t *e, float sample)
{
    return pitch_sample(e, sample, true);
}

bool practice_engine_sample(practice_engine_t *e, float sample, bool *has_event,
                            practice_result_t *result)
{
    *has_event = false;
    if (e->session.state != PRACTICE_ACTIVE)
        return false;
    note_event_sample(&e->events, sample);
    if (!pitch_sample(e, sample, false))
        return false;
    practice_note_event_t event;
    if (note_event_update(&e->events, &e->pitch, &event))
        *has_event = practice_session_event(&e->session, event, result);
    return true;
}
