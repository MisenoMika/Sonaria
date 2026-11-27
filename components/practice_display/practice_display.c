#include "practice_display.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <stdio.h>
#include <inttypes.h>
#include <stdatomic.h>
typedef struct
{
    union
    {
        practice_session_t session;
        tuner_result_t pitch;
    } view;
    bool live;
    bool debug;
    int64_t published;
} packet_t;
static QueueHandle_t queue;
static atomic_bool debug_enabled;
static void display_task(void *arg)
{
    (void)arg;
    oled_init();
    packet_t packet;
    int64_t last_live_log = 0;
    for (;;)
    {
        if (xQueueReceive(queue, &packet, portMAX_DELAY) != pdTRUE)
            continue;
        int64_t start = esp_timer_get_time();
        if (packet.live)
            practice_live_screen_render(oled_frame(), &packet.view.pitch);
        else
            practice_screen_render(oled_frame(), &packet.view.session);
        int64_t rendered = esp_timer_get_time();
        unsigned rows = oled_present();
        int64_t done = esp_timer_get_time();
        if (packet.debug && atomic_load(&debug_enabled) &&
            (!packet.live || done - last_live_log >= 500000))
        {
            if (packet.live)
                last_live_log = done;
            printf("DEBUG display queue_us=%" PRId64 " render_us=%" PRId64 " spi_us=%" PRId64
                   " rows=%u\n",
                   start - packet.published, rendered - start, done - rendered, rows);
        }
        vTaskDelay(1);
    }
}
void practice_display_start(void)
{
    queue = xQueueCreate(1, sizeof(packet_t));
    ESP_ERROR_CHECK(queue ? ESP_OK : ESP_ERR_NO_MEM);
#if CONFIG_FREERTOS_UNICORE
    int core = 0;
#else
    int core = 1;
#endif
    ESP_ERROR_CHECK(
        xTaskCreatePinnedToCore(display_task, "practice_oled", 4096, NULL, 1, NULL, core) == pdPASS
            ? ESP_OK
            : ESP_ERR_NO_MEM);
}
void practice_display_publish(const practice_session_t *session, bool debug)
{
    atomic_store(&debug_enabled, debug);
    packet_t packet = {.view.session = *session, .debug = debug, .published = esp_timer_get_time()};
    xQueueOverwrite(queue, &packet);
}

void practice_display_publish_live(const tuner_result_t *result, bool debug)
{
    atomic_store(&debug_enabled, debug);
    packet_t packet = {
        .view.pitch = *result, .live = true, .debug = debug, .published = esp_timer_get_time()};
    xQueueOverwrite(queue, &packet);
}
