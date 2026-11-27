#include "note_mapper.h"
#include <math.h>

bool frequency_to_note(float frequency_hz, note_result_t *note)
{
    if (!note || !isfinite(frequency_hz) || frequency_hz <= 0.0f) return false;
    float midi = 69.0f + 12.0f * log2f(frequency_hz / 440.0f);
    if (!isfinite(midi) || midi < -0.5f || midi >= 127.5f) return false;
    int n = (int)floorf(midi + 0.5f);
    static const char *const names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    float reference = 440.0f * exp2f((n - 69) / 12.0f);
    *note = (note_result_t){.midi = n, .name = names[n % 12], .octave = n / 12 - 1,
                            .reference_hz = reference, .cents = 1200.0f * log2f(frequency_hz / reference)};
    return true;
}
