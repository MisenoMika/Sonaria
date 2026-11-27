#include "tuner_screen.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void tuner_render(oled_frame_t *frame, const tuner_result_t *result) {
    oled_clear(frame, BLACK);
    oled_centered_text(frame, 4, "SONARIA", 2, CYAN);
    oled_rect(frame, 8, 20, 112, 1, 0x4208);
    const bool valid =
        result->pitch.status == PITCH_VALID && result->note.name &&
        isfinite(result->pitch.frequency_hz) && isfinite(result->note.cents);
    if (!valid) {
        oled_centered_text(frame, 40,
                           result->tracking == TRACK_PENDING       ? "LISTENING"
                           : result->pitch.status == PITCH_SILENCE ? "NO SIGNAL"
                                                                   : "UNSTABLE",
                           2, WHITE);
        oled_centered_text(frame, 65, "PLAY A NOTE", 1, CYAN);
        oled_centered_text(frame, 90, "-- HZ", 2, 0x8410);
        return;
    }
    const float cents = result->note.cents;
    uint16_t color = fabsf(cents) <= 5.0f ? GREEN : YELLOW;
    char label[24];
    snprintf(label, sizeof(label), "%s%d", result->note.name,
             result->note.octave);
    oled_centered_text(frame, 25, label, 4, color);
    snprintf(label, sizeof(label), "%.2f HZ", result->pitch.frequency_hz);
    oled_centered_text(frame, 59, label, 2, WHITE);
    snprintf(label, sizeof(label), "%+.1f CT", cents);
    oled_centered_text(frame, 77, label, 2, color);
    // Meter: One pixel per cent, from -50 (x=14) to +50 (x=114).
    oled_rect(frame, 14, 103, 101, 2, 0x8410);
    oled_rect(frame, 59, 101, 11, 6, 0x0320);
    for (unsigned i = 14; i <= 114; i += 25)
        oled_rect(frame, i, 100, 1, 8, WHITE);
    float clamped = fmaxf(-50, fminf(50, cents));
    unsigned marker = (unsigned)lroundf(64 + clamped);
    oled_rect(frame, marker - 2, 98, 5, 12, color);
    oled_text(frame, 3, 119, "-50", 1, WHITE);
    oled_text(frame, 61, 119, "0", 1, WHITE);
    oled_text(frame, 104, 119, "+50", 1, WHITE);
    oled_centered_text(frame, 111,
                       result->tracking == TRACK_HOLD ? "HOLD"
                       : cents < -5                   ? "FLAT"
                       : cents > 5                    ? "SHARP"
                                                      : "IN TUNE",
                       1, color);
}
