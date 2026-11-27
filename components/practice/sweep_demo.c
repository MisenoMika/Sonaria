#include "sweep_demo.h"
#include <stddef.h>

#define SWEEP_TIMEOUT_SAMPLES (PITCH_SAMPLE_RATE * 3 / 4)

static const int white_keys[SWEEP_NOTE_COUNT] = {
    48, 50, 52, 53, 55, 57, 59, 60, 62, 64, 65,
    67, 69, 71, 72, 74, 76, 77, 79, 81, 83, 84
};

void sweep_demo_start(sweep_demo_t *d, sweep_direction_t direction)
{
    *d = (sweep_demo_t){.direction = direction};
}

int sweep_demo_expected(sweep_direction_t direction, unsigned index)
{
    if (index >= SWEEP_NOTE_COUNT || direction == SWEEP_AUTO)
        return -1;
    return white_keys[direction == SWEEP_DOWN ? SWEEP_NOTE_COUNT - 1 - index : index];
}

static unsigned min(unsigned a, unsigned b) { return a < b ? a : b; }

unsigned sweep_demo_errors(const sweep_demo_t *d, sweep_error_t errors[SWEEP_NOTE_COUNT])
{
    // Full edit-distance matrix permits backtracking the exact error locations.
    // Counts are bounded by 22, so the matrix needs only 529 bytes.
    uint8_t cost[SWEEP_NOTE_COUNT + 1][SWEEP_NOTE_COUNT + 1];
    sweep_direction_t direction = d->direction == SWEEP_AUTO ? SWEEP_UP : d->direction;
    for (unsigned j = 0; j <= SWEEP_NOTE_COUNT; ++j)
        cost[0][j] = j;
    for (unsigned i = 1; i <= d->count; ++i) {
        cost[i][0] = i;
        for (unsigned j = 1; j <= SWEEP_NOTE_COUNT; ++j)
            cost[i][j] = min(min(cost[i - 1][j] + 1, cost[i][j - 1] + 1),
                             cost[i - 1][j - 1] +
                                 (d->notes[i - 1] != sweep_demo_expected(direction, j - 1)));
    }
    unsigned count = cost[d->count][SWEEP_NOTE_COUNT];
    if (!errors)
        return count;

    unsigned i = d->count, j = SWEEP_NOTE_COUNT, remaining = count;
    while (i || j) {
        int actual = i ? d->notes[i - 1] : -1;
        int expected = j ? sweep_demo_expected(direction, j - 1) : -1;
        // Prefer exact matches, then substitutions in equally good alignments.
        // Walk backwards and fill the output backwards to preserve playing order.
        if (i && j && actual == expected && cost[i][j] == cost[i - 1][j - 1]) {
            --i;
            --j;
        } else if (i && j && cost[i][j] == cost[i - 1][j - 1] + 1) {
            errors[--remaining] = (sweep_error_t){SWEEP_WRONG, j, i, expected, actual};
            --i;
            --j;
        } else if (j && cost[i][j] == cost[i][j - 1] + 1) {
            errors[--remaining] = (sweep_error_t){SWEEP_MISSING, j, 0, expected, -1};
            --j;
        } else {
            errors[--remaining] = (sweep_error_t){SWEEP_EXTRA, 0, i, -1, actual};
            --i;
        }
    }
    return count;
}

void sweep_demo_finish(sweep_demo_t *d)
{
    if (d->complete)
        return;
    d->errors = sweep_demo_errors(d, NULL);
    d->complete = true;
}

void sweep_demo_tick(sweep_demo_t *d)
{
    if (!d->complete)
        ++d->samples;
}

void sweep_demo_update(sweep_demo_t *d, const tuner_result_t *p)
{
    if (d->complete)
        return;
    if (p->tracking == TRACK_CONFIRMED && p->pitch.status == PITCH_VALID) {
        int midi = p->note.midi;
        if (!d->count || midi != d->notes[d->count - 1]) {
            if (!d->count) {
                if (d->direction == SWEEP_AUTO)
                    d->direction = midi <= 66 ? SWEEP_UP : SWEEP_DOWN;
                d->first_sample = d->samples;
            }
            d->notes[d->count++] = midi;
            d->last_sample = d->samples;
            if (d->count == SWEEP_NOTE_COUNT)
                sweep_demo_finish(d);
        }
    }
    // A partial sweep must still produce a result if some keys were missed.
    if (d->count && d->samples - d->last_sample >= SWEEP_TIMEOUT_SAMPLES)
        sweep_demo_finish(d);
}

float sweep_demo_accuracy(const sweep_demo_t *d)
{
    return 100.0f * (SWEEP_NOTE_COUNT - d->errors) / SWEEP_NOTE_COUNT;
}

float sweep_demo_rate(const sweep_demo_t *d)
{
    return d->count > 1 && d->last_sample > d->first_sample
        ? (float)(d->count - 1) * PITCH_SAMPLE_RATE / (d->last_sample - d->first_sample) : 0;
}

const char *sweep_demo_direction_name(sweep_direction_t direction)
{
    return direction == SWEEP_UP ? "UP" : direction == SWEEP_DOWN ? "DOWN" : "AUTO";
}
