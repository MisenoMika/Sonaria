#include "practice_engine.h"
#include <stdio.h>
static practice_engine_t engine;
int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;
    const practice_score_t *score = practice_score_find(argv[1]);
    if (!score)
        return 2;
    practice_session_select(&engine.session, score);
    practice_session_start(&engine.session, false);
    practice_engine_reset_audio(&engine);
    float x;
    while (fread(&x, sizeof x, 1, stdin) == 1)
    {
        bool has;
        practice_result_t result;
        if (practice_engine_sample(&engine, x, &has, &result) && has)
        {
            if (result.feedback == PRACTICE_WRONG)
            {
                char line[180];
                practice_format_error(&result, line, sizeof line);
                puts(line);
            }
            printf("EVENT %zu %d %llu %s\n", result.index + 1, result.actual.midi,
                   (unsigned long long)result.actual.sample,
                   result.feedback == PRACTICE_CORRECT ? "OK" : "WRONG");
        }
    }
    printf("SUMMARY %zu/%zu errors=%zu complete=%d\n", engine.session.position, score->count,
           engine.session.errors, engine.session.state == PRACTICE_COMPLETE);
    return 0;
}
