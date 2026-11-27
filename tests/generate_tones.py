"""Generate a reproducible acoustic test sequence for the Sonaria firmware."""
import argparse
import math
import struct
import wave

RATE = 16000
SEGMENTS = [(None, 5), (110, 4), (220, 4), (261.6256, 4), (435, 4),
            (440, 4), (445, 4), (523.2511, 4), (880, 4), (1760, 4), (None, 5)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", nargs="?", default="/tmp/sonaria-test-tones.wav")
    args = parser.parse_args()
    elapsed = 0
    with wave.open(args.output, "wb") as audio:
        audio.setparams((1, 2, RATE, 0, "NONE", "not compressed"))
        for frequency, seconds in SEGMENTS:
            count = RATE * seconds
            if frequency is None:
                samples = bytes(count * 2)
            else:
                # Brief fades avoid clicks while preserving the sustained pitch.
                fade = RATE // 100
                samples = b"".join(struct.pack("<h", round(10000 * min(1, i / fade, (count - 1 - i) / fade)
                                                          * math.sin(math.tau * frequency * i / RATE)))
                                   for i in range(count))
            audio.writeframes(samples)
            print(f"{elapsed:2d}-{elapsed + seconds:2d}s: {frequency or 'silence'}")
            elapsed += seconds
    print(args.output)


if __name__ == "__main__":
    main()
