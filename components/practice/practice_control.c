#include "practice_control.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>
void practice_control_init(practice_control_t *c)
{
    c->debug = false;
    c->mode = PRACTICE_MODE_SCORE;
    const practice_score_t *score = practice_score_find("mary");
    practice_session_select(&c->engine.session, score ? score : &practice_scores[0]);
    practice_engine_reset_audio(&c->engine);
}
bool practice_control_listening(const practice_control_t *c)
{
    return c->mode == PRACTICE_MODE_LIVE || c->engine.session.state == PRACTICE_ACTIVE;
}
bool practice_control_sample(practice_control_t *c, float sample, bool *has_event,
                             practice_result_t *result)
{
    if (c->mode == PRACTICE_MODE_LIVE)
    {
        *has_event = false;
        return practice_engine_live_sample(&c->engine, sample);
    }
    return practice_engine_sample(&c->engine, sample, has_event, result);
}
static void reply(practice_emit_fn emit, void *ctx, const char *text) { emit(text, ctx); }
bool practice_control_command(practice_control_t *c, const char *line, practice_emit_fn emit,
                              void *ctx)
{
    char cmd[80];
    size_t len = strlen(line);
    if (len >= sizeof cmd)
    {
        reply(emit, ctx, "ERROR command too long");
        return false;
    }
    while (*line && isspace((unsigned char)*line))
    {
        ++line;
        --len;
    }
    while (len && isspace((unsigned char)line[len - 1]))
        --len;
    memcpy(cmd, line, len);
    cmd[len] = 0;
    if (!len)
        return false;
    practice_session_t *s = &c->engine.session;
    char out[160];
    if (strcmp(cmd, "help") == 0)
    {
        static const char *const commands[] = {
            "Practice commands:",
            "  help           Show this help",
            "  list           List available scores",
            "  mode           Show active mode",
            "  mode live      Show live single-note pitch",
            "  mode practice  Return to score practice",
            "  select <id>    Select score while stopped or ready",
            "  start          Start selected score from the beginning",
            "  stop           Stop current practice",
            "  restart        Reset and start selected score",
            "  debug on       Enable detailed diagnostic output",
            "  debug off      Disable detailed diagnostic output",
        };
        for (size_t i = 0; i < sizeof commands / sizeof commands[0]; ++i)
            reply(emit, ctx, commands[i]);
        return false;
    }
    if (strcmp(cmd, "list") == 0)
    {
        for (size_t i = 0; i < practice_score_count; ++i)
        {
            const practice_score_t *p = &practice_scores[i];
            snprintf(out, sizeof out, "SCORE %s %s notes=%zu bpm=%u", p->id, p->title, p->count,
                     p->bpm);
            reply(emit, ctx, out);
        }
        return false;
    }
    if (strcmp(cmd, "mode") == 0)
    {
        reply(emit, ctx, c->mode == PRACTICE_MODE_LIVE ? "MODE live" : "MODE practice");
        return false;
    }
    if (strcmp(cmd, "mode live") == 0 || strcmp(cmd, "mode practice") == 0)
    {
        practice_mode_t next =
            strcmp(cmd, "mode live") == 0 ? PRACTICE_MODE_LIVE : PRACTICE_MODE_SCORE;
        if (next == c->mode)
        {
            reply(emit, ctx, next == PRACTICE_MODE_LIVE ? "OK mode live" : "OK mode practice");
            return false;
        }
        if (s->state == PRACTICE_ACTIVE)
            practice_session_stop(s);
        c->mode = next;
        practice_engine_reset_audio(&c->engine);
        reply(emit, ctx, next == PRACTICE_MODE_LIVE ? "OK mode live" : "OK mode practice");
        return true;
    }
    if (c->mode == PRACTICE_MODE_LIVE &&
        (strncmp(cmd, "select ", 7) == 0 || strcmp(cmd, "start") == 0 ||
         strcmp(cmd, "restart") == 0 || strcmp(cmd, "stop") == 0))
    {
        reply(emit, ctx, "ERROR command switch to mode practice first");
        return false;
    }
    if (strncmp(cmd, "select ", 7) == 0)
    {
        if (s->state == PRACTICE_ACTIVE)
        {
            reply(emit, ctx, "ERROR command stop practice before select");
            return false;
        }
        const practice_score_t *score = practice_score_find(cmd + 7);
        if (!score)
        {
            reply(emit, ctx, "ERROR command unknown score");
            return false;
        }
        practice_session_select(s, score);
        practice_engine_reset_audio(&c->engine);
        snprintf(out, sizeof out, "OK selected %s", score->id);
        reply(emit, ctx, out);
        return true;
    }
    if (strcmp(cmd, "start") == 0 || strcmp(cmd, "restart") == 0)
    {
        if (!practice_session_start(s, strcmp(cmd, "restart") == 0))
        {
            reply(emit, ctx, "ERROR command already active or no score");
            return false;
        }
        practice_engine_reset_audio(&c->engine);
        snprintf(out, sizeof out, "OK started %s", s->score->id);
        reply(emit, ctx, out);
        return true;
    }
    if (strcmp(cmd, "stop") == 0)
    {
        practice_session_stop(s);
        practice_engine_reset_audio(&c->engine);
        reply(emit, ctx, "OK stopped");
        return true;
    }
    if (strcmp(cmd, "debug on") == 0 || strcmp(cmd, "debug off") == 0)
    {
        c->debug = strcmp(cmd, "debug on") == 0;
        reply(emit, ctx, c->debug ? "OK debug on" : "OK debug off");
        return true;
    }
    reply(emit, ctx, "ERROR command unknown; type help for available commands");
    return false;
}
void practice_control_report_event(const practice_control_t *c, const practice_result_t *r,
                                   practice_emit_fn emit, void *ctx)
{
    char out[180];
    if (r->feedback == PRACTICE_WRONG)
    {
        practice_format_error(r, out, sizeof out);
        emit(out, ctx);
    }
    if (c->debug)
    {
        char expected[8], actual[8];
        practice_note_name(r->expected.midi, expected, sizeof expected);
        practice_note_name(r->actual.midi, actual, sizeof actual);
        snprintf(out, sizeof out,
                 "DEBUG event articulation=%s sample=%" PRIu64
                 " midi=%d target=%s actual=%s result=%s progress=%zu/%zu%s",
                 r->actual.articulation == PRACTICE_REATTACK ? "REATTACK" : "NEW_NOTE",
                 r->actual.sample, r->actual.midi, expected, actual,
                 r->feedback == PRACTICE_CORRECT ? "CORRECT" : "WRONG", c->engine.session.position,
                 r->score->count, r->complete ? " COMPLETE" : "");
        emit(out, ctx);
    }
}
