#!/usr/bin/env python3
"""Generate original Drag start signals (mono 22.05 kHz, 16-bit PCM).

These simple synthesized tones use no samples or external audio material.
Run from any directory; optional --output-dir writes an isolated copy.
"""
from __future__ import annotations

import argparse
import math
from pathlib import Path
import struct
import wave

ROOT = Path(__file__).resolve().parents[2]
RATE = 22050


def write_tone(path: Path, duration: float, frequency: float) -> None:
    count = round(RATE * duration)
    samples = bytearray()
    for i in range(count):
        time = i / RATE
        envelope = min(1.0, time / 0.005, (count - 1 - i) / (RATE * 0.025))
        tone = math.sin(2 * math.pi * frequency * time)
        tone += 0.18 * math.sin(4 * math.pi * frequency * time)
        samples += struct.pack("<h", round(32767 * 0.24 * envelope * tone))
    path.parent.mkdir(parents=True, exist_ok=True)
    with wave.open(str(path), "wb") as output:
        output.setnchannels(1)
        output.setsampwidth(2)
        output.setframerate(RATE)
        output.writeframes(samples)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, default=ROOT / "baseq3r/sound/feedback")
    args = parser.parse_args()
    write_tone(args.output_dir / "drag_countdown.wav", 0.11, 660)
    write_tone(args.output_dir / "drag_green.wav", 0.27, 1320)


if __name__ == "__main__":
    main()
