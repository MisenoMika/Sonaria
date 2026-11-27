#include "note_event_detector.h"
#include <math.h>
void note_event_reset(note_event_detector_t *d) { *d = (note_event_detector_t){.last_midi = -1}; }
static void attack(note_event_detector_t *d)
{
    d->attack_sample = d->samples;
    d->attack_pending = true;
    d->dipping = false;
    d->dip_samples = 0;
    d->peak = d->rms;
}
void note_event_sample(note_event_detector_t *d, float sample)
{
    ++d->samples;
    d->sum_sq += sample * sample;
    if (++d->block_samples < PRACTICE_ENVELOPE_SAMPLES)
        return;
    d->rms = sqrtf(d->sum_sq / PRACTICE_ENVELOPE_SAMPLES);
    d->sum_sq = 0;
    d->block_samples = 0;
    if (d->attack_pending && d->samples - d->attack_sample > PRACTICE_ATTACK_EXPIRY_SAMPLES)
        d->attack_pending = false;
    if (d->rms < PITCH_RMS_THRESHOLD)
    {
        if (d->quiet_samples < PRACTICE_SILENCE_SAMPLES)
            d->quiet_samples += PRACTICE_ENVELOPE_SAMPLES;
    }
    else
    {
        if (d->quiet_samples >= PRACTICE_SILENCE_SAMPLES)
        {
            attack(d);
            d->quiet_samples = 0;
            return;
        }
        d->quiet_samples = 0;
    }
    // Compare a local low valley to the pre-valley peak, not a fixed absolute gain.
    if (d->dipping)
    {
        if (d->rms < d->trough)
            d->trough = d->rms;
        if (d->rms >= PITCH_RMS_THRESHOLD &&
            (d->rms >= .60f * d->reference || d->rms >= 2 * d->trough))
        {
            attack(d);
            return;
        }
    }
    else if (d->peak > PITCH_RMS_THRESHOLD && d->rms <= .5f * d->peak)
    {
        if (!d->dip_samples)
        {
            d->reference = d->peak;
            d->trough = d->rms;
        }
        if (d->rms < d->trough)
            d->trough = d->rms;
        d->dip_samples += PRACTICE_ENVELOPE_SAMPLES;
        if (d->dip_samples >= PRACTICE_DIP_SAMPLES)
            d->dipping = true;
    }
    else
    {
        d->dip_samples = 0;
    }
    if (!d->dipping)
        d->peak = fmaxf(d->rms, d->peak * .999f);
}
bool note_event_update(note_event_detector_t *d, const tuner_result_t *r,
                       practice_note_event_t *event)
{
    note_result_t raw;
    if (r->tracking != TRACK_CONFIRMED || r->pitch.status != PITCH_VALID ||
        r->raw.status != PITCH_VALID || r->raw.confidence < PITCH_MAINTAIN_CONFIDENCE ||
        !frequency_to_note(r->raw.frequency_hz, &raw) || raw.midi != r->note.midi)
        return false;
    bool repeat = r->note.midi == d->last_midi;
    if (repeat && (!d->attack_pending || d->attack_sample <= d->last_event_sample ||
                   d->samples - d->attack_sample < PRACTICE_ATTACK_WAIT_SAMPLES ||
                   d->samples - d->attack_sample > PRACTICE_ATTACK_EXPIRY_SAMPLES ||
                   d->samples - d->last_event_sample < PRACTICE_REPEAT_MIN_SAMPLES))
        return false;
    *event =
        (practice_note_event_t){.midi = r->note.midi,
                                .frequency = r->pitch.frequency_hz,
                                .cents = r->note.cents,
                                .sample = d->samples,
                                .articulation = repeat ? PRACTICE_REATTACK : PRACTICE_NEW_NOTE};
    d->last_midi = event->midi;
    d->last_event_sample = d->samples;
    d->attack_pending = false;
    return true;
}
