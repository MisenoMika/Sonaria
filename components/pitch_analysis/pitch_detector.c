#include "pitch_detector.h"
#include <math.h>

#define YIN_WINDOW 512
#define YIN_THRESHOLD 0.15f
#define MIN_LAG 8
#define YIN_START (PITCH_FRAME_SIZE-YIN_WINDOW-PITCH_MAX_LAG)
_Static_assert(YIN_START>=0, "YIN window must fit in one frame");

pitch_result_t pitch_detect(pitch_detector_t *detector, const float samples[PITCH_FRAME_SIZE])
{
    pitch_result_t result = {.status = PITCH_UNCERTAIN};
    float mean = 0.0f;
    for (int i = 0; i < PITCH_FRAME_SIZE; ++i) mean += samples[i];
    mean /= PITCH_FRAME_SIZE;
    float energy = 0.0f;
    for (int i = 0; i < PITCH_FRAME_SIZE; ++i) {
        float centered = samples[i] - mean;
        energy += centered * centered;
    }
    result.rms = sqrtf(energy / PITCH_FRAME_SIZE);
    if (!isfinite(result.rms)) return result;
    if (result.rms < PITCH_RMS_THRESHOLD) {
        result.status = PITCH_SILENCE;
        return result;
    }

    // A fixed window keeps the cost and comparison length identical for every lag.
    // Align the comparison to the newest samples rather than leaving 19.4 ms unused.
    // Subtraction also cancels DC, so no separate centered buffer is needed.
    detector->difference[0] = 0.0f;
    detector->normalized[0] = 1.0f;
    float running_sum = 0.0f;
    for (int next = 1; next <= PITCH_MAX_LAG; ++next) {
        int lag = next;
        float sum = 0.0f;
        for (int i = YIN_START; i < YIN_START + YIN_WINDOW; ++i) {
            float delta = samples[i] - samples[i + lag];
            sum += delta * delta;
        }
        detector->difference[lag] = sum;
        running_sum += sum;
        detector->normalized[lag] = running_sum > 0.0f ? sum * lag / running_sum : 1.0f;

        // Once the right neighbor exists, evaluate the first convincing trough.
        // Later lags cannot alter this decision.
        if (next < 3) continue;
        lag = next - 1;
        const float *cmnd = detector->normalized;
        if (cmnd[lag] >= YIN_THRESHOLD || cmnd[lag] > cmnd[lag - 1] || cmnd[lag] > cmnd[lag + 1]) continue;
        if (lag < MIN_LAG) return result;

        // Interpolate the raw difference trough to avoid the high-frequency
        // bias introduced by the lag-dependent normalization factor.
        const float *diff = detector->difference;
        float denominator = diff[lag - 1] - 2.0f * diff[lag] + diff[lag + 1];
        float offset = denominator > 0.0f ? 0.5f * (diff[lag - 1] - diff[lag + 1]) / denominator : 0.0f;
        offset = fmaxf(-0.5f, fminf(0.5f, offset));
        float frequency = PITCH_SAMPLE_RATE / (lag + offset);
        // Allow only a small interpolation error at the exact range endpoints.
        if (frequency < PITCH_MIN_HZ * 0.997f || frequency > PITCH_MAX_HZ * 1.003f) return result;
        result.frequency_hz = fmaxf(PITCH_MIN_HZ, fminf(PITCH_MAX_HZ, frequency));
        result.confidence = 1.0f - cmnd[lag];
        result.status = PITCH_VALID;
        return result;
    }
    return result;
}
