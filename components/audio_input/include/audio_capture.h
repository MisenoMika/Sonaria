#ifndef SONARIA_AUDIO_CAPTURE_H
#define SONARIA_AUDIO_CAPTURE_H
#include <stdint.h>
#include "driver/i2s_std.h"
#define SAMPLE_RATE_HZ 16000
#define MIC_BCLK GPIO_NUM_4
#define MIC_WS GPIO_NUM_5
#define MIC_SD GPIO_NUM_6
#define BLOCK_FRAMES 256
#define WARMUP_FRAMES 8192
#define PCM24_SCALE 8388608.0

i2s_chan_handle_t init_microphone(void);
int32_t pcm24(int32_t word);
#endif
