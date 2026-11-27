#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "audio_capture.h"
#include "esp_err.h"
#include "esp_log.h"

#define REPORT_FRAMES 4096

static const char *TAG = "mic_test";
static int32_t rx_words[BLOCK_FRAMES][2];

typedef struct {
    double sum;
    double sum_sq;
    int32_t min;
    int32_t max;
    unsigned zero_count;
    unsigned clip_count;
    unsigned count;
} mic_stats_t;

static void add_sample(mic_stats_t *stats, int32_t sample)
{
    if (stats->count == 0 || sample < stats->min) stats->min = sample;
    if (stats->count == 0 || sample > stats->max) stats->max = sample;
    stats->sum += sample;
    stats->sum_sq += (double)sample * sample;
    stats->zero_count += sample == 0;
    stats->clip_count += sample <= -8388608 || sample >= 8388607;
    stats->count++;
}

static void report(const mic_stats_t *stats)
{
    double mean = stats->sum / stats->count;
    double variance = stats->sum_sq / stats->count - mean * mean;
    double rms = sqrt(fmax(0.0, variance));
    double dbfs = 20.0 * log10(fmax(rms / PCM24_SCALE, 1e-12));
    printf("samples=%u min=%" PRId32 " max=%" PRId32
           " dc=%.1f rms=%.1f dBFS=%.1f zeros=%u clips=%u\n",
           stats->count, stats->min, stats->max, mean, rms, dbfs,
           stats->zero_count, stats->clip_count);
}

void app_main(void)
{
    i2s_chan_handle_t rx = init_microphone();
    ESP_LOGI(TAG, "INMP441: %d Hz, Philips I2S, 32-bit stereo slots, left channel", SAMPLE_RATE_HZ);
    ESP_LOGI(TAG, "BCLK=GPIO%d WS=GPIO%d SD=GPIO%d; warming up", MIC_BCLK, MIC_WS, MIC_SD);
    unsigned warmup = WARMUP_FRAMES;
    mic_stats_t stats = {0};

    while (1) {
        size_t bytes_read = 0;
        esp_err_t err = i2s_channel_read(rx, rx_words, sizeof(rx_words), &bytes_read, 1000);
        if (err != ESP_OK || bytes_read % sizeof(rx_words[0]) != 0 || bytes_read == 0) {
            ESP_LOGW(TAG, "I2S read failed: %s, bytes=%zu", esp_err_to_name(err), bytes_read);
            stats = (mic_stats_t){0};
            continue;
        }
        size_t frames = bytes_read / sizeof(rx_words[0]);
        for (size_t i = 0; i < frames; ++i) {
            if (warmup > 0) {
                --warmup;
                continue;
            }
            // L/R tied to GND selects the left slot; ignore the right slot.
            add_sample(&stats, pcm24(rx_words[i][0]));
            if (stats.count == REPORT_FRAMES) {
                report(&stats);
                stats = (mic_stats_t){0};
            }
        }
    }
}
