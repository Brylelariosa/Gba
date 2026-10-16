"""Hand-authored props: chests, signs, lamppost, pillar, shrubs and cattails.

Every sprite is drawn as ascii rows (see Canvas.art) with ONE light source (top-left), a dark outline, three body
tones and a highlight, so the props read the same in every area.  Each function takes the palette-index map for the
bank it is drawn in, so one drawing serves the village (bank 7 / 10), the crypt (dungeon bank 4) and the Cinder
Hollows (the same bank with scorched colours).
"""
import math
from common import Canvas

# ----------------------------------------------------------------------------------------------- chest
CHEST_CLOSED = [
    "................",
    "................",
    "....oooooooo....",
    "..ooaahhaaaaoo..",
    ".oaaaaaaaaaaaabo",
    ".oabgaaaaaagabco",
    ".oabgbbbbbbgbbco",
    ".ogggggGGgggggGo",
    ".oabgbboobbgbbco",
    ".obbgbbkkbbgbbco",
    ".obbgbbbbbbgbbco",
    ".occgccccccgccco",
    "..oooooooooooo..",
    "..ssssssssssss..",
    "................",
    "................",
]

CHEST_OPEN = [
    "................",
    "..ooooooooooo...",
    ".occcbbbbbbcco..",
    ".ocbbbbbbbbbbco.",
    ".ocbGggggggGbco.",
    ".oooooooooooooo.",
    ".oddddddddddddo.",
    ".odhdgdd*ddhddo.",
    ".oddgGdgdd*dddo.",
    ".ogggggGGgggggo.",
    ".obbgbbbbbbgbbo.",
    ".obbgbbbbbbgbbo.",
    ".occgccccccgcco.",
    "..oooooooooooo..",
    "..ssssssssssss..",
    "................",
]


def _paint(c, rows, key):
    c.art(rows, 0, 0, key)


def chest(c, closed, M):
    """c: canvas that already holds the background (grass / floor). M: palette indices for the symbols."""
    key = dict(M)
    key.setdefault('*', key['g'])
    _paint(c, CHEST_CLOSED if closed else CHEST_OPEN, key)


# ----------------------------------------------------------------------------------------------- sign
SIGN_ROWS = [
    "................",
    "................",
    "..oooooooooooo..",
    ".oaaaaaaaaaaaao.",
    ".oahhhhhhhhhhbo.",
    ".oatttttttttpbo.",
    ".oabbbbbbbbbbbo.",
    ".oatttttttpttbo.",
    ".oabbbbbbbbbbbo.",
    ".occcccccccccco.",
    "..oooo.cb.oooo..",
    ".......cb.......",
    ".......cb.......",
    "......occo......",
    "......ssss......",
    "................",
]


def sign(c, M):
    c.art(SIGN_ROWS, 0, 0, M)


# ----------------------------------------------------------------------------------------------- lamppost
# bank 10: 8 light iron, 9 dark iron, 7 outline, 13 hot glass, 10 glass, 11 gold cap
LAMP_TOP = [
    ".......oo.......",
    "......ogGo......",
    ".....ogggGo.....",
    "....oooooooo....",
    "....oiwwwwwo....",
    "....oiwhhhwo....",
    "....oiwhhhwo....",
    "....oiwwhwwo....",
    "....oiwwwwwo....",
    "....oooooooo....",
    ".....oiddo......",
    "......oido......",
    "......oido......",
    "......oido......",
    "......oido......",
    "......oido......",
]

# pole + flared foot: (y, x0, x1) -> outline at the ends, light iron on the left half, dark iron on the right half
LAMP_PROFILE = [(0, 6, 9), (1, 6, 9), (2, 6, 9), (3, 6, 9), (4, 5, 10), (5, 5, 10), (6, 4, 11), (7, 4, 11),
                (8, 3, 12), (9, 3, 12), (10, 3, 12)]


def lamp_top(M):
    c = Canvas(16, 16)
    c.art(LAMP_TOP, 0, 0, M)
    return c


def lamp_base(bg, M):
    c = bg
    for x in range(3, 15):                                          # shadow on the ground
        c.set(x, 12, M['s'])
        c.set(x, 13, M['s']) if x > 5 else None
    for (y, x0, x1) in LAMP_PROFILE:
        mid = (x0 + x1) // 2
        c.set(x0, y, M['o'])
        c.set(x1, y, M['o'])
        for x in range(x0 + 1, x1):
            c.set(x, y, M['i'] if x <= mid else M['d'])
    c.rect(3, 11, 12, 11, M['o'])                                   # bottom edge of the foot
    c.rect(4, 8, 5, 8, M['i'])
    c.set(4, 9, M['i'])
    return c


# ----------------------------------------------------------------------------------------------- pillar (2 tiles)
def pillar_parts(floor, M, seed=2):
    """Returns (base, cap): `base` is the solid ground tile (plinth + lower shaft) on the floor, `cap` the overlay
    tile above it (capital + upper shaft, transparent background) so the hero walks behind the tall column.
    M: o outline, l light, m mid, d dark, w pale highlight, s floor shadow tone."""
    cap = Canvas(16, 16)
    base = floor
    # shaft: 8px wide column x=4..11, lit from the left
    def shaft(c, y0, y1):
        for y in range(y0, y1 + 1):
            c.set(3, y, M['o'])
            c.set(12, y, M['o'])
            c.set(4, y, M['m'])
            c.set(5, y, M['w'] if (y % 5) else M['l'])
            c.set(6, y, M['l'])
            c.set(7, y, M['l'])
            c.set(8, y, M['m'])
            c.set(9, y, M['m'])
            c.set(10, y, M['d'])
            c.set(11, y, M['d'])
    # capital (overlay, rows 0..6)
    cap.rect(1, 0, 14, 0, M['o'])
    cap.rect(0, 1, 15, 1, M['o'])
    cap.rect(1, 1, 14, 1, M['l'])
    cap.rect(0, 2, 15, 2, M['o'])
    cap.rect(1, 2, 14, 2, M['m'])
    cap.set(1, 1, M['w'])
    cap.set(2, 1, M['w'])
    cap.rect(2, 3, 13, 3, M['o'])
    cap.rect(3, 3, 12, 3, M['d'])
    cap.rect(3, 4, 12, 4, M['o'])
    cap.rect(4, 4, 11, 4, M['m'])
    shaft(cap, 5, 15)
    cap.rect(4, 5, 11, 5, M['d'])
    cap.set(4, 5, M['m'])
    # base (ground tile, rows 0..15): shaft, torus, plinth, floor shadow
    for y in range(16):
        for x in range(13, 16):
            if base.get(x, y) in (1, 2):
                base.set(x, y, 3 if y > 2 else base.get(x, y))
    shaft(base, 0, 8)
    base.rect(2, 9, 13, 9, M['o'])
    base.rect(3, 9, 12, 9, M['m'])
    base.set(3, 9, M['l'])
    base.rect(2, 10, 13, 10, M['o'])
    base.rect(3, 10, 12, 10, M['d'])
    base.rect(1, 11, 14, 11, M['o'])
    base.rect(2, 11, 13, 11, M['l'])
    base.set(2, 11, M['w'])
    base.rect(1, 12, 14, 12, M['o'])
    base.rect(2, 12, 13, 12, M['m'])
    base.rect(1, 13, 14, 13, M['o'])
    base.rect(2, 13, 13, 13, M['d'])
    base.rect(2, 14, 13, 14, M['o'])
    for x in range(14, 16):
        base.set(x, 14, M['s'])
    return base, cap


# ----------------------------------------------------------------------------------------------- shrubs
def _shade(s, thr=(0.46, -0.12), dither=0.0):
    return 2 if s > thr[0] + dither else (1 if s > thr[1] + dither else 0)


def puffs(c, plist, tones, rim, glint=None, seed=0):
    """Draw overlapping round clumps back-to-front. tones = (dark, mid, light). Every clump is lit from the top-left
    and gets a dark rim along its lower-right edge so the clumps read as separate bunches of leaves."""
    bayer = ((0.0, 0.5), (0.75, 0.25))
    for (cx, cy, rx, ry) in plist:
        inside = set()
        for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
            for x in range(int(cx - rx) - 1, int(cx + rx) + 2):
                if ((x + 0.5 - cx) / rx) ** 2 + ((y + 0.5 - cy) / ry) ** 2 <= 1.0:
                    inside.add((x, y))
        for (x, y) in inside:
            s = -((x + 0.5 - cx) / rx) * 0.62 - ((y + 0.5 - cy) / ry) * 0.78
            d = (bayer[y & 1][x & 1] - 0.4) * 0.22
            c.set(x, y, tones[_shade(s, (0.42, -0.18), d)])
        for (x, y) in inside:                                       # rim: lower-right edge of this clump
            if (x + 1, y) not in inside or (x, y + 1) not in inside or (x + 1, y + 1) not in inside:
                if ((x + 0.5 - cx) + (y + 0.5 - cy)) > 0:
                    c.set(x, y, rim)
    if glint is not None:                                           # bright leaf glints on the lit side of each clump
        for (cx, cy, rx, ry) in plist:
            if rx >= 3.5:
                gx, gy = int(cx - rx * 0.38), int(cy - ry * 0.42)
                for k in range(2):
                    c.set(gx + k, gy, glint)
                c.set(gx - 1, gy + 1, glint)


def bush(bg, M, berry=None):
    """Round shrub of three leaf clumps with a soft shadow on the grass. M: dark, mid, light, rim, ink."""
    c = bg
    for x in range(2, 15):                                          # ground shadow
        for y in (13, 14):
            if c.get(x, y) in (1, 2):
                c.set(x, y, 3)
    big = Canvas(16, 16)
    puffs(big, [(8.0, 8.5, 6.8, 5.0), (5.0, 8.0, 4.2, 3.8), (11.0, 8.2, 4.4, 3.9), (8.0, 6.0, 4.6, 3.6)],
          (M['dark'], M['mid'], M['light']), M['rim'])
    out = Canvas(16, 16)
    out.a[:, :] = big.a
    for y in range(16):                                             # ink outline around the silhouette
        for x in range(16):
            if big.get(x, y) == 0:
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    if big.get(x + dx, y + dy):
                        out.set(x, y, M['ink'])
                        break
    c.blit(out, 0, 0)
    if berry:
        for (bx, by) in ((5, 7), (9, 5), (11, 9), (7, 10), (4, 10)):
            c.set(bx, by, berry[0])
            c.set(bx - 1, by - 1, berry[1]) if False else None
        c.set(5, 6, berry[1])
        c.set(9, 4, berry[1])
    return c


def reeds(bg, M):
    """cattails: tall blades with a leaf curl and brown seed heads. M: stem, leaf, head, headdark, sand."""
    c = bg
    for (x, h, bend) in ((3, 10, -1), (6, 12, 0), (9, 9, 1), (12, 11, 0)):
        for i in range(h):
            xx = x + (bend if i > h // 2 else 0)
            c.set(xx, 14 - i, M['stem'])
        c.set(x + 1, 14 - h // 3, M['leaf'])
        c.set(x - 1, 14 - h // 2 - 1, M['leaf'])
        xt = x + bend
        for k in range(3):
            c.set(xt, 14 - h - k + 1, M['head'])
        c.set(xt, 14 - h - 2 + 1, M['headdark'])
        c.set(xt, 14 - h - 3 + 1, M['tip'])
    for x in range(2, 14):
        c.set(x, 15, M['sand']) if c.get(x, 15) in (1, 2, 3) and x % 3 == 0 else None
    return c
