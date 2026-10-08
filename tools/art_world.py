"""Outdoor + dungeon metatile art. Metatiles are 16x16 drawn with bank-relative palette indices."""
import math
import random
import numpy as np
from common import Canvas, pal16

SOLID, WATERF, CUT, CHEST, SIGN, DOOR = 1, 2, 4, 8, 16, 32

# ----------------------------------------------------------------- palettes (bank -> pal16)
G1, G2, G3 = '#58b848', '#78d058', '#409838'   # grass base/light/dark shared as idx1..3 in many banks
OUT_BANKS = {
    0: pal16([G1, G2, G3, '#2e7a30', '#f8f8f0', '#f8e050', '#f090b8', '#80a8f8', '#e04040', '#f0e8d0', '#b88850', '#a0a0a8', '#3a8a34', '#68c850']),
    1: pal16([G1, G2, G3, '#d8a868', '#ecc888', '#b08048', '#8c6038', '#c8b8a0', '#887868', '#c8c8d4', '#a0a0b4', '#6c6c84', '#8a6a40', '#e8d8b8']),
    2: pal16([G1, G2, G3, '#2a68d0', '#3c88e8', '#68b4f8', '#d8f4ff', '#ecd8a0', '#c0a870', '#8c8c98', '#48b058', '#2c7c38', '#f8a8c8', '#78a050']),
    3: pal16(['#f4e4c0', '#d8c498', '#b8a078', '#9c6a38', '#c08850', '#6c4220', '#90d0f8', '#3868a8', '#7a4a28', '#4a2a14', '#a8a8b8', '#78788c', '#f0d050', '#d04848', '#fff8e8']),
    4: pal16(['#d84838', '#f07858', '#a02828', '#701c20', '#9c6a38', '#6c4220', '#f8f8f0', '#d8d8e0', '#4880d8', '#2c58a0', '#f8d060', '#c08850', '#3a2418', '#e8a090', '#a8a8b8']),
    5: pal16(['#4878c8', '#78a8f0', '#2c58a0', '#1c3c70', '#9c6a38', '#6c4220', '#f8f8f0', '#d8d8e0', '#d84838', '#a02828', '#f8d060', '#c08850', '#3a2418', '#a8c8f8', '#a8a8b8']),
    6: pal16([G1, G2, G3, '#3a9a3c', '#62c050', '#2a7430', '#1c5428', '#8a5a30', '#b07840', '#5c3818', '#98e070', '#f090b8', '#e04050', '#f8d8e0']),
    7: pal16([G1, G2, G3, '#c8c8d4', '#a0a0ac', '#70707c', '#c08850', '#9c6a38', '#6c4220', '#f0c840', '#a87818', '#f0e0b0', '#d04040', '#3a2a20', '#ffffff']),
    8: pal16([G1, G2, G3, '#1c4a30', '#26623c', '#368048', '#52a05c', '#8a5a30', '#b07840', '#5c3818', '#78c070', '#143824', '#e8f0e0', '#a0c898']),
    9: pal16(['#c8c8d4', '#a0a0ac', '#70707c', '#3c88e8', '#68b4f8', '#d8f4ff', '#d84838', '#f8f8f0', '#d8d8e0', '#9c6a38', '#6c4220', '#f0c840', '#2a68d0', '#e8e8f0', '#58b848']),
}
DUN_BANKS = {
    0: pal16(['#505070', '#646488', '#3c3c58', '#26263c', '#3c6050', '#e0dcc8', '#702030', '#a060e0', '#d8a8ff', '#2e2e48', '#7a7aa0', '#8a4a9a', '#484868', '#181828']),
    1: pal16(['#6a5a7a', '#8a7a9c', '#4a3c5a', '#2e2438', '#241c30', '#322840', '#443858', '#406048', '#5a8060', '#a08040', '#181020', '#7a6a8a', '#3a2c4a', '#9a8aac']),
    2: pal16(['#6a5a7a', '#8a7a9c', '#4a3c5a', '#2e2438', '#a08040', '#e86020', '#f8a020', '#fff080', '#d03018', '#181020', '#5a4a6a', '#704820', '#f8f0c0', '#3a2c4a']),
    3: pal16(['#7a4a28', '#9c6a38', '#4a2a14', '#2e2438', '#a0a0b4', '#6c6c84', '#f0c840', '#a87818', '#181020', '#c08850', '#3a2418', '#505070', '#3c3c58', '#d8a8ff']),
    4: pal16(['#505070', '#646488', '#3c3c58', '#26263c', '#a0a0b4', '#c8c8d8', '#6c6c84', '#484868', '#e0dcc8', '#f0c840', '#a87818', '#8a5028', '#5a3018', '#181828', '#a060e0']),
    5: pal16(['#505070', '#646488', '#3c3c58', '#26263c', '#a02838', '#d04050', '#701828', '#f0c840', '#a87818', '#e86020', '#f8c030', '#fff080', '#181828', '#ffd0a0', '#483868']),
}


def H(x, y, s=0):
    return (x * 73856093 ^ y * 19349663 ^ s * 83492791) & 0xFFFF


class Tileset:
    def __init__(self, name, banks):
        self.name = name
        self.banks = banks
        self.tiles = [[0] * 8]       # tile 0 is blank
        self.tmap = {tuple([0] * 8): 0}
        self.meta = []
        self.flags = []
        self.names = {}
        self.cycles = []

    def _tile(self, a):
        words = []
        for y in range(8):
            v = 0
            for x in range(8):
                v |= int(a[y, x]) << (4 * x)
            words.append(v)
        k = tuple(words)
        if k not in self.tmap:
            self.tmap[k] = len(self.tiles)
            self.tiles.append(words)
        return self.tmap[k]

    def add(self, name, canv, bank, flags=0):
        a = canv.a
        ents = []
        for ty in range(2):
            for tx in range(2):
                t = self._tile(a[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8])
                ents.append(t | (bank << 12))
        # fully blank metatile -> tile 0 entries without bank
        if not a.any():
            ents = [0, 0, 0, 0]
        key = (tuple(ents), flags)
        mid = len(self.meta)
        self.meta.append(ents)
        self.flags.append(flags)
        self.names[name] = mid
        return mid

    def id(self, name):
        return self.names[name]

    def palette(self):
        out = []
        for b in range(12):
            p = self.banks.get(b)
            if p is None:
                out += ['#000000'] * 16
            else:
                out += p
        return out


# ----------------------------------------------------------------- generic drawing helpers
def fill(c, idx):
    c.a[:, :] = idx


def speckle(c, rng, colors, n, rect=(0, 0, 15, 15)):
    for _ in range(n):
        x = rng.randint(rect[0], rect[2])
        y = rng.randint(rect[1], rect[3])
        c.set(x, y, rng.choice(colors))


def grass_base(seed, blades=7):
    c = Canvas(16, 16)
    fill(c, 1)
    rng = random.Random(seed)
    for _ in range(blades):
        x, y = rng.randint(0, 15), rng.randint(0, 14)
        c.set(x, y, 2)
        c.set(x, y + 1, 3)
    for _ in range(3):
        x, y = rng.randint(0, 15), rng.randint(0, 15)
        c.set(x, y, 3)
    for _ in range(2):
        x, y = rng.randint(0, 15), rng.randint(0, 15)
        c.set(x, y, 2)
    return c


def flower(c, x, y, col, ctr=6):
    c.set(x, y, col)
    c.set(x - 1, y, col)
    c.set(x + 1, y, col)
    c.set(x, y - 1, col)
    c.set(x, y + 1, col)
    c.set(x, y, ctr)


def rrect_dist(x, y, m):
    """signed distance (px, inside positive) into a rounded rect; margin 0 = open (connected) side."""
    mn, me, ms, mw, rne, rse, rsw, rnw = m
    px, py = x + 0.5, y + 0.5
    d = 99.0
    if mn:
        d = min(d, py - mn)
    if ms:
        d = min(d, 16 - ms - py)
    if mw:
        d = min(d, px - mw)
    if me:
        d = min(d, 16 - me - px)
    if rnw and mn and mw:
        cx, cy = mw + rnw, mn + rnw
        if px < cx and py < cy:
            d = min(d, rnw - math.hypot(px - cx, py - cy))
    if rne and mn and me:
        cx, cy = 16 - me - rne, mn + rne
        if px > cx and py < cy:
            d = min(d, rne - math.hypot(px - cx, py - cy))
    if rse and ms and me:
        cx, cy = 16 - me - rse, 16 - ms - rse
        if px > cx and py > cy:
            d = min(d, rse - math.hypot(px - cx, py - cy))
    if rsw and ms and mw:
        cx, cy = mw + rsw, 16 - ms - rsw
        if px < cx and py > cy:
            d = min(d, rsw - math.hypot(px - cx, py - cy))
    return d


def path_tile(n, e, s, w, ne, se, sw, nw, style='dirt'):
    """n,e,s,w,.. = True if that neighbour is also path (connected)."""
    c = Canvas(16, 16)
    mg = 3
    m = (0 if n else mg, 0 if e else mg, 0 if s else mg, 0 if w else mg,
         6 if (not n and not e) else 0, 6 if (not s and not e) else 0, 6 if (not s and not w) else 0, 6 if (not n and not w) else 0)
    rng = random.Random(5)
    base, lt, dk = (4, 5, 6) if style == 'dirt' else (11, 10, 12)
    for y in range(16):
        for x in range(16):
            d = rrect_dist(x, y, m)
            # inner corner notches
            for (flag, cx, cy, adj) in ((nw, 0, 0, n and w), (ne, 16, 0, n and e), (se, 16, 16, s and e), (sw, 0, 16, s and w)):
                if adj and not flag:
                    dd = math.hypot(x + 0.5 - cx, y + 0.5 - cy) - 3.5
                    d = min(d, dd)
            if d < 0:
                col = 2 if (H(x, y, 1) % 9 == 0) else 1
                if H(x, y, 3) % 23 == 0:
                    col = 3
            elif d < 1.0:
                col = 3
            elif d < 2.0:
                col = 7 if style == 'dirt' else 12
            else:
                col = base
                h = H(x, y, 7) % 41
                if style == 'dirt':
                    if h < 3:
                        col = lt
                    elif h < 5:
                        col = dk
                    elif h == 6:
                        col = 8
                else:
                    # paving: grid
                    if x % 8 == 0 or y % 8 == 0:
                        col = dk
                    elif (x % 8 == 1 or y % 8 == 1):
                        col = lt
            c.set(x, y, col)
    return c


def water_tile(n, e, s, w, ne, se, sw, nw):
    """n..nw = True if neighbour is water."""
    c = Canvas(16, 16)
    mg = 3
    m = (0 if n else mg, 0 if e else mg, 0 if s else mg, 0 if w else mg,
         6 if (not n and not e) else 0, 6 if (not s and not e) else 0, 6 if (not s and not w) else 0, 6 if (not n and not w) else 0)
    for y in range(16):
        for x in range(16):
            d = rrect_dist(x, y, m)
            for (flag, cx, cy, adj) in ((nw, 0, 0, n and w), (ne, 16, 0, n and e), (se, 16, 16, s and e), (sw, 0, 16, s and w)):
                if adj and not flag:
                    dd = math.hypot(x + 0.5 - cx, y + 0.5 - cy) - 3.5
                    d = min(d, dd)
            if d < 0:
                col = 2 if (H(x, y, 1) % 9 == 0) else 1
                if H(x, y, 3) % 23 == 0:
                    col = 3
            elif d < 1.0:
                col = 3
            elif d < 2.0:
                col = 8
            elif d < 3.0:
                col = 7 if (x + y) % 3 else 6 - 0
            else:
                v = (x + 2 * y) % 12
                col = 5
                if v in (0, 1):
                    col = 6
                elif v in (6,):
                    col = 4
                if H(x, y, 9) % 29 == 0:
                    col = 7
            c.set(x, y, col)
    return c


# ----------------------------------------------------------------- trees
def oak_tree(seed=1):
    """returns (canopy_top, trunk_cell) canvases (bank 6)."""
    big = Canvas(16, 32)
    # trunk
    for y in range(18, 31):
        for x in range(6, 10):
            big.set(x, y, 8 if x < 8 else 10)
    big.set(5, 30, 10)
    big.set(10, 30, 10)
    big.set(4, 31, 10)
    big.set(11, 31, 10)
    big.rect(5, 31, 10, 31, 10)
    big.set(7, 22, 9)
    big.set(7, 26, 9)
    # canopy
    big.ellipse(7.5, 11.5, 7.6, 10.2, 5)
    big.ellipse(7.5, 10.5, 6.2, 8.4, 4)
    rng = random.Random(seed)
    for y in range(32):
        for x in range(16):
            v = big.get(x, y)
            if v in (4, 5):
                if (x - 7.5) + (y - 11) > 6:
                    big.set(x, y, 5 if v == 4 else 6)
                if (x - 7.5) + (y - 11) > 9:
                    big.set(x, y, 6)
    for (x, y) in ((4, 5), (6, 3), (9, 4), (5, 9), (10, 8), (7, 7)):
        big.rect(x, y, x + 1, y, 10)
        big.set(x - 1, y + 1, 4)
    big.ellipse(7.5, 21.5, 6.5, 3.2, 5)
    for x in range(1, 15):
        big.set(x, 22 + (1 if x % 3 == 0 else 0), 6)
    big.outline(7, diag=False) if False else None
    # outline: manual dark leaf edge
    out = Canvas(16, 32)
    out.a[:, :] = big.a
    for y in range(32):
        for x in range(16):
            if big.get(x, y) == 0:
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    if big.get(x + dx, y + dy) in (4, 5, 6):
                        out.set(x, y, 7)
                    elif big.get(x + dx, y + dy) in (8, 9, 10):
                        out.set(x, y, 10)
    top = Canvas(16, 16)
    top.a[:, :] = out.a[0:16, :]
    bot = grass_base(11, 5)
    for y in range(16):
        for x in range(16):
            v = out.get(x, 16 + y)
            if v:
                bot.set(x, y, int(v))
    return top, bot


def pine_tree():
    big = Canvas(16, 32)
    big.rect(7, 25, 8, 30, 8)
    big.set(8, 27, 10)
    big.set(8, 28, 10)
    big.rect(5, 31, 10, 31, 10)
    tiers = [(1, 12, 3), (8, 18, 5.5), (14, 24, 7.5)]
    # draw 3 stacked triangles from tip y=1
    for (y0, y1, hw) in tiers:
        for y in range(y0, y1 + 1):
            t = (y - y0) / max(1, (y1 - y0))
            half = 1 + hw * t
            for x in range(int(8 - half), int(8 + half) + 1):
                col = 5
                if x < 8 - half * 0.35:
                    col = 6 if (x + y) % 4 else 7
                elif x > 8 + half * 0.2:
                    col = 4
                if y == y1 or y == y1 - 1:
                    col = 4 if x > 8 else 5
                big.set(x, y, col)
    out = Canvas(16, 32)
    out.a[:, :] = big.a
    for y in range(32):
        for x in range(16):
            if big.get(x, y) == 0:
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    if big.get(x + dx, y + dy) in (4, 5, 6, 7):
                        out.set(x, y, 11)
                    elif big.get(x + dx, y + dy) in (8, 9, 10):
                        out.set(x, y, 11)
    top = Canvas(16, 16)
    top.a[:, :] = out.a[0:16, :]
    bot = grass_base(12, 4)
    for y in range(16):
        for x in range(16):
            v = out.get(x, 16 + y)
            if v:
                bot.set(x, y, int(v))
    return top, bot


def bush():
    c = grass_base(21, 3)
    c.ellipse(7.5, 9.0, 7.2, 5.6, 5)
    c.ellipse(7.5, 8.0, 6.0, 4.4, 4)
    for y in range(16):
        for x in range(16):
            v = c.get(x, y)
            if v == 4 and (x - 7.5) * 0.6 + (y - 8) > 2.4:
                c.set(x, y, 5)
            if v == 4 and (x + y) % 5 == 0 and y < 8:
                c.set(x, y, 10 - 0)
    for p in ((4, 7), (10, 6), (7, 10)):
        c.set(p[0], p[1], 12)
    c.set(8, 5, 11)
    # outline
    src = c.a.copy()
    for y in range(16):
        for x in range(16):
            if src[y, x] in (1, 2, 3):
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    xx, yy = x + dx, y + dy
                    if 0 <= xx < 16 and 0 <= yy < 16 and src[yy, xx] in (4, 5, 10, 11, 12):
                        c.set(x, y, 6)
                        break
    return c


def rock():
    c = grass_base(31, 3)
    c.ellipse(7.5, 9.5, 6.6, 5.2, 4)
    c.ellipse(7.0, 8.5, 5.4, 3.8, 3)
    c.ellipse(6.0, 7.5, 2.6, 1.6, 4)
    for y in range(16):
        for x in range(16):
            v = c.get(x, y)
            if v == 4 and (x - 7.5) + (y - 9) > 3.5:
                c.set(x, y, 5)
    c.set(9, 9, 5)
    c.set(10, 10, 5)
    c.set(5, 11, 5)
    src = c.a.copy()
    for y in range(16):
        for x in range(16):
            if src[y, x] in (1, 2, 3):
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    xx, yy = x + dx, y + dy
                    if 0 <= xx < 16 and 0 <= yy < 16 and src[yy, xx] in (3, 4, 5, 6):
                        pass
    c.outline(6) if False else None
    # dark outline using idx 14 (ink)
    for y in range(16):
        for x in range(16):
            if src[y, x] in (1, 2, 3):
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    xx, yy = x + dx, y + dy
                    if 0 <= xx < 16 and 0 <= yy < 16 and src[yy, xx] in (4, 5, 6):
                        c.set(x, y, 14)
                        break
    return c


def stump():
    c = grass_base(41, 3)
    c.ellipse(7.5, 10, 5.6, 4.2, 9)
    c.ellipse(7.5, 8.6, 5.2, 3.6, 8)
    c.ellipse(7.5, 8.6, 3.6, 2.2, 9)
    c.ellipse(7.5, 8.6, 1.8, 1.0, 8)
    c.set(2, 11, 9)
    c.set(13, 11, 9)
    src = c.a.copy()
    for y in range(16):
        for x in range(16):
            if src[y, x] in (1, 2, 3):
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    xx, yy = x + dx, y + dy
                    if 0 <= xx < 16 and 0 <= yy < 16 and src[yy, xx] in (8, 9):
                        c.set(x, y, 14)
                        break
    return c


def fence(kind):
    c = grass_base(51, 3)
    if kind == 'h':
        c.rect(0, 6, 15, 7, 8)
        c.rect(0, 10, 15, 11, 8)
        c.rect(0, 6, 15, 6, 7)
        c.rect(0, 10, 15, 10, 7)
        c.rect(0, 8, 15, 8, 9) if False else None
        for x in (1, 13):
            c.rect(x, 4, x + 1, 13, 7)
            c.rect(x + 1, 4, x + 1, 13, 8)
            c.rect(x, 3, x + 1, 3, 7)
        c.rect(0, 14, 15, 14, 3)
    else:
        c.rect(6, 0, 9, 15, 7)
        c.rect(8, 0, 9, 15, 8)
        for y in (2, 9):
            c.rect(4, y, 11, y + 1, 7)
            c.rect(4, y + 1, 11, y + 1, 8)
    return c


def sign():
    c = grass_base(61, 3)
    c.rect(7, 8, 8, 14, 8)
    c.rect(2, 2, 13, 8, 12)
    c.rect(2, 2, 13, 2, 7) if False else None
    c.rect(2, 7, 13, 8, 7)
    c.rect(2, 2, 2, 8, 7)
    c.rect(13, 2, 13, 8, 7)
    for x in (4, 6, 8, 10):
        c.rect(x, 4, x + 1, 4, 14)
    for x in (4, 6, 9):
        c.rect(x, 6, x + 1, 6, 14)
    c.outline(14) if False else None
    c.rect(3, 9, 12, 9, 14) if False else None
    return c


def chest(open_):
    c = grass_base(71, 3)
    c.rect(2, 6, 13, 13, 8)
    c.rect(2, 6, 13, 7, 7)
    c.rect(2, 13, 13, 13, 9)
    c.rect(2, 6, 2, 13, 9)
    c.rect(13, 6, 13, 13, 9)
    c.rect(7, 6, 8, 13, 10)
    c.rect(7, 9, 8, 10, 10)
    c.set(7, 9, 9 + 1)
    if not open_:
        c.rect(3, 5, 12, 6, 8)
        c.rect(2, 4, 13, 5, 7)
        c.rect(7, 4, 8, 6, 10)
        c.set(7, 8, 14)
        c.set(8, 8, 14)
    else:
        c.rect(3, 6, 12, 7, 9)
        c.rect(3, 7, 12, 9, 14)
        c.rect(2, 1, 13, 3, 7)
        c.rect(2, 3, 13, 3, 8)
        c.rect(7, 1, 8, 3, 10)
        for p in ((5, 8), (8, 8), (10, 8)):
            c.set(p[0], p[1], 10)
    return c


def flowerbed(seed):
    c = grass_base(seed, 3)
    rng = random.Random(seed)
    cols = [5, 6, 7, 8]
    for _ in range(4):
        x, y = rng.randint(2, 13), rng.randint(2, 13)
        flower(c, x, y, rng.choice(cols), 6 if rng.random() < .5 else 5)
    return c


def tuft(seed):
    c = grass_base(seed, 4)
    for (x, y) in ((4, 9), (9, 5), (11, 11)):
        c.rect(x, y, x, y + 3, 4)
        c.set(x - 1, y + 1, 4)
        c.set(x + 1, y + 1, 4)
        c.set(x, y - 1, 2)
        c.set(x - 1, y, 2)
    return c


def mushrooms():
    c = grass_base(81, 3)
    for (x, y) in ((4, 8), (10, 6)):
        c.rect(x, y + 2, x, y + 4, 10)
        c.rect(x - 1, y, x + 1, y + 1, 9)
        c.set(x, y - 1, 9)
        c.set(x - 1, y, 10)
    return c


def gravestone():
    c = grass_base(91, 3)
    c.rect(4, 4, 11, 13, 4)
    c.rect(5, 2, 10, 3, 4)
    c.rect(11, 5, 11, 13, 5)
    c.rect(4, 13, 11, 14, 5)
    c.rect(7, 5, 8, 9, 6)
    c.rect(5, 7, 10, 8, 6)
    c.rect(5, 2, 5, 3, 3) if False else None
    return c


def roof(side, row, bank_red=True):
    """bank 4/5 roof; side l/m/r; row 0 top / 1 bottom"""
    c = Canvas(16, 16)
    for y in range(16):
        gy = y + 16 * row
        sr, ry = gy // 5, gy % 5
        off = 4 * (sr % 2)
        for x in range(16):
            sx = (x + off) % 8
            if ry == 4:
                col = 3
            elif sx == 0:
                col = 3
            elif ry == 0:
                col = 2
            elif ry == 3 and sx in (1, 7):
                col = 3
            else:
                col = 1
            c.set(x, y, col)
    if row == 0:
        for x in range(16):
            c.set(x, 0, 4)
            c.set(x, 1, 3)
            c.set(x, 2, 2 if x % 2 else 1)
    else:
        for x in range(16):
            c.set(x, 13, 3)
            c.set(x, 14, 4)
            c.set(x, 15, 4)
    if side == 'l':
        c.rect(0, 0, 1, 15, 3)
        c.rect(0, 0, 0, 15, 4)
    if side == 'r':
        c.rect(14, 0, 15, 15, 3)
        c.rect(15, 0, 15, 15, 4)
    return c


def wall(kind):
    c = Canvas(16, 16)
    fill(c, 1)
    # timber frame + plaster texture
    for y in range(16):
        for x in range(16):
            if H(x, y, 4) % 17 == 0:
                c.set(x, y, 2)
    c.rect(0, 0, 15, 0, 4)
    c.rect(0, 15, 15, 15, 11 - 0)
    c.rect(0, 14, 15, 14, 12) if False else None
    c.rect(0, 13, 15, 15, 11)
    c.rect(0, 13, 15, 13, 12)
    c.rect(0, 0, 1, 15, 4) if kind == 'edge' else None
    c.rect(0, 0, 0, 15, 5) if kind == 'edge' else None
    if kind == 'window':
        c.rect(3, 2, 12, 10, 4)
        c.rect(4, 3, 11, 9, 7)
        c.rect(4, 3, 11, 5, 6)
        c.rect(7, 3, 8, 9, 4)
        c.rect(4, 6, 11, 6, 4)
        c.rect(3, 11, 12, 11, 5)
        c.rect(3, 3, 3, 9, 14) if False else None
        c.set(5, 4, 15)
        c.set(9, 8, 15) if False else None
        c.rect(3, 2, 3, 10, 5)
        c.rect(12, 2, 12, 10, 5)
    if kind == 'door':
        c.rect(3, 1, 12, 15, 4)
        c.rect(4, 2, 11, 15, 9)
        c.rect(4, 2, 11, 3, 8)
        c.rect(7, 2, 7, 15, 10)
        c.rect(8, 2, 8, 15, 8)
        c.rect(5, 4, 6, 14, 9) if False else None
        c.set(10, 9, 13)
        c.set(10, 10, 13)
        c.rect(3, 13, 12, 15, 11)
        c.rect(3, 13, 12, 13, 12)
        c.set(7, 14, 12)
    return c


def stall_awning(side):
    c = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            c.set(x, y, 1 if (x // 4) % 2 == 0 else 7)
    for x in range(16):
        c.set(x, 0, 12)
        c.set(x, 1, 5)
    for x in range(16):
        depth = [0, 2, 3, 2][x % 4]
        for y in range(11 + depth, 16):
            c.set(x, y, 0)
        c.set(x, 10 + depth, 3 if (x // 4) % 2 == 0 else 8)
    if side == 'l':
        c.rect(0, 0, 1, 12, 5)
    if side == 'r':
        c.rect(14, 0, 15, 12, 5)
    return c


def stall_back(side):
    c = Canvas(16, 16)
    fill(c, 6)
    for y in range(16):
        for x in range(16):
            if y % 5 == 4:
                c.set(x, y, 13)
            elif H(x, y, 8) % 13 == 0:
                c.set(x, y, 5)
    if side == 'l':
        c.rect(0, 0, 1, 15, 5)
    if side == 'r':
        c.rect(14, 0, 15, 15, 5)
    c.rect(1, 8, 14, 8, 12)
    for x, col in ((3, 1), (6, 9), (9, 11), (12, 7)):
        c.rect(x, 4, x + 1, 7, col)
        c.set(x, 4, 7)
    return c


def stall_counter(side):
    c = Canvas(16, 16)
    fill(c, 5)
    c.rect(0, 0, 15, 4, 12)
    c.rect(0, 0, 15, 0, 7 if False else 12)
    c.rect(0, 4, 15, 4, 6)
    for y in range(5, 16):
        for x in range(16):
            c.set(x, y, 5 if x % 8 else 6)
    c.rect(2, 8, 13, 13, 6)
    c.rect(3, 9, 12, 12, 5)
    c.rect(7, 9, 8, 12, 6) if False else None
    goods = {'l': ((3, 1), (6, 9), (10, 11)), 'm': ((2, 11), (5, 1), (9, 9), (12, 7)), 'r': ((3, 9), (7, 1), (11, 11))}[side]
    for (x, col) in goods:
        c.rect(x, 0, x + 1, 3, col)
        c.set(x, 0, 7)
    return c


def fountain(q):
    """q: tl,tr,bl,br of a 32x32 fountain (bank 9). water cycle colours are 4/5/6."""
    big = Canvas(32, 32)
    big.ellipse(15.5, 17, 15.4, 13.6, 3)
    big.ellipse(15.5, 16.2, 15.0, 12.8, 1)
    big.ellipse(15.5, 16.6, 13.6, 11.2, 2)
    big.ellipse(15.5, 16.6, 12.2, 9.8, 4)
    for y in range(32):
        for x in range(32):
            if big.get(x, y) == 4 and (x + 2 * y) % 8 in (0, 1):
                big.set(x, y, 5)
            if big.get(x, y) == 4 and (x * 3 + y * 5) % 19 == 0:
                big.set(x, y, 6)
    big.ellipse(15.5, 17.5, 4.4, 3.4, 3)
    big.ellipse(15.5, 16.6, 3.8, 2.8, 1)
    big.rect(14, 8, 17, 15, 2)
    big.rect(14, 8, 14, 15, 1)
    big.rect(17, 8, 17, 15, 3)
    big.rect(15, 5, 16, 8, 5)
    big.set(15, 4, 6)
    big.set(16, 4, 6)
    big.set(13, 7, 5)
    big.set(18, 7, 5)
    big.set(12, 10, 6)
    big.set(19, 10, 6)
    r = {'tl': (0, 0), 'tr': (16, 0), 'bl': (0, 16), 'br': (16, 16)}[q]
    c = Canvas(16, 16)
    c.a[:, :] = big.a[r[1]:r[1] + 16, r[0]:r[0] + 16]
    for y in range(16):
        for x in range(16):
            if c.a[y, x] == 0:
                c.a[y, x] = 1 if (x + y) % 7 else 9
    return c


def arch(part):
    """crypt entrance 3x2 (bank 7 stone). part in tl,tm,tr,bl,bm,br"""
    c = Canvas(16, 16)
    fill(c, 5)
    for y in range(16):
        for x in range(16):
            if (y % 6 == 5) or ((x + (3 if (y // 6) % 2 else 0)) % 8 == 0 and y % 6 < 5):
                c.set(x, y, 6)
            elif H(x, y, 2) % 11 == 0:
                c.set(x, y, 4)
    if part[0] == 't':
        c.rect(0, 0, 15, 1, 4)
        c.rect(0, 14, 15, 15, 6) if part[1] == 'm' else None
        if part[1] == 'l':
            c.rect(0, 0, 1, 15, 4)
        if part[1] == 'r':
            c.rect(14, 0, 15, 15, 6)
        if part[1] == 'm':
            c.rect(6, 4, 9, 9, 14)
            c.rect(7, 5, 8, 8, 3)  # skull mark
            c.set(7, 6, 14)
            c.set(8, 6, 14)
    else:
        if part[1] == 'l':
            c.rect(0, 0, 3, 15, 4)
            c.rect(3, 0, 3, 15, 6)
        elif part[1] == 'r':
            c.rect(12, 0, 15, 15, 5)
            c.rect(12, 0, 12, 15, 6)
        else:
            fill(c, 14)
            c.rect(0, 0, 15, 1, 6)
            for y in range(2, 16):
                for x in range(16):
                    c.set(x, y, 14 if (x + y) % 7 else 14)
            c.rect(0, 12, 15, 15, 6) if False else None
            for y in range(4, 16):
                shade = 14
                c.rect(0, y, 15, y, shade)
            for x in range(16):
                c.set(x, 2, 6)
            for y in (6, 9, 12):
                c.rect(0, y, 15, y, 14)
    return c


# ----------------------------------------------------------------- dungeon
def d_floor(seed, kind='plain'):
    c = Canvas(16, 16)
    fill(c, 1)
    rng = random.Random(seed)
    # 8x8 slabs
    for y in range(16):
        for x in range(16):
            if x % 8 == 0 or y % 8 == 0:
                c.set(x, y, 3)
            elif x % 8 == 1 or y % 8 == 1:
                c.set(x, y, 2)
            elif H(x, y, seed) % 19 == 0:
                c.set(x, y, 3)
    if kind == 'crack':
        pts = [(3, 10), (5, 9), (6, 7), (8, 6), (9, 4), (11, 3)]
        for a, b in zip(pts, pts[1:]):
            c.line(a[0], a[1], b[0], b[1], 4)
    elif kind == 'moss':
        for _ in range(14):
            x, y = rng.randint(1, 14), rng.randint(1, 14)
            c.set(x, y, 5)
            c.set(x + 1, y, 5)
    elif kind == 'bones':
        c.line(3, 5, 8, 9, 6)
        c.line(4, 9, 9, 5, 6)
        c.rect(10, 10, 12, 12, 6)
        c.set(11, 11, 4)
        c.set(10, 10, 6 + 0)
    elif kind == 'rune':
        c.rect(4, 4, 11, 11, 4 + 0)
        c.rect(5, 5, 10, 10, 1)
        c.rect(7, 3, 8, 12, 8)
        c.rect(3, 7, 12, 8, 8)
        c.rect(7, 7, 8, 8, 9)
    elif kind == 'blood':
        for p in ((4, 5), (5, 5), (5, 6), (9, 10), (10, 10), (10, 11), (11, 11)):
            c.set(p[0], p[1], 7)
    return c


def d_wall_top():
    c = Canvas(16, 16)
    fill(c, 5)
    for y in range(16):
        for x in range(16):
            if H(x, y, 6) % 9 == 0:
                c.set(x, y, 6)
            if (x % 8 == 0) and (y % 16 < 15):
                c.set(x, y, 4) if False else None
    c.rect(0, 0, 15, 0, 7)
    c.rect(0, 15, 15, 15, 4 - 0) if False else None
    return c


def d_wall_front(kind):
    c = Canvas(16, 16)
    fill(c, 1)
    for y in range(16):
        for x in range(16):
            row = y // 5
            off = 4 if row % 2 else 0
            if y % 5 == 4:
                c.set(x, y, 4)
            elif (x + off) % 8 == 0:
                c.set(x, y, 4)
            elif y % 5 == 0:
                c.set(x, y, 2)
            elif H(x, y, 3) % 15 == 0:
                c.set(x, y, 3)
    c.rect(0, 15, 15, 15, 4)
    c.rect(0, 0, 15, 0, 7 + 0)
    for x in range(16):
        c.set(x, 0, 7 if False else 2)
    if kind == 'moss':
        for (x, y) in ((2, 3), (3, 3), (3, 4), (11, 8), (12, 8), (12, 9), (13, 9)):
            c.set(x, y, 8 if (x + y) % 2 else 7)
    if kind == 'skull':
        c.rect(5, 4, 10, 9, 13 - 0 if False else 2)
        c.rect(6, 5, 9, 8, 14 - 0 if False else 11)
        c.set(6, 6, 10)
        c.set(9, 6, 10)
        c.set(7, 8, 10)
        c.set(8, 8, 10)
    if kind == 'torch':
        c.rect(7, 8, 8, 13, 9 - 0)  # bracket (bank2 idx 4 is gold; use 4)
        c.rect(7, 8, 8, 13, 4)
        c.rect(5, 7, 10, 8, 4)
        c.rect(6, 9, 9, 9, 4) if False else None
        # flame (cycle colours 5,6,7)
        c.rect(6, 3, 9, 7, 5)
        c.rect(7, 2, 8, 6, 6)
        c.rect(7, 4, 8, 6, 7)
        c.set(7, 1, 5)
        c.set(8, 3, 6)
    return c


def d_door(side):
    c = Canvas(16, 16)
    fill(c, 3)
    c.rect(0, 0, 15, 15, 0)
    for y in range(16):
        for x in range(16):
            c.set(x, y, 1 if (x % 5 != 4) else 2)
    c.rect(0, 0, 15, 1, 2)
    for y in (3, 11):
        c.rect(0, y, 15, y + 1, 4)
        c.rect(0, y, 15, y, 5)
    # bolts
    for y in (3, 11):
        for x in (3, 12):
            c.set(x, y, 6)
    # lock plate in the middle
    if side == 'l':
        c.rect(0, 0, 1, 15, 2)
        c.rect(13, 6, 15, 10, 6)
        c.set(14, 8, 8 if False else 2)
    else:
        c.rect(14, 0, 15, 15, 2)
        c.rect(0, 6, 2, 10, 6)
        c.set(1, 8, 2)
    return c


def d_pillar():
    c = d_floor(2)
    c.ellipse(7.5, 9.5, 5.6, 5.0, 4)
    c.ellipse(7.5, 8.5, 5.0, 4.0, 5)
    c.ellipse(6.5, 7.5, 2.4, 2.0, 4 + 1)
    c.ellipse(7.5, 8.5, 3.0, 2.4, 6) if False else None
    for y in range(16):
        for x in range(16):
            v = c.get(x, y)
            if v in (4, 5) and (x - 7.5) + (y - 9) > 4:
                c.set(x, y, 6)
    c.rect(2, 12, 13, 13, 6) if False else None
    src = c.a.copy()
    for y in range(16):
        for x in range(16):
            if src[y, x] in (1, 2, 3):
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    xx, yy = x + dx, y + dy
                    if 0 <= xx < 16 and 0 <= yy < 16 and src[yy, xx] in (4, 5, 6):
                        c.set(x, y, 7)
                        break
    return c


def d_coffin():
    c = d_floor(3)
    c.rect(3, 1, 12, 14, 7)
    c.rect(4, 2, 11, 13, 11 - 0 if False else 4)
    c.rect(5, 3, 10, 12, 5)
    c.rect(7, 4, 8, 11, 6)
    c.rect(5, 6, 10, 7, 6)
    c.rect(4, 13, 11, 13, 6)
    c.rect(3, 14, 12, 14, 7)
    return c


def d_chest(open_):
    c = d_floor(4)
    c.rect(2, 6, 13, 13, 11)
    c.rect(2, 6, 13, 7, 7 + 1 if False else 11)
    c.rect(2, 13, 13, 13, 12)
    c.rect(2, 6, 2, 13, 12)
    c.rect(13, 6, 13, 13, 12)
    c.rect(7, 6, 8, 13, 9)
    if not open_:
        c.rect(2, 4, 13, 5, 11)
        c.rect(3, 5, 12, 6, 11)
        c.rect(7, 4, 8, 6, 9)
        c.rect(2, 4, 13, 4, 11 + 0)
        c.set(7, 8, 10)
        c.set(8, 8, 10)
    else:
        c.rect(3, 6, 12, 9, 13)
        c.rect(3, 7, 12, 9, 12 if False else 13)
        c.rect(2, 1, 13, 3, 11)
        c.rect(2, 3, 13, 3, 12)
        for p in ((5, 8), (8, 8), (10, 8)):
            c.set(p[0], p[1], 9)
    return c


def d_stairs():
    c = d_floor(5)
    for i in range(5):
        c.rect(1, 2 + i * 3, 14, 4 + i * 3, 1 if i % 2 == 0 else 10 - 0 if False else 2)
        c.rect(1, 4 + i * 3, 14, 4 + i * 3, 3)
    return c


def d_carpet(kind):
    c = Canvas(16, 16)
    fill(c, 5)
    for y in range(16):
        for x in range(16):
            if H(x, y, 5) % 12 == 0:
                c.set(x, y, 6)
    if kind == 'l':
        c.rect(0, 0, 1, 15, 8)
        c.rect(2, 0, 2, 15, 7)
    elif kind == 'r':
        c.rect(14, 0, 15, 15, 8)
        c.rect(13, 0, 13, 15, 7)
    elif kind == 'c':
        pass
    return c


def d_brazier():
    c = d_floor(6)
    c.rect(4, 9, 11, 10, 7 - 0 if False else 6)
    c.rect(5, 10, 10, 13, 7)
    c.rect(4, 13, 11, 14, 6)
    c.rect(5, 6, 10, 9, 4 + 5 if False else 12)
    c.rect(5, 8, 10, 8, 4)
    c.rect(4, 3, 11, 8, 9)
    c.rect(5, 2, 10, 7, 10)
    c.rect(6, 1, 9, 6, 11)
    c.rect(7, 3, 8, 6, 12 - 0 if False else 11)
    return c


def d_sign():
    c = d_floor(8)
    c.rect(4, 3, 11, 12, 4)
    c.rect(5, 2, 10, 2, 4)
    c.rect(5, 4, 10, 11, 5)
    c.rect(11, 4, 11, 12, 6)
    for y in (5, 7, 9):
        c.rect(6, y, 9, y, 7)
    return c


def d_throne():
    c = Canvas(16, 16)
    fill(c, 5)
    return c


# ----------------------------------------------------------------- tileset builders
def build_outdoor():
    ts = Tileset('outdoor', OUT_BANKS)
    ts.add('grass_a', grass_base(1), 0)
    ts.add('grass_b', grass_base(2), 0)
    ts.add('grass_c', flowerbed(3), 0)
    ts.add('grass_d', tuft(4), 0)
    ts.add('grass_e', flowerbed(5), 0)
    ts.add('grass_f', mushrooms(), 0)
    top, bot = oak_tree(1)
    ts.add('oak_canopy', top, 6, 0)
    ts.add('oak_trunk', bot, 6, SOLID)
    top, bot = pine_tree()
    ts.add('pine_canopy', top, 8, 0)
    ts.add('pine_trunk', bot, 8, SOLID)
    ts.add('bush', bush(), 6, SOLID | CUT)
    ts.add('rock', rock(), 7, SOLID)
    ts.add('stump', stump(), 6 - 0 if False else 6, SOLID) if False else None
    ts.add('fence_h', fence('h'), 7, SOLID)
    ts.add('fence_v', fence('v'), 7, SOLID)
    ts.add('sign', sign(), 7, SOLID | SIGN)
    ts.add('chest', chest(False), 7, SOLID | CHEST)
    ts.add('chest_open', chest(True), 7, SOLID)
    ts.add('grave', gravestone(), 7, SOLID)
    for side in 'lmr':
        ts.add('roofr_%s0' % side, roof(side, 0), 4, SOLID)
        ts.add('roofr_%s1' % side, roof(side, 1), 4, SOLID)
        ts.add('roofb_%s0' % side, roof(side, 0), 5, SOLID)
        ts.add('roofb_%s1' % side, roof(side, 1), 5, SOLID)
    ts.add('wall_plain', wall('plain'), 3, SOLID)
    ts.add('wall_edge', wall('edge'), 3, SOLID)
    ts.add('wall_window', wall('window'), 3, SOLID)
    ts.add('wall_door', wall('door'), 3, SOLID)
    for side in 'lmr':
        ts.add('awning_' + side, stall_awning(side), 4, SOLID)
        ts.add('stallback_' + side, stall_back(side), 4, SOLID)
        ts.add('counter_' + side, stall_counter(side), 4, SOLID)
    for q in ('tl', 'tr', 'bl', 'br'):
        ts.add('fountain_' + q, fountain(q), 9, SOLID)
    for p in ('tl', 'tm', 'tr', 'bl', 'bm', 'br'):
        flags = SOLID if p != 'bm' else 0
        ts.add('arch_' + p, arch(p), 7, flags)
    ts.add('plaza', path_tile(True, True, True, True, True, True, True, True, style='stone'), 1, 0)
    ts.cycles = [(2, 5, 3, 14), (9, 4, 3, 14)]
    return ts


def build_dungeon():
    ts = Tileset('dungeon', DUN_BANKS)
    ts.add('floor_a', d_floor(1), 0)
    ts.add('floor_b', d_floor(2), 0)
    ts.add('floor_crack', d_floor(3, 'crack'), 0)
    ts.add('floor_moss', d_floor(4, 'moss'), 0)
    ts.add('floor_bones', d_floor(5, 'bones'), 0)
    ts.add('floor_rune', d_floor(6, 'rune'), 0)
    ts.add('floor_blood', d_floor(7, 'blood'), 0)
    ts.add('wall_top', d_wall_top(), 1, SOLID)
    ts.add('wall_front', d_wall_front('plain'), 1, SOLID)
    ts.add('wall_front_moss', d_wall_front('moss'), 1, SOLID)
    ts.add('wall_front_skull', d_wall_front('skull'), 2, SOLID)
    ts.add('wall_front_torch', d_wall_front('torch'), 2, SOLID)
    ts.add('door_l', d_door('l'), 3, SOLID | DOOR)
    ts.add('door_r', d_door('r'), 3, SOLID | DOOR)
    ts.add('pillar', d_pillar(), 4, SOLID)
    ts.add('coffin', d_coffin(), 4, SOLID)
    ts.add('chest', d_chest(False), 4, SOLID | CHEST)
    ts.add('chest_open', d_chest(True), 4, SOLID)
    ts.add('stairs', d_stairs(), 0, 0)
    ts.add('sign', d_sign(), 4, SOLID | SIGN)
    ts.add('carpet_l', d_carpet('l'), 5, 0)
    ts.add('carpet_c', d_carpet('c'), 5, 0)
    ts.add('carpet_r', d_carpet('r'), 5, 0)
    ts.add('brazier', d_brazier(), 5, SOLID)
    ts.cycles = [(2, 5, 3, 6)]
    return ts


def preview_tileset(ts, path, scale=3):
    from PIL import Image
    n = len(ts.meta)
    cols = 12
    rows = (n + cols - 1) // cols
    im = Image.new('RGB', (cols * 17 * scale, rows * 17 * scale), (255, 0, 255))
    pals = ts.palette()
    from common import hx
    px = im.load()
    # build tile arrays back from words
    def tile_arr(t):
        a = np.zeros((8, 8), np.uint8)
        for y in range(8):
            for x in range(8):
                a[y, x] = (ts.tiles[t][y] >> (4 * x)) & 15
        return a
    for mid, ents in enumerate(ts.meta):
        ox = (mid % cols) * 17 * scale
        oy = (mid // cols) * 17 * scale
        for q, e in enumerate(ents):
            t = e & 0x3FF
            bank = (e >> 12) & 15
            a = tile_arr(t)
            for y in range(8):
                for x in range(8):
                    v = a[y, x]
                    col = (255, 0, 255) if (v == 0) else hx(pals[bank * 16 + v])
                    for sy in range(scale):
                        for sx in range(scale):
                            px[ox + ((q % 2) * 8 + x) * scale + sx, oy + ((q // 2) * 8 + y) * scale + sy] = col
    im.save(path)


if __name__ == '__main__':
    o = build_outdoor()
    print('outdoor metatiles', len(o.meta), 'tiles', len(o.tiles))
    preview_tileset(o, '/tmp/ts_out.png')
    d = build_dungeon()
    print('dungeon metatiles', len(d.meta), 'tiles', len(d.tiles))
    preview_tileset(d, '/tmp/ts_dun.png')
    # autotile samples
    from common import sheet
    fr = []
    for mask in range(16):
        n, e, s, w = [(mask >> i) & 1 == 1 for i in range(4)]
        fr.append(path_tile(n, e, s, w, True, True, True, True))
    from PIL import Image
    pal = OUT_BANKS[1]
    sheet(fr, pal, 8, scale=4).save('/tmp/ts_path.png')
    fr = []
    for mask in range(16):
        n, e, s, w = [(mask >> i) & 1 == 1 for i in range(4)]
        fr.append(water_tile(n, e, s, w, True, True, True, True))
    sheet(fr, OUT_BANKS[2], 8, scale=4).save('/tmp/ts_water.png')
