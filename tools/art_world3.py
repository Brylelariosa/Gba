"""World art v2: round-leaf oaks and pines built from shaded leaf clusters, complete 2x2 'forest wall' trees for the map
borders, bare trees, proper boulders, fences, planter boxes, bushes.

Everything is lit from the top-left and written with the palette indices of the outdoor banks:
  bank 6  (oak / bush)  1-3 grass, 4 mid leaf, 5 light leaf, 6 dark leaf, 7 shadow leaf, 8 trunk mid, 9 trunk light,
                        10 trunk dark, 11 highlight leaf, 12 pink, 13 red, 14 pale pink, 15 deepest outline
  bank 7  (props)      1-3 grass, 4/5/6 stone light/mid/dark, 7/8/9 wood light/mid/dark, 10/11 gold, 12 pale, 13 red, 14 ink
  bank 8  (pine)        1-3 grass, 4 dark needle, 5 mid, 6 light, 7 lightest, 8 trunk mid, 9 trunk light, 10 trunk dark,
                        11 highlight, 12 deepest, 13 snow, 14 pale
  bank 11 (flora)       1-3 grass, 4/5/6 greens, 7/8/9 wood light/mid/dark, 10 red, 11 pale
"""
import math
import random
from common import Canvas
from art_world import H

# ----------------------------------------------------------------------------------------------- shading helpers
LEAF_RAMP = (7, 6, 4, 5, 11)             # bank 6, darkest -> lightest
PINE_RAMP = (12, 4, 5, 6, 7)             # bank 8, darkest -> lightest


def _shade(s, ramp):
    """s in roughly [-1, 1] (positive = facing the light) -> ramp colour"""
    if s > 0.62:
        return ramp[4]
    if s > 0.18:
        return ramp[3]
    if s > -0.28:
        return ramp[2]
    if s > -0.62:
        return ramp[1]
    return ramp[0]


def leaf_clusters(c, puffs, seed, ramp=LEAF_RAMP, outline=15, texture=7, dark_under=True):
    """Paint overlapping round leaf clusters. puffs = [(cx, cy, rx, ry)], painted in order (later ones in front).
    Each puff is shaded as a little sphere lit from the top-left, gets a dark crescent under its lower rim so the
    clusters read as separate bunches, and the whole silhouette is outlined."""
    for (cx, cy, rx, ry) in puffs:
        for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
            for x in range(int(cx - rx) - 1, int(cx + rx) + 2):
                t, u = (x + 0.5 - cx) / rx, (y + 0.5 - cy) / ry
                d = t * t + u * u
                if d > 1.0:
                    continue
                s = -0.55 * t - 0.85 * u + 0.10
                if texture and H(x, y, seed) % texture == 0:
                    s += 0.34 if H(x, y, seed + 1) & 1 else -0.34       # leafy flecks
                col = _shade(s, ramp)
                if dark_under and d > 0.80 and (t + u) > 0.55:
                    col = ramp[0]                                         # shadow crescent on the lower right rim
                c.set(x, y, col)
    # silhouette outline
    src = c.a.copy()
    for y in range(c.h):
        for x in range(c.w):
            if src[y, x] != 0:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                xx, yy = x + dx, y + dy
                if 0 <= xx < c.w and 0 <= yy < c.h and src[yy, xx] != 0 and src[yy, xx] not in (1, 2, 3) and src[yy, xx] != 8:
                    c.set(x, y, outline)
                    break


def apples(c, rng, n, region, red=13, pale=14, light=(5, 11)):
    """a few apples (red dot + highlight) on lit leaf pixels inside region = (x0, y0, x1, y1)"""
    tries = 0
    while n > 0 and tries < 300:
        tries += 1
        x, y = rng.randint(region[0], region[2]), rng.randint(region[1], region[3])
        if c.get(x, y) in light and c.get(x + 1, y) in light and c.get(x, y + 1) in light:
            c.set(x, y, red)
            c.set(x + 1, y, red)
            c.set(x, y - 1, pale) if c.get(x, y - 1) else None
            n -= 1


def trunk(c, cx, y0, y1, w, base_flare=True, mid=8, light=9, dark=10, ink=15, grass_bg=True):
    """a straight oak trunk (w pixels wide) from y0 to y1 with light on the left, roots flaring at the base"""
    x0 = cx - w // 2
    for y in range(y0, y1 + 1):
        flare = 1 if (base_flare and y >= y1 - 2) else 0
        for x in range(x0 - flare, x0 + w + flare):
            rel = x - (x0 - flare)
            width = w + 2 * flare
            col = light if rel < width * 0.3 else (mid if rel < width * 0.72 else dark)
            if H(x, y, 3) % 7 == 0 and col == mid:
                col = dark
            c.set(x, y, col)
        c.set(x0 - flare - 1, y, ink)
        c.set(x0 + w + flare, y, ink)
    for x in range(x0 - 2, x0 + w + 2):
        c.set(x, y1 + 1, ink)
    if grass_bg:                                       # ground shadow cast to the lower right
        for x in range(x0 + w, x0 + w + 4):
            if c.get(x, y1) in (1, 2):
                c.set(x, y1, 3)


# ----------------------------------------------------------------------------------------------- oak (bank 6)
def _oak_puffs(w, h, kind):
    """cluster layouts for a canopy that fills a w x h area; kind picks one of three silhouettes"""
    cx = w / 2.0
    if kind == 0:
        return [(cx - w * 0.26, h * 0.40, w * 0.30, h * 0.34), (cx + w * 0.27, h * 0.42, w * 0.28, h * 0.32),
                (cx, h * 0.30, w * 0.32, h * 0.30), (cx - w * 0.12, h * 0.62, w * 0.34, h * 0.30), (cx + w * 0.18, h * 0.64, w * 0.30, h * 0.28)]
    if kind == 1:
        return [(cx - w * 0.28, h * 0.46, w * 0.27, h * 0.30), (cx + w * 0.30, h * 0.48, w * 0.25, h * 0.30),
                (cx - w * 0.06, h * 0.28, w * 0.30, h * 0.26), (cx + w * 0.10, h * 0.52, w * 0.34, h * 0.32), (cx - w * 0.20, h * 0.66, w * 0.28, h * 0.26)]
    return [(cx - w * 0.22, h * 0.34, w * 0.28, h * 0.28), (cx + w * 0.24, h * 0.36, w * 0.28, h * 0.28),
            (cx + w * 0.02, h * 0.58, w * 0.40, h * 0.34), (cx - w * 0.30, h * 0.62, w * 0.22, h * 0.24), (cx + w * 0.32, h * 0.64, w * 0.20, h * 0.24)]


def oak_unit(variant, fruit=True):
    """one complete 32x32 oak standing on grass: leafy crown above, short trunk, tuft of grass. Used 2x2 cells wide for
    the map borders so every tree is whole (bank 6, trunk cell rows hold grass colours 1-3)."""
    c = Canvas(32, 32)
    for y in range(32):
        for x in range(32):
            c.set(x, y, (1, 1, 2, 1, 3)[H(x, y, 40 + variant) % 5] if y > 23 else 0)
    rng = random.Random(500 + variant)
    trunk(c, 16, 21, 29, 6)
    leaf_clusters(c, _oak_puffs(32, 24, variant % 3), 60 + variant)
    if fruit:
        apples(c, rng, 4, (6, 4, 25, 18))
    return c


def oak_wall_fill(variant):
    """top-down-ish dense canopy used where a unit would not show a trunk (not used on borders any more, kept for
    the dungeon-style fills): a 16x16 patch of leaves with no transparency"""
    c = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            c.set(x, y, 6)
    leaf_clusters(c, [(4, 4, 6, 5), (12, 5, 6, 5), (8, 11, 8, 6)], 90 + variant, outline=7)
    for y in range(16):
        for x in range(16):
            if c.get(x, y) == 0:
                c.set(x, y, 7)
    return c


def oak_single(variant):
    """a free-standing oak, 16 wide x 48 tall as three cells: top (overlay), canopy (overlay), trunk (ground).
    The crown is wider than the cell on purpose; neighbouring cells hold the rest via oak_wide()."""
    c = Canvas(16, 48)
    rng = random.Random(700 + variant)
    # trunk cell (rows 32-47): grass, trunk, shadow
    for y in range(32, 48):
        for x in range(16):
            c.set(x, y, (1, 1, 2, 1, 3)[H(x, y, 70 + variant) % 5])
    trunk(c, 8, 32, 44, 5)
    sub = Canvas(16, 36)
    leaf_clusters(sub, _oak_puffs(16, 36, variant % 3)[:4], 80 + variant)
    apples(sub, rng, 2, (3, 8, 12, 26))
    c.blit(sub, 0, 0)
    return c


def giant_oak2(variant=0):
    """the big three-wide oak: 48 x 48 in 3x3 cells. Rows 0-1 are overlay (crown, the hero walks under it), the bottom
    row is the ground layer with the trunk in the middle cell and grass on both sides."""
    c = Canvas(48, 48)
    rng = random.Random(900 + variant)
    for y in range(32, 48):
        for x in range(48):
            c.set(x, y, (1, 1, 2, 1, 3)[H(x, y, 95 + variant) % 5])
    trunk(c, 24, 28, 44, 8)
    for (rx, ry) in ((16, 41), (32, 41)):                            # exposed roots
        c.set(rx, ry, 10)
        c.set(rx, ry + 1, 15)
    crown = Canvas(48, 36)
    puffs = [(10, 20, 10, 9), (38, 20, 10, 9), (24, 12, 15, 11), (14, 12, 11, 9), (34, 13, 11, 9),
             (24, 24, 17, 10), (11, 27, 9, 7), (37, 27, 9, 7)]
    leaf_clusters(crown, puffs, 110 + variant)
    apples(crown, rng, 7, (6, 6, 41, 29))
    c.blit(crown, 0, 0)
    return c


# ----------------------------------------------------------------------------------------------- pine (bank 8)
def pine_tree(w, h, seed, base_y=None):
    """a conifer made of stacked, scalloped tiers: light on the left, shaded on the right, a dark under-edge on every
    tier and a short trunk. w x h canvas."""
    c = Canvas(w, h)
    cx = w / 2.0
    bottom = base_y if base_y is not None else h - 6
    n = 4 if h >= 30 else 3
    th = max(7, int((bottom - 1) / (n * 0.62 + 0.4)))            # tier height: tiers overlap by about a third
    for t in range(n):
        y0 = 1 + int(t * th * 0.62)
        y1 = min(bottom, y0 + th)
        maxhw = (w / 2.0 - 1.0) * (0.50 + 0.50 * (t + 1) / n)
        for y in range(y0, y1 + 1):
            f = (y - y0) / max(1.0, float(y1 - y0))
            hw = 1.2 + (maxhw - 1.2) * (f ** 0.9)
            for x in range(int(cx - hw - 1), int(cx + hw + 2)):
                dx = x + 0.5 - cx
                if abs(dx) > hw:
                    continue
                side = dx / hw
                col = 7 if side < -0.55 else (6 if side < 0.18 else (5 if side < 0.62 else 4))
                if H(x, y, seed + t) % 5 == 0:                                # needle flecks
                    col = {7: 11, 6: 7, 5: 6, 4: 5}[col] if H(x, y, seed) & 1 else {7: 6, 6: 5, 5: 4, 4: 4}[col]
                if y >= y1 - 1:
                    col = 4 if abs(dx) < hw - 1 else 12                        # shaded under-edge of the tier
                c.set(x, y, col)
        for x in range(int(cx - maxhw), int(cx + maxhw) + 1):                   # scalloped lower edge
            if (x + t + seed) % 3 == 0 and abs(x + 0.5 - cx) < maxhw - 1:
                c.set(x, y1 + 1, 12)
    for y in range(bottom + 1, h - 1):                                       # trunk stub
        for x in (int(cx) - 1, int(cx), int(cx) + 1):
            c.set(x, y, 9 if x == int(cx) - 1 else (8 if x == int(cx) else 10))
    src = c.a.copy()
    for y in range(h):
        for x in range(w):
            if src[y, x] != 0:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                xx, yy = x + dx, y + dy
                if 0 <= xx < w and 0 <= yy < h and src[yy, xx] != 0 and src[yy, xx] not in (8, 9, 10):
                    c.set(x, y, 12)
                    break
    return c


def pine_unit(variant):
    """a complete 32x32 pine on grass for the borders; the foot is shared grass so units can sit side by side"""
    c = Canvas(32, 32)
    for y in range(32):
        for x in range(32):
            c.set(x, y, (1, 1, 2, 1, 3)[H(x, y, 140 + variant) % 5] if y > 25 else 0)
    p = pine_tree(24 if variant % 2 else 28, 29, 150 + variant, base_y=24)
    c.blit(p, (32 - p.w) // 2 + (variant % 3) - 1, 0)
    # tiny trunk shadow
    for x in range(17, 21):
        if c.get(x, 29) in (1, 2):
            c.set(x, 29, 3)
    return c


def pine_fill():
    """solid dense pine foliage with no outline, a seamless filler 16x16 (bank 8)"""
    c = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            v = H(x, y, 190) % 9
            c.set(x, y, 4 if v < 5 else (5 if v < 8 else 6))
    for (cx, cy, r) in ((3, 3, 3), (11, 4, 3), (7, 10, 4), (14, 13, 3), (2, 13, 3)):
        for y in range(cy - r, cy + r + 1):
            for x in range(cx - r, cx + r + 1):
                if (x - cx) ** 2 + (y - cy) ** 2 <= r * r:
                    xx, yy = x % 16, y % 16
                    dist = ((x - cx) + (y - cy))
                    c.set(xx, yy, 6 if dist < -1 else (5 if dist < 2 else 4))
    return c


def pine_single():
    """16x48 free-standing pine for forests: top (overlay), mid (overlay), foot (ground)"""
    c = Canvas(16, 48)
    for y in range(32, 48):
        for x in range(16):
            c.set(x, y, (1, 1, 2, 1, 3)[H(x, y, 170) % 5])
    p = pine_tree(16, 44, 171, base_y=38)
    c.blit(p, 0, 0)
    for x in range(9, 13):
        if c.get(x, 44) in (1, 2):
            c.set(x, 44, 3)
    return c


# ----------------------------------------------------------------------------------------------- bare tree (bank 11)
def dead_tree(variant=0):
    """gnarled leafless tree, 16 x 32: the upper half (branches) goes on the overlay layer, the lower half (trunk on grass)
    on the ground layer. Bank 11: 7/8/9 wood light/mid/dark."""
    c = Canvas(16, 32)
    for y in range(16, 32):
        for x in range(16):
            c.set(x, y, (1, 1, 2, 1, 3)[H(x, y, 210 + variant) % 5])
    lean = -1 if variant else 1
    for y in range(12, 29):                                              # trunk, wider toward the roots
        w = 5 if y > 22 else (4 if y > 16 else 3)
        sway = lean * (1 if y < 18 else 0)
        for dx in range(w):
            x = 6 + sway + dx
            c.set(x, y, 7 if dx == 0 else (9 if dx == w - 1 else 8))
        c.set(5 + sway, y, 9) if False else None
    for (x, y, col) in ((5, 27, 9), (4, 28, 9), (11, 27, 9), (12, 28, 9), (5, 28, 9), (10, 28, 9)):   # roots
        c.set(x, y, col)
    for x in (12, 13, 14):                                               # cast shadow
        if c.get(x, 28) in (1, 2):
            c.set(x, 28, 3)
    fork = (7 + lean, 12)
    limbs = [((fork[0], 12), (2, 5), 2), ((fork[0] + 1, 12), (13, 4), 2), ((fork[0], 10), (4, 1), 1), ((fork[0] + 1, 9), (11, 0), 1),
             ((6, 16), (1, 11), 1), ((9, 15), (15, 10), 1)]
    for (p0, p1, th) in limbs:
        c.line(p0[0], p0[1], p1[0], p1[1], 8)
        if th == 2:
            c.line(p0[0] + 1, p0[1], p1[0] + 1, p1[1], 9)
            c.line(p0[0] - 1, p0[1], p1[0] - 1, p1[1], 7)
    for (x, y) in ((2, 5), (1, 4), (3, 3), (13, 4), (14, 3), (12, 2), (4, 1), (3, 0), (5, 2), (11, 0), (10, 1), (1, 11), (0, 10), (2, 9), (15, 10), (14, 8), (15, 12)):
        c.set(x, y, 9)
    return c


# ----------------------------------------------------------------------------------------------- boulders (bank 7)
def boulder(seed=31, moss=False, bg=None):
    """a rounded grey boulder with a smaller stone beside it, light top-left, ink outline, ground shadow, no hole in it"""
    c = bg if bg is not None else Canvas(16, 16)
    if bg is None:
        for y in range(16):
            for x in range(16):
                c.set(x, y, (1, 1, 2, 1, 3)[H(x, y, seed) % 5])
    # ground shadow
    for x in range(2, 15):
        for y in (13, 14):
            if ((x - 8.5) / 6.8) ** 2 + ((y - 13.5) / 1.6) ** 2 <= 1.0:
                c.set(x, y, 3)

    def stone(cx, cy, rx, ry, sd):
        pix = []
        for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
            for x in range(int(cx - rx) - 1, int(cx + rx) + 2):
                t, u = (x + 0.5 - cx) / rx, (y + 0.5 - cy) / ry
                # slightly irregular outline
                wob = 1.0 + 0.07 * math.sin(3.1 * math.atan2(u, t) + sd)
                if t * t + u * u <= wob:
                    s = -0.6 * t - 0.85 * u + 0.12
                    col = 12 if s > 0.80 else (4 if s > 0.30 else (5 if s > -0.30 else 6))
                    if H(x, y, sd) % 9 == 0 and col in (5, 6):
                        col = 6 if col == 5 else 5
                    c.set(x, y, col)
                    pix.append((x, y))
        return pix

    big = stone(7.2, 8.6, 6.2, 5.0, 3)
    small = stone(12.2, 11.4, 3.0, 2.5, 7)
    allp = set(big) | set(small)
    for (x, y) in list(allp):                                        # ink outline on the open sides
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            if (x + dx, y + dy) not in allp:
                c.set(x + dx, y + dy, 14)
    for (x, y) in ((5, 7), (6, 8), (7, 9)):                           # a crack
        if (x, y) in allp:
            c.set(x, y, 6)
    c.set(5, 6, 12)
    c.set(6, 6, 12)
    if moss:
        for (x, y) in ((4, 5), (5, 5), (6, 5), (4, 6), (9, 4), (10, 5), (13, 10), (12, 10)):
            if (x, y) in allp:
                c.set(x, y, 2)
        for (x, y) in ((5, 4), (3, 6), (11, 4)):
            if (x, y) in allp:
                c.set(x, y, 1)
    return c


# ----------------------------------------------------------------------------------------------- bush (bank 6)
def bush2(seed=1, berries=True):
    c = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            c.set(x, y, (1, 1, 2, 1, 3)[H(x, y, 260 + seed) % 5])
    for x in range(2, 15):                                           # shadow
        for y in (13, 14):
            if ((x - 8.5) / 6.8) ** 2 + ((y - 13.5) / 1.6) ** 2 <= 1.0:
                c.set(x, y, 3)
    leaf_clusters(c, [(5.5, 9.0, 4.6, 4.2), (10.5, 9.0, 4.6, 4.2), (8.0, 6.6, 4.8, 4.2)], 270 + seed, texture=6)
    if berries:
        rng = random.Random(280 + seed)
        n = 0
        for _ in range(60):
            x, y = rng.randint(3, 12), rng.randint(4, 12)
            if c.get(x, y) in (4, 5, 11) and n < 4:
                c.set(x, y, 13)
                n += 1
    return c


# ----------------------------------------------------------------------------------------------- fences (bank 7)
def fence2(kind, end=None):
    """wooden fence: sturdy posts with caps, two rails with a lit top edge, grass tufts and a soft ground shadow.
    kind 'h' runs left-right, 'v' runs up-down. end = 'l'/'r'/'t'/'b' puts a post at that end cell edge."""
    c = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            c.set(x, y, (1, 1, 2, 1, 3)[H(x, y, 300) % 5])
    if kind == 'h':
        for x in range(0, 16):                                       # ground shadow under the rails
            c.set(x, 14, 3)
            if x % 2 == 0:
                c.set(x, 15, 3)
        for (y0, y1) in ((5, 6), (9, 10)):                           # rails: lit top, mid face, dark under-edge
            for x in range(16):
                c.set(x, y0 - 1, 14)
                c.set(x, y0, 7)
                c.set(x, y1, 8)
                c.set(x, y1 + 1, 14)
        for px in (1, 9):                                            # posts, two per cell would crowd: one every 8 px
            for y in range(3, 14):
                c.set(px, y, 14)
                c.set(px + 1, y, 7)
                c.set(px + 2, y, 8)
                c.set(px + 3, y, 14)
            c.set(px + 1, 3, 7)
            c.set(px + 2, 3, 7)
            c.set(px + 1, 2, 14)
            c.set(px + 2, 2, 14)
            c.set(px + 1, 13, 14)
            c.set(px + 2, 13, 14)
            c.set(px + 2, 12, 9)
            c.set(px + 1, 5, 12)
        c.set(0, 13, 2)
        c.set(15, 13, 2)
    else:
        for y in range(16):                                          # ground shadow beside the rail
            c.set(11, y, 3) if y % 2 == 0 else None
            c.set(12, y, 3) if y % 4 == 1 else None
        for y in range(16):                                          # the rail seen end-on-ish: 2 px lit/mid with ink sides
            c.set(6, y, 14)
            c.set(7, y, 7)
            c.set(8, y, 8)
            c.set(9, y, 14)
        for py in (1, 9):                                            # posts: square tops with a lit cap and a dark front
            for x in range(4, 12):
                for y in range(py, py + 6):
                    c.set(x, y, 14)
            for x in range(5, 11):
                for y in range(py + 1, py + 5):
                    c.set(x, y, 7 if y == py + 1 else (8 if x < 9 else 9))
            c.set(5, py + 1, 12)
            c.set(6, py + 1, 12)
    return c


# ----------------------------------------------------------------------------------------------- planter boxes
def planter2(bg, wood=(7, 8, 9), ink=14, soil=None, flowers=(13, 11), leaf=(2, 3), pale=12):
    """a wooden planter box with a lit rim, plank lines, iron-free corner posts, a hump of soil and a row of flowers.
    bg is the 16x16 canvas it stands on (grass or paving); wood = (light, mid, dark) indices of the target bank."""
    c = bg
    wl, wm, wd = wood
    sh = leaf[1]
    for x in range(2, 15):                                           # soft shadow to the lower right
        if c.get(x, 15) in (1, 2, 3):
            c.set(x, 15, sh)
    for x in range(1, 14):
        for y in range(8, 15):
            edge = x in (1, 13) or y in (8, 14)
            c.set(x, y, ink if (edge and (x in (1, 13) or y == 14)) else (wl if y == 8 else (wm if y < 13 else wd)))
    for x in (4, 7, 10):                                             # plank joints
        for y in range(9, 14):
            c.set(x, y, wd)
    for y in range(9, 14):
        c.set(2, y, wl)
        c.set(12, y, wd)
    # soil + foliage above the rim
    for x in range(2, 13):
        c.set(x, 7, wd)
    stems = (3, 5, 7, 9, 11)
    for i, x in enumerate(stems):
        top = 3 + (i % 2)
        for y in range(top + 2, 8):
            c.set(x, y, leaf[0] if i % 2 else leaf[1])
        c.set(x + 1, 6, leaf[0])
        col = flowers[i % 2]
        c.set(x, top, col)
        c.set(x - 1, top + 1, col)
        c.set(x + 1, top + 1, col)
        c.set(x, top + 1, pale)
        c.set(x, top - 1, ink) if c.get(x, top - 1) == 0 else None
    return c


# =============================================================================================== buildings
# Two extra roof/wall palettes. Both keep the layout of the red/blue roof banks (1 shingle, 2 lit edge, 3 shade, 4 darkest,
# 5/6 wood trim, 7/8 smoke) so the same roof() art and chimney tiles work in every colour.
SLATE_BANK = ['#6c7488', '#9aa4bc', '#4c5468', '#343a4a', '#9c6a38', '#6c4220', '#d8d8e0', '#a8a8b8', '#7a8096', '#f8d060',
              '#e8782c', '#8a90a4', '#2a2c38', '#c8ccd8', '#4880d8']
SHOP_BANK = ['#4c9a58', '#7cc47c', '#347240', '#22502c', '#9c6a38', '#6c4220', '#f8f8f0', '#d8d8e0', '#d84838', '#a02828',
             '#f8d060', '#c08850', '#3a2418', '#ecdcb4', '#4880d8']


def wall_stone(kind):
    """slate bank (8): masonry. 14 light stone, 12 mid, 9 dark, 13 mortar/ink, 5/6 wood, 10/11 fire, 15 glass, 7 white."""
    c = Canvas(16, 16)
    for y in range(16):
        course, ry = divmod(y, 5)
        off = 4 * (course % 2)
        for x in range(16):
            sx = (x + off) % 8
            if ry == 4:
                col = 13
            elif sx == 0:
                col = 9
            elif ry == 0:
                col = 14
            else:
                col = 12
            if col == 12 and H(x // 8 + course * 3, course, 5) % 4 == 0:
                col = 14 if ry < 2 else 9
            c.set(x, y, col)
    for x in range(16):                                              # eave shadow under the roof
        c.set(x, 0, 13)
        c.set(x, 1, 9)
    if kind == 'edge':
        for x0 in (0, 14):
            for y in range(2, 16):
                c.set(x0, y, 14 if (y // 3) % 2 == 0 else 12)
                c.set(x0 + 1, y, 9)
    if kind == 'window':                                             # arched slit window with the forge glow inside
        c.rect(4, 3, 11, 13, 9)
        c.rect(5, 4, 10, 12, 13)
        c.rect(6, 5, 9, 11, 11)
        c.rect(7, 6, 8, 10, 10)
        c.set(6, 5, 13)
        c.set(9, 5, 13)
        for y in (7, 10):
            c.rect(5, y, 10, y, 13)
        c.rect(7, 4, 8, 12, 13)
        c.rect(3, 13, 12, 14, 14)
        c.rect(3, 14, 12, 14, 9)
    if kind == 'sign':                                               # hanging iron plaque with an anvil
        c.rect(1, 2, 6, 2, 13)
        c.rect(5, 3, 6, 4, 13)
        c.rect(3, 4, 13, 12, 13)
        c.rect(4, 5, 12, 11, 9)
        c.rect(5, 6, 11, 10, 12)
        c.rect(5, 6, 11, 6, 14)
        for (x, y) in ((6, 7), (7, 7), (8, 7), (9, 7), (10, 7), (11, 7), (7, 8), (8, 8), (9, 8), (8, 9), (7, 10), (8, 10), (9, 10)):
            c.set(x, y, 13)
        c.set(11, 6, 10)
        c.set(12, 8, 11)
        c.set(12, 5, 10)
    if kind == 'door':                                               # iron-strapped oak door in a stone arch
        c.rect(2, 1, 13, 15, 13)
        c.rect(3, 2, 12, 15, 14)
        c.rect(4, 3, 11, 15, 6)
        for x in (5, 7, 9):
            c.rect(x, 3, x, 15, 5)
        c.rect(4, 3, 4, 15, 5)
        for y in (5, 11):
            c.rect(4, y, 11, y, 13)
        c.set(4, 3, 13)
        c.set(11, 3, 13)
        c.set(10, 9, 10)
        c.set(10, 10, 10)
        c.rect(2, 14, 13, 15, 12)
        c.rect(2, 14, 13, 14, 14)
        c.rect(2, 15, 13, 15, 9)
    return c


def wall_shop(kind):
    """green bank (11): cream plaster under a striped awning (9 red / 7 white), tan timber, glass 15, 12 tan, 11 gold."""
    c = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            c.set(x, y, 14 if H(x, y, 4) % 15 else 12)
    for x in range(16):                                              # awning: stripes with a scalloped lower edge
        stripe = (x // 4) % 2 == 0
        depth = [0, 2, 3, 2][x % 4]
        for y in range(0, 5 + depth):
            c.set(x, y, 9 if stripe else 7)
        c.set(x, 4 + depth, 10 if stripe else 8)
        c.set(x, 0, 11)
        for y in range(5 + depth, 7 + depth):                       # soft shadow the awning casts on the wall
            if y < 16:
                c.set(x, y, 8)
    for x in range(16):                                              # stone footing
        c.set(x, 14, 8)
        c.set(x, 15, 5 if x % 5 else 6)
    if kind == 'edge':
        for x0 in (0, 14):
            for y in range(0, 16):
                c.set(x0, y, 5)
                c.set(x0 + 1, y, 6)
    if kind == 'window':                                             # wide display window with goods on the sill
        c.rect(1, 7, 14, 14, 6)
        c.rect(2, 8, 13, 13, 5)
        c.rect(3, 8, 12, 12, 15)
        c.rect(3, 8, 7, 9, 7)
        c.rect(7, 8, 8, 13, 5)
        for (x, y, col) in ((4, 11, 9), (4, 10, 9), (5, 11, 9), (10, 11, 11), (10, 10, 11), (11, 11, 11), (11, 10, 4), (5, 10, 4)):
            c.set(x, y, col)
        c.rect(2, 13, 13, 13, 12)
    if kind == 'door':
        c.rect(2, 6, 13, 15, 6)
        c.rect(3, 7, 12, 15, 5)
        c.rect(4, 8, 11, 15, 12)
        for x in (6, 8, 10):
            c.rect(x, 8, x, 15, 5)
        c.rect(5, 9, 10, 11, 15)
        c.rect(5, 9, 6, 9, 7)
        c.rect(7, 9, 8, 11, 5) if False else None
        c.set(10, 13, 11)
        c.set(10, 14, 11)
        c.rect(2, 14, 13, 15, 8)
        c.rect(2, 15, 13, 15, 12)
    return c


def sign_hanging():
    """a wooden shop board for the awning's side: not used as a tile, kept for the preview sheet"""
    return Canvas(16, 16)


def roof_back(seed=1):
    """ground under the top row of a roof: plain grass; the roof itself is on the overlay layer"""
    c = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            c.set(x, y, (1, 1, 2, 1, 3)[H(x, y, 330 + seed) % 5])
    return c


# ----------------------------------------------------------------------------------------------- the 3x3 fountain
def _edge(plist, c, col, dirs=((1, 0), (-1, 0), (0, 1), (0, -1))):
    pset = set(plist)
    for (x, y) in plist:
        for dx, dy in dirs:
            if (x + dx, y + dy) not in pset:
                c.set(x, y, col)
                break


def fountain48():
    """tiered stone fountain, 48 x 48 (3x3 cells), bank 9: stone 1 light / 2 mid / 3 dark, 12 white, water 13/14/15.
    Same design as the old 2x2 one, scaled up with more rim stones and a taller spout. Returns (canvas, shadow cells)."""
    from art_village import pts
    big = Canvas(48, 48)
    cx = 23.5
    top = set(pts(cx, 22.8, 23.0, 17.5))
    bot = set(pts(cx, 27.2, 23.0, 17.5))
    hole = set(pts(cx, 23.6, 18.6, 13.2))
    water = set(pts(cx, 25.0, 17.8, 12.2)) & hole
    shadow = set(pts(cx + 1.4, 28.8, 22.6, 17.2)) - bot
    for (x, y) in bot - top:
        t = (x + 0.5 - cx) / 23.0
        big.set(x, y, 2 if t < 0.2 else 3)
    _edge(bot, big, 3, ((0, 1), (1, 0), (-1, 0)))
    for (x, y) in top - hole:
        t = ((x + 0.5 - cx) / 23.0) * -0.5 + ((y + 0.5 - 22.8) / 17.5) * -0.85
        big.set(x, y, 12 if t > 0.12 else (1 if t > -0.40 else 2))
    for k in range(16):                                              # sixteen stone joints across the rim
        ang = k * math.pi * 2 / 16 + 0.2
        for f in (0.80, 0.88, 0.96, 1.0):
            x = int(cx + math.cos(ang) * 21.0 * f)
            y = int(23.0 + math.sin(ang) * 15.5 * f)
            if (x, y) in top and (x, y) not in hole and big.get(x, y) in (1, 2, 12):
                big.set(x, y, {12: 1, 1: 2, 2: 3}[big.get(x, y)])
    _edge(top, big, 3)
    for (x, y) in hole:
        big.set(x, y, 2)
    for (x, y) in hole:
        if (x, y - 1) not in hole or (x, y - 2) not in hole:
            big.set(x, y, 3)
    for (x, y) in water:
        big.set(x, y, 13 + ((x + 2 * y + int(round(1.4 * math.sin(x * 0.6 + y * 0.4)))) % 6) // 2)
    for (x, y) in ((9, 29), (33, 30), (15, 35), (30, 35), (38, 24), (13, 23), (24, 38)):
        if (x, y) in water:
            big.set(x, y, 12)
    ped = set(pts(cx, 29.6, 7.0, 3.4))                               # pedestal in the water
    for (x, y) in ped:
        big.set(x, y, 1 if (x + 0.5 - cx) < 2.2 else 2)
    _edge(ped, big, 3, ((0, 1), (1, 0), (-1, 0)))
    for y in range(16, 28):                                          # column
        big.set(21, y, 3)
        big.set(22, y, 1)
        big.set(23, y, 1)
        big.set(24, y, 2)
        big.set(25, y, 3)
        big.set(26, y, 3)
    under = set(pts(cx, 17.4, 10.0, 3.6))                            # upper bowl
    bowl = set(pts(cx, 15.6, 10.0, 3.6))
    for (x, y) in under:
        big.set(x, y, 2)
    _edge(under, big, 3, ((0, 1), (1, 0), (-1, 0)))
    for (x, y) in bowl:
        big.set(x, y, 12 if (x + 0.5 - cx) < 1.5 else 1)
    for (x, y) in set(pts(cx, 16.0, 7.6, 1.9)):
        big.set(x, y, 14 if (x + y) % 2 else 15)
    for (x, y) in bowl:
        if (x, y - 1) not in bowl and (x + 1, y) in bowl:
            big.set(x, y, 2)
    for y in range(7, 15):                                           # spout and crown of water
        big.set(22, y, 15)
        big.set(23, y, 12 if y < 10 else 15)
    for (x, y) in ((22, 6), (23, 5), (23, 6), (21, 8), (24, 8), (20, 10), (25, 10), (19, 12), (26, 12)):
        big.set(x, y, 12 if y < 8 else 15)
    for side in (-1, 1):                                             # water pouring over the bowl into the basin
        x0 = int(cx) + side * 8
        for y in range(18, 24):
            big.set(x0, y, 15 if y % 2 == 0 else 14)
        for dx in (-1, 0, 1):
            big.set(x0 + dx, 24, 12 if dx == 0 else 15)
    return big, shadow


def fountain_cells():
    """the nine 16x16 cells of the fountain on plaza paving. Returns {'tl':..,'tm':..,...,'br':..}"""
    import art_world as AW
    big, shadow = fountain48()
    out = {}
    names = (('tl', 'tm', 'tr'), ('ml', 'mm', 'mr'), ('bl', 'bm', 'br'))
    for ry in range(3):
        for rx in range(3):
            c = Canvas(16, 16)
            for y in range(16):
                for x in range(16):
                    v = int(big.a[ry * 16 + y, rx * 16 + x])
                    if v == 0:
                        v = AW.plaza_pattern(x, y)
                        if (rx * 16 + x, ry * 16 + y) in shadow:
                            v = {1: 2, 2: 3, 3: 3}[v]
                    c.a[y, x] = v
            out[names[ry][rx]] = c
    return out


# ----------------------------------------------------------------------------------------------- the sealed south gate
def seal_tile(variant, row):
    """Thorny, ember-lit vines that choke the road while the Cinder Tyrant lives (slate bank 8: 13 black, 9 dark thorn,
    12 grey-brown, 10 yellow, 11 orange). Rows tile on top of each other; variants alternate across the gap."""
    c = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            c.set(x, y, 13 if H(x, y, 400 + variant) % 6 else 9)
    for k in range(-16, 40, 8):                                      # thick criss-crossing vines, lit from the left
        o = variant * 3
        for t in range(3):
            c.line(k + o + t, 0, k + 16 + o + t, 16, 9 if t == 2 else (12 if t == 0 else 9))
            c.line(k + 16 - o + t, 0, k - o + t, 16, 9 if t == 2 else (12 if t == 0 else 9))
    for (x, y) in ((2, 3), (6, 1), (11, 4), (14, 8), (4, 9), (9, 11), (13, 14), (1, 14), (7, 6), (12, 1)):   # thorns
        x, y = (x + variant * 5) % 16, (y + row * 3) % 16
        c.set(x, y, 12)
        c.set((x + 1) % 16, (y - 1) % 16, 12)
        c.set((x - 1) % 16, (y - 1) % 16, 13)
    for (x, y) in ((3, 5), (4, 6), (10, 9), (11, 10), (7, 13), (13, 3), (6, 2), (1, 10)):     # ember light in the cracks
        x, y = (x + variant * 4) % 16, (y + row * 5) % 16
        c.set(x, y, 11)
        c.set((x + 1) % 16, y, 10)
    return c


# =============================================================================================== tileset assembly
def _slice(ts, prefix, big, bank, rows, cols, flags_by_row=None, names=None, mm=None, mmo=None):
    """cut a (rows*16 x cols*16) canvas into metatiles named <prefix>_<r><c>; flags per row"""
    for r in range(rows):
        for c in range(cols):
            sub = Canvas(16, 16)
            sub.a[:, :] = big.a[r * 16:r * 16 + 16, c * 16:c * 16 + 16]
            nm = '%s_%d%d' % (prefix, r, c) if names is None else names[r][c]
            fl = 0 if flags_by_row is None else flags_by_row[r]
            ts.add(nm, sub, bank, fl, mm=mm, mmo=mmo)


def upgrade(ts):
    """add (or override by name) the redrawn world art on a freshly built outdoor tileset. Anything the maps do not use
    is dropped again by Tileset.compact(), so every area only pays for what it shows."""
    import art_world as AW
    import art_world2 as AW2
    SOLID, HALF = AW.SOLID, AW.HALF
    # ---- trees
    for v in range(3):
        for sp, art, bank in (('oaku', oak_unit(v), 6), ('pineu', pine_unit(v), 8)):
            nm = [['%s%d_tl' % (sp, v), '%s%d_tr' % (sp, v)], ['%s%d_bl' % (sp, v), '%s%d_br' % (sp, v)]]
            _slice(ts, sp, art, bank, 2, 2, [SOLID, SOLID], names=nm, mm=7)
    g = giant_oak2(0)
    names = [['goak2_t_l', 'goak2_t_m', 'goak2_t_r'], ['goak2_m_l', 'goak2_m_m', 'goak2_m_r'], ['goak2_f_l', 'goak2_trunk', 'goak2_f_r']]
    for r in range(3):
        for c in range(3):
            sub = Canvas(16, 16)
            sub.a[:, :] = g.a[r * 16:r * 16 + 16, c * 16:c * 16 + 16]
            if r < 2:
                ts.add(names[r][c], sub, 6, 0, mmo=7)
            elif c == 1:
                ts.add(names[r][c], sub, 6, SOLID, mm=7)
            else:
                ts.add(names[r][c], sub, 6, 0, mm=3)
    p = pine_single()
    for k, (r, nm) in enumerate((('0', 'pine2_top'), ('1', 'pine2_mid'), ('2', 'pine2_trunk'))):
        sub = Canvas(16, 16)
        sub.a[:, :] = p.a[k * 16:k * 16 + 16, :]
        if k < 2:
            ts.add(nm, sub, 8, 0, mmo=7)
        else:
            ts.add(nm, sub, 8, SOLID, mm=7)
    ts.add('treefill', pine_fill(), 8, SOLID, mm=7)
    for v, tag in enumerate('ab'):
        d = dead_tree(v)
        top, base = Canvas(16, 16), Canvas(16, 16)
        top.a[:, :] = d.a[:16, :]
        base.a[:, :] = d.a[16:, :]
        ts.add('dead2%s_top' % tag, top, 11, 0)
        ts.add('dead2%s_trunk' % tag, base, 11, SOLID, mm=8)
    # ---- ground props
    ts.add('rock', boulder(31), 7, SOLID, mm=8)
    ts.add('moss_rock', boulder(32, moss=True), 7, SOLID, mm=8)
    ts.add('bush', bush2(1), 6, SOLID | AW.CUT, mm=7)
    ts.add('fence_h', fence2('h'), 7, SOLID, mm=8)
    ts.add('fence_v', fence2('v'), 7, SOLID, mm=8)
    ts.add('planter', planter2(AW.grass_base(117, 2), (4, 5, 6), 7, flowers=(14, 15), pale=13), 10, SOLID, mm=3)
    ts.add('planter_plaza', planter2(AW.plaza_canvas(), (4, 5, 6), 7, flowers=(10, 11), leaf=(8, 9), pale=12), 9, SOLID)
    ts.add('planter_inlay', planter2(AW.plaza_canvas(True), (4, 5, 6), 7, flowers=(10, 11), leaf=(8, 9), pale=12), 9, SOLID)
    # ---- buildings: roofs (the top row lives on the overlay layer), walls, doors you can walk into
    for style, bank in (('r', 4), ('b', 5), ('s', 8), ('g', 11)):
        for side in 'lmr':
            ts.add('roof%s_%s0' % (style, side), AW.roof(side, 0), bank, SOLID, mm=9, mmo=9)
            ts.add('roof%s_%s1' % (style, side), AW.roof(side, 1), bank, SOLID, mm=9, mmo=9)
    ts.add('chimney_roof_s', AW2.chimney_roof(), 8, SOLID, mm=9, mmo=9)
    ts.add('chimney_roof_g', AW2.chimney_roof(), 11, SOLID, mm=9, mmo=9)
    ts.add('chimney_roof_r', AW2.chimney_roof(), 4, SOLID, mm=9, mmo=9)
    ts.add('chimney_roof_b', AW2.chimney_roof(), 5, SOLID, mm=9, mmo=9)
    ts.add('chimney_top_s', AW2.chimney_top(), 8, 0, mmo=9)
    ts.add('chimney_top_g', AW2.chimney_top(), 11, 0, mmo=9)
    ts.add('chimney_top_r', AW2.chimney_top(), 4, 0, mmo=9)
    ts.add('chimney_top_b', AW2.chimney_top(), 5, 0, mmo=9)
    ts.add('roofback', roof_back(), 0, SOLID | HALF, mm=9)
    ts.add('wall_door2', AW.wall('door'), 3, 0, mm=10)
    for k in ('plain', 'edge', 'window', 'sign'):
        ts.add('swall_' + k, wall_stone(k), 8, SOLID, mm=8)
    ts.add('swall_door', wall_stone('door'), 8, 0, mm=10)
    for k in ('plain', 'edge', 'window'):
        ts.add('gwall_' + k, wall_shop(k), 11, SOLID, mm=8)
    ts.add('gwall_door', wall_shop('door'), 11, 0, mm=10)
    # ---- the 3x3 fountain
    fc = fountain_cells()
    for nm, cv in fc.items():
        ts.add('fountain3_' + nm, cv, 9, SOLID, mm=5 if nm in ('tl', 'tr', 'bl', 'br') else 6)
    # ---- the sealed south gate (shown until the Cinder Tyrant dies)
    for row in (0, 1):
        for v, tag in enumerate('ab'):
            ts.add('seal_%s%d' % (tag, row), seal_tile(v, row), 8, SOLID, mm=8)
    return ts
