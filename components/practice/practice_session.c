#include "practice_session.h"
#include <stdio.h>
void practice_session_select(practice_session_t *s, const practice_score_t *score)
{
    *s = (practice_session_t){.score = score, .state = PRACTICE_READY, .last_event = {.midi = -1}};
}
bool practice_session_start(practice_session_t *s, bool restart)
{
    if (!s->score || !s->score->count || (s->state == PRACTICE_ACTIVE && !restart))
        return false;
    const practice_score_t *score = s->score;
    practice_session_select(s, score);
    s->state = PRACTICE_ACTIVE;
    return true;
}
void practice_session_stop(practice_session_t *s) { s->state = PRACTICE_STOPPED; }
bool practice_session_event(practice_session_t *s, practice_note_event_t event,
                            practice_result_t *r)
{
    if (s->state != PRACTICE_ACTIVE || !s->score || s->position >= s->score->count ||
        event.midi < 0 || event.midi > 127)
        return false;
    *r = (practice_result_t){.score = s->score,
                             .index = s->position,
                             .expected = s->score->notes[s->position],
                             .actual = event};
    s->last_event = event;
    if (event.midi == r->expected.midi)
    {
        s->feedback = PRACTICE_CORRECT;
        if (++s->position == s->score->count)
            s->state = PRACTICE_COMPLETE;
    }
    else
    {
        s->feedback = PRACTICE_WRONG;
        ++s->errors;
    }
    r->feedback = s->feedback;
    r->complete = s->state == PRACTICE_COMPLETE;
    return true;
}
void practice_format_error(const practice_result_t *r, char *out, size_t size)
{
    char expected[8], actual[8], beat[16];
    practice_note_name(r->expected.midi, expected, sizeof expected);
    practice_note_name(r->actual.midi, actual, sizeof actual);
    unsigned per_beat = PRACTICE_TICKS_PER_QUARTER * 4 / r->score->denominator;
    if (r->expected.bar_ticks % per_beat == 0)
        snprintf(beat, sizeof beat, "%u", r->expected.bar_ticks / per_beat + 1);
    else
        snprintf(beat, sizeof beat, "%.3f", 1.0 + (double)r->expected.bar_ticks / per_beat);
    snprintf(out, size, "ERROR song=%s note=%zu/%zu bar=%u beat=%s expected=%s actual=%s",
             r->score->id, r->index + 1, r->score->count, r->expected.bar, beat, expected, actual);
}
