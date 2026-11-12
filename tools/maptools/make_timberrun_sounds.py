#!/usr/bin/env python3
"""Synthesize the ambient loops of q3r_timberrun (Timber Run).

  sound/world/q3r_timberrun/forest.ogg   morning forest: leaves in the wind,
                                         chiffchaff, great tit, robin, a
                                         woodpecker and a distant crow
  sound/world/q3r_timberrun/creek.ogg    shallow creek: rushing water and bubbles

Everything is generated (no samples), so the files are free to use. Mono,
22,050 Hz, seamless loops. Needs numpy, scipy and ffmpeg with libvorbis.
"""
from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path

import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, sosfilt

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "baseq3r" / "sound" / "world" / "q3r_timberrun"
SR = 22050


def band(x, lo, hi, order=2):
    return sosfilt(butter(order, [lo, hi], btype="band", fs=SR, output="sos"), x)


def low(x, hi, order=2):
    return sosfilt(butter(order, hi, btype="low", fs=SR, output="sos"), x)


def loop_noise(rng, n, fade):
    """Noise that loops: the tail crossfaded into the head."""
    x = rng.standard_normal(n + fade)
    w = np.linspace(0, 1, fade)
    x[:fade] = x[:fade] * w + x[n:] * (1 - w)
    return x[:n]


def smooth_env(rng, n, rate_hz, depth):
    """Slow random swell, periodic over the loop."""
    t = np.arange(n) / n
    env = np.ones(n)
    for k in range(1, 4):
        cycles = max(1, round(rate_hz * n / SR * k / 2))
        env += depth / k * np.sin(2 * np.pi * cycles * t + rng.uniform(0, 2 * np.pi))
    return env


def tone(f0, f1, dur, shape="exp"):
    """A sweep from f0 to f1 (Hz) with a soft attack and decay."""
    n = int(dur * SR)
    t = np.arange(n) / SR
    f = f0 * (f1 / f0) ** (t / dur) if shape == "exp" else f0 + (f1 - f0) * t / dur
    ph = 2 * np.pi * np.cumsum(f) / SR
    env = np.minimum(1, t / 0.006) * np.minimum(1, (dur - t) / 0.02) ** 1.5
    return np.sin(ph) * env


def put(buf, sig, at, gain):
    i = int(at * SR) % len(buf)
    end = min(len(buf), i + len(sig))
    buf[i:end] += sig[: end - i] * gain
    rest = len(sig) - (end - i)
    if rest > 0:
        buf[:rest] += sig[end - i:] * gain


def chiffchaff(rng):
    out = []
    notes = rng.integers(6, 10)
    for k in range(notes):
        hi = (k + rng.integers(0, 2)) % 2 == 0
        f = rng.uniform(5200, 5700) if hi else rng.uniform(4200, 4600)
        out += [tone(f * 1.08, f * 0.9, 0.07), np.zeros(int(rng.uniform(0.22, 0.32) * SR))]
    return np.concatenate(out)


def great_tit(rng):
    out = []
    for _ in range(rng.integers(3, 6)):
        out += [tone(6800, 6500, 0.09), np.zeros(int(0.04 * SR)), tone(4300, 4100, 0.12),
                np.zeros(int(0.07 * SR))]
    return np.concatenate(out)


def robin(rng):
    out = []
    for _ in range(rng.integers(10, 18)):
        f0, f1 = rng.uniform(2800, 7500), rng.uniform(2800, 7500)
        d = rng.uniform(0.03, 0.09)
        s = tone(f0, f1, d)
        vib = 1 + 0.3 * np.sin(2 * np.pi * rng.uniform(40, 90) * np.arange(len(s)) / SR)
        out += [s * vib, np.zeros(int(rng.uniform(0.01, 0.05) * SR))]
    return np.concatenate(out)


def woodpecker(rng):
    hits = rng.integers(14, 20)
    rate = rng.uniform(15, 18)
    n = int((hits / rate + 0.1) * SR)
    s = np.zeros(n)
    for k in range(hits):
        m = int(0.025 * SR)
        t = np.arange(m) / SR
        knock = np.sin(2 * np.pi * rng.uniform(900, 1100) * t) * np.exp(-t / 0.004)
        knock += 0.5 * rng.standard_normal(m) * np.exp(-t / 0.002)
        put(s, knock, k / rate, 1 - 0.5 * k / hits)
    return band(s, 500, 3000)


def crow(rng):
    out = []
    for _ in range(rng.integers(2, 4)):
        d = rng.uniform(0.35, 0.5)
        n = int(d * SR)
        t = np.arange(n) / SR
        f = 620 - 140 * t / d
        ph = 2 * np.pi * np.cumsum(f) / SR
        saw = sum(np.sin(k * ph) / k for k in range(1, 12))
        rough = 1 + 0.6 * np.sin(2 * np.pi * 70 * t)
        env = np.minimum(1, t / 0.03) * np.minimum(1, (d - t) / 0.08)
        out += [saw * rough * env, np.zeros(int(rng.uniform(0.25, 0.45) * SR))]
    return band(np.concatenate(out), 400, 2600)


def forest(seconds=36):
    rng = np.random.default_rng(2026)
    n = seconds * SR
    leaves = band(loop_noise(rng, n, SR), 160, 1300) * smooth_env(rng, n, 0.12, 0.5)
    leaves = leaves / np.abs(leaves).max() * 0.06
    birds = np.zeros(n)
    events = [(chiffchaff, 1.0, 0.30), (great_tit, 7.5, 0.26), (robin, 12.0, 0.22), (woodpecker, 16.5, 0.30),
              (chiffchaff, 21.0, 0.16), (crow, 24.5, 0.18), (great_tit, 29.0, 0.14), (robin, 32.5, 0.12)]
    for make, at, gain in events:
        s = make(rng)
        far = low(s, 4000 + 3500 * gain / 0.3)                    # distant calls lose their highs
        put(birds, far / max(1e-9, np.abs(far).max()), at + rng.uniform(-0.4, 0.4), gain)
    # a little room: two soft echoes off the trees
    wet = birds.copy()
    for d, g in ((0.11, 0.25), (0.23, 0.12)):
        wet += np.roll(birds, int(d * SR)) * g
    return leaves + wet * 0.55


def creek(seconds=10):
    rng = np.random.default_rng(7)
    n = seconds * SR
    rush = low(loop_noise(rng, n, SR // 2), 1400) * smooth_env(rng, n, 0.4, 0.2)
    rush = rush / np.abs(rush).max() * 0.22
    hiss = band(loop_noise(rng, n, SR // 2), 2500, 7000)
    hiss = hiss / np.abs(hiss).max() * 0.05
    bub = np.zeros(n)
    for _ in range(int(seconds * 45)):
        d = rng.uniform(0.015, 0.05)
        f = rng.uniform(450, 1500)
        m = int(d * SR)
        t = np.arange(m) / SR
        f_t = f * (1 + 2.5 * t / d)                            # a bubble rises in pitch as it closes
        s = np.sin(2 * np.pi * np.cumsum(f_t) / SR) * np.exp(-t / (d / 3))
        put(bub, s, rng.uniform(0, seconds), rng.uniform(0.02, 0.12))
    return rush + hiss + bub


def write(name, x, peak=0.6):
    x = x / np.abs(x).max() * peak
    OUT.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        wav = Path(tmp) / f"{name}.wav"
        wavfile.write(wav, SR, (x * 32767).astype(np.int16))
        subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-i", str(wav), "-c:a", "libvorbis", "-q:a", "3",
                        str(OUT / f"{name}.ogg")], check=True)
    print("wrote", OUT / f"{name}.ogg", f"{len(x) / SR:.0f} s")


if __name__ == "__main__":
    write("forest", forest(), 0.55)
    write("creek", creek(), 0.6)
