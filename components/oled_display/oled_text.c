#include "oled_display.h"
#include <string.h>

// Compact 5x7 glyphs, indexed by uppercase ASCII letter or decimal digit.
static const uint8_t letters[26][7] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30}, {14,17,16,16,16,17,14},
    {30,17,17,17,17,17,30}, {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17}, {14,4,4,4,4,4,14},
    {7,2,2,2,18,18,12}, {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17}, {14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16}, {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4}, {17,17,17,17,17,17,14},
    {17,17,17,17,17,10,4}, {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31},
};
static const uint8_t digits[10][7] = {
    {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14}, {14,17,1,2,4,8,31},
    {30,1,1,14,1,1,30}, {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8}, {14,17,17,14,17,17,14},
    {14,17,17,15,1,1,14},
};
static const uint8_t *glyph(char c)
{
    static const uint8_t space[7] = {0};
    static const uint8_t sharp[7] = {10,10,31,10,31,10,10};
    static const uint8_t plus[7] = {0,4,4,31,4,4,0};
    static const uint8_t minus[7] = {0,0,0,31,0,0,0};
    static const uint8_t dot[7] = {0,0,0,0,0,6,6};
    static const uint8_t slash[7] = {1,1,2,4,8,16,16};
    if (c >= 'a' && c <= 'z') c -= 'a'-'A';
    if (c >= 'A' && c <= 'Z') return letters[c-'A'];
    if (c >= '0' && c <= '9') return digits[c-'0'];
    switch (c) { case '#': return sharp; case '+': return plus; case '-': return minus; case '.': return dot; case '/': return slash; default: return space; }
}

void oled_text(oled_frame_t *frame, unsigned x, unsigned y, const char *s, unsigned scale, uint16_t color)
{
    for (; *s; ++s, x += 6 * scale) {
        const uint8_t *rows = glyph(*s);
        for (unsigned row = 0; row < 7; ++row)
            for (unsigned col = 0; col < 5; ++col)
                if (rows[row] & (1U << (4-col))) oled_rect(frame, x+col*scale, y+row*scale, scale, scale, color);
    }
}

void oled_centered_text(oled_frame_t *frame, unsigned y, const char *s, unsigned scale, uint16_t color)
{
    unsigned width = (unsigned)strlen(s) * 6 * scale;
    if (width) width -= scale;
    oled_text(frame, width < OLED_WIDTH ? (OLED_WIDTH-width)/2 : 0, y, s, scale, color);
}

