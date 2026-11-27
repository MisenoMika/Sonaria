"""Replay audio through the production C algorithm and evaluate against an independent score.

Twinkle reference: transcribed treble staff from assets/twinkle-twinkle-little-star.png.
Audio timing: 100 BPM, first onset at 0 s, verified with spectral flux. No reference
notes or timings are passed to the detector. Repeated notes are scored separately.
"""
import argparse
import csv
import io
import math
from pathlib import Path
import statistics
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[1]
from score_fixtures import SCORES, SCORE_BPMS, generate
NAMES=("C","C#","D","D#","E","F","F#","G","G#","A","A#","B")

def name(n):
    return "--" if n<0 else f"{NAMES[n%12]}{n//12-1}"

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument("audio",type=Path,nargs="?",help="Audio file; defaults to selected score recording")
    p.add_argument("--score",choices=SCORES,default="twinkle")
    p.add_argument("--transpose",type=int,default=0,help="Explicit score-to-recording semitone offset")
    p.add_argument("--synthesize",action="store_true",help="Generate a monophonic fixture from the selected score instead of reading an audio file")
    p.add_argument("--mode",choices=("mono","melody"),default="melody")
    p.add_argument("--bpm",type=float)
    p.add_argument("--offset",type=float,default=0)
    p.add_argument("--csv",type=Path,help="Export per-frame results with expected and detected note names")
    p.add_argument("--reference",type=Path,help="Optional CSV with onset_s,duration_s,midi columns (absolute audio timestamps)")
    p.add_argument("--check",action="store_true",help="Fail unless all score events are recognized, stable accuracy 100%%, changed-note P95 latency <=160 ms")
    args=p.parse_args()
    if args.bpm is None:args.bpm=SCORE_BPMS[args.score]
    if args.bpm<=0: p.error("BPM must be positive")
    pitch=ROOT/"components/pitch_analysis"
    with tempfile.TemporaryDirectory(prefix="sonaria-score-") as tmp:
        exe=Path(tmp)/"simulate"
        sources=[pitch/f for f in ("pitch_detector.c","pitch_tracker.c","note_mapper.c")]
        if (pitch/"melody_detector.c").exists():sources.append(pitch/"melody_detector.c")
        subprocess.run(["cc","-O2","-std=c11",f"-I{pitch/'include'}",str(ROOT/"tests/simulate_score.c"),*map(str,sources),"-lm","-o",str(exe)],check=True)
        if args.synthesize:
            audio=Path(tmp)/"monophonic.wav"
            generate(args.score,audio,args.bpm,args.transpose,args.offset)
        else:
            audio=args.audio or ROOT/"assets"/("twinkle_twinkle.mp3" if args.score=="twinkle" else "mary_had_a_little_lamb.mp3")
        pcm=subprocess.check_output(["ffmpeg","-v","error","-i",str(audio),"-ac","1","-ar","16000","-f","f32le","-"])
        output=subprocess.check_output([str(exe),args.mode],input=pcm).decode()
    rows=list(csv.DictReader(io.StringIO(output)))
    for r in rows:
        r.update(t=int(r["end_sample"])/16000,display=int(r["display_midi"]),
                 expected_note="--",raw_note=name(int(r["raw_midi"])),display_note=name(int(r["display_midi"])))
    if args.reference:
        with args.reference.open() as fp:
            reference=[(float(r["onset_s"]),float(r["duration_s"]),int(r["midi"])) for r in csv.DictReader(fp)]
    else:
        beat=60/args.bpm; onset=args.offset; reference=[]
        for m,d in SCORES[args.score]:
            reference.append((onset,d*beat,m+args.transpose)); onset+=d*beat
    if not reference or any(d<=0 or m<0 or m>127 for _,d,m in reference):
        raise SystemExit("Reference must contain positive durations and MIDI 0..127")
    events=[]; total=correct=0; delays=[]; transition_delays=[]; all_hits=all_total=0; prev=-1
    print("event start(s) expected first_correct(ms) stable correct/total")
    for i,(onset,duration,m) in enumerate(reference,1):
        end=onset+duration
        active=[r for r in rows if onset-1e-8<=r["t"]<end-1e-8]
        for r in active:r["expected_note"]=name(m)
        all_total+=len(active);all_hits+=sum(r["display"]==m for r in active)
        stable=[r for r in active if r["t"]>=onset+.160-1e-8 and r["t"]<end-.032-1e-8]
        hits=sum(r["display"]==m for r in stable); total+=len(stable);correct+=hits
        first=next((r for r in active if r["display"]==m and r["tracking"]=="CONFIRMED"),None)
        delay=max(0,(first["t"]-onset)*1000) if first else None
        if delay is not None:
            delays.append(delay)
            if m!=prev:transition_delays.append(delay)
        events.append(first is not None)
        print(f"{i:2d} {onset:6.3f} {name(m):4s} {f'{delay:.1f}' if delay is not None else 'MISS':>9} {hits}/{len(stable)}")
        prev=m
    if args.csv:
        with args.csv.open("w",newline="") as fp:
            writer=csv.DictWriter(fp,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
    acc=correct/total if total else 0
    p95=sorted(transition_delays)[math.ceil(.95*len(transition_delays))-1] if transition_delays else math.inf
    print(f"events={sum(events)}/{len(events)} stable_accuracy={acc:.2%} all_active_accuracy={all_hits/all_total if all_total else 0:.2%}")
    print(f"all-event median_ms={statistics.median(delays) if delays else math.inf:.1f}; changed-note count={len(transition_delays)} median_ms={statistics.median(transition_delays) if transition_delays else math.inf:.1f} p95_ms={p95:.1f} max_ms={max(transition_delays) if transition_delays else math.inf:.1f}")
    print(f"host compute_us median={statistics.median(float(r['compute_us']) for r in rows):.1f}; not ESP32 timing")
    if args.check and not (all(events) and total>0 and acc==1 and p95<=160+1e-6):raise SystemExit(1)

if __name__=="__main__":main()
