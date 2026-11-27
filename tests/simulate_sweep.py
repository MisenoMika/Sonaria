"""Replay a sampled-piano white-key sweep in both directions."""
import argparse
from pathlib import Path
import subprocess
import wave

from piano_samples import GrandPiano

NOTES = [48, 50, 52, 53, 55, 57, 59, 60, 62, 64, 65,
         67, 69, 71, 72, 74, 76, 77, 79, 81, 83, 84]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("executable")
    parser.add_argument("--wav-dir", type=Path)
    args = parser.parse_args()
    piano = GrandPiano()
    for duration in (100, 120, 150):
        for label, notes in (("up", NOTES), ("down", NOTES[::-1])):
            audio = bytes(6400) + b"".join(piano.note(n, 16 * duration) for n in notes) + bytes(32000)
            if args.wav_dir:
                args.wav_dir.mkdir(parents=True, exist_ok=True)
                with wave.open(str(args.wav_dir / f"sweep_{label}_{duration}ms.wav"), "wb") as wav:
                    wav.setparams((1, 2, 16000, 0, "NONE", "not compressed"))
                    wav.writeframes(audio)
            output = subprocess.check_output([args.executable], input=audio, text=False).decode()
            print(f"{duration} ms {label}:\n{output.strip()}")
            assert f"direction={label.upper()} detected=22/22 errors=0 accuracy=100.0%" in output


if __name__ == "__main__":
    main()
