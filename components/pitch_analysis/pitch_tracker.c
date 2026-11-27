#include "pitch_tracker.h"
#include <string.h>

_Static_assert(PITCH_HOLD_MS % PITCH_HOP_MS == 0, "Hold must align to audio hops");
_Static_assert(PITCH_LIVE_HOLD_MS % PITCH_LIVE_HOP_MS == 0, "Live hold must align to audio hops");

void pitch_tracker_reset(pitch_tracker_t *tracker)
{
    memset(tracker, 0, sizeof(*tracker));
}

static void push(float history[3], unsigned *count, float value)
{
    if (*count < 3) history[(*count)++] = value;
    else {
        history[0] = history[1];
        history[1] = history[2];
        history[2] = value;
    }
}

static float median3(float a, float b, float c)
{
    if (a > b) { float t = a; a = b; b = t; }
    if (b > c) { float t = b; b = c; c = t; }
    return a > b ? a : b;
}

static void clear_candidate(pitch_tracker_t *tracker)
{
    tracker->candidate_frames = 0;
    tracker->candidate_count = 0;
}

static void update_confirmed(pitch_tracker_t *tracker, pitch_result_t raw)
{
    tracker->confirmed_pitch = raw;
    if (tracker->confirmed_count == 3)
        tracker->confirmed_pitch.frequency_hz = median3(tracker->confirmed_history[0], tracker->confirmed_history[1], tracker->confirmed_history[2]);
    frequency_to_note(tracker->confirmed_pitch.frequency_hz, &tracker->confirmed_note);
    tracker->has_confirmed = true;
    tracker->lost_frames = 0;
}

static tuner_result_t output(const pitch_tracker_t *tracker, pitch_result_t raw, pitch_tracking_state_t state)
{
    tuner_result_t result = {.raw = raw, .tracking = state,
        .candidate_midi = tracker->candidate_frames ? tracker->candidate_midi : -1,
        .candidate_frames = tracker->candidate_frames};
    if (state == TRACK_CONFIRMED || state == TRACK_HOLD) {
        result.pitch = tracker->confirmed_pitch;
        result.note = tracker->confirmed_note;
    } else {
        result.pitch = (pitch_result_t){.status = raw.status == PITCH_SILENCE ? PITCH_SILENCE : PITCH_UNCERTAIN,
                                       .rms = raw.rms, .confidence = raw.confidence};
    }
    return result;
}

static tuner_result_t update(pitch_tracker_t *tracker, pitch_result_t raw, unsigned confirm_frames, unsigned hold_frames)
{
    note_result_t note = {0};
    const bool valid = raw.status == PITCH_VALID && frequency_to_note(raw.frequency_hz, &note);
    if (tracker->has_confirmed && valid && note.midi == tracker->confirmed_note.midi &&
        raw.confidence >= PITCH_MAINTAIN_CONFIDENCE) {
        if (tracker->lost_frames) tracker->confirmed_count = 0;
        push(tracker->confirmed_history, &tracker->confirmed_count, raw.frequency_hz);
        update_confirmed(tracker, raw);
        clear_candidate(tracker);
        return output(tracker, raw, TRACK_CONFIRMED);
    }

    if (valid && raw.confidence >= PITCH_CONFIRM_CONFIDENCE) {
        if (!tracker->candidate_frames || note.midi != tracker->candidate_midi) {
            clear_candidate(tracker);
            tracker->candidate_midi = note.midi;
        }
        ++tracker->candidate_frames;
        push(tracker->candidate_history, &tracker->candidate_count, raw.frequency_hz);
        if (tracker->candidate_frames >= confirm_frames) {
            memcpy(tracker->confirmed_history, tracker->candidate_history, sizeof(tracker->confirmed_history));
            tracker->confirmed_count = tracker->candidate_count;
            update_confirmed(tracker, raw);
            clear_candidate(tracker);
            return output(tracker, raw, TRACK_CONFIRMED);
        }
    } else {
        clear_candidate(tracker);
    }

    if (tracker->has_confirmed) {
        // First lost frame starts the hold clock.
        if (tracker->lost_frames++ < hold_frames)
            return output(tracker, raw, TRACK_HOLD);
        tracker->has_confirmed = false;
        tracker->confirmed_count = 0;
        tracker->lost_frames = 0;
    }
    return output(tracker, raw, tracker->candidate_frames ? TRACK_PENDING : TRACK_IDLE);
}

tuner_result_t pitch_tracker_update_with_confirmation(pitch_tracker_t *tracker, pitch_result_t raw, unsigned confirm_frames)
{
    return update(tracker, raw, confirm_frames, PITCH_HOLD_MS / PITCH_HOP_MS);
}

tuner_result_t pitch_tracker_update(pitch_tracker_t *tracker, pitch_result_t raw)
{
    return pitch_tracker_update_with_confirmation(tracker, raw, PITCH_CONFIRM_FRAMES);
}

tuner_result_t pitch_track(pitch_tracker_t *tracker, const float samples[PITCH_FRAME_SIZE])
{
    return pitch_tracker_update(tracker, pitch_detect(&tracker->detector, samples));
}

tuner_result_t pitch_track_live(pitch_tracker_t *tracker, const float samples[PITCH_FRAME_SIZE])
{
    return update(tracker, pitch_detect(&tracker->detector, samples),
                  PITCH_LIVE_CONFIRM_FRAMES, PITCH_LIVE_HOLD_MS / PITCH_LIVE_HOP_MS);
}

const char *pitch_tracking_name(pitch_tracking_state_t state)
{
    switch (state) {
    case TRACK_PENDING: return "PENDING";
    case TRACK_CONFIRMED: return "CONFIRMED";
    case TRACK_HOLD: return "HOLD";
    default: return "IDLE";
    }
}
