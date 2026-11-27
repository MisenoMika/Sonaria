#include "practice_control.h"
#include <stdio.h>
#include <stdint.h>

int main(void)
{
    static practice_control_t control;
    practice_control_init(&control);
    control.mode = PRACTICE_MODE_LIVE;
    int16_t sample;
    unsigned index = 0;
    while (fread(&sample, sizeof sample, 1, stdin) == 1) {
        bool event;
        practice_result_t result;
        ++index;
        if (practice_control_sample(&control, sample / 32768.0f, &event, &result)) {
            const tuner_result_t *p = &control.engine.pitch;
            printf("%u %d %d\n", index,
                   p->pitch.status == PITCH_VALID ? p->note.midi : -1, p->tracking);
        }
    }
    return ferror(stdin) ? 1 : 0;
}
