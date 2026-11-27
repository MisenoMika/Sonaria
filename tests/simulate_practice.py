"""Evaluate the actual C practice engine with WAV/MP3 input and score reference."""
import argparse
import math
from pathlib import Path
import statistics
import subprocess
from score_fixtures import SCORES, SCORE_BPMS

ROOT=Path(__file__).resolve().parents[1]
def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('executable',type=Path,help='Built simulate_practice executable')
    p.add_argument('score',choices=SCORES)
    p.add_argument('audio',type=Path)
    p.add_argument('--bpm',type=float)
    p.add_argument('--check',action='store_true')
    args=p.parse_args()
    if args.bpm is None:args.bpm=SCORE_BPMS[args.score]
    pcm=subprocess.check_output(['ffmpeg','-v','error','-i',str(args.audio),'-ac','1','-ar','16000','-f','f32le','-'])
    result=subprocess.check_output([str(args.executable),args.score],input=pcm).decode()
    print(result,end='')
    events=[line.split() for line in result.splitlines() if line.startswith('EVENT ') and line.endswith(' OK')]
    reference=SCORES[args.score];onset=0;delays=[]
    for row,(midi,beats) in zip(events,reference):
        assert int(row[2])==midi
        delays.append(int(row[3])/16-onset*1000);onset+=beats*60/args.bpm
    p95=sorted(delays)[math.ceil(.95*len(delays))-1] if delays else math.inf
    print(f'Correct {len(events)}/{len(reference)}, delay median={statistics.median(delays) if delays else math.inf:.1f} ms P95={p95:.1f} ms')
    if args.check and not (len(events)==len(reference) and f'SUMMARY {len(reference)}/{len(reference)} errors=0 complete=1' in result and p95<=200 and min(delays)>=-1e-6):raise SystemExit(1)
if __name__=='__main__':main()
