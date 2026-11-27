#include "practice_display.h"
#include "tuner_screen.h"
#include <stdio.h>
#include <string.h>

#define CAROUSEL_MUTED 0x6B4D
#define CAROUSEL_BORDER 0x4208

static void carousel_note(oled_frame_t *frame, int midi, unsigned center, unsigned y,
                          unsigned scale, uint16_t color)
{
    char label[8];
    practice_note_name(midi, label, sizeof label);
    unsigned width = (unsigned)strlen(label) * 6 * scale - scale;
    oled_text(frame, center - width / 2, y, label, scale, color);
}

static void render_targets(oled_frame_t *frame, const practice_session_t *s)
{
    size_t position = s->position;
    size_t count = s->score->count;
    int previous = position > 0 && position <= count ? s->score->notes[position - 1].midi : -1;
    int current = position < count ? s->score->notes[position].midi : -1;
    int next = position < count && count - position > 1 ? s->score->notes[position + 1].midi : -1;

    // Three fixed slots: 3-character sharp notes fit without overlapping.
    oled_rect(frame, 37, 41, 1, 25, CAROUSEL_BORDER);
    oled_rect(frame, 90, 41, 1, 25, CAROUSEL_BORDER);
    oled_rect(frame, 37, 65, 54, 1, CAROUSEL_BORDER);
    carousel_note(frame, previous, 19, 46, 2, CAROUSEL_MUTED);
    carousel_note(frame, current, 64, 42, 3, CYAN);
    carousel_note(frame, next, 109, 46, 2, CAROUSEL_MUTED);
}

void practice_screen_render(oled_frame_t *frame, const practice_session_t *s)
{
    oled_clear(frame, BLACK);
    oled_centered_text(frame, 3, "PRACTICE", 2, CYAN);
    if (!s->score)
    {
        oled_centered_text(frame, 52, "NO SCORE", 2, WHITE);
        return;
    }
    char text[32];
    if (s->state == PRACTICE_COMPLETE)
    {
        oled_centered_text(frame, 34, "COMPLETE", 2, GREEN);
        snprintf(text, sizeof text, "%zu/%zu", s->position, s->score->count);
        oled_centered_text(frame, 61, text, 2, WHITE);
        snprintf(text, sizeof text, "ERRORS %zu", s->errors);
        oled_centered_text(frame, 84, text, 1, YELLOW);
        oled_centered_text(frame, 110, "RESTART TO PLAY", 1, CYAN);
        return;
    }
    size_t pos = s->position < s->score->count ? s->position + 1 : s->score->count;
    snprintf(text, sizeof text, "%.8s %zu/%zu", s->score->id, pos, s->score->count);
    oled_centered_text(frame, 21, text, 1, WHITE);
    oled_centered_text(frame, 32, "TARGET", 1, CYAN);
    render_targets(frame, s);
    oled_centered_text(frame, 67, "PLAYED", 1, WHITE);
    practice_note_name(s->last_event.midi, text, sizeof text);
    uint16_t color = s->feedback == PRACTICE_WRONG     ? RED
                     : s->feedback == PRACTICE_CORRECT ? GREEN
                                                       : WHITE;
    oled_centered_text(frame, 77, text, 2, color);
    if (s->last_event.midi >= 0)
    {
        snprintf(text, sizeof text, "%+.1f CT", s->last_event.cents);
        oled_centered_text(frame, 95, text, 1, color);
    }
    const char *status = s->state == PRACTICE_READY        ? "READY"
                         : s->state == PRACTICE_STOPPED    ? "STOPPED"
                         : s->feedback == PRACTICE_WRONG   ? "WRONG"
                         : s->feedback == PRACTICE_CORRECT ? "OK"
                                                           : "LISTENING";
    oled_centered_text(frame, 110, status, 2, s->state == PRACTICE_ACTIVE ? color : WHITE);
}

void practice_live_screen_render(oled_frame_t *frame, const tuner_result_t *result)
{
    // Reuse the validated tuner layout and only change the title to distinguish modes.
    tuner_render(frame, result);
    oled_rect(frame, 0, 0, OLED_WIDTH, 20, BLACK);
    oled_centered_text(frame, 4, "LIVE PITCH", 2, CYAN);
}

void practice_demo_screen_render(oled_frame_t *frame, const sweep_demo_t *d)
{
    char text[32];
    oled_clear(frame, BLACK);
    oled_centered_text(frame, 3, "SWEEP DEMO", 2, CYAN);
    snprintf(text, sizeof text, "%s  %u/%d", sweep_demo_direction_name(d->direction),
             d->count, SWEEP_NOTE_COUNT);
    oled_centered_text(frame, 24, text, 1, WHITE);
    if (d->complete) {
        snprintf(text, sizeof text, "%.1f%%", sweep_demo_accuracy(d));
        oled_centered_text(frame, 42, text, 3, d->errors ? YELLOW : GREEN);
        snprintf(text, sizeof text, "ERRORS %u", d->errors);
        oled_centered_text(frame, 68, text, 1, WHITE);
        oled_centered_text(frame, 110, "RESTART TO PLAY", 1, CYAN);
    } else {
        practice_note_name(d->count ? d->notes[d->count - 1] : -1, text, sizeof text);
        oled_centered_text(frame, 42, text, 4, WHITE);
        oled_centered_text(frame, 110, d->count ? "KEEP PLAYING" : "SWIPE C3-C6", 1, CYAN);
    }
    snprintf(text, sizeof text, "%.1f NOTES/S", sweep_demo_rate(d));
    oled_centered_text(frame, 84, text, 1, WHITE);
    oled_rect(frame, 5, 98, 118, 5, CAROUSEL_BORDER);
    if (d->count)
        oled_rect(frame, 5, 98, 118 * d->count / SWEEP_NOTE_COUNT, 5, CYAN);
}
