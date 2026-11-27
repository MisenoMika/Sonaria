#include "oled_display.h"
#include <string.h>

void oled_rect(oled_frame_t *frame, unsigned x, unsigned y, unsigned w, unsigned h, uint16_t color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    if (w > OLED_WIDTH - x) w = OLED_WIDTH - x;
    if (h > OLED_HEIGHT - y) h = OLED_HEIGHT - y;
    for (unsigned row = y; row < y + h; ++row) {
        for (unsigned col = x; col < x + w; ++col) {
            frame->pixels[row][col][0] = color >> 8;
            frame->pixels[row][col][1] = color & 0xFF;
        }
    }
}

void oled_clear(oled_frame_t *frame, uint16_t color)
{
    if (color == BLACK) memset(frame, 0, sizeof(*frame));
    else oled_rect(frame, 0, 0, OLED_WIDTH, OLED_HEIGHT, color);
}

bool oled_next_dirty_rows(const oled_frame_t *frame, const oled_frame_t *previous,
                          unsigned start, unsigned *first, unsigned *end)
{
    unsigned row = start;
    if (row >= OLED_HEIGHT) return false;
    if (!previous) {
        *first = row;
        *end = OLED_HEIGHT;
        return true;
    }
    while (row < OLED_HEIGHT && !memcmp(frame->pixels[row], previous->pixels[row], sizeof(frame->pixels[row]))) ++row;
    if (row == OLED_HEIGHT) return false;
    *first = row++;
    while (row < OLED_HEIGHT && memcmp(frame->pixels[row], previous->pixels[row], sizeof(frame->pixels[row]))) ++row;
    *end = row;
    return true;
}
