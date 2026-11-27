#ifndef SONARIA_PITCH_TRACKER_H
#define SONARIA_PITCH_TRACKER_H
#include "pitch_detector.h"
#include "note_mapper.h"

#define PITCH_CONFIRM_FRAMES 4
#define PITCH_CONFIRM_CONFIDENCE 0.90f
#define PITCH_MAINTAIN_CONFIDENCE 0.85f
#define PITCH_HOLD_MS 128
#define PITCH_HOP_MS (PITCH_HOP_SIZE * 1000 / PITCH_SAMPLE_RATE)

typedef enum {
    TRACK_IDLE,
    TRACK_PENDING,
    TRACK_CONFIRMED,
    TRACK_HOLD,
} pitch_tracking_state_t;

typedef struct {
    pitch_result_t pitch; // Confirmed/held output, otherwise no valid frequency.
    note_result_t note;
    pitch_result_t raw;   // Current frame, including during HOLD.
    pitch_tracking_state_t tracking;
    int candidate_midi;  // -1 means no candidate.
    unsigned candidate_frames;
} tuner_result_t;

typedef struct {
    pitch_detector_t detector;
    bool has_confirmed;
    pitch_result_t confirmed_pitch;
    note_result_t confirmed_note;
    float confirmed_history[3];
    unsigned confirmed_count;
    int candidate_midi;
    unsigned candidate_frames;
    float candidate_history[3];
    unsigned candidate_count;
    unsigned lost_frames;
} pitch_tracker_t;

// Initialize to zero before first use; reset after a discontinuity in audio.
void pitch_tracker_reset(pitch_tracker_t *tracker);
// One update per audio hop. Accepts raw detector output, for deterministic replay as well.
tuner_result_t pitch_tracker_update(pitch_tracker_t *tracker, pitch_result_t raw);
tuner_result_t pitch_tracker_update_with_confirmation(pitch_tracker_t *tracker, pitch_result_t raw, unsigned confirm_frames);
tuner_result_t pitch_track(pitch_tracker_t *tracker, const float samples[PITCH_FRAME_SIZE]);
const char *pitch_tracking_name(pitch_tracking_state_t state);
#endif
