"""Redrawn village props: barrel, well, lamppost and the plaza bench.

Everything is lit from the top-left, has a dark outline and a soft shadow to the lower right, like the other props.
Palette indices are those of outdoor bank 7 (grass 1-3, iron/stone 4-6, wood 7-9, gold 10-11, pale 12, ink 14, white 15)
except the bench, which lives in bank 9 (the plaza bank: stone 1-3, wood 4-7).
"""
import math
from common import Canvas

# ---- bank 7 indices
S_L, S_M, S_D = 4, 5, 6            # light / mid / dark stone or iron
W_L, W_M, W_D = 7, 8, 9            # light / mid / dark wood
GOLD, GOLD_D, PALE, RED, INK, WHITE = 10, 11, 12, 13, 14, 15


def pts(cx, cy, rx, ry):
    """pixel cells whose centre lies inside the ellipse"""
    out = []
    for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
        for x in range(int(cx - rx) - 1, int(cx + rx) + 2):
            if ((x + 0.5 - cx) / rx) ** 2 + ((y + 0.5 - cy) / ry) ** 2 <= 1.0:
                out.append((x, y))
    return out


def shade_ground(c, plist, mapping):
    for (x, y) in plist:
        v = c.get(x, y)
        if v in mapping:
            c.set(x, y, mapping[v])


def ink_around(c, solid, ink, bg_ok=None):
    """ink outline on every non-solid pixel that touches a solid pixel (4-neighbourhood)"""
    src = c.a.copy()
    for y in range(c.h):
        for x in range(c.w):
            if int(src[y, x]) in solid:
                continue
            if bg_ok is not None and int(src[y, x]) not in bg_ok:
                continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                xx, yy = x + dx, y + dy
                if 0 <= xx < c.w and 0 <= yy < c.h and int(src[yy, xx]) in solid:
                    c.set(x, y, ink)
                    break


# ------------------------------------------------------------------------------------------------ barrel
def barrel(bg):
    """upright oak barrel: iron-rimmed lid, straight-ish staves, two dark hoops, soft shadow (16x16 on grass)"""
    c = bg
    shade_ground(c, pts(8.8, 13.6, 6.4, 2.0), {1: 3, 2: 1})
    b = Canvas(16, 16)
    cx = 7.5
    for y in range(5, 14):
        u = (y - 5) / 8.0
        hw = 5.3 + 0.6 * math.sin(math.pi * u) - (0.7 if y == 13 else 0.0)
        x0, x1 = int(math.floor(cx - hw + 0.5)), int(math.floor(cx + hw + 0.5)) - 1
        for x in range(x0, x1 + 1):
            t = (x + 0.5 - cx) / hw
            col = W_L if t < -0.42 else (W_M if t < 0.30 else W_D)
            k = x - x0
            if k in (3, 6, 9):                                      # stave seams
                col = W_M if col == W_L else W_D
            b.set(x, y, col)
    for hy in (8, 12):                                              # two thin iron hoops that follow the curve
        for x in range(0, 16):
            if b.get(x, hy) == 0:
                continue
            t = (x + 0.5 - cx) / 6.0
            y = hy + (1 if abs(t) < 0.42 else 0)
            col = S_M if t < -0.5 else S_D
            b.set(x, y, col)
        b.set(3, hy, S_L)
    lid = pts(cx, 4.2, 5.9, 3.0)
    inner = set(pts(cx, 4.2, 4.5, 2.0))
    for (x, y) in lid:
        t = ((x + 0.5 - cx) / 5.9) * -0.7 + ((y + 0.5 - 4.2) / 3.0) * -0.7
        if (x, y) in inner:
            tt = (x + 0.5 - cx) / 4.5
            b.set(x, y, W_L if tt < 0.15 else W_M)
        else:
            b.set(x, y, S_L if t > 0.3 else (S_M if t > -0.4 else S_D))
    for x in (6, 9):                                                # plank seams on the lid
        for y in range(3, 6):
            if (x, y) in inner:
                b.set(x, y, W_D)
    b.set(7, 4, W_D)                                                # bung
    ink_around(b, set(range(1, 16)), INK)
    c.blit(b, 0, 0)
    return c


# ------------------------------------------------------------------------------------------------ lamppost
LAMP_TOP2 = [
    "................",
    ".......oo.......",
    "......oggo......",
    ".....ogggGo.....",
    "....oggggGGo....",
    "...ogggggGGGo...",
    "..oooooooooooo..",
    "...oMppppppDo...",
    "...oMpwwwwpDo...",
    "...oMpwwwwpDo...",
    "...oMppwwppDo...",
    "...oMppppppDo...",
    "..oooMMMDDDooo..",
    ".....oMMDDo.....",
    "......oMDo......",
    "......oMDo......",
]

LAMP_BASE2 = [
    "......oMDo......",
    "......oMDo......",
    ".....oMMDDo.....",
    "......oMDo......",
    "......oMDo......",
    "......oMDo......",
    ".....oMMDDo.....",
    "....oMMMDDDo....",
    "...oLMMMDDDDo...",
    "..oLLMMMMDDDDo..",
    "..oooooooooooo..",
    "..oMMMMMDDDDDo..",
    "..oMMMMMDDDDDo..",
    "..oooooooooooo..",
    "................",
    "................",
]


def _key7():
    return dict(o=INK, L=S_L, M=S_M, D=S_D, g=GOLD, G=GOLD_D, p=PALE, w=WHITE)


def lamp_top(_M=None):
    c = Canvas(16, 16)
    c.art(LAMP_TOP2, 0, 0, _key7())
    return c


def lamp_base(bg, _M=None):
    c = bg
    shade_ground(c, pts(9.5, 12.6, 7.4, 2.0), {1: 3, 2: 1})
    c.art(LAMP_BASE2, 0, 0, _key7())
    return c


# ------------------------------------------------------------------------------------------------ well
def well_base(bg):
    """ground tile: stone cylinder with a dark mouth and the lower ends of the two posts (16x16)"""
    c = bg
    shade_ground(c, pts(8.6, 12.8, 7.8, 2.4), {1: 3, 2: 1})
    w = Canvas(16, 16)
    wall = pts(8.0, 9.2, 7.9, 4.3)
    for (x, y) in wall:                                              # front wall: lit left, shaded right
        t = (x + 0.5 - 8.0) / 7.9
        w.set(x, y, S_L if t < -0.45 else (S_M if t < 0.25 else S_D))
    for (x, y) in wall:                                              # two courses of blocks with staggered joints
        if y == 11 or (y == 10 and (x + 2) % 4 == 0) or (y >= 12 and (x + 0) % 4 == 0):
            w.set(x, y, S_D if w.get(x, y) != S_D else INK)
    rim = pts(8.0, 6.4, 7.9, 3.8)
    for (x, y) in rim:                                               # top ring of the cylinder
        t = ((x + 0.5 - 8.0) / 7.9) * -0.6 + ((y + 0.5 - 6.4) / 3.8) * -0.8
        w.set(x, y, WHITE if t > 0.45 else (S_L if t > -0.2 else S_M))
    for (x, y) in pts(8.0, 6.6, 5.3, 2.3):                           # the dark mouth
        w.set(x, y, S_D if y < 6 else INK)
    for (x, y) in ((6, 8), (7, 8), (9, 7), (10, 7)):
        w.set(x, y, PALE if x in (6, 10) else WHITE)                 # glint of water far below
    for x0 in (1, 13):                                               # posts stand on the rim at both sides
        for y in range(0, 6):
            w.set(x0, y, W_L)
            w.set(x0 + 1, y, W_M)
    ink_around(w, set(range(4, 10)), INK)
    c.blit(w, 0, 0)
    return c


def well_roof():
    """overlay tile above the well: a small shingled gable roof, the crossbeam, the tops of the posts and a hanging bucket"""
    c = Canvas(16, 16)
    for y in range(1, 6):
        half = 2.4 + (y - 1) * 1.4
        x0, x1 = int(math.floor(7.5 - half + 0.5)), int(math.floor(7.5 + half + 0.5)) - 1
        x0, x1 = max(0, x0), min(15, x1)
        for x in range(x0, x1 + 1):
            col = W_L if ((x + y * 2) % 4) < 2 else W_M            # staggered shingles
            if y in (3, 5):
                col = W_D if (x + y) % 3 == 0 else col            # course lines
            if x >= x1 - 1 and x1 - x0 > 5:
                col = W_M if col == W_L else W_D                   # right slope in shade
            c.set(x, y, col)
    c.rect(6, 0, 9, 0, W_D)                                       # ridge cap
    c.rect(6, 0, 7, 0, W_M)
    c.rect(0, 6, 15, 6, INK)                                      # eave
    c.rect(1, 7, 14, 7, W_D)                                      # underside in shadow
    for x0 in (1, 13):                                            # posts
        c.rect(x0, 8, x0, 15, W_L)
        c.rect(x0 + 1, 8, x0 + 1, 15, W_M)
    c.rect(1, 8, 14, 8, W_L)                                      # crossbeam
    c.rect(1, 9, 14, 9, W_D)
    c.rect(7, 10, 7, 12, W_L)                                     # rope
    c.rect(8, 10, 8, 12, W_M)
    c.rect(6, 13, 9, 15, W_M)                                     # bucket
    c.rect(6, 13, 6, 15, W_L)
    c.rect(9, 13, 9, 15, W_D)
    c.rect(6, 14, 9, 14, S_M)
    ink_around(c, set(range(4, 10)), INK, bg_ok={0})
    return c


# ------------------------------------------------------------------------------------------------ plaza bench (bank 9)
def bench_wide(bg32):
    """two metatiles wide (32x16) wooden park bench with a two-slat back, planked seat and end legs.
    bank 9: stone 1-3, wood 4 (light) 5 (mid) 6 (dark) 7 (ink)."""
    c = bg32
    L, M, D, O = 4, 5, 6, 7
    # shadow on the paving
    for (x, y) in pts(16.5, 14.2, 15.5, 1.9):
        v = c.get(x, y)
        if v in (1, 2):
            c.set(x, y, v + 1)
    # legs
    for x0 in (3, 26):
        c.rect(x0, 11, x0 + 2, 14, D)
        c.rect(x0, 11, x0, 14, M)
        c.rect(x0 - 1, 15, x0 + 3, 15, 0) if False else None
        c.rect(x0, 14, x0 + 2, 14, O)
        c.rect(x0 - 1, 11, x0 - 1, 14, O)
        c.rect(x0 + 3, 11, x0 + 3, 14, O)
    # backrest: two slats on posts
    c.rect(2, 0, 29, 0, O)
    c.rect(2, 1, 29, 1, L)
    c.rect(2, 2, 29, 3, M)
    c.rect(2, 4, 29, 4, D)
    c.rect(2, 5, 29, 5, M)
    c.rect(2, 6, 29, 6, D)
    c.rect(2, 7, 29, 7, O)
    c.rect(1, 1, 1, 6, O)
    c.rect(30, 1, 30, 6, O)
    for x in (2, 29):                                  # end caps of the slats
        c.rect(x, 1, x, 6, D)
    c.rect(2, 1, 2, 1, L)
    # seat
    c.rect(0, 8, 31, 8, O)
    c.rect(1, 9, 30, 9, L)
    c.rect(1, 10, 30, 10, M)
    c.rect(1, 11, 30, 11, D)
    c.rect(0, 9, 0, 11, O)
    c.rect(31, 9, 31, 11, O)
    c.rect(1, 12, 30, 12, O)
    for x in (9, 16, 23):                              # plank joints on the seat top
        c.rect(x, 9, x, 10, D)
    # back posts visible between slat and seat
    c.rect(5, 1, 6, 7, M) if False else None
    return c


# ------------------------------------------------------------------------------------------------ fountain (bank 9, 32x32)
def _edge(cells, c, color, sides=((1, 0), (-1, 0), (0, -1), (0, 1))):
    for (x, y) in cells:
        for dx, dy in sides:
            if (x + dx, y + dy) not in cells:
                c.set(x, y, color)
                break


def fountain32():
    """tiered stone fountain, 32x32, final bank-9 indices: stone 1 light / 2 mid / 3 dark, 12 white, water 13/14/15.
    Returns (canvas, shadow): shadow is the set of cells the caller should darken on the paving."""
    big = Canvas(32, 32)
    cx = 15.5
    top = set(pts(cx, 15.2, 15.3, 11.5))                  # outer edge of the rim
    bot = set(pts(cx, 18.2, 15.3, 11.5))                  # bottom edge of the basin wall
    hole = set(pts(cx, 15.7, 12.2, 8.6))                  # opening inside the rim
    water = set(pts(cx, 16.5, 11.7, 8.0)) & hole          # water surface
    shadow = set(pts(cx + 0.9, 19.0, 15.2, 11.4)) - bot
    # front wall: mid on the left, dark on the right, with a few cut-stone joints
    for (x, y) in bot - top:
        t = (x + 0.5 - cx) / 15.3
        big.set(x, y, 2 if t < 0.2 else 3)
    _edge(bot, big, 3, ((0, 1), (1, 0), (-1, 0)))
    # rim top face: light, shaded to mid on the lower right
    for (x, y) in top - hole:
        t = ((x + 0.5 - cx) / 15.3) * -0.5 + ((y + 0.5 - 15.2) / 11.5) * -0.85
        big.set(x, y, 12 if t > 0.12 else (1 if t > -0.40 else 2))
    for k in range(12):                                    # twelve stone joints, short radial strokes across the ring
        ang = k * math.pi * 2 / 12 + 0.26
        for f in (0.80, 0.90, 1.0):
            x = int(cx + math.cos(ang) * 14.0 * f)
            y = int(15.4 + math.sin(ang) * 10.2 * f)
            if (x, y) in top and (x, y) not in hole and big.get(x, y) in (1, 2, 12):
                big.set(x, y, {12: 1, 1: 2, 2: 3}[big.get(x, y)])
    _edge(top, big, 3)
    # opening: inner back wall (a thin shaded band), then the water
    for (x, y) in hole:
        big.set(x, y, 2)
    for (x, y) in hole:
        if (x, y - 1) not in hole or (x, y - 2) not in hole:
            big.set(x, y, 3)
    for (x, y) in water:
        big.set(x, y, 13 + ((x + 2 * y + int(round(1.2 * math.sin(x * 0.7 + y * 0.45)))) % 6) // 2)
    for (x, y) in ((6, 19), (22, 20), (10, 23), (20, 23), (25, 16), (9, 15)):
        if (x, y) in water:
            big.set(x, y, 12)
    # pedestal standing in the water
    ped = set(pts(cx, 19.6, 4.6, 2.3))
    for (x, y) in ped:
        big.set(x, y, 1 if (x + 0.5 - cx) < 1.5 else 2)
    _edge(ped, big, 3, ((0, 1), (1, 0), (-1, 0)))
    # column
    for y in range(12, 19):
        big.set(13, y, 3)
        big.set(14, y, 1)
        big.set(15, y, 1)
        big.set(16, y, 2)
        big.set(17, y, 3)
    # upper bowl: light stone rim with water inside, a darker underside
    under = set(pts(cx, 12.0, 6.2, 2.3))
    bowl = set(pts(cx, 10.7, 6.2, 2.3))
    for (x, y) in under:
        big.set(x, y, 2)
    _edge(under, big, 3, ((0, 1), (1, 0), (-1, 0)))
    for (x, y) in bowl:
        big.set(x, y, 12 if (x + 0.5 - cx) < 1 else 1)
    for (x, y) in set(pts(cx, 10.9, 4.6, 1.2)):
        big.set(x, y, 14 if (x + y) % 2 else 15)
    for (x, y) in bowl:
        if (x, y - 1) not in bowl and (x + 1, y) in bowl:
            big.set(x, y, 2)
    # spout with a little crown of water
    for y in range(5, 10):
        big.set(15, y, 15)
        big.set(16, y, 12 if y < 7 else 15)
    big.set(15, 4, 12)
    big.set(16, 4, 15)
    for (x, y) in ((14, 5), (17, 5), (13, 7), (18, 7)):
        big.set(x, y, 15)
    # water pouring over the bowl rim into the basin
    for side in (-1, 1):
        x0 = 16 + side * 5
        for y in range(12, 16):
            big.set(x0, y, 15 if y % 2 == 0 else 14)
        for dx in (-1, 0, 1):
            big.set(x0 + dx, 16, 12 if dx == 0 else 15)
    return big, shadow


# ------------------------------------------------------------------------------------------------ oak canopies (bank 6)
def bunch_canopy(c, ellipses, seed=1, tones=(6, 4, 5), rim=6, glint=11):
    """Leafy canopy built from many small round bunches of leaves instead of a few big smooth blobs.
    ellipses = the overall silhouette (union of ellipses); tones = (dark, mid, light) palette indices.
    Each bunch is lit from the top-left, and bunches lower / further right in the crown are in more shade."""
    import random
    rnd = random.Random(seed)
    mask = set()
    for e in ellipses:
        mask |= set(pts(*e))
    xs = [p[0] for p in mask]
    ys = [p[1] for p in mask]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    w, h = max(1, x1 - x0), max(1, y1 - y0)
    xc = (x0 + x1) / 2.0
    allowed = set(mask)
    for (x, y) in mask:
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            allowed.add((x + dx, y + dy))
    bayer = ((0.0, 0.5), (0.75, 0.25))
    dark, mid, light = tones
    for (x, y) in mask:                                     # base layer so no gaps show between bunches
        c.set(x, y, mid)
    bunches = []
    row = 0
    gy = y0 - 1.0
    while gy <= y1 + 1.0:
        gx = x0 - 1.5 + (1.7 if row % 2 else 0.0)
        while gx <= x1 + 1.5:
            cx, cy = gx + rnd.uniform(-0.7, 0.7), gy + rnd.uniform(-0.6, 0.6)
            if (int(cx), int(cy)) in mask:
                bunches.append((cx, cy, rnd.uniform(2.5, 3.5)))
            gx += 3.5
        gy += 2.8
        row += 1
    bunches.sort(key=lambda b: (b[1], b[0]))
    for (cx, cy, r) in bunches:
        g = 0.30 - 0.60 * (cy - y0) / h - 0.30 * (cx - xc) / max(1.0, w * 0.5)
        inside = set(pts(cx, cy, r, r * 0.92)) & allowed
        for (x, y) in inside:
            s = -((x + 0.5 - cx) / r) * 0.62 - ((y + 0.5 - cy) / (r * 0.92)) * 0.78 + g
            d = (bayer[y & 1][x & 1] - 0.4) * 0.22
            v = s + d
            c.set(x, y, light if v > 0.42 else (mid if v > -0.20 else dark))
        for (x, y) in inside:                               # dark rim on the lower-right edge of the bunch
            if (x + 1, y) not in inside or (x, y + 1) not in inside:
                if ((x + 0.5 - cx) + (y + 0.5 - cy)) > 0.4:
                    c.set(x, y, rim)
    if glint is not None:                                   # bright leaf glints on the lit side
        for (cx, cy, r) in bunches:
            if (cy - y0) / h < 0.7 and (cx - xc) < w * 0.25 and r > 2.7:
                gx, gy2 = int(cx - r * 0.35), int(cy - r * 0.45)
                if c.get(gx, gy2) == light:
                    c.set(gx, gy2, glint)
                    if c.get(gx + 1, gy2) == light:
                        c.set(gx + 1, gy2, glint)
    for (x, y) in mask:                                     # shaded underside of the whole crown
        if (x, y + 3) not in mask and c.get(x, y) != rim:
            v = c.get(x, y)
            if v == light:
                c.set(x, y, mid)
            elif v == mid and (x + y) % 2 == 0:
                c.set(x, y, dark)
    return bunches
