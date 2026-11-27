#include "tuner_display.h"
#include "tuner_screen.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <inttypes.h>

static QueueHandle_t latest;

typedef struct {
    tuner_result_t result;
    int64_t published_us;
    int64_t compute_us;
} display_packet_t;

static void display_task(void *arg)
{
    (void)arg;
    oled_init();
    tuner_result_t result = {.pitch = {.status = PITCH_SILENCE}};
    tuner_render(oled_frame(), &result);
    oled_present();
    ESP_LOGI("tuner_display", "SSD1351 ready, event-driven dirty-row refresh, SPI %d Hz", OLED_CLOCK_HZ);
    display_packet_t packet;
    int last_midi = -1;
    pitch_status_t last_status = PITCH_SILENCE;
    pitch_tracking_state_t last_tracking = TRACK_IDLE;
    int64_t last_log_us = 0;
    for (;;) {
        // Wait for a new result rather than sampling the queue every 200 ms.
        if (xQueueReceive(latest, &packet, portMAX_DELAY) != pdTRUE) continue;
        int64_t started = esp_timer_get_time();
        tuner_render(oled_frame(), &packet.result);
        int64_t rendered = esp_timer_get_time();
        unsigned rows = oled_present();
        int64_t finished = esp_timer_get_time();
        int midi = packet.result.pitch.status == PITCH_VALID ? packet.result.note.midi : -1;
        // Keep diagnostics bounded; log note/status changes plus one periodic sample.
        if (midi != last_midi || packet.result.pitch.status != last_status || packet.result.tracking != last_tracking || finished-last_log_us >= 500000) {
            ESP_LOGI("latency", "midi=%d compute_us=%" PRId64 " queue_us=%" PRId64
                     " render_us=%" PRId64 " spi_us=%" PRId64 " rows=%u result_to_screen_us=%" PRId64 " tracking=%s",
                     midi, packet.compute_us, started-packet.published_us, rendered-started,
                     finished-rendered, rows, finished-packet.published_us, pitch_tracking_name(packet.result.tracking));
            last_log_us = finished;
            last_midi = midi;
            last_status = packet.result.pitch.status;
            last_tracking = packet.result.tracking;
        }
        // Yield if pending changes keep the queue continuously occupied.
        vTaskDelay(1);
    }
}

void tuner_display_start(void)
{
    latest = xQueueCreate(1, sizeof(display_packet_t));
    ESP_ERROR_CHECK(latest ? ESP_OK : ESP_ERR_NO_MEM);
#if CONFIG_FREERTOS_UNICORE
    const BaseType_t core = 0;
#else
    const BaseType_t core = 1;
#endif
    BaseType_t created = xTaskCreatePinnedToCore(display_task, "tuner_oled", 4096, NULL, 1, NULL, core);
    ESP_ERROR_CHECK(created == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
}

void tuner_display_publish(const tuner_result_t *result, int64_t compute_us)
{
    // A full queue is overwritten, not waited on: audio never waits for rendering.
    display_packet_t packet = {.result = *result, .published_us = esp_timer_get_time(), .compute_us = compute_us};
    xQueueOverwrite(latest, &packet);
}
