#include "practice_score.h"
#include <stdio.h>
#include <string.h>
const practice_score_t *practice_score_find(const char *id)
{
    for (size_t i = 0; i < practice_score_count; ++i)
        if (strcmp(practice_scores[i].id, id) == 0)
            return &practice_scores[i];
    return NULL;
}
void practice_note_name(int midi, char *out, size_t size)
{
    static const char *names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    if (midi < 0 || midi > 127)
        snprintf(out, size, "--");
    else
        snprintf(out, size, "%s%d", names[midi % 12], midi / 12 - 1);
}
