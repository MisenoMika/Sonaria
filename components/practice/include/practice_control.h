#ifndef SONARIA_PRACTICE_CONTROL_H
#define SONARIA_PRACTICE_CONTROL_H
#include "practice_engine.h"
typedef void (*practice_emit_fn)(const char *line, void *context);
typedef struct
{
    practice_engine_t engine;
    bool debug;
} practice_control_t;
void practice_control_init(practice_control_t *c);
// All mutations run on the audio/control owner; input task only queues command strings.
bool practice_control_command(practice_control_t *c, const char *line, practice_emit_fn emit,
                              void *context);
void practice_control_report_event(const practice_control_t *c, const practice_result_t *r,
                                   practice_emit_fn emit, void *context);
#endif
