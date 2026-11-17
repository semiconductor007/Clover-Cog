"""Original deterministic PCM cues; no sampled game audio. Run with Python 3."""
from pathlib import Path
import math
import random
import struct
import wave

folder = Path(__file__).resolve().parents[1] / 'resources/audio'
for name, duration, decay, frequency in [('axe_hit', .18, 36, 170), ('tree_fall', .65, 7, 85)]:
    randomizer = random.Random(381 if name == 'axe_hit' else 382)
    data = bytearray()
    for i in range(round(22050 * duration)):
        t = i / 22050
        envelope = min(1, t * 2000) * math.exp(-decay*t) * min(1, (duration-t)*80)
        sample = envelope * (.3*math.sin(2*math.pi*frequency*t) + .27*randomizer.uniform(-1,1))
        data += struct.pack('<h', round(sample * 32767))
    with wave.open(str(folder / (name + '.wav')), 'wb') as output:
        output.setparams((1, 2, 22050, 0, 'NONE', 'not compressed'))
        output.writeframes(data)
