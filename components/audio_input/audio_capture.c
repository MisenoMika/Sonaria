#include "audio_capture.h"
#include "esp_err.h"

i2s_chan_handle_t init_microphone(void)
{
    i2s_chan_handle_t rx = NULL;
    i2s_chan_config_t channel = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&channel, NULL, &rx));

    // INMP441 requires 64 BCLK cycles per stereo frame, even with one mic.
    i2s_std_config_t config = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE_HZ),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT,
                                                       I2S_SLOT_MODE_STEREO),
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = MIC_BCLK,
            .ws = MIC_WS,
            .dout = I2S_GPIO_UNUSED,
            .din = MIC_SD,
        },
    };
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx, &config));
    ESP_ERROR_CHECK(i2s_channel_enable(rx));
    return rx;
}

int32_t pcm24(int32_t word)
{
    // The signed 24-bit sample occupies the upper 24 bits of the slot.
    return word >> 8;
}

