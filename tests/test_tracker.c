#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pitch_tracker.h"
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); exit(1); } } while (0)
static pitch_tracker_t tracker;
static float frame[PITCH_FRAME_SIZE];
static tuner_result_t sample(float frequency, float confidence)
{
    return pitch_tracker_update(&tracker, (pitch_result_t){.status=PITCH_VALID, .frequency_hz=frequency, .confidence=confidence, .rms=.02f});
}
static tuner_result_t silence(void)
{
    return pitch_tracker_update(&tracker, (pitch_result_t){.status=PITCH_SILENCE});
}
static tuner_result_t establish(float frequency)
{
    for (unsigned i=1; i<PITCH_CONFIRM_FRAMES; ++i) {
        tuner_result_t r=sample(frequency,.98f);
        CHECK(r.tracking==TRACK_PENDING && r.pitch.status!=PITCH_VALID && r.note.name==NULL && r.candidate_frames==i);
    }
    tuner_result_t r=sample(frequency,.98f);
    CHECK(r.tracking==TRACK_CONFIRMED && r.pitch.status==PITCH_VALID);
    return r;
}
int main(void)
{
    tuner_result_t r=establish(440);
    CHECK(r.note.midi==69);
    float old=r.pitch.frequency_hz;
    for (unsigned i=1;i<PITCH_CONFIRM_FRAMES;++i) {
        r=sample(880,.99f);
        CHECK(r.tracking==TRACK_HOLD && r.note.midi==69 && r.pitch.frequency_hz==old && r.candidate_frames==i);
    }
    r=sample(440,.86f);
    CHECK(r.tracking==TRACK_CONFIRMED && r.note.midi==69 && r.candidate_frames==0);
    for (unsigned i=1;i<PITCH_CONFIRM_FRAMES;++i) CHECK(sample(880,.99f).tracking==TRACK_HOLD);
    r=sample(880,.99f);
    CHECK(r.tracking==TRACK_CONFIRMED && r.note.midi==81 && fabsf(r.pitch.frequency_hz-880)<.01f);
    for (unsigned i=0;i<4;++i) CHECK(silence().tracking==TRACK_HOLD);
    r=silence();
    CHECK(r.tracking==TRACK_IDLE && r.pitch.status==PITCH_SILENCE && r.note.name==NULL);
    for (unsigned i=0;i<8;++i) CHECK(sample(220,.89f).pitch.status!=PITCH_VALID);
    for (unsigned i=0;i<8;++i) CHECK(sample(i%2?220:440,.98f).pitch.status!=PITCH_VALID);
    pitch_tracker_reset(&tracker);
    CHECK(establish(220).note.midi==57);
    for (unsigned i=0;i<4;++i) CHECK(sample(220,.84f).tracking==TRACK_HOLD);
    r=sample(220,.84f); CHECK(r.tracking==TRACK_IDLE && r.pitch.status==PITCH_UNCERTAIN);
    pitch_tracker_reset(&tracker);
    CHECK(sample(440,.90f).tracking==TRACK_PENDING);
    silence();
    CHECK(sample(440,.95f).candidate_frames==1);
    pitch_tracker_reset(&tracker);
    for (unsigned n=0;n<4;++n) {
        unsigned random=2345+n;
        for (int i=0;i<PITCH_FRAME_SIZE;++i) {
            random=random*1664525u+1013904223u;
            frame[i]=.03f*sinf(6.283185307179586f*440*i/PITCH_SAMPLE_RATE)+.006f*((random>>8)/8388608.0f-1);
        }
        r=pitch_track(&tracker,frame);
        CHECK(r.tracking==(n==3?TRACK_CONFIRMED:TRACK_PENDING));
    }
    CHECK(r.note.midi==69);
    r=sample(441,.96f); CHECK(r.tracking==TRACK_CONFIRMED && r.note.midi==69);
    r=sample(439,.96f); CHECK(r.tracking==TRACK_CONFIRMED && r.note.midi==69 && fabsf(r.pitch.frequency_hz-440)<1);
    // Changing candidates must not reset the old note's hold deadline.
    CHECK(sample(880,.99f).tracking==TRACK_HOLD);
    CHECK(sample(220,.99f).tracking==TRACK_HOLD);
    CHECK(sample(880,.99f).tracking==TRACK_HOLD);
    CHECK(sample(220,.99f).tracking==TRACK_HOLD);
    r=sample(880,.99f); CHECK(r.tracking==TRACK_PENDING && r.note.name==NULL && r.candidate_frames==1);
    CHECK(sample(880,.99f).candidate_frames==2);
    CHECK(sample(880,.99f).candidate_frames==3);
    CHECK(sample(880,.99f).note.midi==81);
    pitch_tracker_reset(&tracker);
    unsigned random=12345;
    for (unsigned n=0;n<8;++n) {
        for (int i=0;i<PITCH_FRAME_SIZE;++i) {
            random=random*1664525u+1013904223u;
            frame[i]=.03f*((random>>8)/8388608.0f-1);
        }
        CHECK(pitch_track(&tracker,frame).pitch.status!=PITCH_VALID);
    }
    pitch_tracker_reset(&tracker);
    CHECK(sample(440,.95f).tracking==TRACK_PENDING);
    puts("Stable pitch tracker tests passed");
    return 0;
}
