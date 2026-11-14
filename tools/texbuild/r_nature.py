"""Rock, dirt and cloud layers."""
from common import *


def ridged(w, h, seed, fmin, fmax=None, beta=1.6):
    return 1 - np.abs(noise(w, h, seed, fmin, fmax, beta) * 2 - 1)


@recipe
def ground():
    out = {}
    w = h = 256
    base = noise(w, h, 800, 2, 40, 1.6)
    col = gradient_map(base, [(0, '#2a1a10'), (0.5, '#4a3020'), (1, '#6a4a30')])
    f1, f2, _ = worley(w, h, 60, 801)
    f1b, f2b, _ = worley(w, h, 220, 802)
    crack = np.maximum(smooth(1.4, 0, f2 - f1), smooth(0.8, 0, f2b - f1b) * 0.6)
    hm = noise(w, h, 803, 10, None, 1.2) * 0.3 - crack * 0.5
    col = light(col, 1 - crack * 0.6)
    out['textures/organics/dirt2.jpg'] = (finish(col, hm, 4, 0.4, 2, 800), None)

    w = h = 512
    r = ridged(w, h, 804, 2, 50, 2.2)
    strata = noise(w, h, 805, 2, 50, 2.0, aniso=(1, 4))
    f1, f2, _ = worley(w, h, 30, 815)
    plates = smooth(0, 10, f2 - f1)
    hm = r * 0.6 + strata * 0.2 + plates * 0.2
    col = gradient_map(clamp(norm01(hm) * 0.8 + noise(w, h, 806, 20) * 0.2), [(0, '#3a3834'), (0.4, '#6e6a62'), (0.75, '#9a968c'), (1, '#c4c0b4')])
    moss = cavity(hm, 4) * smooth(0.4, 0.7, noise(w, h, 807, 3, 30))
    col = lerp(col, gradient_map(noise(w, h, 808, 20), [(0, "#2e3a18"), (1, "#5a6a28")]), clamp(moss * 14))
    out['textures/organics/pjrock7d.jpg'] = (finish(col, hm, 25, 0.8, 4, 804, amb=0.3), None)

    w = h = 256
    r = ridged(w, h, 809, 2, 40, 2.1) * 0.6 + noise(w, h, 817, 3, 60, 1.9, aniso=(1, 3)) * 0.4
    f1, f2, _ = worley(w, h, 24, 819)
    st = clamp(r * 0.8 + smooth(0, 6, f2 - f1) * 0.2)
    col = gradient_map(clamp(st * 0.8 + noise(w, h, 820, 20) * 0.2), [(0, '#1a100a'), (0.5, '#4a3020'), (0.8, '#6a4a34'), (1, '#8a6a50')])
    out['textures/stone/pjrock1.jpg'] = (finish(col, st, 22, 0.7, 3, 809, amb=0.3), None)

    w = h = 512
    r = ridged(w, h, 812, 2, 40, 2.0) * 0.6 + ridged(w, h, 813, 6, 80, 1.8) * 0.4
    col = gradient_map(clamp(r * 0.7 + noise(w, h, 814, 2, 10) * 0.3), [(0, '#1e0e0c'), (0.5, '#4a2420'), (1, '#7a4a44')])
    out['textures/stone/rockwall.jpg'] = (finish(col, r, 25, 0.7, 3, 812, amb=0.3), None)
    return out


def clouds(w, h, seed, stops, contrast=1.0):
    n = noise(w, h, seed, 1, None, 3.0) * 0.75 + noise(w, h, seed + 1, 3, 40, 2.6) * 0.25
    n = clamp((n - 0.5) * contrast + 0.5)
    return gradient_map(blur(n, 1.5), stops)


@recipe
def skies():
    return {
        'textures/skies/dimclouds.jpg': (clouds(512, 512, 820, [(0, '#0a0806'), (0.45, '#3a3028'), (0.75, '#8a8070'), (1, '#d0c8b8')], 1.4), None),
        'textures/skies/killsky_1.jpg': (clouds(512, 512, 821, [(0, '#000000'), (0.4, '#200000'), (0.7, '#900808'), (1, '#e02010')], 1.6), None),
        'textures/skies/killsky_2.jpg': (clouds(512, 512, 822, [(0, '#000000'), (0.5, '#100000'), (0.75, '#800606'), (1, '#c81810')], 1.8), None),
    }
