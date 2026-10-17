"""Dungeon tile art: flagstone floors, wall faces / caps / ledges, stairways and pillars.

Palette contract (indices are bank-relative):
  floor-backed banks (0, 4, 5): 1 slab, 2 slab light, 3 slab dark, 4 mortar. Everything that stands on a floor
  (pillar, coffin, chest...) uses floor_tile() as its background, so these four tones must match in those banks.
  bank 0 also carries 5 moss, 6 bone, 7 blood, 8/9 rune and 10-15 for stairways.
  bank 1 (walls): 1 brick, 2 lit edge, 3 shade, 4 mortar, 5 deep rock, 6 rock block, 7 cap stone,
                  11 abyss, 12 light brick, 13 dark brick, 14 brightest lip.
"""
import math
import random
from common import Canvas

H = lambda x, y, s=0: (x * 73856093 ^ y * 19349663 ^ s * 83492791) & 0xFFFF

# slab layouts: rectangles (x0, y0, x1, y1) that cover the 16x16 tile. Mortar runs along the top and left edge of
# every slab, so neighbouring tiles always join cleanly whatever their layout.
LAYOUTS = {
    'quad': [(0, 0, 7, 7), (8, 0, 15, 7), (0, 8, 7, 15), (8, 8, 15, 15)],
    'bond': [(0, 0, 7, 7), (8, 0, 15, 7), (0, 8, 15, 15)],
    'wide': [(0, 0, 15, 7), (0, 8, 7, 15), (8, 8, 15, 15)],
    'tall': [(0, 0, 7, 15), (8, 0, 15, 7), (8, 8, 15, 15)],
    'trip': [(0, 0, 10, 7), (11, 0, 15, 7), (0, 8, 4, 15), (5, 8, 15, 15)],
    'big': [(0, 0, 15, 15)],
}
LAYOUT_NAMES = ['quad', 'bond', 'wide', 'tall', 'trip', 'big']


def floor_tile(seed, layout='quad', kind='plain'):
    c = Canvas(16, 16)
    rng = random.Random(seed * 31 + 7)
    for (x0, y0, x1, y1) in LAYOUTS[layout]:
        tone = rng.choice((0, 0, 1, 2))            # 0 plain, 1 worn (dark flecks), 2 polished (light flecks)
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                col = 1
                if x == x0 or y == y0:
                    col = 4                         # mortar
                elif x == x0 + 1 or y == y0 + 1:
                    col = 2                         # lit top / left bevel
                elif x == x1 or y == y1:
                    col = 3                         # shaded bottom / right bevel
                elif tone == 1 and rng.random() < 0.22:
                    col = 3
                elif tone == 2 and rng.random() < 0.2:
                    col = 2
                elif rng.random() < 0.05:
                    col = 3
                c.set(x, y, col)
        # a chipped corner now and then
        if rng.random() < 0.35:
            cx, cy = rng.choice(((x0 + 2, y0 + 2), (x1 - 1, y1 - 1)))
            c.set(cx, cy, 4)
    if kind == 'crack':
        x, y = rng.randint(2, 5), rng.randint(1, 3)
        for _ in range(16):
            c.set(x, y, 4)
            if rng.random() < 0.5:
                c.set(x + 1, y, 3)
            x += rng.choice((0, 1, 1))
            y += rng.choice((1, 1, 0))
            if x > 14 or y > 14:
                break
    elif kind == 'moss':
        for _ in range(16):
            x, y = rng.randint(1, 14), rng.randint(1, 14)
            if c.get(x, y) in (1, 2, 3):
                c.set(x, y, 5)
                if rng.random() < 0.6:
                    c.set(x + 1, y, 5)
    elif kind == 'bones':
        c.line(3, 5, 8, 9, 6)
        c.line(4, 9, 9, 5, 6)
        c.rect(10, 10, 12, 12, 6)
        c.set(11, 11, 4)
    elif kind == 'rune':
        c.rect(3, 3, 12, 12, 8)
        c.rect(4, 4, 11, 11, 1)
        c.rect(7, 2, 8, 13, 9)
        c.rect(2, 7, 13, 8, 9)
        c.rect(7, 7, 8, 8, 8)
    elif kind == 'blood':
        for p in ((4, 5), (5, 5), (5, 6), (9, 10), (10, 10), (10, 11), (11, 11), (6, 7), (8, 9)):
            c.set(p[0], p[1], 7)
    return c


def shaded(tile):
    """a floor tile lying right under a wall face: ambient shadow fades out over its first rows"""
    c = tile.copy()
    for y in range(1, 6):
        for x in range(16):
            v = int(c.a[y, x])
            if y <= 3:
                c.a[y, x] = {1: 3, 2: 1, 3: 4}.get(v, v)
            else:
                c.a[y, x] = {2: 1}.get(v, v)
    return c


def shaded_side(tile, left):
    """a floor tile next to a side wall (wall on its left when left=True)"""
    c = tile.copy()
    xs = range(0, 4) if left else range(12, 16)
    for x in xs:
        for y in range(16):
            v = int(c.a[y, x])
            c.a[y, x] = {1: 3, 2: 1, 3: 4}.get(v, v)
    return c


# ---------------------------------------------------------------- walls (bank 1)
def wall_front(kind='plain'):
    """vertical brick face, three courses; lit lip on top so it reads as standing up from the floor"""
    c = Canvas(16, 16)
    rng = random.Random(11 if kind == 'plain' else 12)
    courses = [(1, 5), (6, 10), (11, 15)]
    for ci, (y0, y1) in enumerate(courses):
        off = 4 if ci % 2 else 0
        for x in range(16):
            joint = ((x + off) % 8) == 0
            tone = 1
            bx = (x + off) // 8
            r = H(bx, ci, 5) % 7
            tone = 12 if r == 0 else (13 if r in (1, 2) else 1)
            for y in range(y0, y1 + 1):
                if joint or y == y1:
                    col = 4
                elif y == y0:
                    col = 2                          # lit top of each brick
                elif y == y1 - 1:
                    col = 3
                else:
                    col = tone
                    if rng.random() < 0.08:
                        col = 3
                c.set(x, y, col)
    for x in range(16):
        c.set(x, 0, 14)                              # bright lip where the face meets the cap
    for x in range(16):
        c.set(x, 15, 11)                             # dark base line on the floor
    if kind == 'moss':
        for (x, y) in ((2, 3), (3, 3), (3, 4), (3, 5), (11, 8), (12, 8), (12, 9), (13, 9), (13, 10), (6, 12), (7, 12)):
            c.set(x, y, 8 if (x + y) % 2 else 9)
    if kind == 'skull':
        c.rect(5, 4, 10, 9, 2)
        c.rect(6, 5, 9, 8, 11)
        c.set(6, 6, 10)
        c.set(9, 6, 10)
        c.set(7, 8, 10)
        c.set(8, 8, 10)
    if kind == 'web':
        for i in range(0, 8):
            c.set(i, 1 + i // 2, 14)
            c.set(1 + i // 2, i, 14)
        for (x, y) in ((3, 2), (5, 4), (2, 4), (7, 6), (4, 3), (3, 5)):
            c.set(x, y, 14)
    if kind == 'torch':
        c.rect(7, 8, 8, 13, 4)                       # (bank 2) bracket + cycling flame colours 5,6,7
        c.rect(5, 7, 10, 8, 4)
        c.rect(6, 3, 9, 7, 5)
        c.rect(7, 2, 8, 6, 6)
        c.rect(7, 4, 8, 6, 7)
        c.set(7, 1, 5)
        c.set(8, 3, 6)
    return c


def wall_cap():
    """the top of a wall seen from above: a lighter band of cap stones with a lit front edge"""
    c = Canvas(16, 16)
    rng = random.Random(21)
    for y in range(16):
        for x in range(16):
            col = 7
            if y < 8 and (x + (0 if y < 4 else 4)) % 8 == 0 and y % 4 != 3:
                col = 13
            elif y % 4 == 3:
                col = 13 if y < 12 else 7
            elif rng.random() < 0.07:
                col = 6
            c.set(x, y, col)
    for x in range(16):
        c.set(x, 15, 12)
        c.set(x, 14, 12 if x % 5 else 14)
    return c


def wall_cap_s():
    """cap of a wall that has floor directly north of it: the lit lip faces the floor"""
    c = wall_cap()
    c.a = c.a[::-1, :].copy()
    return c


def wall_rock():
    """deep rock beyond the wall caps: dark with faint blocks so large masses are not a flat colour"""
    c = Canvas(16, 16)
    rng = random.Random(31)
    for y in range(16):
        for x in range(16):
            col = 5
            if (y % 8 == 0) or ((x + (4 if y >= 8 else 0)) % 8 == 0):
                col = 6
            elif rng.random() < 0.06:
                col = 6
            elif rng.random() < 0.02:
                col = 13
            c.set(x, y, col)
    return c


def wall_edge(floor_on_right):
    """side wall: the face of a wall whose neighbour to the left/right is floor"""
    c = wall_rock()
    xs = range(10, 16) if floor_on_right else range(0, 6)
    for x in xs:
        d = (x - 10) if floor_on_right else (5 - x)            # 0 at the rock edge .. 5 at the floor
        for y in range(16):
            col = 1
            if d == 0:
                col = 14 if floor_on_right else 3
            elif d == 1:
                col = 2 if floor_on_right else 1
            elif d >= 4:
                col = 11
            elif (y + (2 if d == 2 else 0)) % 5 == 4:
                col = 4
            elif d == 3:
                col = 3
            c.set(x, y, col)
    if not floor_on_right:
        # mirror lighting: lit edge on the floor side
        for y in range(16):
            c.set(5, y, 11)
    return c


# ---------------------------------------------------------------- stairways (32x16, bank 0)
def stairs(down=True):
    c = Canvas(32, 16)
    for y in range(16):
        for x in range(32):
            c.set(x, y, 1 if (x % 8 and y % 8) else 4)
    # stone frame
    c.rect(0, 0, 31, 0, 4)
    c.rect(1, 1, 30, 1, 2)
    c.rect(1, 1, 1, 15, 2)
    c.rect(30, 1, 30, 15, 3)
    c.rect(0, 0, 0, 15, 4)
    c.rect(31, 0, 31, 15, 4)
    treads = [13, 12, 11, 10] if down else [10, 11, 12, 13]
    ys = [(2, 4), (5, 7), (8, 10), (11, 14)]
    for i, (y0, y1) in enumerate(ys):
        t = treads[i]
        for x in range(3, 29):
            c.set(x, y0, t)                                   # tread
            c.set(x, y0 + 1, t)
            c.set(x, y1, 4 if down else 3)                    # riser shadow
        if not down:
            for x in range(3, 29):
                c.set(x, y0 + 1, 14 if i == 3 else t)
    if down:
        for x in range(3, 29):                                 # abyss glow at the bottom
            c.set(x, 14, 10)
            c.set(x, 13, 10)
    else:
        for x in range(3, 29):                                 # daylight spilling down the steps
            c.set(x, 2, 15)
            c.set(x, 3, 14)
    l, r = Canvas(16, 16), Canvas(16, 16)
    l.a = c.a[:, 0:16].copy()
    r.a = c.a[:, 16:32].copy()
    return l, r


# ---------------------------------------------------------------- props on floor (banks 4 / 5)
def pillar(seed=2):
    c = floor_tile(seed, 'quad')
    for y in range(3, 15):                                     # cast shadow to the right
        for x in range(12, 16):
            if c.get(x, y) in (1, 2):
                c.set(x, y, 3)
    c.rect(3, 12, 12, 14, 7)                                   # plinth
    c.rect(3, 12, 12, 12, 5)
    c.rect(3, 14, 12, 14, 8)
    c.rect(4, 11, 11, 11, 7)                                   # base ring
    for x in range(5, 11):                                     # fluted shaft
        col = 6 if x == 5 else (5 if x < 8 else (7 if x < 10 else 8))
        c.rect(x, 3, x, 10, col)
    c.rect(4, 2, 11, 2, 5)                                     # capital
    c.rect(3, 1, 12, 1, 6)
    c.rect(4, 3, 11, 3, 7)
    c.rect(3, 1, 3, 2, 5)
    c.rect(12, 1, 12, 2, 7)
    src = c.a.copy()                                           # outline
    for y in range(16):
        for x in range(16):
            if src[y, x] in (1, 2, 3):
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    xx, yy = x + dx, y + dy
                    if 0 <= xx < 16 and 0 <= yy < 16 and src[yy, xx] in (5, 6, 7, 8):
                        c.set(x, y, 4)
                        break
    return c


# ---------------------------------------------------------------- lava (bank 6 of the Cinder Hollows)
def lava_tile(seed):
    """glowing lava. 1 white-hot, 2-4 cycling orange/red, 5 crust, 6 deep crust (colours 2-4 are animated)."""
    c = Canvas(16, 16)
    rng = random.Random(seed * 17)
    for y in range(16):
        for x in range(16):
            n = math.sin((x + seed * 3) * 0.9) + math.sin(y * 1.3 + x * 0.4) + math.sin((x - y) * 0.7 + seed)
            c.set(x, y, 2 if n > 1.6 else (4 if n < -1.1 else 3))
    for _ in range(3):
        bx, by = rng.randint(1, 14), rng.randint(1, 14)
        c.ellipse(bx, by, rng.uniform(1.2, 2.4), rng.uniform(1.0, 1.8), 5)
        c.set(bx, by, 6)
    for _ in range(4):
        c.set(rng.randint(0, 15), rng.randint(0, 15), 1)
    for i in range(16):                                   # a darker rim so the pools read as sunken
        for e in (0, 15):
            if c.get(i, e) in (2, 3):
                c.set(i, e, 4)
            if c.get(e, i) in (2, 3):
                c.set(e, i, 4)
    return c
