/* Host-only helper for replay_latency.py: production detector, tracker and renderer. */
#include "pitch_tracker.h"
#include "tuner_screen.h"
#include <stdio.h>
#include <string.h>

static float frame[PITCH_FRAME_SIZE];
static pitch_detector_t raw;
static pitch_tracker_t tracker;
static oled_frame_t display;
static float legacy_history[3];
static unsigned legacy_count;

// Historical three-frame median only, as used before stable note confirmation.
static int legacy_note(pitch_result_t r)
{
    if (r.status != PITCH_VALID) { legacy_count=0; return -1; }
    if (legacy_count<3) legacy_history[legacy_count++]=r.frequency_hz;
    else { legacy_history[0]=legacy_history[1]; legacy_history[1]=legacy_history[2]; legacy_history[2]=r.frequency_hz; }
    if (legacy_count==3) {
        float a=legacy_history[0], b=legacy_history[1], c=legacy_history[2];
        if (a>b) { float t=a; a=b; b=t; }
        if (b>c) { float t=b; b=c; c=t; }
        r.frequency_hz=a>b?a:b;
    }
    note_result_t n;
    return frequency_to_note(r.frequency_hz,&n)?n.midi:-1;
}

int main(int argc, char **argv)
{
    if (argc != 2) return 1;
    FILE *out = fopen(argv[1], "wb");
    if (!out) return 1;
    if (fread(frame, sizeof(float), PITCH_FRAME_SIZE, stdin) != PITCH_FRAME_SIZE) {
        fclose(out);
        return 1;
    }
    unsigned end = PITCH_FRAME_SIZE;
    do {
        pitch_result_t r = pitch_detect(&raw, frame);
        note_result_t n = {0};
        if (r.status == PITCH_VALID) frequency_to_note(r.frequency_hz, &n);
        tuner_result_t t = pitch_track(&tracker, frame);
        tuner_render(&display, &t);
        if (fwrite(&display, sizeof(display), 1, out) != 1) { fclose(out); return 1; }
        int legacy=legacy_note(r);
        printf("%u,%d,%d,%d,%d,%d,%d,%d\n", end, r.status, n.midi, t.pitch.status, t.note.midi,
               t.tracking, legacy<0?PITCH_UNCERTAIN:PITCH_VALID, legacy);
        memmove(frame, frame + PITCH_HOP_SIZE, (PITCH_FRAME_SIZE-PITCH_HOP_SIZE)*sizeof(float));
        end += PITCH_HOP_SIZE;
    } while (fread(frame+PITCH_FRAME_SIZE-PITCH_HOP_SIZE, sizeof(float), PITCH_HOP_SIZE, stdin) == PITCH_HOP_SIZE);
    return fclose(out) != 0;
}
