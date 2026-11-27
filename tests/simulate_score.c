/* Host-side firmware algorithm runner; one CSV record per completed audio hop. */
#include "pitch_tracker.h"
#include "melody_detector.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
static pitch_tracker_t tracker;
static melody_detector_t melody;
static float frame[PITCH_FRAME_SIZE];
int main(int argc, char **argv)
{
    if (argc != 2 || (strcmp(argv[1],"mono") && strcmp(argv[1],"melody"))) return 2;
    bool use_melody=!strcmp(argv[1],"melody");
    if (use_melody) melody_detector_init(&melody);
    if (fread(frame,sizeof(float),PITCH_FRAME_SIZE,stdin)!=PITCH_FRAME_SIZE) return 1;
    unsigned end=PITCH_FRAME_SIZE;
    puts("end_sample,raw_midi,display_midi,tracking,frequency,confidence,compute_us");
    do {
        clock_t start=clock();
        tuner_result_t r=use_melody?pitch_tracker_update_with_confirmation(&tracker,melody_detect(&melody,frame),MELODY_CONFIRM_FRAMES):pitch_track(&tracker,frame);
        double us=(clock()-start)*1e6/CLOCKS_PER_SEC;
        note_result_t note={0};
        int raw=r.raw.status==PITCH_VALID && frequency_to_note(r.raw.frequency_hz,&note)?note.midi:-1;
        printf("%u,%d,%d,%s,%.4f,%.4f,%.1f\n",end,raw,r.pitch.status==PITCH_VALID?r.note.midi:-1,
               pitch_tracking_name(r.tracking),r.pitch.frequency_hz,r.raw.confidence,us);
        memmove(frame,frame+PITCH_HOP_SIZE,(PITCH_FRAME_SIZE-PITCH_HOP_SIZE)*sizeof(float));
        end+=PITCH_HOP_SIZE;
    }while(fread(frame+PITCH_FRAME_SIZE-PITCH_HOP_SIZE,sizeof(float),PITCH_HOP_SIZE,stdin)==PITCH_HOP_SIZE);
    return 0;
}
