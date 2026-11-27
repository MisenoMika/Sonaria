#ifndef SONARIA_MELODY_DETECTOR_H
#define SONARIA_MELODY_DETECTOR_H
#include "pitch_detector.h"

// Upper-voice mode: configure the expected register, not the song's notes.
#define MELODY_MIN_MIDI 60
#define MELODY_MAX_MIDI 84
#define MELODY_CONFIRM_FRAMES 2
#define MELODY_FFT_SIZE 2048
#define MELODY_PEAK_MIN_MIDI 36
#define MELODY_PEAK_MAX_MIDI 96
#define MELODY_PEAK_COUNT (MELODY_PEAK_MAX_MIDI-MELODY_PEAK_MIN_MIDI+1)

typedef struct {
    float real[MELODY_FFT_SIZE], imag[MELODY_FFT_SIZE];
    float twiddle_real[MELODY_FFT_SIZE/2], twiddle_imag[MELODY_FFT_SIZE/2];
    float window[PITCH_FRAME_SIZE];
    float previous[MELODY_PEAK_COUNT];
    int previous_note;
    bool initialized;
} melody_detector_t;

// Allocate zero-initialized static storage; initialization is lazy.
void melody_detector_init(melody_detector_t *detector);
void melody_detector_reset(melody_detector_t *detector);
pitch_result_t melody_detect(melody_detector_t *detector, const float samples[PITCH_FRAME_SIZE]);
#endif
