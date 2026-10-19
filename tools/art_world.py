"""Outdoor + dungeon metatile art. Metatiles are 16x16 drawn with bank-relative palette indices."""
import math
import random
import numpy as np
from common import Canvas, pal16
import art_props as AP
import art_village as AV

SOLID, WATERF, CUT, CHEST, SIGN, DOOR, DOOR2 = 1, 2, 4, 8, 16, 32, 64
HALF = 128          # with SOLID: only the south half of the tile blocks (the hero can step behind a roof's top edge)

# ----------------------------------------------------------------- palettes (bank -> pal16)
G1, G2, G3 = '#58b848', '#78d058', '#409838'   # grass base/light/dark shared as idx1..3 in many banks
OUT_BANKS = {
    0: pal16([G1, G2, G3, '#2e7a30', '#f8f8f0', '#f8e050', '#f090b8', '#80a8f8', '#e04040', '#f0e8d0', '#b88850', '#a0a0a8', '#3a8a34', '#68c850']),
    1: pal16([G1, G2, G3, '#d8a868', '#ecc888', '#b08048', '#8c6038', '#c8b8a0', '#887868', '#c8c8d4', '#a0a0b4', '#6c6c84', '#8a6a40', '#e8d8b8']),
    2: pal16([G1, G2, G3, '#2f74dc', '#3d89e8', '#4f9cf1', '#3d89e8', '#ecd8a0', '#c0a870', '#8c8c98', '#48b058', '#2c7c38', '#f8a8c8', '#7a4a28', '#e6f6ff']),   # 4-7 animated water ramp, 15 static foam
    3: pal16(['#f4e4c0', '#d8c498', '#b8a078', '#9c6a38', '#c08850', '#6c4220', '#90d0f8', '#3868a8', '#7a4a28', '#4a2a14', '#a8a8b8', '#78788c', '#f0d050', '#d04848', '#fff8e8']),
    4: pal16(['#d84838', '#f07858', '#a02828', '#701c20', '#9c6a38', '#6c4220', '#f8f8f0', '#d8d8e0', '#4880d8', '#2c58a0', '#f8d060', '#c08850', '#3a2418', '#e8a090', '#a8a8b8']),
    5: pal16(['#4878c8', '#78a8f0', '#2c58a0', '#1c3c70', '#9c6a38', '#6c4220', '#f8f8f0', '#d8d8e0', '#d84838', '#a02828', '#f8d060', '#c08850', '#3a2418', '#a8c8f8', '#a8a8b8']),
    6: pal16([G1, G2, G3, '#3a9a3c', '#62c050', '#2a7430', '#1c5428', '#8a5a30', '#b07840', '#5c3818', '#98e070', '#f090b8', '#e04050', '#f8d8e0', '#143824']),
    7: pal16([G1, G2, G3, '#c8c8d4', '#a0a0ac', '#70707c', '#c08850', '#9c6a38', '#6c4220', '#f0c840', '#a87818', '#f0e0b0', '#d04040', '#3a2a20', '#ffffff']),
    8: pal16([G1, G2, G3, '#1c4a30', '#26623c', '#368048', '#52a05c', '#8a5a30', '#b07840', '#5c3818', '#78c070', '#143824', '#e8f0e0', '#a0c898']),
    9: pal16(['#c8c8d4', '#a0a0b4', '#6c6c84', '#d8a868', '#b08048', '#8c6038', '#5a3a20', '#78d058', '#409838', '#fff0a0', '#f8a830', '#f8f8f0', '#3a84e4', '#4c98f0', '#62aef6']),
    10: pal16([G1, G2, G3, '#d8a868', '#b08048', '#8c6038', '#5a3a20', '#b8b8c8', '#6c6c84', '#f0d870', '#d8b040', '#a88020', '#fff0a0', '#f8a830', '#3a8ad8']),
    11: pal16([G1, G2, G3, '#1c4a28', '#2a6a38', '#58a858', '#6a4a30', '#8a6a40', '#4a3020', '#c84040', '#f0e8d0', '#4aa0e0', '#a0e0ff', '#78c070', '#4a7a3a']),
}
def village_banks():
    """the village has no pines or ferns: banks 8 and 11 hold the forge's slate roof and stone walls, and the shop's green roof"""
    import art_world3 as W3
    b = dict(OUT_BANKS)
    b[8] = pal16(W3.SLATE_BANK)
    b[11] = pal16(W3.SHOP_BANK)
    return b


DUN_BANKS = {
    0: pal16(['#4e4e6e', '#6a6a92', '#3a3a56', '#22223a', '#3c6050', '#e0dcc8', '#702030', '#a060e0', '#d8a8ff', '#12101c', '#241e32', '#352c48', '#4a4060', '#f0d890', '#c8a860']),
    1: pal16(['#6a5a7a', '#8a7a9c', '#4a3c5a', '#2e2438', '#241c30', '#322840', '#443858', '#406048', '#5a8060', '#a08040', '#181020', '#7a6a8a', '#3a2c4a', '#9a8aac']),
    2: pal16(['#6a5a7a', '#8a7a9c', '#4a3c5a', '#2e2438', '#f8c030', '#e86020', '#f8a020', '#fff080', '#d03018', '#a08040', '#181020', '#7a6a8a', '#3a2c4a', '#9a8aac']),
    3: pal16(['#7a4a28', '#9c6a38', '#4a2a14', '#2e2438', '#a0a0b4', '#6c6c84', '#f0c840', '#a87818', '#181020', '#c08850', '#3a2418', '#505070', '#3c3c58', '#d8a8ff']),
    4: pal16(['#4e4e6e', '#6a6a92', '#3a3a56', '#22223a', '#a0a0b4', '#c8c8d8', '#6c6c84', '#484868', '#e0dcc8', '#f0c840', '#a87818', '#8a5028', '#5a3018', '#181828', '#a060e0']),
    5: pal16(['#4e4e6e', '#6a6a92', '#3a3a56', '#22223a', '#a02838', '#d04050', '#701828', '#f0c840', '#a87818', '#e86020', '#f8c030', '#fff080', '#181828', '#ffd0a0', '#483868']),
}


# symbol maps for art_props (ascii sprites): outline, wood light/mid/dark, interior, gold light/dark, glint, shadow, text
P7 = dict(o=14, a=7, b=8, c=9, d=14, g=10, G=11, h=12, k=14, s=3, t=9, p=9)
PD4 = dict(o=14, a=12, b=13, c=13, d=14, g=10, G=11, h=12, k=14, s=4, t=14, p=14)
PD4_PILLAR = dict(o=14, l=6, m=5, d=7, w=9, s=3)
P10_LAMP = dict(o=7, i=8, d=9, g=11, G=12, w=10, h=13, s=3)
P6_BUSH = dict(dark=6, mid=4, light=5, rim=7, ink=7)
P2_REEDS = dict(stem=12, leaf=11, head=14, headdark=14, sand=8, tip=9)


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
        self.mm_ground = {}          # metatile -> minimap class overriding the colour guess
        self.mm_over = {}            # metatile -> minimap class when used on the overlay layer (canopies, roofs)

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

    def add(self, name, canv, bank, flags=0, mm=None, mmo=None):
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
        if mm is not None:
            self.mm_ground[mid] = mm
        if mmo is not None:
            self.mm_over[mid] = mmo
        return mid

    def id(self, name):
        return self.names[name]

    def compact(self, maps, keep=()):
        """Drop every metatile (and 8x8 tile) that no map of this tileset uses, renumbering the rest. The maps' ground and
        overlay arrays are rewritten in place. `keep` = names the engine looks up at run time (opened chest, stairs...).
        Metatile 0 and tile 0 stay blank at index 0."""
        used = {0} | {self.names[n] for n in keep}
        for m in maps:
            for row in m.ground:
                used.update(row)
            for row in m.overlay:
                used.update(row)
        order = sorted(used)
        remap = {old: new for new, old in enumerate(order)}
        tiles_used = {0}
        for old in order:
            for e in self.meta[old]:
                tiles_used.add(e & 0xFFF)
        tord = sorted(tiles_used)
        tmap = {old: new for new, old in enumerate(tord)}
        self.tiles = [self.tiles[t] for t in tord]
        self.tmap = {tuple(t): i for i, t in enumerate(self.tiles)}
        self.meta = [[(tmap[e & 0xFFF] | (e & 0xF000)) for e in self.meta[old]] for old in order]
        self.flags = [self.flags[old] for old in order]
        self.names = {n: remap[i] for n, i in self.names.items() if i in remap}
        self.mm_ground = {remap[i]: c for i, c in self.mm_ground.items() if i in remap}
        self.mm_over = {remap[i]: c for i, c in self.mm_over.items() if i in remap}
        for m in maps:
            m.ground = [[remap[v] for v in row] for row in m.ground]
            m.overlay = [[remap[v] for v in row] for row in m.overlay]
        return remap

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
    """n..nw = True if neighbour is water. The open water uses wavy diagonal bands whose palette index is fixed by
    position; the palette cycle (bank 2, idx 4-7) slides a gentle ping-pong ramp along them, so the surface ripples
    softly instead of blinking between blue and white."""
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
                col = 15 if (x + y) % 3 == 0 else 6            # static foam over the lightest water
            else:
                ph = (x + 2 * y + int(round(1.6 * math.sin(x * 0.75 + y * 0.4)))) % 8
                col = 4 + ph // 2
                if H(x, y, 9) % 37 == 0:
                    col = 15                                     # a glint that never changes
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
    return AP.bush(grass_base(21, 3), P6_BUSH, berry=(13, 14))


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
    AP.sign(c, P7)
    return c


def chest(open_):
    c = grass_base(71, 3)
    AP.chest(c, not open_, P7)
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
    """bank 4/5 roof; side l/m/r; row 0 top / 1 bottom. Idx: 1 shingle, 2 lit edge, 3 shade, 4 darkest, 5/6 wood trim.
    Row 0 carries the ridge cap, row 1 the overhanging eave; scalloped shingle rows are offset like real tiles."""
    c = Canvas(16, 16)
    for y in range(16):
        gy = y + 16 * row
        sr, ry = (gy - 4) // 5, (gy - 4) % 5          # shingle rows start under the 4-px ridge band
        off = 4 * (sr % 2)
        for x in range(16):
            sx = (x + off) % 8
            if ry == 4:
                col = 3                                  # shadow under each course
            elif ry == 0:
                col = 2                                  # lit upper lip
            elif sx == 0 or (ry == 3 and sx in (1, 7)):
                col = 3                                  # vertical joint / curved lower corners
            else:
                col = 1
            if col == 1 and ry in (1, 2) and H(x // 8 + sr * 5, sr, 7) % 6 == 0:
                col = 2 if sx > 1 else 1                 # the odd lighter shingle
            c.set(x, y, col)
    if row == 0:                                         # ridge cap
        for x in range(16):
            c.set(x, 0, 4)
            c.set(x, 1, 2)
            c.set(x, 2, 2 if (x // 4) % 2 else 1)
            c.set(x, 3, 3)
    else:                                                # eave: thick lip, then a deep shadow line
        for x in range(16):
            c.set(x, 12, 3)
            c.set(x, 13, 1 if x % 8 else 3)
            c.set(x, 14, 3)
            c.set(x, 15, 4)
    if side == 'l':                                      # bargeboard on the gable ends
        for y in range(16):
            c.set(0, y, 6)
            c.set(1, y, 5)
            c.set(2, y, 3) if y > 3 and c.get(2, y) != 3 else None
    if side == 'r':
        for y in range(16):
            c.set(15, y, 6)
            c.set(14, y, 5)
            c.set(13, y, 3) if y > 3 and c.get(13, y) != 3 else None
    return c


def wall(kind):
    """bank 3. Plaster with a shaded eave band, a stone footing and (per kind) a corner post, a shuttered window or a
    framed arched door. Idx: 1-3 plaster, 4/5/6 wood mid/light/dark, 7/8 glass, 11/12 stone, 13 brass, 14 red, 15 white."""
    c = Canvas(16, 16)
    fill(c, 1)
    for y in range(16):
        for x in range(16):
            if H(x, y, 4) % 19 == 0:
                c.set(x, y, 2)
    c.rect(0, 0, 15, 0, 6)                               # fascia board under the roof
    c.rect(0, 1, 15, 1, 3)
    for x in range(16):
        c.set(x, 2, 2 if x % 2 else 3)
    c.rect(0, 12, 15, 12, 12)                            # stone footing with staggered blocks
    c.rect(0, 13, 15, 15, 11)
    c.rect(0, 15, 15, 15, 12)
    for x in (3, 11):
        c.rect(x, 13, x, 14, 12)
    c.rect(7, 14, 7, 14, 12)
    if kind == 'edge':
        for (x0, x1) in ((0, 1), (14, 15)):              # timber corner posts on both sides of an end wall
            c.rect(x0, 1, x1, 12, 4)
        c.rect(0, 1, 0, 12, 5)
        c.rect(14, 1, 14, 12, 5)
        c.rect(1, 1, 1, 12, 6)
        c.rect(15, 1, 15, 12, 6)
    if kind == 'window':
        c.rect(1, 3, 2, 10, 14)                          # shutters
        c.rect(13, 3, 14, 10, 14)
        c.rect(2, 3, 2, 10, 3)
        c.rect(13, 3, 13, 10, 3)
        for y in (5, 8):
            c.rect(1, y, 2, y, 4)
            c.rect(13, y, 14, y, 4)
        c.rect(3, 2, 12, 11, 6)                          # frame
        c.rect(4, 3, 11, 10, 4)
        c.rect(5, 4, 10, 9, 7)                           # glass: pale sky above, deeper below
        c.rect(5, 7, 10, 9, 8)
        c.rect(7, 4, 8, 9, 4)                            # mullion + transom
        c.rect(5, 6, 10, 6, 4)
        c.set(5, 4, 15)
        c.set(6, 4, 15)
        c.set(5, 5, 15)
        c.set(9, 8, 7)
        c.rect(2, 11, 13, 11, 5)                         # sill
        c.rect(2, 12, 13, 12, 6)
    if kind == 'door':
        c.rect(2, 1, 13, 1, 6)                           # lintel
        c.rect(3, 2, 12, 15, 6)                          # frame
        c.rect(4, 3, 11, 15, 4)                          # planks
        for x in (5, 8, 10):
            c.rect(x, 3, x, 15, 9)
        c.rect(4, 3, 4, 15, 5)
        c.set(4, 3, 6)                                   # arched head
        c.set(11, 3, 6)
        c.set(3, 2, 2)
        c.set(12, 2, 3)
        c.rect(6, 4, 9, 5, 7)                            # little glass light
        c.set(6, 4, 15)
        c.rect(6, 6, 9, 6, 6)
        c.set(11, 9, 13)                                 # brass handle
        c.set(11, 10, 13)
        c.rect(2, 14, 13, 15, 11)                        # worn stone step
        c.rect(2, 14, 13, 14, 12)
        c.rect(2, 15, 13, 15, 12)
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


def plaza_pattern(x, y, lt=1, base=2, dk=3, inlay=False):
    """the paving pattern of the stone plaza (see path_tile), in a bank where stone is lt/base/dk."""
    if x % 8 == 0 or y % 8 == 0:
        col = dk
    elif x % 8 == 1 or y % 8 == 1:
        col = lt
    else:
        col = base
    if inlay:
        col = {lt: base, base: dk}.get(col, col)
    return col


def plaza_canvas(inlay=False):
    c = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            c.set(x, y, plaza_pattern(x, y, inlay=inlay))
    return c


_FOUNTAIN = []


def fountain(q):
    """q: tl,tr,bl,br of the 32x32 fountain (bank 9). water cycle colours are 13/14/15; art lives in art_village.py"""
    if not _FOUNTAIN:
        _FOUNTAIN.append(AV.fountain32())
    big, shadow = _FOUNTAIN[0]
    ox, oy = {'tl': (0, 0), 'tr': (16, 0), 'bl': (0, 16), 'br': (16, 16)}[q]
    c = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            v = int(big.a[oy + y, ox + x])
            if v == 0:
                v = plaza_pattern(x, y)                  # same paving as the surrounding plaza
                if (ox + x, oy + y) in shadow:
                    v = {1: 2, 2: 3, 3: 3}[v]
            c.a[y, x] = v
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
import art_dungeon as AD


def d_floor(seed, kind='plain'):
    return AD.floor_tile(seed, AD.LAYOUT_NAMES[seed % len(AD.LAYOUT_NAMES)], kind)


def d_wall_top():
    return AD.wall_rock()


def d_wall_front(kind):
    return AD.wall_front(kind)


def d_pillar():
    """(ground base, overlay capital): the column is two metatiles tall so the hero walks behind its top"""
    return AP.pillar_parts(AD.floor_tile(2, 'quad'), PD4_PILLAR)


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


def d_skull_door():
    """the skull seal in front of the king's chamber: a 32x16 drawing split into a left and a right tile (bank 4)."""
    c = Canvas(32, 16)
    fill(c, 2)
    for y in range(16):
        for x in range(32):
            if x in (15, 16) or y in (0, 15):
                c.set(x, y, 4)
            elif y == 1 or x in (0, 31):
                c.set(x, y, 3)
            elif H(x, y, 11) % 17 == 0:
                c.set(x, y, 7)
    c.ellipse(15.5, 7.0, 8.0, 6.2, 9)            # skull
    c.rect(11, 11, 20, 14, 9)
    c.ellipse(11.5, 6.5, 2.3, 2.3, 14)           # eye sockets
    c.ellipse(19.5, 6.5, 2.3, 2.3, 14)
    c.set(11, 6, 15); c.set(19, 6, 15)           # glow
    c.rect(15, 9, 16, 10, 14)                    # nose
    for x in (12, 14, 17, 19):
        c.rect(x, 12, x, 14, 14)
    c.rect(14, 14, 17, 14, 10)                   # gold lock
    for (x, y) in ((3, 3), (28, 3), (3, 12), (28, 12), (7, 2), (24, 2)):
        c.set(x, y, 15)
    l, r = Canvas(16, 16), Canvas(16, 16)
    l.a = c.a[:, 0:16].copy()
    r.a = c.a[:, 16:32].copy()
    return l, r


def d_bone_pile():
    c = d_floor(9)
    c.ellipse(7.5, 10.0, 6.0, 3.6, 7)
    c.ellipse(7.5, 9.0, 5.0, 2.8, 8)
    for (x0, y0, x1, y1) in ((3, 9, 9, 6), (5, 11, 12, 8), (9, 11, 13, 12)):
        c.line(x0, y0, x1, y1, 9)
    c.ellipse(7.0, 6.5, 2.6, 2.4, 9)             # skull on top
    c.set(6, 6, 14); c.set(8, 6, 14); c.set(7, 8, 14)
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
    AP.chest(c, not open_, PD4)
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
    AP.sign(c, PD4)
    return c


def d_throne():
    c = Canvas(16, 16)
    fill(c, 5)
    return c


# ----------------------------------------------------------------- tileset builders
def build_outdoor(banks=None):
    ts = Tileset('outdoor', banks or OUT_BANKS)
    ts.add('grass_a', grass_base(1), 0)
    ts.add('grass_b', grass_base(2), 0)
    ts.add('grass_c', flowerbed(3), 0)
    ts.add('grass_d', tuft(4), 0)
    ts.add('grass_e', flowerbed(5), 0)
    ts.add('grass_f', mushrooms(), 0)
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
    ts.cycles = [(2, 4, 4, 12), (9, 13, 3, 16)]
    import art_world2
    art_world2.extend_outdoor(ts)
    return ts


def build_dungeon(banks=None, name='dungeon'):
    ts = Tileset(name, banks or DUN_BANKS)
    L = AD.LAYOUT_NAMES
    ts.add('floor_a', AD.floor_tile(1, 'quad'), 0)
    ts.add('floor_b', AD.floor_tile(2, 'bond'), 0)
    ts.add('floor_c', AD.floor_tile(3, 'wide'), 0)
    ts.add('floor_d', AD.floor_tile(4, 'tall'), 0)
    ts.add('floor_e', AD.floor_tile(5, 'trip'), 0)
    ts.add('floor_f', AD.floor_tile(6, 'big'), 0)
    ts.add('floor_crack', AD.floor_tile(7, 'wide', 'crack'), 0)
    ts.add('floor_moss', AD.floor_tile(8, 'quad', 'moss'), 0)
    ts.add('floor_bones', AD.floor_tile(9, 'bond', 'bones'), 0)
    ts.add('floor_rune', AD.floor_tile(10, 'quad', 'rune'), 0)
    ts.add('floor_blood', AD.floor_tile(11, 'tall', 'blood'), 0)
    for nm, seed, lay in (('a', 1, 'quad'), ('b', 2, 'bond'), ('c', 3, 'wide'), ('d', 4, 'tall'), ('e', 5, 'trip')):
        ts.add('floor_%s_s' % nm, AD.shaded(AD.floor_tile(seed, lay)), 0)
    ts.add('floor_a_sl', AD.shaded_side(AD.floor_tile(1, 'quad'), True), 0)
    ts.add('floor_a_sr', AD.shaded_side(AD.floor_tile(1, 'quad'), False), 0)
    ts.add('wall_top', AD.wall_rock(), 1, SOLID)
    ts.add('wall_cap', AD.wall_cap(), 1, SOLID)
    ts.add('wall_cap_s', AD.wall_cap_s(), 1, SOLID)
    ts.add('wall_edge_r', AD.wall_edge(True), 1, SOLID)
    ts.add('wall_edge_l', AD.wall_edge(False), 1, SOLID)
    ts.add('wall_front', AD.wall_front('plain'), 1, SOLID)
    ts.add('wall_front_moss', AD.wall_front('moss'), 1, SOLID)
    ts.add('wall_front_skull', AD.wall_front('skull'), 2, SOLID)
    ts.add('wall_front_torch', AD.wall_front('torch'), 2, SOLID)
    ts.add('door_l', d_door('l'), 3, SOLID | DOOR)
    ts.add('door_r', d_door('r'), 3, SOLID | DOOR)
    pb, pc = d_pillar()
    ts.add('pillar', pb, 4, SOLID)
    ts.add('pillar_cap', pc, 4, 0)
    ts.add('coffin', d_coffin(), 4, SOLID)
    ts.add('chest', d_chest(False), 4, SOLID | CHEST)
    ts.add('chest_open', d_chest(True), 4, SOLID)
    for nm, down in (('down', True), ('up', False)):
        l, r = AD.stairs(down)
        ts.add('stairs_%s_l' % nm, l, 0, 0)
        ts.add('stairs_%s_r' % nm, r, 0, 0)
    ts.add('sign', d_sign(), 4, SOLID | SIGN)
    ts.add('carpet_l', d_carpet('l'), 5, 0)
    ts.add('carpet_c', d_carpet('c'), 5, 0)
    ts.add('carpet_r', d_carpet('r'), 5, 0)
    ts.add('brazier', d_brazier(), 5, SOLID)
    sl, sr = d_skull_door()
    ts.add('skull_door_l', sl, 4, SOLID | DOOR2)
    ts.add('skull_door_r', sr, 4, SOLID | DOOR2)
    ts.add('bone_pile', d_bone_pile(), 4, SOLID)
    ts.add('wall_front_web', AD.wall_front('web'), 1, SOLID)
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


def _warm(c):
    """recolour a hex colour to scorched stone; gold / fire / red tones stay as they are"""
    r, g, b = int(c[1:3], 16), int(c[3:5], 16), int(c[5:7], 16)
    if r > b + 70 and r > g + 25:
        return c
    lum = 0.3 * r + 0.59 * g + 0.11 * b
    return '#%02x%02x%02x' % (min(255, int(lum * 1.3 + 10)), int(lum * 0.82), int(lum * 0.66))


EMBER_BANKS = {k: [v[0]] + [_warm(c) for c in v[1:]] for k, v in DUN_BANKS.items()}
EMBER_BANKS[6] = pal16(['#fff0a0', '#ffc030', '#ff7010', '#d03008', '#701004', '#3a0a04', '#1c0402', '#ffe060', '#ffa020', '#c02000', '#901808', '#601008', '#401008', '#2a0a06'])


def build_ember():
    """the Cinder Hollows: the dungeon art in scorched colours, plus lava pools"""
    ts = build_dungeon(EMBER_BANKS, 'ember')
    for i, seed in enumerate((1, 2, 3)):
        ts.add('lava_' + 'abc'[i], AD.lava_tile(seed), 6, SOLID)
    ts.cycles = [(2, 5, 3, 6), (6, 2, 3, 9)]
    return ts


def autumn_palette(pal):
    """Autumn version of an outdoor palette (192 hex colours = 12 banks x 16): grass turns golden-olive, oak and bush
    foliage (bank 6) turns orange, pines (bank 8) keep most of their green. Water, wood, stone and roofs are untouched,
    so the very same metatiles serve Sunmeadow Road, Amberwood and Amberfall in their own season."""
    import colorsys
    out = []
    for i, c in enumerate(pal):
        bank, idx = divmod(i, 16)
        r, g, b = [int(c[k:k + 2], 16) / 255.0 for k in (1, 3, 5)]
        h, l, sat = colorsys.rgb_to_hls(r, g, b)
        hd = h * 360.0
        if idx and 70.0 <= hd <= 170.0 and sat > 0.15:
            boost = 1.0
            if bank == 6 and idx >= 4:
                target, k, boost = 33.0, 1.0, 1.2               # oak and bush foliage: bright amber-orange
            elif bank == 8 and idx >= 4:
                target, k = 85.0, 0.40
            else:
                target, k = 52.0, 0.60
            hd += (target - hd) * k
            sat = min(1.0, sat * (1.04 if boost == 1.0 else 1.3))
            l = min(0.92, l * (boost if bank == 6 and idx >= 4 else 1.0))
            r, g, b = colorsys.hls_to_rgb(hd / 360.0, l, sat)
            c = '#%02x%02x%02x' % (int(round(r * 255)), int(round(g * 255)), int(round(b * 255)))
        out.append(c)
    return out
