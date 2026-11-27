#include "oled_display.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_attr.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define OLED_MOSI GPIO_NUM_11
#define OLED_SCK GPIO_NUM_12
#define OLED_CS GPIO_NUM_10
#define OLED_DC GPIO_NUM_9
#define OLED_RST GPIO_NUM_8
#define TRANSFER_BYTES 4096

static spi_device_handle_t oled;
// Store RGB565 in wire order (most significant byte first).
static DMA_ATTR oled_frame_t framebuffer;
static oled_frame_t previous_frame;
static bool previous_valid;

oled_frame_t *oled_frame(void) { return &framebuffer; }

static void delay_ms(unsigned ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static void write_spi(bool data, const uint8_t *bytes, size_t count)
{
    ESP_ERROR_CHECK(gpio_set_level(OLED_DC, data));
    spi_transaction_t transaction = {.length = count * 8};
    if (count <= sizeof(transaction.tx_data)) {
        transaction.flags = SPI_TRANS_USE_TXDATA;
        for (size_t i = 0; i < count; ++i) transaction.tx_data[i] = bytes[i];
    } else {
        transaction.tx_buffer = bytes;
    }
    ESP_ERROR_CHECK(spi_device_polling_transmit(oled, &transaction));
}

static void command(uint8_t cmd, const uint8_t *data, size_t count)
{
    write_spi(false, &cmd, 1);
    if (count != 0) write_spi(true, data, count);
}

static void set_window(unsigned first_row, unsigned end_row)
{
    const uint8_t columns[] = {0, OLED_WIDTH - 1};
    const uint8_t rows[] = {first_row, end_row - 1};
    command(0x15, columns, sizeof(columns));
    command(0x75, rows, sizeof(rows));
    command(0x5C, NULL, 0);
}

unsigned oled_present(void)
{
    unsigned start = 0, first, end, sent = 0;
    const size_t row_bytes = sizeof(framebuffer.pixels[0]);
    while (oled_next_dirty_rows(&framebuffer, previous_valid ? &previous_frame : NULL,
                                start, &first, &end)) {
        set_window(first, end);
        const uint8_t *pixels = (const uint8_t *)&framebuffer.pixels[first];
        size_t total = (end-first)*row_bytes;
        for (size_t offset = 0; offset < total;) {
            size_t chunk = total-offset < TRANSFER_BYTES ? total-offset : TRANSFER_BYTES;
            write_spi(true, pixels+offset, chunk);
            offset += chunk;
        }
        memcpy(previous_frame.pixels[first], pixels, total);
        sent += end-first;
        start = end;
    }
    previous_valid = true;
    return sent;
}

void oled_init(void)
{
    previous_valid = false;
    gpio_config_t outputs = {
        .pin_bit_mask = (1ULL << OLED_DC) | (1ULL << OLED_RST),
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&outputs));
    ESP_ERROR_CHECK(gpio_set_level(OLED_DC, 0));
    ESP_ERROR_CHECK(gpio_set_level(OLED_RST, 1));

    spi_bus_config_t bus = {
        .mosi_io_num = OLED_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = OLED_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = TRANSFER_BYTES,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO));
    spi_device_interface_config_t device = {
        .clock_speed_hz = OLED_CLOCK_HZ,
        .mode = 3, // Matches the Waveshare module example.
        .spics_io_num = OLED_CS,
        .queue_size = 1,
        .flags = SPI_DEVICE_HALFDUPLEX,
        .cs_ena_pretrans = 1,
        .cs_ena_posttrans = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &device, &oled));

    delay_ms(100);
    ESP_ERROR_CHECK(gpio_set_level(OLED_RST, 0));
    delay_ms(100);
    ESP_ERROR_CHECK(gpio_set_level(OLED_RST, 1));
    delay_ms(100);

    // Panel-specific register values from docs/oled Waveshare 1.5 RGB example.
    static const struct { uint8_t cmd, count, data[3]; } init[] = {
        {0xFD, 1, {0x12}}, {0xFD, 1, {0xB1}},
        {0xAE, 0, {0}}, {0xA4, 0, {0}},
        {0x15, 2, {0, 127}}, {0x75, 2, {0, 127}},
        {0xB3, 1, {0xF1}}, {0xCA, 1, {0x7F}},
        {0xA0, 1, {0x74}}, // 65K colors, horizontal addressing.
        {0xA1, 1, {0}}, {0xA2, 1, {0}}, {0xAB, 1, {1}},
        {0xB4, 3, {0xA0, 0xB5, 0x55}},
        {0xC1, 3, {0xC8, 0x80, 0xC0}}, {0xC7, 1, {0x0F}},
        {0xB1, 1, {0x32}}, {0xB2, 3, {0xA4, 0, 0}},
        {0xBB, 1, {0x17}}, {0xB6, 1, {1}}, {0xBE, 1, {5}},
        {0xA6, 0, {0}},
    };
    for (size_t i = 0; i < sizeof(init) / sizeof(init[0]); ++i) {
        command(init[i].cmd, init[i].data, init[i].count);
    }
    oled_clear(&framebuffer, BLACK);
    oled_present();
    delay_ms(200);
    command(0xAF, NULL, 0);
}

