"""Replay an audio file through Sonaria and model old/new OLED scheduling.

Requires a host C compiler and ffmpeg; no ESP32 or third-party Python modules.
Timing is a model (not device measurement): 14.5 ms compute, 1 ms rendering/
transaction overhead and a conservative 10 ms task yield after each refresh.
"""
import argparse
import bisect
import csv
import math
from pathlib import Path
import statistics
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
ROW_BYTES = 128 * 2
FRAME_BYTES = ROW_BYTES * 128


def runs(records, status_col, note_col, min_frames=3):
    i = 0
    while i < len(records):
        j = i + 1
        while j < len(records) and records[j][status_col:note_col+1] == records[i][status_col:note_col+1]:
            j += 1
        if records[i][status_col] == 0 and j-i >= min_frames:
            yield i, j
        i = j


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("audio", type=Path)
    args = parser.parse_args()
    pitch = ROOT / "components/pitch_analysis"
    oled = ROOT / "components/oled_display"
    tuner = ROOT / "components/tuner_display"
    with tempfile.TemporaryDirectory(prefix="sonaria-replay-") as tmp:
        tmp = Path(tmp)
        exe = tmp / "replay"
        pcm = tmp / "input.f32"
        rendered = tmp / "frames.bin"
        subprocess.run(["cc", "-O2", *[f"-I{x/'include'}" for x in (pitch, oled, tuner)],
                        str(ROOT / "tests/replay_latency.c"),
                        *[str(pitch / f) for f in ("pitch_detector.c", "pitch_tracker.c", "note_mapper.c")],
                        str(oled / "oled_graphics.c"), str(tuner / "tuner_screen.c"), "-lm", "-o", str(exe)], check=True)
        subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", str(args.audio), "-ac", "1", "-ar", "16000", "-f", "f32le", str(pcm)], check=True)
        with pcm.open("rb") as stream:
            output = subprocess.check_output([str(exe), str(rendered)], stdin=stream, text=True)
        records = [list(map(int, row)) for row in csv.reader(output.splitlines())]
        data = rendered.read_bytes()
    if not records:
        raise SystemExit("No complete audio frames")
    frames = [data[i:i+FRAME_BYTES] for i in range(0, len(data), FRAME_BYTES)]
    times = [r[0]/16+14.5 for r in records]
    print(f"{len(records)} frames; {sum(r[1] == 0 for r in records)} valid raw frames")
    for name, status_col, note_col in (("legacy median", 6, 7), ("stable tracker", 3, 4)):
        episodes = list(runs(records, status_col, note_col, 1))
        print(f"{name}: {len(episodes)} displayed-note episodes; {sum(j-i<4 for i,j in episodes)} shorter than 4 hops; {sum(j-i for i,j in episodes)} visible frames")
    print("Tracking frames: " + ", ".join(f"{name}={sum(r[5]==i for r in records)}" for i,name in enumerate(("IDLE","PENDING","CONFIRMED","HOLD"))))
    tracker_delays = []
    for i, j in runs(records, 1, 2):
        k = next((k for k in range(i, j) if records[k][5] == 2 and records[k][3] == 0 and records[k][4] == records[i][2]), None)
        if k is not None:
            tracker_delays.append((k-i)*32)
    if tracker_delays:
        print(f"Tracker: {len(tracker_delays)} stable matching raw runs (confirmed frames only), median {statistics.median(tracker_delays):.1f} ms, max {max(tracker_delays):.1f} ms")
    old = []
    for start in range(0, int(times[-1])+201, 200):
        i = bisect.bisect_right(times, start)-1
        if i >= 0 and (not old or old[-1][0] != i):
            old.append((i, start+131.072))
    new = []
    previous = bytes(FRAME_BYTES)
    end = 0
    last = -1
    row_counts = []
    while last < len(times)-1:
        start = max(end, times[last+1])
        i = bisect.bisect_right(times, start)-1
        frame = frames[i]
        rows = sum(frame[y:y+ROW_BYTES] != previous[y:y+ROW_BYTES] for y in range(0, FRAME_BYTES, ROW_BYTES))
        end = start+rows*0.512+1
        new.append((i, end))
        row_counts.append(rows)
        end += 10
        last, previous = i, frame
    print(f"New dirty rows: median {statistics.median(row_counts):.1f}, max {max(row_counts)}")
    stable = list(runs(records, 3, 4))
    for name, trace in (("old display schedule", old), ("new display schedule", new)):
        delays = []
        missed = 0
        for i, j in stable:
            match = next((x for x in trace if i <= x[0] < j), None)
            if match is None:
                missed += 1
            else:
                delays.append(match[1]-times[i])
        if delays:
            p95 = sorted(delays)[math.ceil(.95*len(delays))-1]
            print(f"{name}: {len(delays)} shown, {missed} missed stable runs, median {statistics.median(delays):.2f} ms, p95 {p95:.2f} ms, max {max(delays):.2f} ms")
        else:
            print(f"{name}: no stable runs shown ({missed} missed)")


if __name__ == "__main__":
    main()
