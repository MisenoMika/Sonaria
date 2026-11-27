#ifndef SONARIA_NOTE_EVENT_DETECTOR_H
#define SONARIA_NOTE_EVENT_DETECTOR_H
#include "practice_session.h"
#include "pitch_tracker.h"
#define PRACTICE_ENVELOPE_SAMPLES 64
#define PRACTICE_SILENCE_SAMPLES (PITCH_SAMPLE_RATE * 32 / 1000)
#define PRACTICE_DIP_SAMPLES (PITCH_SAMPLE_RATE * 8 / 1000)
#define PRACTICE_REPEAT_MIN_SAMPLES (PITCH_SAMPLE_RATE * 100 / 1000)
#define PRACTICE_ATTACK_WAIT_SAMPLES (PITCH_SAMPLE_RATE * 64 / 1000)
#define PRACTICE_ATTACK_EXPIRY_SAMPLES (PITCH_SAMPLE_RATE * 500 / 1000)
typedef struct
{
    uint64_t samples, last_event_sample, attack_sample;
    int last_midi;
    float sum_sq, peak, trough, reference, rms;
    unsigned block_samples, quiet_samples, dip_samples;
    bool dipping, attack_pending;
} note_event_detector_t;
void note_event_reset(note_event_detector_t *d);
// Called once for each newly acquired sample, not on overlapping frame data.
void note_event_sample(note_event_detector_t *d, float sample);
bool note_event_update(note_event_detector_t *d, const tuner_result_t *r,
                       practice_note_event_t *event);
#endif
