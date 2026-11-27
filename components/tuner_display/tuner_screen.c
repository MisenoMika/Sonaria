#include "tuner_screen.h"
#include <math.h>
#include <stdio.h>
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
    if (c >= 'A' && c <= 'Z') return letters[c-'A'];
    if (c >= '0' && c <= '9') return digits[c-'0'];
    switch (c) { case '#': return sharp; case '+': return plus; case '-': return minus; case '.': return dot; default: return space; }
}

static void text(oled_frame_t *frame, unsigned x, unsigned y, const char *s, unsigned scale, uint16_t color)
{
    for (; *s; ++s, x += 6 * scale) {
        const uint8_t *rows = glyph(*s);
        for (unsigned row = 0; row < 7; ++row)
            for (unsigned col = 0; col < 5; ++col)
                if (rows[row] & (1U << (4-col))) oled_rect(frame, x+col*scale, y+row*scale, scale, scale, color);
    }
}

static void centered(oled_frame_t *frame, unsigned y, const char *s, unsigned scale, uint16_t color)
{
    unsigned width = (unsigned)strlen(s) * 6 * scale;
    if (width) width -= scale;
    text(frame, width < OLED_WIDTH ? (OLED_WIDTH-width)/2 : 0, y, s, scale, color);
}

void tuner_render(oled_frame_t *frame, const tuner_result_t *result)
{
    oled_clear(frame, BLACK);
    centered(frame, 4, "SONARIA", 2, CYAN);
    oled_rect(frame, 8, 20, 112, 1, 0x4208);
    const bool valid = result->pitch.status == PITCH_VALID && result->note.name &&
                       isfinite(result->pitch.frequency_hz) && isfinite(result->note.cents);
    if (!valid) {
        centered(frame, 40, result->tracking == TRACK_PENDING ? "LISTENING" : result->pitch.status == PITCH_SILENCE ? "NO SIGNAL" : "UNSTABLE", 2, WHITE);
        centered(frame, 65, "PLAY A NOTE", 1, CYAN);
        centered(frame, 90, "-- HZ", 2, 0x8410);
        return;
    }
    const float cents = result->note.cents;
    uint16_t color = fabsf(cents) <= 5.0f ? GREEN : YELLOW;
    char label[24];
    snprintf(label, sizeof(label), "%s%d", result->note.name, result->note.octave);
    centered(frame, 25, label, 4, color);
    snprintf(label, sizeof(label), "%.2f HZ", result->pitch.frequency_hz);
    centered(frame, 59, label, 2, WHITE);
    snprintf(label, sizeof(label), "%+.1f CT", cents);
    centered(frame, 77, label, 2, color);
    // Meter: One pixel per cent, from -50 (x=14) to +50 (x=114).
    oled_rect(frame, 14, 103, 101, 2, 0x8410);
    oled_rect(frame, 59, 101, 11, 6, 0x0320);
    for (unsigned i = 14; i <= 114; i += 25) oled_rect(frame, i, 100, 1, 8, WHITE);
    float clamped = fmaxf(-50, fminf(50, cents));
    unsigned marker = (unsigned)lroundf(64 + clamped);
    oled_rect(frame, marker-2, 98, 5, 12, color);
    text(frame, 3, 119, "-50", 1, WHITE);
    text(frame, 61, 119, "0", 1, WHITE);
    text(frame, 104, 119, "+50", 1, WHITE);
    centered(frame, 111, result->tracking == TRACK_HOLD ? "HOLD" : cents < -5 ? "FLAT" : cents > 5 ? "SHARP" : "IN TUNE", 1, color);
}
