#include "melody_detector.h"
#include "note_mapper.h"
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323846f

void melody_detector_reset(melody_detector_t *d)
{
    memset(d->previous,0,sizeof(d->previous));
    d->previous_note=-1;
}

void melody_detector_init(melody_detector_t *d)
{
    if (d->initialized) return;
    for (unsigned i=0;i<PITCH_FRAME_SIZE;++i)
        d->window[i]=.5f-.5f*cosf(2*PI*i/(PITCH_FRAME_SIZE-1));
    for (unsigned i=0;i<MELODY_FFT_SIZE/2;++i) {
        float phase=-2*PI*i/MELODY_FFT_SIZE;
        d->twiddle_real[i]=cosf(phase);
        d->twiddle_imag[i]=sinf(phase);
    }
    melody_detector_reset(d);
    d->initialized=true;
}

static void fft(melody_detector_t *d)
{
    for (unsigned i=1,j=0;i<MELODY_FFT_SIZE;++i) {
        unsigned bit=MELODY_FFT_SIZE>>1;
        for (;j&bit;bit>>=1) j^=bit;
        j^=bit;
        if (i<j) { float t=d->real[i];d->real[i]=d->real[j];d->real[j]=t; }
    }
    for (unsigned len=2;len<=MELODY_FFT_SIZE;len<<=1) {
        unsigned step=MELODY_FFT_SIZE/len;
        for (unsigned start=0;start<MELODY_FFT_SIZE;start+=len) {
            for (unsigned j=0;j<len/2;++j) {
                unsigned u=start+j,v=u+len/2,k=j*step;
                float tr=d->real[v]*d->twiddle_real[k]-d->imag[v]*d->twiddle_imag[k];
                float ti=d->real[v]*d->twiddle_imag[k]+d->imag[v]*d->twiddle_real[k];
                float ur=d->real[u],ui=d->imag[u];
                d->real[u]=ur+tr;d->imag[u]=ui+ti;
                d->real[v]=ur-tr;d->imag[v]=ui-ti;
            }
        }
    }
}

static float at(const float *v,int midi)
{
    return midi>=MELODY_PEAK_MIN_MIDI && midi<=MELODY_PEAK_MAX_MIDI?v[midi-MELODY_PEAK_MIN_MIDI]:0;
}

pitch_result_t melody_detect(melody_detector_t *d,const float samples[PITCH_FRAME_SIZE])
{
    pitch_result_t result={.status=PITCH_UNCERTAIN};
    melody_detector_init(d);
    float mean=0,energy=0;
    for (unsigned i=0;i<PITCH_FRAME_SIZE;++i) mean+=samples[i];
    mean/=PITCH_FRAME_SIZE;
    for (unsigned i=0;i<PITCH_FRAME_SIZE;++i) {
        float x=samples[i]-mean;energy+=x*x;d->real[i]=x*d->window[i];
    }
    result.rms=sqrtf(energy/PITCH_FRAME_SIZE);
    if (!isfinite(result.rms)) { melody_detector_reset(d);return result; }
    if (result.rms<PITCH_RMS_THRESHOLD) { result.status=PITCH_SILENCE;melody_detector_reset(d);return result; }
    memset(d->real+PITCH_FRAME_SIZE,0,(MELODY_FFT_SIZE-PITCH_FRAME_SIZE)*sizeof(float));
    memset(d->imag,0,sizeof(d->imag));
    fft(d);
    // Use interpolated spectral peaks; FFT zero padding improves sampling, not actual resolution.
    for (unsigned i=0;i<=MELODY_FFT_SIZE/4;++i) d->real[i]=d->real[i]*d->real[i]+d->imag[i]*d->imag[i];
    float amp[MELODY_PEAK_COUNT]={0},freq[MELODY_PEAK_COUNT]={0},maximum=0;
    for (unsigned k=MELODY_FFT_SIZE*55/PITCH_SAMPLE_RATE;k<MELODY_FFT_SIZE/4;++k) {
        if (d->real[k]<d->real[k-1] || d->real[k]<=d->real[k+1]) continue;
        float a=sqrtf(d->real[k]),left=sqrtf(d->real[k-1]),right=sqrtf(d->real[k+1]);
        float denom=left-2*a+right;
        float offset=denom<0?.5f*(left-right)/denom:0;
        float f=(k+offset)*PITCH_SAMPLE_RATE/MELODY_FFT_SIZE;
        float m=69+12*log2f(f/440);int n=(int)floorf(m+.5f);
        if (n<MELODY_PEAK_MIN_MIDI || n>MELODY_PEAK_MAX_MIDI || fabsf(m-n)>=.45f) continue;
        unsigned i=n-MELODY_PEAK_MIN_MIDI;
        if (a>amp[i]) { amp[i]=a;freq[i]=f; }
        if (a>maximum) maximum=a;
    }
    // Reject broadband noise: coherent spectral peaks occupy much more of the frame energy.
    if (maximum < .08f*result.rms*PITCH_FRAME_SIZE) { melody_detector_reset(d);return result; }
    static const int offsets[]={12,19,24,28,31,34,36,38,40,42,43};
    float best=-1;int selected=-1;
    for (int m=MELODY_MIN_MIDI;m<=MELODY_MAX_MIDI;++m) {
        float a=at(amp,m);
        float score=a+.6f*(at(amp,m+12)+.6f*at(amp,m+19)+.35f*at(amp,m+24));
        // Permit octave overlap with bass below the melody register, but penalize other harmonics.
        score-=at(amp,m-12)*(m<MELODY_MIN_MIDI+12?.5f:1.2f);
        for (unsigned h=3;h<=12;++h) score-=1.8f/h*at(amp,m-offsets[h-2]);
        score+=.5f*fmaxf(0,a-at(d->previous,m)); // New attack evidence.
        if (m==d->previous_note && a>.05f*maximum) score+=.5f*maximum;
        if (score>best) { best=score;selected=m; }
    }
    memcpy(d->previous,amp,sizeof(amp));
    if (selected<0 || best<.06f*maximum) { d->previous_note=-1;return result; }
    float f=at(freq,selected);
    if (f<=0) {
        // Missing fundamental: use a measured harmonic, never a score-derived nominal frequency.
        float strength=0;
        for (unsigned h=2;h<=4;++h) {
            int m=selected+offsets[h-2];float a=at(amp,m),candidate=at(freq,m)/h;
            note_result_t note;
            if (a>strength && a>.05f*maximum && frequency_to_note(candidate,&note) && note.midi==selected) {
                strength=a;f=candidate;
            }
        }
    }
    if (f<=0) { d->previous_note=-1;return result; }
    d->previous_note=selected;
    result.status=PITCH_VALID;result.frequency_hz=f;
    // Heuristic spectral evidence score, not YIN periodicity or a probability.
    result.confidence=.90f+.10f*fminf(1,best/maximum);
    return result;
}
