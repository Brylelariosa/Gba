"""Extra outdoor art: richer grass, cast shadows, house details, village props and forest flora.
Everything here is registered into the outdoor Tileset by extend_outdoor(); art_maps.py places it."""
import math
import random
from common import Canvas
import art_world as AW
import art_props as AP
import art_village as AV
from art_world import grass_base, fill, SOLID, CUT, H

# palette banks 10 and 11 (outdoor.palette() already has room for them)
BANK_PROPS = 10     # warm props: wood, stone, hay, lantern, water
BANK_FLORA = 11     # forest flora: ferns, bark, mushrooms, glow flowers


def outline_around(c, solid, ink, idxs=(1, 2, 3)):
    """ink-coloured outline on background pixels (grass idx) that touch any pixel in `solid`."""
    src = c.a.copy()
    for y in range(c.h):
        for x in range(c.w):
            if src[y, x] in idxs:
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    xx, yy = x + dx, y + dy
                    if 0 <= xx < c.w and 0 <= yy < c.h and src[yy, xx] in solid:
                        c.set(x, y, ink)
                        break


# ---------------------------------------------------------------- grass
def grass_patch(kind, seed):
    c = grass_base(seed, 2 if kind == 'tall' else 4)
    r = random.Random(seed * 7 + 3)

    def rp():
        return r.randint(0, 15), r.randint(0, 15)
    if kind == 'light':
        for _ in range(18):
            c.set(*rp(), 2)
        for _ in range(5):
            c.set(*rp(), 14)
    elif kind == 'dark':
        for _ in range(16):
            c.set(*rp(), 3)
        for _ in range(4):
            x, y = rp()
            c.set(x, y, 13)
    elif kind == 'tall':
        for (x, y) in ((3, 7), (8, 4), (12, 9), (6, 11)):
            c.rect(x, y, x, y + 4, 4)
            c.set(x - 1, y + 1, 4)
            c.set(x + 1, y, 13)
            c.set(x + 1, y + 2, 13)
            c.set(x, y - 1, 14)
    elif kind == 'clover':
        for (x, y) in ((3, 4), (10, 8), (5, 12)):
            for dx, dy in ((0, 0), (1, 0), (0, 1), (1, 1)):
                c.set(x + dx, y + dy, 13)
            c.set(x, y, 14)
            c.set(x + 2, y + 1, 4)
            c.set(x - 1, y + 1, 4)
    elif kind == 'dandelion':
        for (x, y) in ((3, 4), (11, 7), (7, 12)):
            c.set(x, y, 6)
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                c.set(x + dx, y + dy, 5)
            c.set(x, y + 2, 4)
            c.set(x, y + 3, 4)
    elif kind == 'pebbles':
        for (x, y) in ((3, 5), (10, 9), (6, 12), (12, 3)):
            c.rect(x, y, x + 1, y, 12)
            c.set(x, y + 1, 3)
            c.set(x + 1, y + 1, 3)
            c.set(x, y - 1 if y > 0 else y, 10)
    return c


def shadow(kind):
    """cast shadow on grass (light comes from the top-left): dithered dark grass."""
    c = grass_base(5, 3)
    for y in range(16):
        for x in range(16):
            if kind == 'eave':
                t = y
            elif kind == 'side':
                t = x
            else:                      # corner: shadow in the top-left quadrant of the tile
                t = max(x, y) - 1
            if t < 2 or (t == 2 and (x + y) % 2 == 0) or (t == 3 and (x % 2 == 0 and y % 2 == 0)) or (t == 4 and x % 4 == 0 and y % 4 == 0):
                c.set(x, y, 3)
    return c


# ---------------------------------------------------------------- house details
def window_box():
    c = AW.wall('window')
    c.rect(3, 12, 12, 14, 4)
    c.rect(3, 12, 12, 12, 5)
    c.rect(3, 14, 12, 14, 6)
    for x, col in ((4, 14), (6, 13), (8, 14), (10, 13), (11, 14)):
        c.set(x, 11, col)
        c.set(x, 10, col if x % 2 == 0 else 13)
    return c


def step_stone():
    c = grass_base(9, 2)
    c.rect(3, 1, 12, 7, 5)          # bank 7: 4 light stone, 5 mid, 6 dark
    c.rect(3, 1, 12, 1, 4)
    c.rect(3, 7, 12, 7, 6)
    c.rect(12, 1, 12, 7, 6)
    c.set(6, 4, 6)
    c.set(9, 3, 6)
    return c


def chimney_roof(side_bank_red=True):
    """roof tile (middle, top row) with a brick chimney growing out of it."""
    c = AW.roof('m', 0)
    c.rect(9, 1, 13, 9, 5)          # roof banks 4/5: 5 brown, 12 tan, 6 dark brown, 13 darkest
    c.rect(9, 1, 9, 9, 12)
    c.rect(13, 1, 13, 9, 6)
    for y in (3, 6):
        c.rect(9, y, 13, y, 6)
    c.rect(8, 0, 14, 0, 13)
    return c


def chimney_top():
    """overlay above the roof row: the upper chimney and a puff of smoke (transparent background)."""
    c = Canvas(16, 16)
    c.rect(9, 7, 13, 15, 5)
    c.rect(9, 7, 9, 15, 12)
    c.rect(13, 7, 13, 15, 6)
    for y in (9, 12):
        c.rect(9, y, 13, y, 6)
    c.rect(8, 6, 14, 6, 13)
    c.rect(8, 7, 8, 7, 13)
    c.rect(14, 7, 14, 7, 13)
    for (x, y, col) in ((10, 3, 7), (11, 2, 7), (12, 3, 8), (11, 0, 7), (13, 1, 7), (10, 1, 8), (9, 4, 7)):
        c.set(x, y, col)
    return c


# ---------------------------------------------------------------- plaza / water variants
def plaza_variant(kind):
    c = AW.path_tile(True, True, True, True, True, True, True, True, style='stone')
    r = random.Random(77 if kind == 'crack' else 78)
    if kind == 'crack':
        x, y = 3, 2
        for _ in range(10):
            c.set(x, y, 12)
            x += r.choice((0, 1, 1))
            y += r.choice((1, 1, 0))
    elif kind == 'moss':
        for _ in range(9):
            c.set(r.randint(1, 14), r.randint(1, 14), r.choice((2, 3)))
        for (x, y) in ((4, 4), (10, 11)):
            c.rect(x, y, x + 1, y + 1, 3)
    elif kind == 'inlay':
        c = c.remap({10: 11, 11: 12})
    return c


def lily_water():
    c = AW.water_tile(True, True, True, True, True, True, True, True)
    for (cx, cy, rr) in ((4.5, 5.0, 3.0), (11.0, 10.5, 3.4)):
        c.ellipse(cx, cy, rr, rr * 0.75, 12)
        c.ellipse(cx - 0.6, cy - 0.5, rr * 0.65, rr * 0.45, 11)
        c.set(int(cx + rr * 0.6), int(cy), 4)         # the notch
    c.set(10, 9, 13)
    c.set(11, 9, 13)
    c.set(10, 10, 13)
    return c


def reeds():
    return AP.reeds(grass_base(13, 2), AW.P2_REEDS)


# ---------------------------------------------------------------- village props (bank 10)
def barrel():
    return AV.barrel(grass_base(111, 2))


def crate():
    c = grass_base(112, 2)
    c.rect(2, 3, 13, 13, 5)
    c.rect(2, 3, 13, 3, 4)
    c.rect(2, 13, 13, 13, 7)
    c.rect(2, 3, 2, 13, 4)
    c.rect(13, 3, 13, 13, 6)
    c.line(3, 4, 12, 12, 6)
    c.line(12, 4, 3, 12, 6)
    c.line(3, 5, 11, 12, 7) if False else None
    c.rect(2, 3, 13, 3, 7) if False else None
    outline_around(c, (4, 5, 6, 7), 7)
    return c


def hay():
    c = grass_base(113, 2)
    c.ellipse(7.5, 9.0, 7.0, 5.4, 12)
    c.ellipse(7.5, 8.4, 6.4, 4.6, 11)
    for (x0, y0, x1, y1) in ((2, 7, 6, 7), (8, 6, 12, 6), (4, 10, 9, 10), (10, 9, 13, 9), (3, 8, 4, 8)):
        c.line(x0, y0, x1, y1, 10)
    c.rect(7, 4, 8, 12, 6)
    outline_around(c, (11, 12), 7)
    return c


def well_base():
    return AV.well_base(grass_base(114, 2))


def well_roof():
    return AV.well_roof()


def lamp_base():
    return AV.lamp_base(grass_base(115, 2))


def lamp_top():
    return AV.lamp_top()


def bench(bg=None):
    c = bg if bg is not None else grass_base(116, 2)
    c.rect(1, 3, 14, 6, 5)
    c.rect(1, 3, 14, 3, 4)
    c.rect(1, 6, 14, 6, 6)
    c.rect(1, 8, 14, 10, 5)
    c.rect(1, 8, 14, 8, 4)
    c.rect(1, 10, 14, 10, 6)
    c.rect(2, 11, 3, 14, 7)
    c.rect(12, 11, 13, 14, 7)
    c.rect(2, 11, 2, 14, 6)
    outline_around(c, (4, 5, 6, 7), 7)
    return c


def planter(bg=None, leaf=(2, 3), flower=(13, 14)):
    c = bg if bg is not None else grass_base(117, 2)
    c.rect(2, 8, 13, 14, 5)
    c.rect(2, 8, 13, 8, 4)
    c.rect(2, 14, 13, 14, 7)
    c.rect(13, 8, 13, 14, 6)
    for x in range(3, 13, 2):
        c.rect(x, 5, x, 7, leaf[0])
        c.set(x + 1, 6, leaf[1])
    for (x, y, col) in ((3, 4, flower[1]), (5, 3, flower[0]), (7, 4, flower[1]), (9, 3, flower[0]), (11, 4, flower[1])):
        c.set(x, y, col)
        c.set(x, y + 1, col)
    outline_around(c, (4, 5, 6, 7), 7)
    return c


def garden(kind):
    c = Canvas(16, 16)
    fill(c, 6)
    for y in (3, 7, 11, 15):
        c.rect(0, y, 15, y, 7)
    for y in (2, 6, 10, 14):
        for x in range(0, 16, 4):
            xx = x + (y // 4) % 2 * 2
            if kind == 'a':                         # round leafy crops
                c.ellipse(xx + 1.5, y - 0.5, 1.8, 1.6, 2)
                c.set(xx + 1, y - 1, 3)
            else:                                    # carrot tops
                c.rect(xx, y - 2, xx, y, 3)
                c.set(xx - 1, y - 2, 2)
                c.set(xx + 1, y - 2, 2)
                c.set(xx, y + 1, 14)
    return c


def log_pile():
    c = grass_base(118, 2)
    for (cx, cy) in ((4.5, 10.5), (11.0, 10.5), (7.8, 5.5)):
        c.ellipse(cx, cy, 3.6, 3.6, 7)
        c.ellipse(cx, cy, 3.0, 3.0, 4)
        c.ellipse(cx, cy, 1.6, 1.6, 5)
        c.set(int(cx), int(cy), 6)
    outline_around(c, (4, 5, 7), 7)
    return c


# ---------------------------------------------------------------- forest flora (bank 11)
def fern(seed):
    c = grass_base(seed, 2)
    for (ang, ln) in ((-62, 6), (-32, 8), (0, 9), (32, 8), (62, 6)):
        a = math.radians(ang)
        x1, y1 = 7.5 + math.sin(a) * ln, 13 - math.cos(a) * ln
        c.line(7, 13, int(round(x1)), int(round(y1)), 5)
        c.set(int(round(x1)), int(round(y1)), 6)
        mx, my = (7.5 + x1) / 2, (13 + y1) / 2
        c.set(int(round(mx)) - 1, int(round(my)), 6)
        c.set(int(round(mx)) + 1, int(round(my)), 4)
    c.rect(6, 12, 8, 14, 4)
    c.set(7, 13, 5)
    return c


def log_fallen(side):
    c = grass_base(119, 2)
    c.rect(side == 'l' and 4 or 0, 6, side == 'l' and 15 or 11, 12, 7)
    x0 = 4 if side == 'l' else 0
    x1 = 15 if side == 'l' else 11
    c.rect(x0, 6, x1, 7, 8)
    c.rect(x0, 11, x1, 12, 9)
    for x in range(x0 + 2, x1, 4):
        c.set(x, 9, 9)
        c.set(x + 1, 8, 9)
    # moss on top
    for x in range(x0 + 1, x1, 3):
        c.set(x, 6, 14)
        c.set(x + 1, 6, 15)
    if side == 'l':
        c.ellipse(5.0, 9.0, 2.6, 3.4, 8)
        c.ellipse(5.0, 9.0, 1.6, 2.2, 7)
        c.set(5, 9, 8)
    else:
        c.ellipse(11.0, 9.0, 2.6, 3.4, 8)
        c.ellipse(11.0, 9.0, 1.6, 2.2, 7)
        c.set(11, 9, 8)
    outline_around(c, (7, 8, 9), 9)
    return c


def mushroom_big():
    c = grass_base(120, 2)
    for (x, y, rr) in ((4, 8, 3.2), (10, 6, 2.6), (8, 11, 2.2)):
        c.rect(int(x) - 1, int(y) + 1, int(x), int(y) + 4, 11)
        c.ellipse(x, y, rr, rr * 0.7, 10)
        c.set(int(x) - 1, int(y) - 1, 11)
        c.set(int(x) + 1, int(y), 11)
        c.set(int(x), int(y) + 1, 9)
    return c


def dead_tree():
    base = grass_base(121, 2)
    base.rect(6, 0, 9, 13, 7)
    base.rect(6, 0, 7, 13, 8)
    base.rect(9, 0, 9, 13, 9)
    base.rect(4, 12, 11, 14, 7)
    base.set(4, 14, 9)
    base.set(11, 14, 9)
    base.rect(5, 11, 5, 13, 8)
    base.set(8, 6, 9)
    base.set(7, 9, 9)
    top = Canvas(16, 16)
    top.rect(6, 8, 9, 15, 7)
    top.rect(6, 8, 7, 15, 8)
    top.rect(9, 8, 9, 15, 9)
    top.line(6, 9, 2, 3, 7)
    top.line(9, 8, 13, 2, 7)
    top.line(7, 8, 8, 0, 7)
    top.line(4, 5, 1, 6, 9)
    top.line(12, 4, 15, 5, 9)
    top.line(2, 3, 2, 1, 9)
    return base, top


def floor_leaves(seed, dense):
    c = grass_base(seed, 3)
    r = random.Random(seed)
    for _ in range(dense):
        c.set(r.randint(0, 15), r.randint(0, 15), r.choice((8, 9, 8)))
    for _ in range(4):
        c.set(r.randint(0, 15), r.randint(0, 15), 14)
    return c


def glow_flowers():
    c = grass_base(122, 2)
    for (x, y) in ((4, 5), (10, 4), (7, 10), (12, 12)):
        c.set(x, y, 12)
        c.set(x - 1, y, 13)
        c.set(x + 1, y, 13)
        c.set(x, y - 1, 13)
        c.set(x, y + 1, 12)
        c.set(x, y, 11)
        c.set(x, y + 2, 5)
    return c


def moss_rock():
    c = AW.rock()
    for (x, y) in ((5, 5), (6, 5), (7, 5), (6, 6), (9, 6), (10, 6), (4, 7), (5, 7)):
        if c.get(x, y) in (3, 4, 5, 6):
            c.set(x, y, 3 if (x + y) % 2 else 2)
    return c



# ---------------------------------------------------------------- tall trees (16x48 = trunk cell + 2 overlay rows)
def _leaf_outline(big, leafs, wood, ink_leaf, ink_wood):
    out = Canvas(big.w, big.h)
    out.a[:, :] = big.a
    for y in range(big.h):
        for x in range(big.w):
            if big.get(x, y) == 0:
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    v = big.get(x + dx, y + dy)
                    if v in leafs:
                        out.set(x, y, ink_leaf)
                    elif v in wood:
                        out.set(x, y, ink_wood)
    return out


def _slice_tree(out, bg_seed, blades, cells):
    """cut a 16-wide tree into (top, mid, trunk) 16x16 cells; the trunk cell gets a grass background."""
    top = Canvas(16, 16)
    top.a[:, :] = out.a[0:16, :]
    mid = Canvas(16, 16)
    mid.a[:, :] = out.a[16:32, :]
    trunk = grass_base(bg_seed, blades)
    for y in range(16):
        for x in range(16):
            v = out.get(x, 32 + y)
            if v:
                trunk.set(x, y, int(v))
    return top, mid, trunk


def _trunk(big, cx, y0, y1, half):
    """bark column with a lit left edge, dark right side, a few knots and flared roots (bank 6: 8 mid, 9 light, 10 dark)."""
    for y in range(y0, y1 + 1):
        flare = max(0, y - (y1 - 4)) * 0.9
        hw = half + flare
        for x in range(int(cx - hw), int(cx + hw) + 1):
            t = (x + 0.5 - cx) / hw
            big.set(x, y, 9 if t < -0.45 else (8 if t < 0.25 else 10))
    for k, (dx, dy) in enumerate(((-1, 3), (1, 7), (0, 11))):
        if y0 + dy < y1 - 3:
            big.set(int(cx) + dx, y0 + dy, 10)
            big.set(int(cx) + dx, y0 + dy + 1, 10)


def _apples(big, pts):
    for (x, y) in pts:
        if big.get(x, y) in (4, 5, 6):
            big.set(x, y, 13)
            big.set(x + 1, y, 13)
            big.set(x, y - 1, 14) if big.get(x, y - 1) in (4, 5, 6) else None


TALL_OAK_SHAPES = [
    [(8.0, 6.0, 5.6, 4.6), (4.2, 10.0, 4.4, 4.6), (11.8, 10.0, 4.4, 4.6), (8.0, 11.5, 6.8, 5.4),
     (4.0, 19.0, 4.8, 5.0), (12.0, 19.0, 4.8, 5.0), (8.0, 20.5, 7.4, 5.8),
     (4.6, 27.0, 5.0, 4.4), (11.4, 27.0, 5.0, 4.4), (8.0, 28.0, 7.6, 4.6)],
    [(7.0, 5.0, 5.0, 4.2), (11.5, 9.0, 4.2, 4.4), (4.5, 9.5, 4.4, 4.6), (8.0, 10.5, 6.6, 5.0),
     (4.2, 17.0, 4.6, 4.6), (11.8, 16.5, 4.6, 4.4), (8.2, 18.0, 7.2, 5.4),
     (5.0, 24.5, 5.2, 4.6), (11.2, 25.0, 5.0, 4.8), (8.0, 26.5, 7.6, 5.4)],
    [(9.0, 7.0, 5.2, 4.8), (5.0, 11.5, 4.6, 4.6), (11.6, 12.0, 4.4, 4.4), (8.0, 13.0, 6.8, 5.2),
     (4.4, 20.5, 4.6, 4.8), (12.0, 21.0, 4.4, 4.6), (8.0, 22.0, 7.0, 5.6),
     (5.2, 28.0, 4.8, 4.0), (11.0, 28.5, 4.8, 3.8), (8.0, 29.0, 7.4, 3.8)],
]


def tall_oak(seed=1, variant=0):
    """16x48 broadleaf: stacked lobes of leaf bunches on a short buttressed trunk. bank 6.
    Three variants with lobes at different heights, so a row of border trees does not read as horizontal stripes."""
    big = Canvas(16, 48)
    _trunk(big, 8, 33, 47, 2.6)
    AV.bunch_canopy(big, TALL_OAK_SHAPES[variant], seed=seed)
    _apples(big, ((6, 15), (10, 24), (4, 29), (11, 30)) if variant == 0 else (((9, 13), (5, 22), (11, 27)) if variant == 1 else ((7, 17), (11, 20), (5, 30))))
    out = _leaf_outline(big, (4, 5, 6, 7), (8, 9, 10), 7, 10)
    return _slice_tree(out, 11 + variant, 5, None)


def tall_pine():
    """16x48 conifer: four stacked tiers, narrow trunk. bank 8."""
    big = Canvas(16, 48)
    big.rect(7, 39, 8, 46, 8)
    big.set(8, 41, 10)
    big.set(8, 43, 10)
    big.rect(5, 46, 10, 47, 10)
    tiers = [(1, 14, 4.2), (9, 23, 6.0), (17, 32, 7.4), (25, 41, 8.2)]
    for (y0, y1, hw) in tiers:
        for y in range(y0, y1 + 1):
            t = (y - y0) / max(1, (y1 - y0))
            half = 0.8 + hw * t
            for x in range(max(0, int(8 - half)), min(16, int(8 + half) + 1)):
                col = 5
                if x < 8 - half * 0.30:
                    col = 6 if (x + y) % 4 else 7
                elif x > 8 + half * 0.25:
                    col = 4
                if y >= y1 - 1:
                    col = 4 if x > 7 else 5            # shaded tier underside
                big.set(x, y, col)
    out = _leaf_outline(big, (4, 5, 6, 7), (8, 9, 10), 12, 12)
    return _slice_tree(out, 12, 4, None)


def giant_oak(seed=5):
    """48x48 oak: canopy rows (3x2 metatiles, overlay) + trunk metatile (ground, centre). bank 6."""
    big = Canvas(48, 48)
    _trunk(big, 24, 30, 47, 4.2)
    AV.bunch_canopy(big, [(24.0, 7.5, 10.5, 6.5), (13.0, 12.0, 8.5, 6.5), (35.0, 12.0, 8.5, 6.5), (24.0, 13.5, 13.0, 8.5),
                          (8.0, 19.0, 7.5, 6.0), (40.0, 19.0, 7.5, 6.0), (16.0, 20.5, 9.5, 7.0), (32.0, 20.5, 9.5, 7.0),
                          (24.0, 21.5, 12.5, 7.0), (10.0, 25.5, 6.5, 4.6), (38.0, 25.5, 6.5, 4.6), (18.0, 26.0, 8.5, 4.8),
                          (30.0, 26.0, 8.5, 4.8), (24.0, 27.0, 10.5, 4.2)], seed=seed)
    _apples(big, ((9, 17), (21, 19), (33, 13), (40, 22), (28, 24), (15, 10), (36, 21)))
    out = _leaf_outline(big, (4, 5, 6, 7), (8, 9, 10), 7, 10)
    cells = {}
    for ry, name in ((0, 'top'), (1, 'mid')):
        for cx, side in ((0, 'l'), (1, 'm'), (2, 'r')):
            c = Canvas(16, 16)
            c.a[:, :] = out.a[ry * 16:ry * 16 + 16, cx * 16:cx * 16 + 16]
            cells['%s_%s' % (name, side)] = c
    trunk = grass_base(21, 5)
    for y in range(16):
        for x in range(16):
            v = out.get(16 + x, 32 + y)
            if v:
                trunk.set(x, y, int(v))
    cells['trunk'] = trunk
    return cells


# ---------------------------------------------------------------- registration
def extend_outdoor(ts):
    ts.add('grass_light', grass_patch('light', 11), 0)
    ts.add('grass_dark', grass_patch('dark', 12), 0)
    ts.add('grass_tall', grass_patch('tall', 13), 0)
    ts.add('grass_clover', grass_patch('clover', 14), 0)
    ts.add('grass_dand', grass_patch('dandelion', 15), 0)
    ts.add('grass_peb', grass_patch('pebbles', 16), 0)
    ts.add('shadow_eave', shadow('eave'), 0)
    ts.add('shadow_side', shadow('side'), 0)
    ts.add('window_box', window_box(), 3, SOLID)
    ts.add('step_stone', step_stone(), 7, 0)
    ts.add('chimney_roof_r', chimney_roof(), 4, SOLID)
    ts.add('chimney_roof_b', chimney_roof(), 5, SOLID)
    ts.add('chimney_top_r', chimney_top(), 4, 0)
    ts.add('chimney_top_b', chimney_top(), 5, 0)
    ts.add('plaza_crack', plaza_variant('crack'), 1, 0)
    ts.add('plaza_moss', plaza_variant('moss'), 1, 0)
    ts.add('plaza_inlay', plaza_variant('inlay'), 1, 0)
    ts.add('water_lily', lily_water(), 2, 3)
    ts.add('reeds', reeds(), 2, 0)
    ts.add('barrel', barrel(), 7, SOLID)
    ts.add('crate', crate(), BANK_PROPS, SOLID)
    ts.add('hay', hay(), BANK_PROPS, SOLID)
    ts.add('well_base', well_base(), 7, SOLID)
    ts.add('well_roof', well_roof(), 7, 0)
    ts.add('lamp_base', lamp_base(), 7, SOLID)
    ts.add('lamp_top', lamp_top(), 7, 0)
    ts.add('bench', bench(), BANK_PROPS, SOLID)
    ts.add('planter', planter(), BANK_PROPS, SOLID)
    ts.add('garden_a', garden('a'), BANK_PROPS, SOLID)
    ts.add('garden_b', garden('b'), BANK_PROPS, SOLID)
    ts.add('log_pile', log_pile(), BANK_PROPS, SOLID)
    ts.add('fern_a', fern(131), BANK_FLORA, 0)
    ts.add('fern_b', fern(132), BANK_FLORA, 0)
    ts.add('log_l', log_fallen('l'), BANK_FLORA, SOLID)
    ts.add('log_r', log_fallen('r'), BANK_FLORA, SOLID)
    ts.add('mushroom_big', mushroom_big(), BANK_FLORA, 0)
    base, top = dead_tree()
    ts.add('dead_trunk', base, BANK_FLORA, SOLID)
    ts.add('dead_top', top, BANK_FLORA, 0)
    ts.add('floor_leaves_a', floor_leaves(141, 9), BANK_FLORA, 0)
    ts.add('floor_leaves_b', floor_leaves(142, 16), BANK_FLORA, 0)
    ts.add('glow_flowers', glow_flowers(), BANK_FLORA, 0)
    ts.add('moss_rock', moss_rock(), 7, SOLID)
    ts.add('bench_plaza', bench(AW.plaza_canvas()), 9, SOLID)
    wide = Canvas(32, 16)
    for (ox, part) in ((0, 'l'), (16, 'r')):
        pc = AW.plaza_canvas()
        wide.blit(pc, ox, 0)
    wide = AV.bench_wide(wide)
    for (ox, part) in ((0, 'l'), (16, 'r')):
        half = Canvas(16, 16)
        half.a[:, :] = wide.a[:, ox:ox + 16]
        ts.add('bench_plaza_' + part, half, 9, SOLID)
    ts.add('planter_plaza', planter(AW.plaza_canvas(), (8, 9), (10, 11)), 9, SOLID)
    ts.add('planter_inlay', planter(AW.plaza_canvas(True), (8, 9), (10, 11)), 9, SOLID)
    ts.add('grass_block', grass_base(1), 0, SOLID)
    for v, nm in enumerate(('oak', 'oak2', 'oak3')):
        top, mid, trunk = tall_oak(1 + v, v)
        ts.add(nm + '_top', top, 6, 0)
        ts.add(nm + '_canopy', mid, 6, 0)
        ts.add(nm + '_trunk', trunk, 6, SOLID)
    top, mid, trunk = tall_pine()
    ts.add('pine_top', top, 8, 0)
    ts.add('pine_canopy', mid, 8, 0)
    ts.add('pine_trunk', trunk, 8, SOLID)
    g = giant_oak()
    for k, v in g.items():
        ts.add('goak_' + k, v, 6, SOLID if k == 'trunk' else 0)
