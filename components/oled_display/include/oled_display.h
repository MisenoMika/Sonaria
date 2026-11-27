#ifndef SONARIA_OLED_DISPLAY_H
#define SONARIA_OLED_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

#define OLED_WIDTH 128
#define OLED_HEIGHT 128
#define OLED_CLOCK_HZ 4000000
#define BLACK 0x0000
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define WHITE 0xFFFF
#define YELLOW 0xFFE0
#define CYAN 0x07FF
#define MAGENTA 0xF81F

typedef struct {
    uint8_t pixels[OLED_HEIGHT][OLED_WIDTH][2];
} oled_frame_t;

// Graphics are hardware-independent; RGB565 pixels are stored MSB first.
void oled_rect(oled_frame_t *frame, unsigned x, unsigned y, unsigned w, unsigned h, uint16_t color);
void oled_clear(oled_frame_t *frame, uint16_t color);
void oled_text(oled_frame_t *frame,unsigned x,unsigned y,const char *s,unsigned scale,uint16_t color);
void oled_centered_text(oled_frame_t *frame,unsigned y,const char *s,unsigned scale,uint16_t color);

// Find the next contiguous range of changed rows at or after start.
// Pass NULL for previous to force a complete first refresh.
bool oled_next_dirty_rows(const oled_frame_t *frame, const oled_frame_t *previous,
                          unsigned start, unsigned *first, unsigned *end);

// The display task exclusively owns this DMA-capable framebuffer and SPI device.
void oled_init(void);
oled_frame_t *oled_frame(void);
// Returns the number of pixel rows actually transmitted.
unsigned oled_present(void);

#endif
