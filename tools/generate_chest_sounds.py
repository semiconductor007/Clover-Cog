"""Generate original wooden hinge/latch cues. Standard library, no sampled audio.

Run with Python 3 from any directory; outputs are project-owned PCM16 WAV files.
"""
from pathlib import Path
import math
import random
import struct
import wave


def write_cue(path, opening):
    rate = 22050
    duration = 0.34 if opening else 0.24
    noise = random.Random(710 if opening else 711)
    samples = []
    phase = 0.0
    filtered = 0.0
    for index in range(round(rate * duration)):
        t = index / rate
        progress = t / duration
        phase += 2 * math.pi * ((145 + 170 * progress) if opening else (230 - 100 * progress)) / rate
        filtered = 0.78 * filtered + 0.22 * noise.uniform(-1, 1)
        envelope = math.sin(math.pi * progress) ** 1.3
        hinge = envelope * (0.12 * math.sin(phase) + 0.05 * math.sin(phase * 2.13) + 0.16 * filtered)
        latch_time = t - (0.015 if opening else 0.055)
        latch = 0.0
        if latch_time >= 0:
            latch = math.exp(-latch_time * (65 if opening else 40)) * (
                0.30 * math.sin(2 * math.pi * 390 * latch_time)
                + 0.18 * math.sin(2 * math.pi * 670 * latch_time)
                + 0.12 * noise.uniform(-1, 1))
        tail = min(1, (duration - t) / 0.025)
        sample = max(-1, min(1, (hinge + latch) * tail))
        samples.append(struct.pack('<h', round(sample * 32767)))
    with wave.open(str(path), 'wb') as output:
        output.setparams((1, 2, rate, 0, 'NONE', 'not compressed'))
        output.writeframes(b''.join(samples))


if __name__ == '__main__':
    folder = Path(__file__).resolve().parents[1] / 'resources' / 'audio'
    write_cue(folder / 'chest_open.wav', True)
    write_cue(folder / 'chest_close.wav', False)
