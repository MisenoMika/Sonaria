#ifndef SONARIA_PITCH_DETECTOR_H
#define SONARIA_PITCH_DETECTOR_H

#include <stdbool.h>

#define PITCH_SAMPLE_RATE 16000
#define PITCH_FRAME_SIZE 1024
#define PITCH_HOP_SIZE 512
#define PITCH_MIN_HZ 80.0f
#define PITCH_MAX_HZ 2000.0f
#define PITCH_RMS_THRESHOLD 0.0005623413f
#define PITCH_MAX_LAG 201

typedef enum {
    PITCH_VALID,
    PITCH_SILENCE,
    PITCH_UNCERTAIN,
} pitch_status_t;

typedef struct {
    pitch_status_t status;
    float frequency_hz;
    float confidence;
    float rms;
} pitch_result_t;

// Reusable scratch storage; no heap allocation. Input is normalized PCM.
typedef struct {
    float difference[PITCH_MAX_LAG + 1];
    float normalized[PITCH_MAX_LAG + 1];
} pitch_detector_t;

pitch_result_t pitch_detect(pitch_detector_t *detector, const float samples[PITCH_FRAME_SIZE]);

#endif
