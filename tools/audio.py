"""Sound effects (8-bit PCM @16384Hz) and PSG music data for Embervale."""
import math
import numpy as np

SR = 16384
RNG = np.random.RandomState(7)


def tt(sec):
    return np.arange(int(sec * SR)) / SR


def env(n, a=0.005, d=None, curve=3.0, sustain=0.0):
    x = np.arange(n) / SR
    tot = n / SR
    d = tot - a if d is None else d
    e = np.ones(n)
    att = np.minimum(1.0, x / max(a, 1e-4))
    dec = np.exp(-curve * np.clip(x - a, 0, None) / max(d, 1e-3))
    return att * (sustain + (1 - sustain) * dec)


def phase(freq):
    return np.cumsum(freq) / SR


def sine(f):
    return np.sin(2 * np.pi * phase(f))


def square(f, duty=0.5):
    p = phase(f) % 1.0
    return np.where(p < duty, 1.0, -1.0)


def saw(f):
    p = phase(f) % 1.0
    return 2 * p - 1


def tri(f):
    p = phase(f) % 1.0
    return 4 * np.abs(p - 0.5) - 1


def noise(n):
    return RNG.uniform(-1, 1, n)


def lowpass(x, fc):
    """one-pole lowpass with (possibly time varying) cutoff"""
    fc = np.broadcast_to(np.asarray(fc, dtype=float), x.shape)
    a = 1 - np.exp(-2 * np.pi * fc / SR)
    y = np.zeros_like(x)
    s = 0.0
    for i in range(len(x)):
        s += a[i] * (x[i] - s)
        y[i] = s
    return y


def highpass(x, fc):
    return x - lowpass(x, fc)


def sweep(f0, f1, n, curve=1.0):
    u = (np.arange(n) / max(n - 1, 1)) ** curve
    return f0 + (f1 - f0) * u


def mix(*parts):
    n = max(len(p) for p in parts)
    out = np.zeros(n)
    for p in parts:
        out[:len(p)] += p
    return out


def cat(*parts):
    return np.concatenate(parts)


def note_f(m):
    return 440.0 * 2 ** ((m - 69) / 12.0)


def tone(kind, m, dur, vol=1.0, a=0.004, curve=4.0, sustain=0.0, vib=0.0, duty=0.5):
    n = int(dur * SR)
    f = np.full(n, note_f(m) if m < 200 else m)
    if vib:
        f = f * (1 + vib * np.sin(2 * np.pi * 5.5 * np.arange(n) / SR))
    w = {'sine': sine, 'tri': tri, 'saw': saw}.get(kind)
    sig = square(f, duty) if kind == 'sq' else w(f)
    return sig * env(n, a, None, curve, sustain) * vol


def finish(x, peak=0.7, tail=0.006):
    x = np.asarray(x, dtype=float)
    m = np.max(np.abs(x)) or 1.0
    x = x / m * peak
    n = int(tail * SR)
    x[-n:] *= np.linspace(1, 0, n)
    q = np.round(x * 127).astype(np.int16)
    return np.clip(q, -127, 127).astype(np.int8)


# ----------------------------------------------------------------------------- SFX
def sfx_menu_move():
    return finish(tone('sq', 1400, 0.03, curve=5, duty=0.25), 0.45)


def sfx_menu_ok():
    return finish(cat(tone('sq', 1000, 0.04, curve=3, duty=0.25), tone('sq', 1500, 0.07, curve=4, duty=0.25)), 0.5)


def sfx_menu_back():
    return finish(cat(tone('sq', 900, 0.04, curve=3, duty=0.25), tone('sq', 600, 0.07, curve=4, duty=0.25)), 0.5)


def sfx_error():
    n = int(0.16 * SR)
    return finish(mix(square(np.full(n, 150.0), 0.5), square(np.full(n, 112.0), 0.5)) * env(n, 0.003, None, 2.0), 0.5)


def sfx_text(f):
    return finish(tone('sq', f, 0.022, curve=2.5, duty=0.25), 0.35)


def whoosh(dur, f0, f1, vol=1.0, curve=1.0, a=0.02):
    n = int(dur * SR)
    x = noise(n)
    y = lowpass(x, sweep(f0, f1, n, curve))
    e = np.sin(np.linspace(0, np.pi, n)) ** 1.5
    return y * e * vol


def sfx_slash(hi=False):
    return finish(whoosh(0.16, 700, 4200 if not hi else 5200, 1.0, 1.2), 0.65)


def sfx_hit(power=1.0):
    n = int(0.14 * SR)
    th = sine(sweep(240, 70, n, 0.6)) * env(n, 0.002, None, 5)
    nz = lowpass(noise(n), 3500) * env(n, 0.001, None, 14)
    return finish(mix(th * 0.9, nz * 0.9), 0.75)


def sfx_crit():
    n = int(0.28 * SR)
    th = sine(sweep(300, 60, n, 0.6)) * env(n, 0.002, None, 5)
    nz = lowpass(noise(n), 5000) * env(n, 0.001, None, 10)
    ting = sine(np.full(n, 1960.0)) * env(n, 0.001, None, 7) * 0.5
    return finish(mix(th, nz, ting), 0.8)


def sfx_hurt():
    n = int(0.24 * SR)
    f = sweep(430, 150, n, 0.8) * (1 + 0.04 * np.sin(2 * np.pi * 28 * np.arange(n) / SR))
    s = square(f, 0.4) * env(n, 0.002, None, 3.0)
    nz = lowpass(noise(n), 2500) * env(n, 0.001, None, 9) * 0.6
    return finish(mix(s * 0.8, nz), 0.7)


def sfx_enemy_die():
    n = int(0.34 * SR)
    nz = lowpass(noise(n), sweep(4000, 250, n, 0.7)) * env(n, 0.002, None, 3.5)
    s = sine(sweep(500, 90, n, 0.5)) * env(n, 0.002, None, 4) * 0.7
    return finish(mix(nz, s), 0.75)


def sfx_roll():
    return finish(whoosh(0.2, 300, 1800, 1.0, 1.0), 0.45)


def sfx_arrow():
    n = int(0.11 * SR)
    s = sine(sweep(1000, 520, n, 0.5)) * env(n, 0.001, None, 6)
    c = highpass(noise(n), 2000) * env(n, 0.001, None, 20) * 0.5
    return finish(mix(s, c), 0.6)


def sfx_magic():
    n = int(0.22 * SR)
    f = sweep(500, 1600, n, 0.8)
    s = sine(f) * (0.6 + 0.4 * np.sin(2 * np.pi * 30 * np.arange(n) / SR)) * env(n, 0.003, None, 3)
    s2 = sine(f * 1.5) * env(n, 0.003, None, 4) * 0.4
    return finish(mix(s, s2), 0.6)


def sfx_fire():
    n = int(0.3 * SR)
    nz = lowpass(noise(n), sweep(400, 2400, n, 1.0)) * np.sin(np.linspace(0, np.pi, n)) ** 1.2
    s = sine(sweep(120, 480, n, 1.0)) * np.sin(np.linspace(0, np.pi, n)) * 0.4
    return finish(mix(nz, s), 0.7)


def sfx_explode():
    n = int(0.55 * SR)
    nz = lowpass(noise(n), sweep(3000, 150, n, 0.5)) * env(n, 0.002, None, 3.2)
    th = sine(sweep(110, 35, n, 0.5)) * env(n, 0.002, None, 3.5)
    return finish(mix(nz * 0.9, th * 1.0), 0.85)


def sfx_coin():
    return finish(cat(tone('sq', 988, 0.055, curve=2, duty=0.5), tone('sq', 1319, 0.22, curve=5, duty=0.5)), 0.5)


def sfx_heart():
    return finish(cat(tone('sine', 660, 0.07, curve=2), tone('sine', 880, 0.07, curve=2), tone('sine', 1108, 0.2, curve=5)), 0.6)


def sfx_mana():
    n = int(0.26 * SR)
    f = sweep(700, 1800, n, 0.9)
    s = sine(f) * (0.7 + 0.3 * np.sin(2 * np.pi * 24 * np.arange(n) / SR)) * env(n, 0.004, None, 3)
    return finish(s, 0.55)


def sfx_chest():
    seq = [72, 76, 79, 84]
    parts = [tone('sq', m, 0.08, curve=2, duty=0.25) for m in seq]
    tail = tone('tri', 88, 0.35, curve=4) * 0.8 + tone('sine', 91, 0.35, curve=4, vib=0.01) * 0.4
    return finish(cat(*parts, tail), 0.6)


def sfx_item():
    parts = [tone('sq', m, 0.07, curve=2, duty=0.25) for m in (79, 83, 86)]
    tail = tone('tri', 91, 0.3, curve=3)
    return finish(cat(*parts, tail), 0.6)


def sfx_levelup():
    seq = [72, 76, 79, 84, 88, 91]
    parts = [tone('sq', m, 0.085, curve=1.6, duty=0.25) * 0.8 + tone('tri', m - 12, 0.085, curve=1.6) * 0.6 for m in seq]
    n = int(0.75 * SR)
    chord = sum(tone('sq', m, 0.75, curve=2.4, vib=0.008, duty=0.25) * 0.5 + tone('tri', m - 12, 0.75, curve=2.4) * 0.5 for m in (79, 84, 88, 91))
    return finish(cat(*parts, chord), 0.7)


def sfx_potion():
    g = [tone('sine', f, 0.06, curve=2) for f in (210, 260, 320)]
    n = int(0.3 * SR)
    sp = sine(sweep(900, 1900, n, 0.7)) * (0.6 + 0.4 * np.sin(2 * np.pi * 20 * np.arange(n) / SR)) * env(n, 0.01, None, 3) * 0.5
    return finish(cat(*g, sp), 0.6)


def sfx_door():
    n = int(0.3 * SR)
    c = mix(sine(np.full(n, 310.0)), sine(np.full(n, 470.0)) * 0.7) * env(n, 0.001, None, 6)
    nz = lowpass(noise(n), 2500) * env(n, 0.001, None, 14)
    s = whoosh(0.3, 200, 900, 0.5)
    return finish(cat(mix(c, nz * 0.8), s), 0.7)


def sfx_warp():
    n = int(0.45 * SR)
    nz = lowpass(noise(n), sweep(300, 5000, n, 1.5)) * np.sin(np.linspace(0, np.pi, n)) ** 1.5
    s = sine(sweep(250, 1400, n, 1.2)) * np.sin(np.linspace(0, np.pi, n)) * 0.5
    return finish(mix(nz, s), 0.55)


def sfx_boss_roar():
    n = int(0.8 * SR)
    f = sweep(95, 60, n, 0.6) * (1 + 0.08 * np.sin(2 * np.pi * 9 * np.arange(n) / SR))
    s = lowpass(saw(f), 700) * env(n, 0.04, None, 1.6)
    nz = lowpass(noise(n), 900) * env(n, 0.05, None, 2.0) * 0.6
    return finish(mix(s, nz), 0.8)


def sfx_boss_die():
    parts = []
    for i in range(3):
        n = int(0.4 * SR)
        parts.append(lowpass(noise(n), sweep(3500, 150, n, 0.6)) * env(n, 0.002, None, 3) + sine(sweep(120, 35, n, 0.5)) * env(n, 0.002, None, 3))
    n = int(0.7 * SR)
    parts.append(lowpass(noise(n), sweep(2500, 100, n, 0.5)) * env(n, 0.01, None, 2.5))
    return finish(cat(*parts), 0.85)


def sfx_bush():
    n = int(0.13 * SR)
    return finish(highpass(noise(n), 1800) * env(n, 0.002, None, 9), 0.5)


def sfx_spin():
    n = int(0.32 * SR)
    nz = lowpass(noise(n), sweep(600, 3600, n, 0.6)) * (0.6 + 0.4 * np.sin(2 * np.pi * 14 * np.arange(n) / SR)) * np.sin(np.linspace(0, np.pi, n)) ** 0.8
    return finish(nz, 0.65)


def sfx_dash():
    n = int(0.22 * SR)
    nz = lowpass(noise(n), sweep(5000, 600, n, 0.6)) * env(n, 0.004, None, 3)
    s = sine(sweep(900, 200, n, 0.5)) * env(n, 0.004, None, 4) * 0.4
    return finish(mix(nz, s), 0.6)


def sfx_alert():
    return finish(cat(tone('sq', 880, 0.04, curve=2, duty=0.25), tone('sq', 1320, 0.09, curve=4, duty=0.25)), 0.45)


def melody(notes, vol=1.0, kind='sq', duty=0.25, tail_ring=0.0):
    parts = []
    for (m, d) in notes:
        if m is None:
            parts.append(np.zeros(int(d * SR)))
        else:
            parts.append(tone(kind, m, d, vol, curve=2.2, duty=duty) * 0.7 + tone('tri', m - 12, d, vol, curve=2.2) * 0.6)
    return cat(*parts)


def sfx_victory():
    q = 0.13
    notes = [(72, q), (72, q), (72, q), (72, q * 2.5), (68, q * 2.5), (70, q * 2.5), (72, q * 1.5), (70, q * .5), (72, q * 4)]
    notes += [(None, 0.05), (76, q), (79, q), (84, q * 4)]
    return finish(melody(notes), 0.7)


def sfx_gameover():
    q = 0.28
    notes = [(67, q), (66, q), (65, q), (64, q * 2), (None, q * 0.5), (62, q), (60, q * 4)]
    return finish(melody(notes, kind='tri', duty=0.5) * 0.9, 0.65)


def sfx_equip():
    n = int(0.05 * SR)
    c = highpass(noise(n), 1500) * env(n, 0.001, None, 14)
    return finish(cat(c, tone('sq', 700, 0.05, curve=3, duty=0.5)), 0.5)


def sfx_save():
    return finish(cat(tone('tri', 72, 0.1, curve=2), tone('tri', 76, 0.1, curve=2), tone('tri', 79, 0.3, curve=3)), 0.6)


def sfx_bat():
    n = int(0.05 * SR)
    return finish(highpass(noise(n), 800) * env(n, 0.002, None, 10), 0.3)


def sfx_wolf():
    n = int(0.3 * SR)
    f = sweep(150, 95, n, 0.7) * (1 + 0.05 * np.sin(2 * np.pi * 14 * np.arange(n) / SR))
    s = lowpass(saw(f), 900) * np.sin(np.linspace(0, np.pi, n)) ** 0.7
    return finish(s, 0.6)


def sfx_slime():
    n = int(0.1 * SR)
    f = sweep(180, 420, n, 0.8) * (1 + 0.1 * np.sin(2 * np.pi * 40 * np.arange(n) / SR))
    return finish(sine(f) * env(n, 0.003, None, 4), 0.55)


SFX = [
    ('MENU_MOVE', sfx_menu_move), ('MENU_OK', sfx_menu_ok), ('MENU_BACK', sfx_menu_back), ('ERROR', sfx_error),
    ('TEXT', lambda: sfx_text(520)), ('TEXT2', lambda: sfx_text(690)), ('TEXT3', lambda: sfx_text(400)),
    ('SLASH', lambda: sfx_slash(False)), ('SLASH2', lambda: sfx_slash(True)),
    ('HIT', sfx_hit), ('CRIT', sfx_crit), ('HURT', sfx_hurt), ('ENEMY_DIE', sfx_enemy_die), ('ROLL', sfx_roll),
    ('ARROW', sfx_arrow), ('MAGIC', sfx_magic), ('FIRE', sfx_fire), ('EXPLODE', sfx_explode),
    ('COIN', sfx_coin), ('HEART', sfx_heart), ('MANA', sfx_mana), ('CHEST', sfx_chest), ('ITEM', sfx_item),
    ('LEVELUP', sfx_levelup), ('POTION', sfx_potion), ('DOOR', sfx_door), ('WARP', sfx_warp),
    ('BOSS_ROAR', sfx_boss_roar), ('BOSS_DIE', sfx_boss_die), ('BUSH', sfx_bush), ('SPIN', sfx_spin),
    ('DASH', sfx_dash), ('ALERT', sfx_alert), ('VICTORY', sfx_victory), ('GAMEOVER', sfx_gameover),
    ('EQUIP', sfx_equip), ('SAVE', sfx_save), ('BAT', sfx_bat), ('WOLF', sfx_wolf), ('SLIME', sfx_slime),
]

# ----------------------------------------------------------------------------- MUSIC
NOTE = {'c': 0, 'd': 2, 'e': 4, 'f': 5, 'g': 7, 'a': 9, 'b': 11}


def midi(name):
    n = name[0]
    s = NOTE[n]
    i = 1
    while i < len(name) and name[i] in '#b':
        s += 1 if name[i] == '#' else -1
        i += 1
    o = int(name[i:])
    return 12 * (o + 1) + s


def parse(s):
    out = []
    for tok in s.split():
        if tok.startswith('@'):
            out.append(('inst', int(tok[1:])))
            continue
        nm, d = tok.split(':')
        out.append((0 if nm == 'r' else midi(nm), int(d)))
    return out


class Song:
    def __init__(self, name, spd, bars=16):
        self.name, self.spd, self.bars = name, spd, bars
        self.tr = [[], [], [], []]

    def add(self, ch, events, expect=None):
        self.tr[ch] += events if isinstance(events, list) else parse(events)

    def total(self, ch):
        return sum(e[1] for e in self.tr[ch] if e[0] != 'inst')

    def encode(self, loop=True):
        out = []
        for ch in range(4):
            b = [0xFD]
            for e in self.tr[ch]:
                if e[0] == 'inst':
                    b.append(0xE0 | e[1])
                else:
                    assert 0 <= e[0] < 128 and 1 <= e[1] <= 255, (self.name, ch, e)
                    b += [e[0], e[1]]
            b.append(0xFE if loop else 0xFF)
            out.append(b)
        return out


TRIAD = {'M': (0, 4, 7), 'm': (0, 3, 7), '7': (0, 4, 7), 'd': (0, 3, 6)}


def chord_notes(root, q, octave=4):
    r = midi(root + str(octave))
    return [r + i for i in TRIAD[q]]


def bass_bar(style, root, q, oct_=2):
    r = midi(root + str(oct_))
    f = r + 7
    o = r + 12
    if style == 'march':
        return [(r, 4), (f, 4), (r, 4), (f, 4)]
    if style == 'oompah':
        return [(r, 2), (0, 2), (f, 2), (0, 2)] * 2
    if style == 'drive':
        return [(r, 2), (r, 2), (o, 2), (r, 2), (r, 2), (r, 2), (o, 2), (f, 2)]
    if style == 'drone':
        return [(r, 8), (f, 8)]
    if style == 'slow':
        return [(r, 6), (0, 2), (f, 4), (0, 4)]
    raise ValueError(style)


def arp_bar(style, root, q, oct_=4):
    n = chord_notes(root, q, oct_)
    a, b, c = n
    if style == 'harp8':
        return [(x, 2) for x in (a, b, c, b, a, b, c, b)]
    if style == 'harp8up':
        return [(x, 2) for x in (a, b, c, a + 12, c, b, a, b)]
    if style == 'offbeat':
        return [(0, 2), (b, 2), (0, 2), (c, 2)] * 2
    if style == 'stab16':
        return [(x, 1) for x in (a, c, a + 12, c)] * 4
    if style == 'slowarp':
        return [(a, 4), (b, 4), (c, 4), (b, 4)]
    raise ValueError(style)


def perc_bar(pattern):
    """pattern: dict step -> drum id ; returns events covering 16 steps"""
    ev = []
    t = 0
    steps = sorted(pattern)
    for s in steps:
        if s > t:
            ev.append((0, s - t))
            t = s
        nxt = [x for x in steps if x > s]
        d = (nxt[0] if nxt else 16) - s
        ev.append((pattern[s], d))
        t = s + d
    if t < 16:
        ev.append((0, 16 - t))
    return ev


def make_song(name, spd, chords, melody_str, bass_style, arp_style, drums, insts, bars_expected=16):
    s = Song(name, spd)
    s.add(0, [('inst', insts[0])])
    s.add(1, [('inst', insts[1])])
    s.add(2, [('inst', insts[2])])
    s.add(3, [('inst', 0)])
    s.add(0, parse(melody_str))
    for (root, q) in chords:
        s.add(1, arp_bar(arp_style, root, q, 4))
        s.add(2, bass_bar(bass_style, root, q, 2))
    for i in range(len(chords)):
        s.add(3, perc_bar(drums[i % len(drums)]) if drums else [(0, 16)])
    for ch in range(4):
        tot = s.total(ch)
        assert tot == 16 * len(chords), (name, ch, tot, 16 * len(chords))
    return s


K, S, Hh, O, T, C = 1, 2, 3, 4, 5, 6

TITLE_CH = [('d', 'm'), ('b', 'M'), ('f', 'M'), ('c', 'M'), ('d', 'm'), ('b', 'M'), ('g', 'm'), ('a', 'M')] * 2
# Bb chord is 'bb' root; fix names
TITLE_CH = [('d', 'm'), ('bb', 'M'), ('f', 'M'), ('c', 'M'), ('d', 'm'), ('bb', 'M'), ('g', 'm'), ('a', 'M')] * 2
TITLE_MEL = """
d5:6 e5:2 f5:4 a5:4  g5:6 f5:2 d5:8  c5:6 d5:2 f5:4 a5:4  g5:12 e5:4
d5:6 e5:2 f5:4 a5:4  bb5:6 a5:2 g5:8  g5:4 a5:4 bb5:4 d6:4  a5:12 e5:4
f5:4 a5:4 d6:8  d6:6 c6:2 bb5:4 g5:4  a5:6 g5:2 f5:4 c5:4  e5:4 g5:4 c6:8
d6:4 c6:4 a5:4 f5:4  g5:4 bb5:4 d6:8  g5:8 a5:4 c#6:4  e6:8 d6:4 c#6:4
"""
TITLE_DR = [{0: K, 8: K}, {0: K, 8: K, 12: Hh}, {0: K, 4: Hh, 8: K, 12: Hh}, {0: K, 8: K, 12: S}]

VILLAGE_CH = [('g', 'M'), ('c', 'M'), ('d', 'M'), ('g', 'M'), ('g', 'M'), ('c', 'M'), ('d', 'M'), ('g', 'M'),
              ('e', 'm'), ('c', 'M'), ('g', 'M'), ('d', 'M'), ('e', 'm'), ('c', 'M'), ('d', 'M'), ('g', 'M')]
VILLAGE_MEL = """
b5:4 d6:2 b5:2 a5:4 g5:4  g5:4 e5:2 g5:2 c6:8  a5:4 f#5:2 a5:2 d6:4 c#6:2 d6:2  b5:8 g5:4 r:4
b5:4 d6:2 b5:2 a5:4 g5:4  e6:4 c6:2 e6:2 g6:8  f#6:4 d6:2 f#6:2 a6:4 f#6:4  g6:8 d6:4 b5:4
e6:4 g6:2 e6:2 b5:4 e6:4  c6:4 e6:2 c6:2 g5:4 c6:4  d6:4 b5:2 d6:2 g6:4 d6:4  a5:4 d6:2 a5:2 f#5:4 a5:4
g6:4 e6:2 g6:2 b6:4 g6:4  e6:4 c6:2 e6:2 g6:4 e6:4  f#6:4 e6:2 d6:2 c#6:4 a5:4  d6:6 c6:2 b5:4 d6:4
"""
VILLAGE_DR = [{0: K, 4: S, 8: K, 12: S, 2: Hh, 6: Hh, 10: Hh, 14: Hh}]

FOREST_CH = [('e', 'm'), ('c', 'M'), ('g', 'M'), ('d', 'M'), ('e', 'm'), ('c', 'M'), ('g', 'M'), ('d', 'M'),
             ('a', 'm'), ('e', 'm'), ('c', 'M'), ('b', 'M'), ('e', 'm'), ('c', 'M'), ('d', 'M'), ('e', 'm')]
FOREST_MEL = """
e5:8 g5:4 b5:4  c6:6 b5:2 g5:8  d5:4 g5:4 b5:8  a5:8 f#5:4 a5:4
b5:6 a5:2 g5:4 e5:4  g5:8 e5:4 g5:4  b5:6 a5:2 g5:8  f#5:8 a5:4 d6:4
c6:8 a5:4 e5:4  b5:8 g5:4 e5:4  e6:6 d6:2 c6:8  d#6:8 b5:4 f#5:4
e6:12 b5:4  g6:6 e6:2 c6:8  a5:4 d6:4 f#6:4 a6:4  g6:8 e6:4 r:4
"""
FOREST_DR = [{0: K}, {0: K, 12: Hh}, {0: K, 8: Hh}, {0: K, 4: Hh, 12: Hh}]

CRYPT_CH = [('c', 'm'), ('ab', 'M'), ('f', 'm'), ('g', 'M'), ('c', 'm'), ('ab', 'M'), ('f', 'm'), ('g', 'M'),
            ('c', 'm'), ('ab', 'M'), ('f', 'm'), ('g', 'M'), ('c', 'm'), ('ab', 'M'), ('f', 'm'), ('g', 'M')]
CRYPT_MEL = """
g5:6 f#5:2 g5:8  ab5:6 g5:2 eb5:8  f5:6 e5:2 f5:8  d5:4 f5:4 g5:4 b5:4
c6:8 g5:4 eb5:4  c6:6 bb5:2 ab5:8  ab5:6 g5:2 f5:4 c5:4  b5:8 d6:4 g5:4
eb6:8 d6:4 c6:4  c6:8 ab5:8  f6:6 eb6:2 c6:8  d6:4 b5:4 g5:8
g5:4 c6:4 eb6:4 g6:4  ab6:8 g6:4 eb6:4  f6:6 e6:2 f6:8  b5:8 g5:8
"""
CRYPT_DR = [{0: K}, {0: K, 8: T}, {0: K}, {0: K, 8: T, 14: Hh}]

BOSS_CH = [('d', 'm'), ('d', 'm'), ('bb', 'M'), ('c', 'M'), ('d', 'm'), ('d', 'm'), ('g', 'm'), ('a', 'M'),
           ('d', 'm'), ('d', 'm'), ('f', 'M'), ('c', 'M'), ('bb', 'M'), ('a', 'M'), ('g', 'm'), ('a', 'M')]
BOSS_MEL = """
d5:2 d5:2 f5:2 d5:2 a5:4 g5:2 f5:2  e5:2 e5:2 g5:2 e5:2 bb5:4 a5:2 g5:2  d6:4 c6:2 bb5:2 a5:4 bb5:2 c6:2  c6:4 b5:2 c6:2 e6:4 g6:4
d6:2 d6:2 f6:2 d6:2 a6:4 g6:2 f6:2  e6:2 f6:2 g6:2 e6:2 d6:8  bb5:2 d6:2 g6:4 f6:2 d6:2 bb5:4  a5:2 c#6:2 e6:4 a6:4 g6:2 e6:2
a5:4 d6:4 f6:4 a6:4  g6:2 f6:2 e6:2 d6:2 c6:4 a5:4  a5:2 c6:2 f6:4 e6:2 c6:2 a5:4  g5:2 c6:2 e6:4 d6:2 c6:2 g5:4
d6:2 f6:2 bb6:4 a6:2 f6:2 d6:4  e6:2 a6:2 g6:4 e6:2 c#6:2 e6:4  g6:4 f6:2 d6:2 bb5:4 d6:4  a5:2 c#6:2 e6:2 a6:2 e6:2 c#6:2 a5:4
"""
BOSS_DR = [{0: K, 4: S, 8: K, 10: K, 12: S, 2: Hh, 6: Hh, 14: Hh}, {0: K, 4: S, 8: K, 12: S, 14: S, 2: Hh, 6: Hh, 10: Hh}]


def build_songs():
    songs = []
    songs.append(make_song('title', 9, TITLE_CH, TITLE_MEL, 'march', 'slowarp', TITLE_DR, (4, 3, 0)))
    songs.append(make_song('village', 7, VILLAGE_CH, VILLAGE_MEL, 'oompah', 'offbeat', VILLAGE_DR, (0, 1, 0)))
    songs.append(make_song('forest', 10, FOREST_CH, FOREST_MEL, 'slow', 'harp8', FOREST_DR, (2, 3, 1)))
    songs.append(make_song('crypt', 11, CRYPT_CH, CRYPT_MEL, 'drone', 'slowarp', CRYPT_DR, (2, 5, 0)))
    songs.append(make_song('boss', 5, BOSS_CH, BOSS_MEL, 'drive', 'stab16', BOSS_DR, (4, 1, 0)))
    return songs


def sq_freq_table():
    out = []
    for m in range(128):
        f = note_f(m)
        v = 2048 - round(131072.0 / f) if f >= 64 else 0
        out.append(max(0, min(2047, v)))
    return out


def wv_freq_table():
    out = []
    for m in range(128):
        f = note_f(m)
        v = 2048 - round(65536.0 / f) if f >= 33 else 0
        out.append(max(0, min(2047, v)))
    return out


def wave_ram():
    """two 32-sample (4-bit) waveforms packed into 16 bytes each: bank0 triangle-ish bass, bank1 soft sine."""
    def pack(samples):
        b = []
        for i in range(0, 32, 2):
            b.append((samples[i] << 4) | samples[i + 1])
        return b
    tri_w = []
    for i in range(32):
        v = i / 31.0
        t = 1 - abs(2 * v - 1) * 2 if False else (abs(((i + 8) % 32) - 16) / 16.0)
        tri_w.append(int(round(t * 15)))
    sin_w = [int(round((math.sin(2 * math.pi * i / 32) * 0.5 + 0.5) * 15)) for i in range(32)]
    # add mild harmonic to the sine for a flute-ish tone
    flu = [int(round(max(0, min(15, (math.sin(2 * math.pi * i / 32) * 0.42 + math.sin(4 * math.pi * i / 32) * 0.12 + 0.5) * 15)))) for i in range(32)]
    return pack(tri_w), pack(flu)


def emit_audio(c_path, h_path):
    from common import u8arr, u16arr
    songs = build_songs()
    hdr = ['#ifndef AUDIO_DATA_H', '#define AUDIO_DATA_H', '#include "hw.h"', '']
    hdr.append('typedef struct { const s8 *data; u16 len; u8 prio; } SfxDef;')
    hdr.append('typedef struct { u8 spd; const u8 *tr[4]; } SongDef;')
    for i, (nm, _) in enumerate(SFX):
        hdr.append('#define SFX_%s %d' % (nm, i))
    hdr.append('#define SFX_COUNT %d' % len(SFX))
    for i, s in enumerate(songs):
        hdr.append('#define SONG_%s %d' % (s.name.upper(), i))
    hdr.append('#define SONG_GAMEOVER_NONE 255')
    hdr += ['#define SONG_COUNT %d' % len(songs), '',
            'extern const SfxDef sfx_table[SFX_COUNT];', 'extern const SongDef song_table[SONG_COUNT];',
            'extern const u16 sq_freq[128];', 'extern const u16 wv_freq[128];', 'extern const u8 wave_ram0[16];', 'extern const u8 wave_ram1[16];', '#endif', '']
    src = ['#include "audio_data.h"', '']
    total = 0
    names = []
    for i, (nm, fn) in enumerate(SFX):
        d = fn()
        total += len(d)
        vals = [int(v) for v in d] + [0] * 48
        names.append((nm, len(d)))
        src.append('static const s8 sfx_%d[%d] __attribute__((aligned(4))) = {' % (i, len(vals)))
        for j in range(0, len(vals), 32):
            src.append('  ' + ','.join(str(v) for v in vals[j:j + 32]) + ',')
        src.append('};')
    prios = {'LEVELUP': 9, 'VICTORY': 9, 'GAMEOVER': 9, 'BOSS_DIE': 9, 'BOSS_ROAR': 8, 'EXPLODE': 6, 'CHEST': 7, 'ITEM': 7, 'HURT': 6, 'HIT': 3, 'CRIT': 4,
             'TEXT': 1, 'TEXT2': 1, 'TEXT3': 1, 'MENU_MOVE': 2, 'MENU_OK': 3, 'MENU_BACK': 3, 'SLASH': 2, 'SLASH2': 2, 'COIN': 3}
    src.append('const SfxDef sfx_table[SFX_COUNT] = {')
    for i, (nm, ln) in enumerate(names):
        src.append('  { sfx_%d, %d, %d },' % (i, ln, prios.get(nm, 4)))
    src.append('};')
    songarr = []
    for si, s in enumerate(songs):
        loop = True
        enc = s.encode(loop)
        for ch in range(4):
            src.append(u8arr('song_%d_%d' % (si, ch), enc[ch], 24).replace('const u8', 'static const u8'))
        songarr.append('  { %d, { song_%d_0, song_%d_1, song_%d_2, song_%d_3 } },' % (s.spd, si, si, si, si))
    src.append('const SongDef song_table[SONG_COUNT] = {')
    src += songarr
    src.append('};')
    src.append(u16arr('sq_freq', sq_freq_table()))
    src.append(u16arr('wv_freq', wv_freq_table()))
    w0, w1 = wave_ram()
    src.append(u8arr('wave_ram0', w0))
    src.append(u8arr('wave_ram1', w1))
    open(c_path, 'w').write('\n'.join(src) + '\n')
    open(h_path, 'w').write('\n'.join(hdr) + '\n')
    return total


if __name__ == '__main__':
    import wave
    tot = emit_audio('/tmp/audio_data.c', '/tmp/audio_data.h')
    print('sfx bytes', tot)
    for s in build_songs():
        print(s.name, [len(x) for x in s.encode()])
