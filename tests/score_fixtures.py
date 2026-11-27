"""Independent melody transcriptions and deterministic monophonic WAV fixtures."""
import argparse
import math
from pathlib import Path
import struct
import wave

RATE=16000
# JSON is the canonical independent reference for synthesis, scoring and practice.
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/"scores"))
from compile_scores import load_scores, TICKS
CATALOG=load_scores(Path(__file__).resolve().parents[1]/"scores")
SCORE_BPMS={ident:bpm for ident,_,bpm,_,_ in CATALOG}
SCORES={ident:[(m,ticks/TICKS) for m,ticks,_,_ in notes]
        for ident,_,_,_,notes in CATALOG}

def performance(score, transpose=0, wrong=()):
    """Insert deliberate wrong attempts before their intended correct notes."""
    errors={}
    for index,midi in wrong:
        if not 1<=index<=len(SCORES[score]):raise ValueError(f'wrong-note index {index} out of range')
        if midi==SCORES[score][index-1][0]+transpose:raise ValueError(f'note {index}: wrong attempt equals target')
        errors.setdefault(index,[]).append(midi)
    for index,(midi,beats) in enumerate(SCORES[score],1):
        for wrong_midi in errors.get(index,[]):yield wrong_midi,1
        yield midi+transpose,beats

def generate(score, output, bpm=None, transpose=0, lead=0, *, timbre="harmonic", soundfont=None, wrong=()):
    """One note at a time, with separated attacks; optional deliberate wrong attempts."""
    if bpm is None:bpm=SCORE_BPMS[score]
    if bpm<=0 or lead<0:raise ValueError("BPM must be positive and lead nonnegative")
    if timbre not in ("piano","harmonic"):raise ValueError("unknown timbre")
    piano=None
    if timbre=="piano":
        from piano_samples import GrandPiano
        piano=GrandPiano(soundfont,RATE)
    sequence=list(performance(score,transpose,wrong))
    with wave.open(str(output),"wb") as audio:
        audio.setparams((1,2,RATE,0,"NONE","not compressed"))
        audio.writeframes(bytes(round(lead*RATE)*2))
        elapsed=0
        for midi,beats in sequence:
            count=round((elapsed+beats)*60/bpm*RATE)-round(elapsed*60/bpm*RATE)
            elapsed+=beats
            if piano:
                audio.writeframes(piano.note(midi,count))
                continue
            f=440*2**((midi-69)/12)
            fade=max(1,min(RATE//100,count//4))
            data=bytearray()
            for i in range(count):
                phase=math.tau*f*i/RATE
                env=min(1,i/fade,(count-1-i)/fade)
                v=.20*math.sin(phase)+.08*math.sin(2*phase)+.035*math.sin(3*phase)
                data.extend(struct.pack('<h',round(32767*env*v)))
            audio.writeframes(data)
        audio.writeframes(bytes(RATE*2))

def parse_wrong(value):
    from compile_scores import midi_name
    try:
        index,note=value.split(":",1)
        return int(index),midi_name(note)
    except (ValueError,TypeError) as exc:
        raise argparse.ArgumentTypeError("expected INDEX:NOTE, e.g. 3:F4") from exc

if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("score",choices=SCORES)
    p.add_argument("output",type=Path)
    p.add_argument("--bpm",type=float)
    p.add_argument("--transpose",type=int,default=0)
    p.add_argument("--lead",type=float,default=0)
    p.add_argument("--timbre",choices=("piano","harmonic"),default="piano")
    p.add_argument("--soundfont",type=Path)
    p.add_argument("--wrong",type=parse_wrong,action="append",default=[],metavar="INDEX:NOTE")
    a=p.parse_args()
    try:generate(a.score,a.output,a.bpm,a.transpose,a.lead,timbre=a.timbre,soundfont=a.soundfont,wrong=a.wrong)
    except (ValueError,OSError) as exc:p.error(str(exc))
    print(a.output)
