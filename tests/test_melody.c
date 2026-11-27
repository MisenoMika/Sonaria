#include "melody_detector.h"
#include "note_mapper.h"
#include "pitch_tracker.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(c) do { if (!(c)) { fprintf(stderr,"%s:%d: %s\n",__FILE__,__LINE__,#c);exit(1); } } while(0)
static melody_detector_t detector;
static pitch_tracker_t tracker;
static float samples[PITCH_FRAME_SIZE];
static const double tau=6.283185307179586;
static void synth(float melody,float bass)
{
    for(int i=0;i<PITCH_FRAME_SIZE;++i) {
        double t=tau*i/PITCH_SAMPLE_RATE;
        samples[i]=.035f*sin(melody*t)+.012f*sin(2*melody*t)+.006f*sin(3*melody*t);
        if(bass>0)samples[i]+=.055f*sin(bass*t)+.025f*sin(2*bass*t)+.012f*sin(3*bass*t);
    }
}
static void expect_note(float melody,float bass,int midi)
{
    melody_detector_reset(&detector);pitch_tracker_reset(&tracker);
    synth(melody,bass);
    pitch_result_t raw=melody_detect(&detector,samples);note_result_t note;
    CHECK(raw.status==PITCH_VALID && frequency_to_note(raw.frequency_hz,&note));
    if(note.midi!=midi)fprintf(stderr,"actual %d expected %d (%.2f Hz)\n",note.midi,midi,raw.frequency_hz);
    CHECK(note.midi==midi);
    tuner_result_t r=pitch_tracker_update_with_confirmation(&tracker,raw,MELODY_CONFIRM_FRAMES);
    CHECK(r.tracking==TRACK_PENDING);
    r=pitch_tracker_update_with_confirmation(&tracker,melody_detect(&detector,samples),MELODY_CONFIRM_FRAMES);
    CHECK(r.tracking==TRACK_CONFIRMED && r.note.midi==midi);
}
int main(void)
{
    expect_note(440,0,69);
    expect_note(698.456f,0,77);
    expect_note(392,164.8138f,67);
    expect_note(349.2282f,174.6141f,65);
    expect_note(587.3295f,220,74);
    memset(samples,0,sizeof(samples));
    CHECK(melody_detect(&detector,samples).status==PITCH_SILENCE);
    unsigned random=4567;
    for(unsigned n=0;n<8;++n){
        for(int i=0;i<PITCH_FRAME_SIZE;++i){random=random*1664525u+1013904223u;samples[i]=.03f*((random>>8)/8388608.0f-1);}
        CHECK(melody_detect(&detector,samples).status!=PITCH_VALID);
    }
    puts("Upper-voice melody tests passed");return 0;
}
