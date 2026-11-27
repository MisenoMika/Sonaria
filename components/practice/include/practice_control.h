#ifndef SONARIA_PRACTICE_CONTROL_H
#define SONARIA_PRACTICE_CONTROL_H
#include "practice_engine.h"
#include "sweep_demo.h"
typedef void (*practice_emit_fn)(const char *line, void *context);
typedef enum
{
    PRACTICE_MODE_SCORE,
    PRACTICE_MODE_LIVE,
    PRACTICE_MODE_DEMO
} practice_mode_t;
typedef struct
{
    practice_engine_t engine;
    practice_mode_t mode;
    bool debug;
    sweep_demo_t demo;
    sweep_direction_t demo_direction;
} practice_control_t;
void practice_control_init(practice_control_t *c);
bool practice_control_listening(const practice_control_t *c);
bool practice_control_sample(practice_control_t *c, float sample, bool *has_event,
                             practice_result_t *result);
// All mutations run on the audio/control owner; input task only queues command strings.
bool practice_control_command(practice_control_t *c, const char *line, practice_emit_fn emit,
                              void *context);
void practice_control_report_demo(const practice_control_t *c, practice_emit_fn emit, void *context);
void practice_control_report_event(const practice_control_t *c, const practice_result_t *r,
                                   practice_emit_fn emit, void *context);
#endif
