#include "oled_display.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stddef.h>

static const char *TAG = "oled_test";
#define OLED_MOSI 11
#define OLED_SCK 12
#define OLED_CS 10
#define OLED_DC 9
#define OLED_RST 8

static void delay_ms(unsigned ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }
static void rect(unsigned x, unsigned y, unsigned w, unsigned h, uint16_t color) { oled_rect(oled_frame(), x, y, w, h, color); }
static void fill(uint16_t color) { oled_clear(oled_frame(), color); oled_present(); }

static void geometry_pattern(void)
{
    rect(0, 0, OLED_WIDTH, OLED_HEIGHT, BLACK);
    rect(0, 0, 128, 1, WHITE);
    rect(0, 127, 128, 1, WHITE);
    rect(0, 0, 1, 128, WHITE);
    rect(127, 0, 1, 128, WHITE);
    rect(3, 3, 12, 12, RED);
    rect(113, 3, 12, 12, GREEN);
    rect(3, 113, 12, 12, BLUE);
    rect(113, 113, 12, 12, YELLOW);
    const uint16_t colors[] = {RED, GREEN, BLUE, CYAN, MAGENTA, YELLOW, WHITE, BLACK};
    for (unsigned i = 0; i < 8; ++i) {
        rect(8 + i * 14, 35, 14, 30, colors[i]);
        uint16_t gray = ((i * 31 / 7) << 11) | ((i * 63 / 7) << 5) | (i * 31 / 7);
        rect(8 + i * 14, 75, 14, 18, gray);
    }
    oled_present();
}

void app_main(void)
{
    ESP_LOGI(TAG, "SSD1351 128x128 RGB565: SPI2 mode 3, %d Hz", OLED_CLOCK_HZ);
    ESP_LOGI(TAG, "MOSI=%d SCK=%d CS=%d DC=%d RST=%d", OLED_MOSI, OLED_SCK, OLED_CS, OLED_DC, OLED_RST);
    oled_init();
    ESP_LOGI(TAG, "Initialization commands sent (write-only SPI; verify screen visually)");
    const uint16_t colors[] = {RED, GREEN, BLUE, WHITE, BLACK};
    const char *names[] = {"RED", "GREEN", "BLUE", "WHITE", "BLACK"};
    while (1) {
        for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); ++i) {
            ESP_LOGI(TAG, "Full screen %s", names[i]);
            fill(colors[i]);
            delay_ms(1000);
        }
        ESP_LOGI(TAG, "Geometry: border, corners, color bars, gray ramp");
        geometry_pattern();
        delay_ms(4000);
    }
}
