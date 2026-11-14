"""Synthesised sounds for Q3Rally.

    python3 tools/soundbuild/build_sounds.py              # write into baseq3r/sound/
    python3 tools/soundbuild/build_sounds.py --out /tmp/s # somewhere else
    python3 tools/soundbuild/build_sounds.py --wav        # also keep a .wav next to each .ogg

Every sound is made from numpy noise and oscillators, nothing is sampled.
File names, sample rates and lengths stay as the game and maps expect them.
Looping sounds are built on a circle (periodic noise, events wrapped round
the end, whole cycles for tones), so they loop without a click.
Needs numpy and ffmpeg with libvorbis.
"""
import argparse
import os
import subprocess
import numpy as np

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
RNG = np.random.default_rng


# ---------------------------------------------------------------- building blocks
def band_noise(n, sr, lo, hi, seed, tilt=0.0, order=2):
    """Periodic noise in a soft band lo..hi Hz (FFT, Butterworth-like slopes),
    tilt in dB/octave."""
    spec = np.fft.rfft(RNG(seed).standard_normal(n))
    f = np.maximum(np.fft.rfftfreq(n, 1 / sr), 1e-3)
    g = 1 / np.sqrt(1 + (f / hi) ** (2 * order))
    if lo > 0:
        g /= np.sqrt(1 + (lo / f) ** (2 * order))
    if tilt:
        g *= (f / 1000.0) ** (tilt / 6.02)
    g[0] = 0
    out = np.fft.irfft(spec * g, n)
    return out / (np.abs(out).max() + 1e-12)


def swells(n, sr, count, seed, width=(0.6, 1.4), floor=0.0):
    """Periodic envelope made of `count` soft swells at random places."""
    rng = RNG(seed)
    t = np.arange(n)
    e = np.zeros(n)
    for _ in range(count):
        c = rng.integers(n)
        w = rng.uniform(*width) * sr / 2
        d = np.minimum(np.abs(t - c), n - np.abs(t - c))
        e = np.maximum(e, np.exp(-(d / w) ** 4) * rng.uniform(0.7, 1.0))
    return floor + (1 - floor) * e


def slow(n, sr, rate, seed, depth=1.0):
    """Periodic slow random control signal around 1.0 (rate = Hz upper limit)."""
    c = band_noise(n, sr, 0.0, rate, seed)
    return 1 + depth * c


def resonator(x, sr, freq, q):
    """Two-pole band-pass (biquad), applied circularly by running twice."""
    w = 2 * np.pi * freq / sr
    alpha = np.sin(w) / (2 * q)
    b0, b2 = alpha, -alpha
    a0, a1, a2 = 1 + alpha, -2 * np.cos(w), 1 - alpha
    b0, b2, a1, a2 = b0 / a0, b2 / a0, a1 / a0, a2 / a0
    y = np.zeros(len(x))
    y1 = y2 = x1 = x2 = 0.0
    for _ in range(2):                       # second pass starts with the filter state of the end: no click at the loop
        for i, v in enumerate(x):
            o = b0 * v + b2 * x2 - a1 * y1 - a2 * y2
            x2, x1 = x1, v
            y2, y1 = y1, o
            y[i] = o
    return y


def place(buf, ev, pos, wrap=True):
    """Add an event into buf at sample pos (wrapping round the end for loops)."""
    n = len(buf)
    idx = pos + np.arange(len(ev))
    if wrap:
        np.add.at(buf, idx % n, ev)
    else:
        m = idx < n
        buf[idx[m]] += ev[m]


def env_exp(n, sr, attack, decay):
    t = np.arange(n) / sr
    return np.minimum(t / max(attack, 1e-4), 1.0) * np.exp(-t / decay)


def whole_cycles(f, seconds):
    """Nearest frequency that fits a whole number of cycles into the loop."""
    return round(f * seconds) / seconds


def level(x, rms):
    """Scale to the target loudness; if that would clip, saturate softly
    (tanh) instead, like a compressed recording."""
    x = x - x.mean()
    x = x / (np.abs(x).max() + 1e-12)
    cur = lambda y: np.sqrt((y * y).mean())
    if cur(x) * 0.97 >= rms:
        return x * (rms / cur(x))
    lo, hi = 1.0, 60.0
    for _ in range(40):                      # find the drive that gives the target loudness
        g = (lo + hi) / 2
        y = 0.97 * np.tanh(x * g) / np.tanh(g)
        lo, hi = (g, hi) if cur(y) < rms else (lo, g)
    return y


# ---------------------------------------------------------------- the sounds
def lava(sr=22050, seconds=16.1):
    """Steam hissing up in surges over a low rumble, with some bubbles."""
    n = int(sr * seconds)
    rng = RNG(10)
    rumble = band_noise(n, sr, 25, 220, 11) * slow(n, sr, 0.4, 12, 0.35) * 0.5
    hiss = band_noise(n, sr, 600, 9000, 13, tilt=1) * swells(n, sr, 5, 14, (1.0, 1.8), 0.02)
    hiss *= slow(n, sr, 6, 15, 0.35)
    bub = np.zeros(n)
    for _ in range(int(seconds * 3)):                         # bubbles: rising, quickly damped
        L = int(sr * rng.uniform(0.03, 0.09))
        t = np.arange(L) / sr
        f0 = rng.uniform(120, 420)
        ph = 2 * np.pi * np.cumsum(f0 * (1 + 2.5 * t / t[-1])) / sr
        place(bub, np.sin(ph) * env_exp(L, sr, 0.004, L / sr / 3) * rng.uniform(0.2, 0.5), rng.integers(n))
    for _ in range(int(seconds * 0.6)):                       # big slow bloops
        L = int(sr * rng.uniform(0.18, 0.35))
        t = np.arange(L) / sr
        f0 = rng.uniform(45, 90)
        ph = 2 * np.pi * np.cumsum(f0 * (1 + 1.2 * t / t[-1])) / sr
        place(bub, np.sin(ph) * env_exp(L, sr, 0.02, L / sr / 3) * rng.uniform(0.5, 0.9), rng.integers(n))
    return rumble + hiss + bub * 0.5


def fire(sr=44100, seconds=6.58):
    n = int(sr * seconds)
    rng = RNG(20)
    roar = band_noise(n, sr, 50, 16000, 21, tilt=-2.5) * slow(n, sr, 3, 22, 0.5)
    breath = band_noise(n, sr, 400, 6000, 23) * slow(n, sr, 1.2, 24, 0.7) * 0.25
    crack = np.zeros(n)
    for _ in range(int(seconds * 28)):
        L = int(sr * rng.uniform(0.001, 0.006))
        ev = band_noise(max(L, 32), sr, 1500, 14000, int(rng.integers(1 << 30))) * env_exp(max(L, 32), sr, 0.0002, L / sr / 2)
        place(crack, ev * rng.uniform(0.2, 1.0) ** 2, rng.integers(n))
    for _ in range(int(seconds * 1.5)):                       # louder pops with a short body
        L = int(sr * 0.05)
        ev = band_noise(L, sr, 200, 4000, int(rng.integers(1 << 30))) * env_exp(L, sr, 0.0005, 0.008)
        place(crack, ev * rng.uniform(0.6, 1.2), rng.integers(n))
    return roar * 0.55 + breath + crack * 2.2


def hum(sr=22050, seconds=5.45):
    n = int(sr * seconds)
    seconds = n / sr                         # exact loop length, so tones close their cycles
    t = np.arange(n) / sr
    base = whole_cycles(50, seconds)
    x = np.zeros(n)
    for k, a in ((1, 1.0), (2, 0.55), (3, 0.35), (4, 0.18), (6, 0.12), (8, 0.06)):
        f = whole_cycles(base * k, seconds)
        x += a * np.sin(2 * np.pi * f * t)
        x += a * 0.3 * np.sin(2 * np.pi * whole_cycles(f + 0.8 * k, seconds) * t)   # slight beating
    x *= 1 + 0.15 * np.sin(2 * np.pi * whole_cycles(0.6, seconds) * t)
    fan = band_noise(n, sr, 400, 2500, 31) * 0.12
    whine = 0.05 * np.sin(2 * np.pi * whole_cycles(1870, seconds) * t) * (1 + 0.5 * np.sin(2 * np.pi * whole_cycles(0.37, seconds) * t))
    tick = np.zeros(n)
    rng = RNG(32)
    for _ in range(int(seconds * 2)):                           # relay / drive ticks
        L = int(sr * 0.01)
        place(tick, band_noise(L, sr, 2000, 7000, int(rng.integers(1 << 30))) * env_exp(L, sr, 0.0003, 0.002) * 0.25, rng.integers(n))
    bleep = np.zeros(n)
    for _ in range(6):                                          # short computer bleeps
        L = int(sr * rng.uniform(0.04, 0.09))
        tt = np.arange(L) / sr
        f = rng.choice([880, 1320, 1760, 2640, 3520])
        ev = np.sin(2 * np.pi * f * tt) * np.minimum(1, np.minimum(tt, tt[-1] - tt) / 0.005)
        place(bleep, ev * 0.12, rng.integers(n))
    return x * 0.6 + fan + whine + tick + bleep


def hole(sr=22050, seconds=2.51):
    """Air pulled through an opening: five swells per loop, each with a
    resonance that falls while it fades."""
    n = int(sr * seconds)
    seconds = n / sr                         # exact loop length, so tones close their cycles
    t = np.arange(n) / sr
    x = band_noise(n, sr, 60, 6000, 41, tilt=-3)
    per = seconds / 5
    phase = (t % per) / per                                       # 0..1 inside each swell
    centre = 900 * (1 - phase) + 220 * phase
    out = np.zeros(n)
    for fc in (220, 300, 400, 530, 700, 900):
        wgt = np.exp(-((centre - fc) / (0.35 * fc)) ** 2)
        out += resonator(x, sr, fc, 3.0) * wgt
    swell = np.sin(np.pi * phase) ** 1.5
    return out * (0.25 + 0.75 * swell) + x * 0.08


def chainsaw_run(sr=22050, seconds=2.0):
    """Gauntlet idle-while-firing loop: a small two-stroke buzz."""
    n = int(sr * seconds)
    seconds = n / sr                         # exact loop length, so tones close their cycles
    t = np.arange(n) / sr
    f = whole_cycles(62, seconds)
    wob = 0.03 * np.sin(2 * np.pi * whole_cycles(5, seconds) * t)
    ph = 2 * np.pi * np.cumsum(f * (1 + wob)) / sr
    saw = 2 * ((ph / (2 * np.pi)) % 1.0) - 1
    pulse = np.sign(np.sin(ph)) * 0.4
    body = saw * 0.6 + pulse
    body = np.tanh(body * 2.2)
    rasp = band_noise(n, sr, 1500, 7000, 51) * (0.5 + 0.5 * (np.sin(ph) > 0.3)) * 0.35
    chain = band_noise(n, sr, 3000, 9000, 52) * 0.15
    return body + rasp + chain


def chainsaw_hit(sr=22050, seconds=0.77):
    """Gauntlet hit: metallic clang over a short motor rev."""
    n = int(sr * seconds)
    t = np.arange(n) / sr
    x = np.zeros(n)
    for f, a, d in ((410, 1.0, 0.18), (1120, 0.7, 0.12), (2230, 0.5, 0.08), (3470, 0.35, 0.05), (5110, 0.2, 0.03)):
        x += a * np.sin(2 * np.pi * f * t + f) * np.exp(-t / d)
    click = band_noise(n, sr, 1000, 9000, 61) * np.exp(-t / 0.008) * 1.2
    thump = np.sin(2 * np.pi * 70 * t) * np.exp(-t / 0.05) * 0.8
    rev_f = 62 * (1 + 0.8 * np.exp(-t / 0.15))
    ph = 2 * np.pi * np.cumsum(rev_f) / sr
    rev = np.tanh((2 * ((ph / (2 * np.pi)) % 1.0) - 1) * 2) * np.exp(-t / 0.15) * 0.35
    out = x * 0.8 + click + thump + rev
    return out * np.minimum(1, (n - np.arange(n)) / (sr * 0.05))   # fade the tail to zero


# target loudness (RMS) of each file, so the mix in the game stays as it was
SOUNDS = [
    ('sound/world/lava1.ogg', lava, 22050, 0.099, 50),
    ('sound/world/fireloud.ogg', fire, 44100, 0.047, 96),
    ('sound/world/tim_comp_hum3.ogg', hum, 22050, 0.064, 50),
    ('sound/world/tim_hole.ogg', hole, 22050, 0.140, 50),
    ('sound/weapons/melee/fstrun.ogg', chainsaw_run, 22050, 0.167, 50),
    ('sound/weapons/melee/fstatck.ogg', chainsaw_hit, 22050, 0.246, 50),
]


def write(path, x, sr, kbps, keep_wav):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    pcm = (np.clip(x, -1, 1) * 32767).astype('<i2').tobytes()
    cmd = ['ffmpeg', '-v', 'error', '-y', '-f', 's16le', '-ar', str(sr), '-ac', '1', '-i', '-',
           '-c:a', 'libvorbis', '-b:a', f'{kbps}k', path]
    subprocess.run(cmd, input=pcm, check=True)
    if keep_wav:
        subprocess.run(['ffmpeg', '-v', 'error', '-y', '-f', 's16le', '-ar', str(sr), '-ac', '1', '-i', '-',
                        path[:-4] + '.wav'], input=pcm, check=True)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', default=os.path.join(ROOT, 'baseq3r'))
    ap.add_argument('--only', default='')
    ap.add_argument('--wav', action='store_true')
    a = ap.parse_args()
    for rel, fn, sr, rms, kbps in SOUNDS:
        if a.only and a.only not in rel:
            continue
        x = level(fn(sr=sr), rms)
        write(os.path.join(a.out, rel), x, sr, kbps, a.wav)
        print(f'{rel:36s} {len(x) / sr:5.2f}s  rms {np.sqrt((x * x).mean()):.3f}  peak {np.abs(x).max():.2f}')


if __name__ == '__main__':
    main()
