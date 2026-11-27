#ifndef SONARIA_PRACTICE_SCORE_H
#define SONARIA_PRACTICE_SCORE_H
#include <stddef.h>
#include <stdint.h>
#define PRACTICE_TICKS_PER_QUARTER 96

typedef struct
{
    uint8_t midi;
    uint16_t ticks;
    uint16_t bar;       // 1-based
    uint16_t bar_ticks; // Offset from the measure start.
} practice_score_note_t;
typedef struct
{
    const char *id, *title;
    uint16_t bpm;
    uint8_t numerator, denominator;
    const practice_score_note_t *notes;
    size_t count;
} practice_score_t;
extern const practice_score_t practice_scores[];
extern const size_t practice_score_count;
const practice_score_t *practice_score_find(const char *id);
void practice_note_name(int midi, char *out, size_t size);
#endif
