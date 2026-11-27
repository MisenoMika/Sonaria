"""Replay the committed piano error fixtures and assert exact reports and completion."""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
CASES = {
    'twinkle_twinkle_wrong1.wav': ('twinkle', 42, [
        'ERROR song=twinkle note=3/42 bar=1 beat=3 expected=G4 actual=F4']),
    'twinkle_twinkle_wrong2.wav': ('twinkle', 42, [
        'ERROR song=twinkle note=5/42 bar=2 beat=1 expected=A4 actual=G#4',
        'ERROR song=twinkle note=11/42 bar=3 beat=4 expected=E4 actual=D#4']),
    'mary_had_a_little_lamb_wrong1.wav': ('mary', 26, [
        'ERROR song=mary note=3/26 bar=1 beat=3 expected=C5 actual=C#5']),
    'mary_had_a_little_lamb_wrong2.wav': ('mary', 26, [
        'ERROR song=mary note=8/26 bar=3 beat=1 expected=D5 actual=D#5',
        'ERROR song=mary note=17/26 bar=5 beat=4 expected=D5 actual=C5']),
}

def main():
    for file, (score, count, expected) in CASES.items():
        pcm = subprocess.check_output(['ffmpeg', '-v', 'error', '-i', str(ROOT/'assets'/file),
                                       '-ac', '1', '-ar', '16000', '-f', 'f32le', '-'])
        output = subprocess.check_output([sys.argv[1], score], input=pcm).decode()
        actual = [line for line in output.splitlines() if line.startswith('ERROR ')]
        summary = f'SUMMARY {count}/{count} errors={len(expected)} complete=1'
        if actual != expected or summary not in output:
            raise AssertionError(f'{file}\nexpected {expected}\n{output}')
        print(f'{file}: {summary}')
        print('\n'.join(actual))

if __name__ == '__main__':
    main()
