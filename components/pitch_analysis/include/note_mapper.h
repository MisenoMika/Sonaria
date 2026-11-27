#ifndef SONARIA_NOTE_MAPPER_H
#define SONARIA_NOTE_MAPPER_H
#include <stdbool.h>

typedef struct {
    int midi;
    const char *name;
    int octave;
    float reference_hz;
    float cents;
} note_result_t;

bool frequency_to_note(float frequency_hz, note_result_t *note);
#endif
