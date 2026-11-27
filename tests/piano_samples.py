"""Offline mono grand-piano sample renderer for the GeneralUser SF2 bank.

Reads the named acoustic-piano PCM samples directly, without an audio device.
This is a simple sample player, not a complete SoundFont synthesizer.
"""
from array import array
from collections import Counter, defaultdict
import math
import os
from pathlib import Path
import struct
import sys

DEFAULT_SOUNDFONT = Path('/usr/share/minuet/soundfonts/GeneralUser-v1.47.sf2')

def chunks(data, start=0):
    cursor = start
    while cursor+8 <= len(data):
        name, count = struct.unpack_from('<4sI', data, cursor)
        end = cursor+8+count
        if end > len(data):
            raise ValueError('truncated SoundFont chunk')
        yield name, data[cursor+8:end]
        cursor = end+(count & 1)

class GrandPiano:
    def __init__(self, soundfont=None, rate=16000):
        path = Path(soundfont or os.environ.get('SONARIA_SOUNDFONT', DEFAULT_SOUNDFONT))
        if not path.is_file():
            raise ValueError(f'GeneralUser piano SoundFont not found: {path}; use --soundfont or --timbre harmonic')
        data = path.read_bytes()
        if data[:4] != b'RIFF' or data[8:12] != b'sfbk':
            raise ValueError('expected an uncompressed SF2 SoundFont')
        lists = {part[:4]: dict(chunks(part, 4)) for name, part in chunks(data, 12) if name == b'LIST'}
        try:
            self.samples = array('h')
            self.samples.frombytes(lists[b'sdta'][b'smpl'])
            if sys.byteorder != 'little':
                self.samples.byteswap()
            headers = list(struct.iter_unpack('<20sIIIIIBbHH', lists[b'pdta'][b'shdr']))
            generators=list(struct.iter_unpack('<Hh', lists[b'pdta'][b'igen']))
            bags=list(struct.iter_unpack('<HH', lists[b'pdta'][b'ibag']))
            roots=defaultdict(Counter)
            for i in range(len(bags)-1):
                zone=dict(generators[bags[i][0]:bags[i+1][0]])
                sample_id=zone.get(53)
                if sample_id is not None and 0<=sample_id<len(headers):
                    roots[sample_id][zone.get(58,headers[sample_id][6])]+=1
            headers=[(*h[:6], roots[i].most_common(1)[0][0] if roots[i] else h[6], *h[7:]) for i,h in enumerate(headers)]
            self.pianos = [h for h in headers if h[0].startswith(b'Grand Piano') and
                           0 <= h[1] < h[3] < h[4] <= h[2] <= len(self.samples) and h[5] > 0]
        except (KeyError, struct.error) as exc:
            raise ValueError('invalid SF2 sample data') from exc
        if not self.pianos:
            raise ValueError('SoundFont has no supported Grand Piano samples; use GeneralUser GS')
        self.rate = rate
        self.cache = {}

    def note(self, midi, count):
        key = (midi, count)
        if key in self.cache:
            return self.cache[key]
        if count <= 0:
            raise ValueError('note must contain at least one sample')
        # Fixed reference sample provides a consistent decay across fixture notes.
        # This is a controlled test timbre (not a full multi-sampled piano engine).
        _, start, end, loop_start, loop_end, source_rate, root, correction, _, _ = min(
            self.pianos, key=lambda h: abs(h[6]-63))
        step = source_rate/self.rate * 2**((midi-root+correction/100)/12)
        values = []
        for i in range(count):
            pos = start+i*step
            if pos >= end-1:
                pos = loop_start+(pos-loop_start) % (loop_end-loop_start)
            k = int(pos)
            next_index = loop_start if k+1 >= end else k+1
            value = self.samples[k]+(self.samples[next_index]-self.samples[k])*(pos-k)
            # Keep the sampled piano attack/decay, with a gentle additional sustain decay.
            values.append(value*math.exp(-i/self.rate/4))
        peak = max(map(abs, values))
        if peak == 0:
            raise ValueError('selected piano sample is silent')
        attack = max(1, min(self.rate//500, count//4))
        release = max(1, min(self.rate*15//1000, count//4))
        # Equal peak levels make playback tests reproducible; no overlapping note tails.
        out = array('h', (round(16384*v/peak*min(1, i/attack, (count-1-i)/release))
                          for i, v in enumerate(values)))
        if sys.byteorder != 'little':
            out.byteswap()
        self.cache[key] = out.tobytes()
        return self.cache[key]
