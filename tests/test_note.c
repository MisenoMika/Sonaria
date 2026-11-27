#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "note_mapper.h"
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); exit(1); } } while (0)
int main(void)
{
    note_result_t note;
    CHECK(frequency_to_note(440.0f, &note));
    CHECK(note.midi == 69 && strcmp(note.name, "A") == 0 && note.octave == 4);
    CHECK(fabs(note.cents) < 0.01 && fabs(note.reference_hz - 440) < 0.01);
    CHECK(frequency_to_note(435.0f, &note) && fabs(note.cents + 19.79) < 0.05);
    CHECK(frequency_to_note(445.0f, &note) && fabs(note.cents - 19.56) < 0.05);
    CHECK(frequency_to_note(261.6256f, &note));
    CHECK(strcmp(note.name, "C") == 0 && note.octave == 4 && fabs(note.cents) < 0.01);
    CHECK(frequency_to_note(277.1826f, &note));
    CHECK(strcmp(note.name, "C#") == 0 && note.octave == 4);
    CHECK(frequency_to_note(880.0f, &note) && note.midi == 81 && note.octave == 5);
    CHECK(!frequency_to_note(0, &note) && !frequency_to_note(-1, &note));
    CHECK(!frequency_to_note(NAN, &note) && !frequency_to_note(INFINITY, &note));
    puts("Note conversion tests passed");
    return 0;
}
