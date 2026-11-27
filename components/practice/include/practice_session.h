#ifndef SONARIA_PRACTICE_SESSION_H
#define SONARIA_PRACTICE_SESSION_H
#include "practice_score.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    PRACTICE_READY,
    PRACTICE_ACTIVE,
    PRACTICE_STOPPED,
    PRACTICE_COMPLETE
} practice_state_t;
typedef enum
{
    PRACTICE_NO_FEEDBACK,
    PRACTICE_CORRECT,
    PRACTICE_WRONG
} practice_feedback_t;
typedef enum
{
    PRACTICE_NEW_NOTE,
    PRACTICE_REATTACK
} practice_articulation_t;
typedef struct
{
    int midi;
    float frequency, cents;
    uint64_t sample;
    practice_articulation_t articulation;
} practice_note_event_t;
typedef struct
{
    const practice_score_t *score;
    practice_state_t state;
    size_t position, errors;
    practice_feedback_t feedback;
    practice_note_event_t last_event;
} practice_session_t;
typedef struct
{
    const practice_score_t *score;
    size_t index;
    practice_score_note_t expected;
    practice_note_event_t actual;
    practice_feedback_t feedback;
    bool complete;
} practice_result_t;
void practice_session_select(practice_session_t *s, const practice_score_t *score);
bool practice_session_start(practice_session_t *s, bool restart);
void practice_session_stop(practice_session_t *s);
bool practice_session_event(practice_session_t *s, practice_note_event_t event,
                            practice_result_t *result);
// Exact, single-line machine-readable learner error (no newline).
void practice_format_error(const practice_result_t *r, char *out, size_t size);
#endif
