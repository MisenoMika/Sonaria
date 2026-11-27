"""Independent melody transcriptions and deterministic monophonic WAV fixtures."""
import argparse
import math
from pathlib import Path
import struct
import wave

RATE=16000
# Right-hand treble staff, 42 events / 48 quarter-note beats.
A=[(60,1),(60,1),(67,1),(67,1),(69,1),(69,1),(67,2),
   (65,1),(65,1),(64,1),(64,1),(62,1),(62,1),(60,2)]
B=[(67,1),(67,1),(65,1),(65,1),(64,1),(64,1),(62,2)]
TWINKLE=A+B+B+A
# Upper melody voice of mary-had-a-little-lamb.png, actually written C5–G5.
MARY=[(76,1),(74,1),(72,1),(74,1),
      (76,1),(76,1),(76,2),
      (74,1),(74,1),(74,2),
      (76,1),(79,1),(79,2),
      (76,1),(74,1),(72,1),(74,1),
      (76,1),(76,1),(76,1),(76,1),
      (74,1),(74,1),(76,1),(74,1),
      (72,4)]
SCORES={"twinkle":TWINKLE,"mary":MARY}

def generate(score, output, bpm=100, transpose=0, lead=0):
    """One harmonic tone at a time (no simultaneous notes); short attack/release."""
    if bpm<=0 or lead<0:raise ValueError("BPM must be positive and lead nonnegative")
    with wave.open(str(output),"wb") as audio:
        audio.setparams((1,2,RATE,0,"NONE","not compressed"))
        audio.writeframes(bytes(round(lead*RATE)*2))
        elapsed=0
        for midi,beats in SCORES[score]:
            count=round((elapsed+beats)*60/bpm*RATE)-round(elapsed*60/bpm*RATE)
            elapsed+=beats
            f=440*2**((midi+transpose-69)/12)
            fade=max(1,min(RATE//100,count//4))
            data=bytearray()
            for i in range(count):
                phase=math.tau*f*i/RATE
                env=min(1,i/fade,(count-1-i)/fade)
                # Instrument-like overtone mixture, but a single fundamental.
                v=.20*math.sin(phase)+.08*math.sin(2*phase)+.035*math.sin(3*phase)
                data.extend(struct.pack('<h',round(32767*env*v)))
            audio.writeframes(data)
        audio.writeframes(bytes(RATE*2))

if __name__=="__main__":
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("score",choices=SCORES)
    p.add_argument("output",type=Path)
    p.add_argument("--bpm",type=float,default=100)
    p.add_argument("--transpose",type=int,default=0)
    p.add_argument("--lead",type=float,default=0)
    a=p.parse_args();generate(a.score,a.output,a.bpm,a.transpose,a.lead)
    print(a.output)
