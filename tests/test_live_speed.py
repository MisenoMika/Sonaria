"""Replay continuous single-note sequences through the firmware's live engine."""
import argparse
import math
import random
import statistics
import struct
import subprocess
import wave
from pathlib import Path

RATE = 16000
NOTES = list(range(60, 85)) + list(range(83, 59, -1))


def evaluate(executable, duration, piano, noise, wav_dir=None):
    count = RATE * duration // 1000
    rng = random.Random(42)
    audio = bytearray(bytes(RATE // 5 * 2))
    start = len(audio) // 2
    for note in NOTES:
        if piano:
            samples = struct.unpack(f'<{count}h', piano.note(note, count))
        else:
            hz = 440 * 2 ** ((note - 69) / 12)
            samples = [int(12000 * (math.sin(2 * math.pi * hz * i / RATE)
                       + .25 * math.sin(4 * math.pi * hz * i / RATE))
                       * min(1, i / 32, (count - 1 - i) / 64)) for i in range(count)]
        audio += struct.pack(f'<{count}h', *(int(s + rng.uniform(-noise, noise)) for s in samples))
    audio += bytes(RATE // 5 * 2)
    if wav_dir is not None:
        wav_dir.mkdir(parents=True, exist_ok=True)
        with wave.open(str(wav_dir / f"live_{duration}ms.wav"), "wb") as wav:
            wav.setparams((1, 2, RATE, 0, "NONE", "not compressed"))
            wav.writeframes(audio)
    output = subprocess.check_output([executable], input=audio).decode()
    delays = {}
    wrong = 0
    confirmed = 0
    transitions = []
    previous = None
    for line in output.splitlines():
        sample, midi, state = map(int, line.split())
        idx = (sample - start - 1) // count
        if state == 2 and midi != previous:
            transitions.append(midi)
            previous = midi
        if not 0 <= idx < len(NOTES) or state != 2:
            continue
        confirmed += 1
        if midi == NOTES[idx]:
            delays.setdefault(idx, (sample - start - idx * count) * 1000 / RATE)
        else:
            wrong += 1
    median = statistics.median(delays.values()) if delays else float('inf')
    worst = max(delays.values(), default=float("inf"))
    print(f'{duration:3} ms: {len(delays)}/{len(NOTES)} notes, median {median:.1f} ms, max {worst:.1f} ms, '
          f'{wrong}/{confirmed} late or incorrect confirmed frames; sequence exact={transitions == NOTES}')
    return len(delays), median, transitions == NOTES


def check_noise(executable):
    rng = random.Random(21)
    audio = struct.pack(f"<{RATE}h", *(rng.randint(-12000, 12000) for _ in range(RATE)))
    output = subprocess.check_output([executable], input=audio).decode()
    assert all(int(line.split()[2]) != 2 for line in output.splitlines()), "noise confirmed as a note"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable')
    parser.add_argument('--piano', action='store_true')
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--noise', type=int, default=300, metavar='PCM_AMPLITUDE')
    parser.add_argument('--wav-dir', type=Path, help='also save the replay signals as WAV files')
    args = parser.parse_args()
    if not 0 <= args.noise <= 4000:
        parser.error("--noise must be between 0 and 4000")
    piano = None
    if args.piano:
        from piano_samples import GrandPiano
        piano = GrandPiano()
    if args.check:
        check_noise(args.executable)
    for duration in (80, 100, 120, 150, 200):
        hit, delay, exact = evaluate(args.executable, duration, piano, args.noise, args.wav_dir)
        if args.check and duration >= (80 if args.noise <= 300 else 100):
            assert hit == len(NOTES) and delay <= 80 and exact


if __name__ == '__main__':
    main()
