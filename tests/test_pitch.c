#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "pitch_detector.h"

#define PI 3.14159265358979323846
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); exit(1); } } while (0)

static float frame[PITCH_FRAME_SIZE];
static pitch_detector_t detector;

static void sine(float frequency, float amplitude, float phase)
{
    for (int i = 0; i < PITCH_FRAME_SIZE; ++i)
        frame[i] = amplitude * sin(2 * PI * frequency * i / PITCH_SAMPLE_RATE + phase);
}

int main(void)
{
    sine(440, 0.03f, 0.2f);
    pitch_result_t result = pitch_detect(&detector, frame);
    CHECK(result.status == PITCH_VALID);
    CHECK(fabs(1200 * log2(result.frequency_hz / 440.0)) <= 5);
    const float frequencies[] = {80, 82.41f, 110, 220, 261.63f, 329.63f, 523.25f, 880, 1760, 1990, 2000};
    for (size_t i = 0; i < sizeof(frequencies) / sizeof(frequencies[0]); ++i) {
        sine(frequencies[i], 0.03f, 1.3f);
        result = pitch_detect(&detector, frame);
        CHECK(result.status == PITCH_VALID);
        double error = 1200 * log2(result.frequency_hz / frequencies[i]);
        printf("%.2f Hz -> %.3f Hz (%+.3f cents)\n", frequencies[i], result.frequency_hz, error);
        CHECK(fabs(error) <= 5);
    }
    double worst_error = 0;
    for (int frequency = 80; frequency <= 2000; frequency += 5) {
        for (int phase = 0; phase < 3; ++phase) {
            sine(frequency, 0.03f, phase * 1.3f);
            result = pitch_detect(&detector, frame);
            CHECK(result.status == PITCH_VALID);
            double error = fabs(1200 * log2(result.frequency_hz / frequency));
            if (error > worst_error) worst_error = error;
            if (error > 5) fprintf(stderr, "Sweep %d Hz phase %d: %.3f cents\n", frequency, phase, error);
            CHECK(error <= 5);
        }
    }
    printf("Sweep worst error: %.3f cents\n", worst_error);
    for (int i = 0; i < PITCH_FRAME_SIZE; ++i) {
        const double t = 2 * PI * 220 * i / PITCH_SAMPLE_RATE;
        frame[i] = 0.02f * sin(t) + 0.04f * sin(2*t) + 0.015f * sin(3*t) + 0.1f;
    }
    result = pitch_detect(&detector, frame);
    CHECK(result.status == PITCH_VALID);
    CHECK(fabs(1200 * log2(result.frequency_hz / 220.0)) <= 5);
    sine(440, 0.0001f, 0);
    CHECK(pitch_detect(&detector, frame).status == PITCH_SILENCE);
    for (int i = 0; i < PITCH_FRAME_SIZE; ++i) frame[i] = 0.25f;
    CHECK(pitch_detect(&detector, frame).status == PITCH_SILENCE);
    unsigned random = 12345;
    for (int i = 0; i < PITCH_FRAME_SIZE; ++i) {
        random = random * 1664525u + 1013904223u;
        frame[i] = 0.03f * ((random >> 8) / 8388608.0f - 1.0f);
    }
    CHECK(pitch_detect(&detector, frame).status == PITCH_UNCERTAIN);
    sine(3000, 0.03f, 0);
    CHECK(pitch_detect(&detector, frame).status != PITCH_VALID);
    sine(55, 0.03f, 0);
    CHECK(pitch_detect(&detector, frame).status != PITCH_VALID);
    puts("Pitch sine, harmonic, silence and noise tests passed");
    return 0;
}
