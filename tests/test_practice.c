#include "practice_control.h"
#include "practice_display.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(c)                                                                                   \
    do                                                                                             \
    {                                                                                              \
        if (!(c))                                                                                  \
        {                                                                                          \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c);                                \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
static practice_control_t ctrl;
static oled_frame_t screen;
static unsigned lines;
static char last[180];
static char captured[1024];
static void collect(const char *line, void *ctx)
{
    (void)ctx;
    ++lines;
    snprintf(last, sizeof last, "%s", line);
    size_t used = strlen(captured);
    if (used < sizeof captured - 1)
        snprintf(captured + used, sizeof captured - used, "%s\n", line);
}
static bool command(const char *s) { return practice_control_command(&ctrl, s, collect, NULL); }
static const practice_score_note_t notes[] = {{60, 96, 1, 0}, {60, 96, 1, 96}, {67, 96, 1, 192}};
static const practice_score_t score = {"test", "TEST", 100, 4, 4, notes, 3};
static void reset(void)
{
    practice_session_select(&ctrl.engine.session, &score);
    CHECK(practice_session_start(&ctrl.engine.session, false));
    practice_engine_reset_audio(&ctrl.engine);
    ctrl.debug = false;
    ctrl.mode = PRACTICE_MODE_SCORE;
    lines = 0;
}
static void event(int midi, float cents)
{
    practice_result_t r;
    CHECK(practice_session_event(&ctrl.engine.session,
                                 (practice_note_event_t){.midi = midi, .cents = cents}, &r));
    practice_control_report_event(&ctrl, &r, collect, NULL);
}
static unsigned count_color(uint16_t color)
{
    unsigned count = 0;
    for (unsigned y = 0; y < 128; ++y)
        for (unsigned x = 0; x < 128; ++x)
            if ((((unsigned)screen.pixels[y][x][0] << 8) | screen.pixels[y][x][1]) == color)
                ++count;
    return count;
}
// Compare each slot to an independently rendered expected glyph, including missing neighbors.
static void check_target_slot(unsigned first, unsigned last_x, unsigned center, unsigned y,
                              unsigned scale, const char *label, uint16_t color)
{
    static oled_frame_t expected;
    oled_clear(&expected, BLACK);
    unsigned width = (unsigned)strlen(label) * 6 * scale - scale;
    oled_text(&expected, center - width / 2, y, label, scale, color);
    for (unsigned row = 42; row < 64; ++row)
        for (unsigned x = first; x <= last_x; ++x)
            CHECK(screen.pixels[row][x][0] == expected.pixels[row][x][0] &&
                  screen.pixels[row][x][1] == expected.pixels[row][x][1]);
}
static void check_carousel(void)
{
    static const practice_score_note_t carousel_notes[] = {
        {60, 96, 1, 0}, {63, 96, 1, 96}, {67, 96, 1, 192}};
    static const practice_score_t carousel_score = {"wheel", "Wheel", 100, 4, 4, carousel_notes, 3};
    practice_session_t session;
    practice_session_select(&session, &carousel_score);
    CHECK(practice_session_start(&session, false));
    practice_result_t result;
    practice_screen_render(&screen, &session);
    check_target_slot(0, 36, 19, 46, 2, "--", 0x6B4D);
    check_target_slot(38, 89, 64, 42, 3, "C4", CYAN);
    check_target_slot(91, 127, 109, 46, 2, "D#4", 0x6B4D);
    CHECK(practice_session_event(&session, (practice_note_event_t){.midi = 60}, &result));
    practice_screen_render(&screen, &session);
    check_target_slot(0, 36, 19, 46, 2, "C4", 0x6B4D);
    check_target_slot(38, 89, 64, 42, 3, "D#4", CYAN);
    check_target_slot(91, 127, 109, 46, 2, "G4", 0x6B4D);
    CHECK(practice_session_event(&session, (practice_note_event_t){.midi = 62}, &result));
    practice_screen_render(&screen, &session);
    check_target_slot(38, 89, 64, 42, 3, "D#4", CYAN);
    CHECK(count_color(RED) > 0);
    CHECK(practice_session_event(&session, (practice_note_event_t){.midi = 63}, &result));
    practice_screen_render(&screen, &session);
    check_target_slot(0, 36, 19, 46, 2, "D#4", 0x6B4D);
    check_target_slot(38, 89, 64, 42, 3, "G4", CYAN);
    check_target_slot(91, 127, 109, 46, 2, "--", 0x6B4D);
}
static void tone(float frequency, unsigned ms, float gain)
{
    unsigned n = ms * 16;
    for (unsigned i = 0; i < n; ++i)
    {
        float x = gain * sinf(6.28318530718f * frequency * i / 16000);
        bool has;
        practice_result_t result;
        if (practice_engine_sample(&ctrl.engine, x, &has, &result) && has)
            practice_control_report_event(&ctrl, &result, collect, NULL);
    }
}
static void live_tone(float hz, unsigned ms, float gain)
{
    for (unsigned i = 0; i < ms * 16; ++i)
    {
        bool has_event = true;
        practice_result_t result;
        practice_control_sample(&ctrl, gain * sinf(6.28318530718f * hz * i / 16000), &has_event,
                                &result);
        CHECK(!has_event);
    }
}
static void check_live_mode(void)
{
    reset();
    event(60, 0);
    CHECK(ctrl.engine.session.position == 1);
    CHECK(command("mode live") && ctrl.mode == PRACTICE_MODE_LIVE);
    CHECK(ctrl.engine.session.state == PRACTICE_STOPPED && ctrl.engine.session.position == 1);
    CHECK(practice_control_listening(&ctrl));
    CHECK(!command("mode") && strcmp(last, "MODE live") == 0);
    CHECK(!command("start") && strstr(last, "mode practice"));
    unsigned before = lines;
    live_tone(440, 600, .05f);
    CHECK(ctrl.engine.pitch.tracking == TRACK_CONFIRMED && ctrl.engine.pitch.note.midi == 69);
    CHECK(ctrl.engine.session.position == 1 && ctrl.engine.session.errors == 0 && lines == before);
    practice_live_screen_render(&screen, &ctrl.engine.pitch);
    CHECK(count_color(GREEN) > 0);
    live_tone(880, 600, .05f);
    CHECK(ctrl.engine.pitch.note.midi == 81);
    live_tone(0, 500, 0);
    CHECK(ctrl.engine.pitch.pitch.status == PITCH_SILENCE);
    practice_live_screen_render(&screen, &ctrl.engine.pitch);
    CHECK(count_color(GREEN) == 0);
    CHECK(command("mode practice") && ctrl.mode == PRACTICE_MODE_SCORE);
    CHECK(!practice_control_listening(&ctrl) && ctrl.engine.session.position == 1 &&
          ctrl.engine.filled == 0);
    CHECK(command("start") && ctrl.engine.session.position == 0);
}
static void check_demo(void)
{
    practice_control_init(&ctrl);
    for (int down = 0; down <= 1; ++down) {
        CHECK(command(down ? "mode demo down" : "mode demo up"));
        for (unsigned i = 0; i < SWEEP_NOTE_COUNT; ++i) {
            int midi = sweep_demo_expected(down ? SWEEP_DOWN : SWEEP_UP, i);
            if (i == 4) ++midi; // one wrong note must not block the next.
            live_tone(440 * powf(2, (midi - 69) / 12.f), 180, .05f);
        }
        CHECK(ctrl.demo.complete && ctrl.demo.count == 22 && ctrl.demo.errors == 1);
        CHECK(!practice_control_listening(&ctrl));
        practice_control_report_demo(&ctrl, collect, NULL);
        CHECK(strstr(captured, "accuracy=95.5%"));
        CHECK(strstr(captured, down ?
            "DEMO ERROR type=WRONG expected_pos=5 actual_pos=5 expected=F5 actual=F#5" :
            "DEMO ERROR type=WRONG expected_pos=5 actual_pos=5 expected=G3 actual=G#3"));
        practice_demo_screen_render(&screen, &ctrl.demo);
        CHECK(count_color(YELLOW) > 0);
        CHECK(command("restart") && !ctrl.demo.complete && !ctrl.demo.count);
        captured[0] = 0;
    }
    CHECK(command("mode demo"));
    for (unsigned i = 0; i < 22; ++i) {
        if (i == 7) continue;
        int midi = sweep_demo_expected(SWEEP_UP, i);
        live_tone(440 * powf(2, (midi - 69) / 12.f), 180, .05f);
    }
    live_tone(0, 1000, 0);
    CHECK(ctrl.demo.complete && ctrl.demo.count == 21 && ctrl.demo.errors == 1);
    CHECK(ctrl.demo.direction == SWEEP_UP);
    captured[0] = 0;
    practice_control_report_demo(&ctrl, collect, NULL);
    CHECK(strstr(captured, "DEMO ERROR type=MISSING expected_pos=8 actual_pos=0 expected=C4 actual=--"));
    CHECK(command("mode demo"));
    for (unsigned i = 0; i < 22; ++i) {
        int midi = sweep_demo_expected(SWEEP_DOWN, i);
        live_tone(440 * powf(2, (midi - 69) / 12.f), 120, .05f);
    }
    CHECK(ctrl.demo.complete && ctrl.demo.count == 22 && ctrl.demo.errors == 0);
    CHECK(ctrl.demo.direction == SWEEP_DOWN && sweep_demo_accuracy(&ctrl.demo) == 100);
    CHECK(sweep_demo_rate(&ctrl.demo) > 7 && sweep_demo_rate(&ctrl.demo) < 10);
    captured[0] = 0;
    practice_control_report_demo(&ctrl, collect, NULL);
    CHECK(!strstr(captured, "DEMO ERROR"));
    practice_demo_screen_render(&screen, &ctrl.demo);
    CHECK(count_color(GREEN) > 0);
    CHECK(command("restart"));
    live_tone(0, 1000, 0);
    CHECK(!ctrl.demo.complete && !ctrl.demo.count);
    CHECK(command("stop") && ctrl.demo.complete && ctrl.demo.errors == 22);
    CHECK(command("mode demo up"));
    for (unsigned i = 0; i < 21; ++i) {
        if (i == 2)
            ctrl.demo.notes[ctrl.demo.count++] = 51; // extra D#3
        ctrl.demo.notes[ctrl.demo.count++] = sweep_demo_expected(SWEEP_UP, i);
    }
    sweep_demo_finish(&ctrl.demo);
    CHECK(ctrl.demo.errors == 2);
    captured[0] = 0;
    practice_control_report_demo(&ctrl, collect, NULL);
    CHECK(strstr(captured, "DEMO ERROR type=EXTRA expected_pos=0 actual_pos=3 expected=-- actual=D#3"));
    CHECK(strstr(captured, "DEMO ERROR type=MISSING expected_pos=22 actual_pos=0 expected=C6 actual=--"));
    CHECK(command("mode live") && practice_control_listening(&ctrl));
}

int main(void)
{
    check_carousel();
    check_live_mode();
    captured[0] = 0;
    check_demo();
    lines = 0;
    captured[0] = 0;
    practice_control_init(&ctrl);
    CHECK(ctrl.engine.session.state == PRACTICE_READY &&
          strcmp(ctrl.engine.session.score->id, "mary") == 0 && !ctrl.debug);
    CHECK(!command("help") && lines == 13);
    const char *expected_commands[] = {"help", "list",      "select <id>",  "start",
                                       "stop", "restart",   "debug on",     "debug off",
                                       "mode", "mode live", "mode practice", "mode demo"};
    for (size_t i = 0; i < sizeof expected_commands / sizeof expected_commands[0]; ++i)
        CHECK(strstr(captured, expected_commands[i]));
    CHECK(ctrl.engine.session.state == PRACTICE_READY && !ctrl.debug);
    lines = 0;
    captured[0] = 0;
    CHECK(!command("list") && lines == 2);
    CHECK(command("select twinkle") && ctrl.engine.session.score->count == 42);
    CHECK(command("start"));
    CHECK(!command("help") && ctrl.engine.session.state == PRACTICE_ACTIVE && !ctrl.debug);
    CHECK(!command("start"));
    CHECK(!command("select mary"));
    CHECK(command("debug on") && ctrl.debug);
    CHECK(command("restart") && ctrl.debug && ctrl.engine.session.position == 0);
    CHECK(command("stop") && ctrl.engine.session.state == PRACTICE_STOPPED);
    CHECK(command("debug off") && !ctrl.debug);
    CHECK(!command("bad") && strstr(last, "type help"));
    reset();
    event(60, 28);
    CHECK(ctrl.engine.session.position == 1 && lines == 0);
    event(72, 0);
    CHECK(ctrl.engine.session.position == 1 && lines == 1 &&
          strstr(last, "note=2/3 bar=1 beat=2 expected=C4 actual=C5"));
    practice_screen_render(&screen, &ctrl.engine.session);
    CHECK(count_color(RED) > 0);
    event(60, 0);
    CHECK(ctrl.engine.session.position == 2 && lines == 1);
    practice_screen_render(&screen, &ctrl.engine.session);
    CHECK(count_color(GREEN) > 0 && count_color(RED) == 0);
    event(67, 0);
    CHECK(ctrl.engine.session.state == PRACTICE_COMPLETE && ctrl.engine.session.errors == 1 &&
          lines == 1);
    practice_screen_render(&screen, &ctrl.engine.session);
    CHECK(count_color(GREEN) > 0);
    practice_result_t unused;
    CHECK(!practice_session_event(&ctrl.engine.session, (practice_note_event_t){.midi = 67},
                                  &unused));
    CHECK(command("restart") && ctrl.engine.session.position == 0 &&
          ctrl.engine.session.errors == 0);
    reset();
    tone(261.6256f, 1200, .05f);
    CHECK(ctrl.engine.session.position == 1 && ctrl.engine.session.errors == 0);
    tone(0, 100, 0);
    tone(261.6256f, 500, .05f);
    CHECK(ctrl.engine.session.position == 2 && ctrl.engine.session.errors == 0);
    tone(392, 500, .05f);
    CHECK(ctrl.engine.session.state == PRACTICE_COMPLETE);
    reset();
    tone(293.6648f, 1000, .05f);
    CHECK(ctrl.engine.session.position == 0 && ctrl.engine.session.errors == 1 && lines == 1);
    tone(0, 100, 0);
    tone(293.6648f, 500, .05f);
    CHECK(ctrl.engine.session.errors == 2 && lines == 2);
    tone(0, 100, 0);
    tone(261.6256f, 500, .05f);
    CHECK(ctrl.engine.session.position == 1);
    reset();
    unsigned random = 1234;
    for (unsigned i = 0; i < 32000; ++i)
    {
        random = random * 1664525u + 1013904223u;
        float x = .03f * ((random >> 8) / 8388608.f - 1);
        bool has;
        practice_result_t result;
        practice_engine_sample(&ctrl.engine, x, &has, &result);
        CHECK(!has);
    }
    CHECK(ctrl.engine.session.errors == 0);
    static const practice_score_note_t low_notes[] = {{40, 96, 1, 0}, {40, 96, 1, 96}};
    static const practice_score_t low_score = {"low", "LOW", 100, 4, 4, low_notes, 2};
    practice_session_select(&ctrl.engine.session, &low_score);
    CHECK(practice_session_start(&ctrl.engine.session, false));
    practice_engine_reset_audio(&ctrl.engine);
    tone(82.4069f, 1500, .05f);
    CHECK(ctrl.engine.session.position == 1 && ctrl.engine.session.errors == 0);
    tone(0, 100, 0);
    tone(82.4069f, 500, .04f);
    CHECK(ctrl.engine.session.state == PRACTICE_COMPLETE);
    reset();
    CHECK(command("debug on"));
    lines = 0;
    event(60, 0);
    CHECK(lines == 1 && strstr(last, "DEBUG event"));
    puts("Practice session, audio events, commands and screen passed");
    return 0;
}
