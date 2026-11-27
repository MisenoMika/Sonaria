#ifndef SONARIA_SWEEP_DEMO_H
#define SONARIA_SWEEP_DEMO_H
#include "pitch_tracker.h"
#include <stdint.h>

#define SWEEP_NOTE_COUNT 22
typedef enum { SWEEP_AUTO, SWEEP_UP, SWEEP_DOWN } sweep_direction_t;
typedef struct {
    sweep_direction_t direction;
    bool complete;
    unsigned count;
    int notes[SWEEP_NOTE_COUNT];
    unsigned errors;
    uint64_t samples, first_sample, last_sample;
} sweep_demo_t;

typedef enum { SWEEP_WRONG, SWEEP_MISSING, SWEEP_EXTRA } sweep_error_kind_t;
typedef struct {
    sweep_error_kind_t kind;
    unsigned expected_pos, actual_pos; // 1-based; 0 when absent.
    int expected_midi, actual_midi;     // -1 when absent.
} sweep_error_t;

// Reconstruct the same minimum-edit alignment used for the final accuracy.
unsigned sweep_demo_errors(const sweep_demo_t *demo, sweep_error_t errors[SWEEP_NOTE_COUNT]);

void sweep_demo_start(sweep_demo_t *demo, sweep_direction_t direction);
int sweep_demo_expected(sweep_direction_t direction, unsigned index);
// Count capture samples independently of the analysis hop.
void sweep_demo_tick(sweep_demo_t *demo);
void sweep_demo_update(sweep_demo_t *demo, const tuner_result_t *pitch);
void sweep_demo_finish(sweep_demo_t *demo);
float sweep_demo_accuracy(const sweep_demo_t *demo);
float sweep_demo_rate(const sweep_demo_t *demo);
const char *sweep_demo_direction_name(sweep_direction_t direction);
#endif
