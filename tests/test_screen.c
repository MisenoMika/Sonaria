#include "tuner_screen.h"
#include <stdio.h>
#include <stdlib.h>
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); exit(1); } } while (0)
static oled_frame_t frame;
static oled_frame_t previous;
static uint16_t pixel(unsigned x, unsigned y)
{
    return (uint16_t)(frame.pixels[y][x][0] << 8) | frame.pixels[y][x][1];
}
int main(void)
{
    tuner_result_t result = {.pitch = {.status=PITCH_VALID, .frequency_hz=440}, .note={.name="A", .octave=4, .cents=0}, .tracking=TRACK_CONFIRMED};
    tuner_render(&frame, &result);
    CHECK(pixel(64, 98) == GREEN);
    result.note.cents = -20;
    tuner_render(&frame, &result);
    CHECK(pixel(44, 98) == YELLOW && pixel(64, 98) == BLACK);
    result.note.cents = 20;
    tuner_render(&frame, &result);
    CHECK(pixel(84, 98) == YELLOW && pixel(44, 98) == BLACK);
    result.note.cents = 50;
    tuner_render(&frame, &result);
    CHECK(pixel(114, 98) == YELLOW);
    previous=frame;
    result.tracking=TRACK_HOLD;
    tuner_render(&frame,&result);
    CHECK(pixel(114,98)==YELLOW);
    unsigned hold_first, hold_end;
    CHECK(oled_next_dirty_rows(&frame,&previous,0,&hold_first,&hold_end) && hold_first>=111);
    result.tracking=TRACK_PENDING;
    result.pitch.status=PITCH_UNCERTAIN;
    tuner_render(&frame,&result);
    CHECK(pixel(114,98)==BLACK);
    previous=frame;
    result.tracking=TRACK_IDLE;
    tuner_render(&frame,&result);
    CHECK(oled_next_dirty_rows(&frame,&previous,0,&hold_first,&hold_end));
    result.pitch.status = PITCH_SILENCE;
    tuner_render(&frame, &result);
    CHECK(pixel(114, 98) == BLACK && pixel(64, 103) == BLACK);
    oled_rect(&frame, 127, 127, 100, 100, RED);
    CHECK(pixel(127, 127) == RED);
    // Repeated identical screen: no SPI region should be scheduled.
    previous = frame;
    unsigned first, end;
    CHECK(!oled_next_dirty_rows(&frame, &previous, 0, &first, &end));
    // A changed range must include precisely the affected rows, including row 127.
    oled_rect(&frame, 4, 10, 3, 2, RED);
    oled_rect(&frame, 0, 127, 1, 1, GREEN);
    CHECK(oled_next_dirty_rows(&frame, &previous, 0, &first, &end) && first == 10 && end == 12);
    CHECK(oled_next_dirty_rows(&frame, &previous, end, &first, &end) && first == 127 && end == 128);
    CHECK(!oled_next_dirty_rows(&frame, &previous, end, &first, &end));
    CHECK(oled_next_dirty_rows(&frame, NULL, 0, &first, &end) && first == 0 && end == 128);
    CHECK(!oled_next_dirty_rows(&frame, &previous, 129, &first, &end));
    puts("Tuner screen tests passed");
    return 0;
}
