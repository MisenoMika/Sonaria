#include "practice_control.h"
#include <stdio.h>
static void emit(const char *line, void *ctx) { (void)ctx; puts(line); }
int main(int argc, char **argv) {
    static practice_control_t c;
    practice_control_init(&c);
    practice_control_command(&c, argc > 1 ? argv[1] : "mode demo", emit, NULL);
    int16_t v; bool event; practice_result_t result;
    while (fread(&v, sizeof v, 1, stdin) == 1 && practice_control_listening(&c)) {
        practice_control_sample(&c, v / 32768.f, &event, &result);
    }
    if (!c.demo.complete) sweep_demo_finish(&c.demo);
    practice_control_report_demo(&c, emit, NULL);
}
