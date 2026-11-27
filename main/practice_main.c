#include "audio_capture.h"
#include "practice_control.h"
#include "practice_display.h"
#include "driver/uart.h"
#include "driver/uart_vfs.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define COMMAND_MAX 80
#define COMMAND_QUEUE_LENGTH 8
static practice_control_t control;
static QueueHandle_t commands;
static int32_t rx_words[BLOCK_FRAMES][2];
typedef struct
{
    char line[COMMAND_MAX];
} command_t;

static void emit(const char *line, void *ctx)
{
    (void)ctx;
    puts(line);
}

static void serial_task(void *arg)
{
    (void)arg;
    command_t cmd = {0};
    size_t length = 0;
    bool overflow = false;
    for (;;)
    {
        uint8_t bytes[64];
        int count = uart_read_bytes(UART_NUM_0, bytes, sizeof bytes, pdMS_TO_TICKS(50));
        for (int i = 0; i < count; ++i)
        {
            uint8_t ch = bytes[i];
            if (ch == '\r' || ch == '\n')
            {
                if (overflow)
                    puts("ERROR command too long");
                else if (length)
                {
                    cmd.line[length] = 0;
                    if (xQueueSend(commands, &cmd, 0) != pdTRUE)
                        puts("ERROR command queue full");
                }
                length = 0;
                overflow = false;
            }
            else if (ch == 8 || ch == 127)
            {
                if (length && !overflow)
                    --length;
            }
            else if (ch >= 32 && ch <= 126)
            {
                if (length < sizeof(cmd.line) - 1 && !overflow)
                    cmd.line[length++] = (char)ch;
                else
                    overflow = true;
            }
        }
    }
}

static void publish_view(void)
{
    if (control.mode == PRACTICE_MODE_LIVE)
        practice_display_publish_live(&control.engine.pitch, control.debug);
    else if (control.mode == PRACTICE_MODE_DEMO)
        practice_display_publish_demo(&control.demo, control.debug);
    else
        practice_display_publish(&control.engine.session, control.debug);
}

static void serial_start(void)
{
    commands = xQueueCreate(COMMAND_QUEUE_LENGTH, sizeof(command_t));
    ESP_ERROR_CHECK(commands ? ESP_OK : ESP_ERR_NO_MEM);
    if (!uart_is_driver_installed(UART_NUM_0))
        ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 512, 2048, 0, NULL, 0));
    uart_vfs_dev_use_driver(UART_NUM_0);
    ESP_ERROR_CHECK(xTaskCreate(serial_task, "practice_uart", 4096, NULL, 2, NULL) == pdPASS
                        ? ESP_OK
                        : ESP_ERR_NO_MEM);
}

void app_main(void)
{
    // Normal mode deliberately suppresses library informational/warning logs.
    esp_log_level_set("*", ESP_LOG_ERROR);
    practice_control_init(&control);
    serial_start();
    practice_display_start();
    publish_view();
    i2s_chan_handle_t rx = init_microphone();
    unsigned warmup = WARMUP_FRAMES, report_count = 0;
    uint64_t last_attack = 0;
    pitch_tracking_state_t previous = TRACK_IDLE;
    int previous_midi = -1;
    for (;;)
    {
        command_t command;
        while (xQueueReceive(commands, &command, 0) == pdTRUE)
        {
            if (practice_control_command(&control, command.line, emit, NULL))
            {
                report_count = 0;
                previous = TRACK_IDLE;
                previous_midi = -1;
                last_attack = control.engine.events.attack_sample;
                publish_view();
            }
        }
        size_t bytes = 0;
        esp_err_t err = i2s_channel_read(rx, rx_words, sizeof rx_words, &bytes, 1000);
        if (err != ESP_OK || !bytes || bytes % sizeof(rx_words[0]))
        {
            ESP_LOGE("practice", "I2S read failed: %s, bytes=%zu", esp_err_to_name(err), bytes);
            practice_engine_reset_audio(&control.engine);
            last_attack = 0;
            if (control.mode == PRACTICE_MODE_LIVE)
                publish_view();
            continue;
        }
        for (size_t i = 0; i < bytes / sizeof(rx_words[0]); ++i)
        {
            if (warmup)
            {
                --warmup;
                continue;
            }
            if (!practice_control_listening(&control))
                continue;
            bool frame_due = control.engine.filled + 1 == PITCH_FRAME_SIZE;
            int64_t start = frame_due ? esp_timer_get_time() : 0;
            practice_result_t result;
            bool event;
            if (!practice_control_sample(&control, pcm24(rx_words[i][0]) / (float)PCM24_SCALE,
                                         &event, &result))
                continue;
            int64_t compute = esp_timer_get_time() - start;
            if (control.mode != PRACTICE_MODE_SCORE)
                publish_view();
            if (control.mode == PRACTICE_MODE_DEMO && control.demo.complete)
                practice_control_report_demo(&control, emit, NULL);
            if (event)
            {
                practice_control_report_event(&control, &result, emit, NULL);
                publish_view();
            }
            if (control.debug)
            {
                const tuner_result_t *r = &control.engine.pitch;
                note_event_detector_t *d = &control.engine.events;
                int midi = r->pitch.status == PITCH_VALID ? r->note.midi : -1;
                if (control.mode == PRACTICE_MODE_SCORE && d->attack_sample != last_attack)
                {
                    printf("DEBUG onset sample=%" PRIu64
                           " rms=%.6f trough=%.6f reference=%.6f pending=%d\n",
                           d->attack_sample, d->rms, d->trough, d->reference, d->attack_pending);
                    last_attack = d->attack_sample;
                }
                bool live = control.mode != PRACTICE_MODE_SCORE;
                if (++report_count >= (live ? 6 : 3) || r->tracking != previous ||
                    midi != previous_midi || event)
                {
                    char note[8], target[8];
                    practice_note_name(midi, note, sizeof note);
                    const practice_session_t *s = &control.engine.session;
                    practice_note_name(control.mode == PRACTICE_MODE_SCORE &&
                                               s->position < s->score->count
                                           ? s->score->notes[s->position].midi
                                           : -1,
                                       target, sizeof target);
                    printf("DEBUG pitch mode=%s frequency=%.2f note=%s target=%s tracking=%s "
                           "candidate=%d "
                           "count=%u/%d confidence=%.3f dBFS=%.1f compute_us=%" PRId64 "\n",
                           control.mode == PRACTICE_MODE_DEMO ? "demo" : live ? "live" : "practice",
                           r->pitch.frequency_hz, note, target, pitch_tracking_name(r->tracking),
                           r->candidate_midi, r->candidate_frames, live ? PITCH_LIVE_CONFIRM_FRAMES : PITCH_CONFIRM_FRAMES,
                           r->raw.confidence, 20.0f * log10f(fmaxf(r->raw.rms, 1e-12f)), compute);
                    report_count = 0;
                    previous = r->tracking;
                    previous_midi = midi;
                }
            }
        }
    }
}
