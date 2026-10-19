"""House interiors: the Elder's house, the chapel, Tobin's shop and Garrick's forge (one 15x10-tile screen each).

Bank layout (BG palette banks of the 'interior' tileset):
  0 wood floor + rug      1 stone floor + stone walls      2 plaster walls      3 wooden furniture on a wood floor
  4 forge furniture on stone (iron, fire)    5 chapel furniture on stone    6 shop goods on a wood floor
Banks 3-6 start with the colours of the floor they stand on (1 light, 2 mid, 3 joint) so furniture tiles blend in.
"""
import random
from common import Canvas, hx
from art_world import Tileset, pal16, H, SOLID

AREA_OF = {'elder': 8, 'chapel': 9, 'shop': 10, 'forge': 11, 'inn': 12, 'alchemist': 13, 'study': 14}
ENTRY = {n: (7, 8) for n in ('elder', 'chapel', 'shop', 'forge', 'inn', 'alchemist', 'study')}      # where the hero arrives (just inside the door)
DOOR = (7, 9)

WOOD = ['#c08850', '#a8703c', '#7a4c2a']                    # floor planks light / mid / joint
STONE = ['#a0a0b4', '#8a8a9e', '#6c6c84']                   # flagstones light / mid / joint

IN_BANKS = {
    0: pal16(WOOD + ['#d8a868', '#e8c890', '#3a2418', '#a02838', '#c84858', '#701c28', '#f8d060', '#f0e8d0', '#4880d8', '#2c58a0', '#f8f8f0', '#5c3818']),
    1: pal16(STONE + ['#c8c8d4', '#50506a', '#383850', '#d8d8e4', '#f8d060', '#e8782c', '#c04420', '#2a2c38', '#7a8096', '#f8f8f0', '#4880d8', '#a8d8ff']),
    2: pal16(['#ecdcb4', '#d8c498', '#b8a078', '#9c6a38', '#6c4220', '#3a2418', '#c08850', '#f8f8f0', '#4880d8', '#a8d8ff', '#d84838', '#f8d060', '#58a050', '#8a5a30', '#2c58a0']),
    3: pal16(WOOD + ['#3a2418', '#d89858', '#b07038', '#80502c', '#d84838', '#f07858', '#a02828', '#f8f8f0', '#d8d8e0', '#f8d060', '#58a050', '#4880d8']),
    4: pal16(STONE + ['#2a2c38', '#8a90a4', '#6a7084', '#4c5468', '#f8d060', '#e8782c', '#c04420', '#f8f8f0', '#9c6a38', '#6c4220', '#c8ccd8', '#e04050']),
    5: pal16(STONE + ['#3a2a40', '#f8f8f0', '#d8d8e4', '#a8a8c0', '#f8d060', '#c8a038', '#4880d8', '#2c58a0', '#a8d8ff', '#d84838', '#9c6a38', '#6c4220']),
    6: pal16(WOOD + ['#3a2418', '#d89858', '#b07038', '#80502c', '#d84838', '#4880d8', '#58a050', '#f8d060', '#f8f8f0', '#b060f0', '#d8b878', '#a88850']),
    7: pal16(['#000000']),
}


# ------------------------------------------------------------------------------------------------ floors and walls
def floor_wood(v):
    c = Canvas(16, 16)
    for r in range(4):
        y0 = r * 4
        seam = 3 + (H(r, v, 71) % 10)
        for y in range(y0, y0 + 4):
            for x in range(16):
                if y == y0 + 3:
                    c.set(x, y, 3)
                elif x == seam:
                    c.set(x, y, 3)
                else:
                    light = (((x > seam) + r + v) % 2) == 0
                    c.set(x, y, 1 if light else 2)
        if H(r, v, 72) % 3 == 0:
            c.set((seam + 5) % 16, y0 + 1, 2 if (r + v) % 2 == 0 else 1)
    return c


def floor_stone(v):
    c = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            col = 1 if ((x // 8) + (y // 8) + v) % 2 == 0 else 2
            if x % 8 == 0 or y % 8 == 0:
                col = 3
            elif x % 8 == 1 or y % 8 == 1:
                col = 4 if ((x // 8) + (y // 8) + v) % 2 == 0 else col
            c.set(x, y, col)
    return c


def wall_plaster(part):
    """bank 2. 'top' = upper wall with a timber beam, 'base' = lower wall with skirting, 'win' = window"""
    c = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            c.set(x, y, 1 if H(x, y, 80) % 11 else 2)
    if part == 'top':
        c.rect(0, 0, 15, 2, 5)
        c.rect(0, 0, 15, 0, 4)
        c.rect(0, 3, 15, 3, 3)
        for x in (3, 11):
            c.rect(x, 4, x, 15, 2)
    if part == 'base':
        c.rect(0, 0, 15, 0, 2)
        c.rect(0, 11, 15, 15, 7)
        c.rect(0, 11, 15, 11, 4)
        c.rect(0, 15, 15, 15, 5)
        for x in (3, 11):
            c.rect(x, 1, x, 10, 2)
    if part == 'win':
        c.rect(0, 0, 15, 2, 5)
        c.rect(0, 0, 15, 0, 4)
        c.rect(3, 3, 12, 14, 5)
        c.rect(4, 4, 11, 13, 9)
        c.rect(4, 4, 7, 7, 10)
        c.rect(7, 4, 8, 13, 5)
        c.rect(4, 8, 11, 9, 5)
        c.rect(2, 3, 3, 12, 11)            # curtains
        c.rect(12, 3, 13, 12, 11)
        c.set(2, 13, 11)
        c.set(13, 13, 11)
        c.rect(2, 14, 13, 15, 7)
        c.rect(2, 14, 13, 14, 4)
    return c


def wall_stone(part):
    """bank 1: dressed stone back wall of the forge and the chapel"""
    c = Canvas(16, 16)
    for y in range(16):
        course, ry = divmod(y, 8)
        off = 8 * (course % 2)
        for x in range(16):
            sx = (x + off) % 16
            col = 2
            if ry == 7:
                col = 11
            elif sx == 0 or sx == 8 and False:
                col = 3
            elif ry == 0:
                col = 4
            elif H(x // 8 + course * 5, course, 6) % 4 == 0:
                col = 1
            c.set(x, y, col)
        if ry != 7:
            c.set((8 - off) % 16, y, 3)
    if part == 'top':
        c.rect(0, 0, 15, 1, 11)
        c.rect(0, 2, 15, 2, 3)
    if part == 'win':                                          # tall stained window: lead lines, blue and red glass
        c.rect(4, 0, 11, 15, 11)
        c.rect(5, 1, 10, 14, 13)
        for y in range(1, 15):
            for x in range(5, 11):
                c.set(x, y, 14 if (x + y) % 3 else 15)
        c.rect(7, 1, 8, 14, 11)
        c.rect(5, 6, 10, 7, 11)
        c.rect(5, 2, 6, 5, 10)
        c.rect(9, 8, 10, 12, 10)
        c.rect(7, 4, 8, 5, 8)
    return c


def wall_dark(kind):
    """bank 2: the dark timber frame of the room (sides and bottom)"""
    c = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            c.set(x, y, 5 if (x // 4) % 2 == 0 else 6)
    if kind == 'l':
        c.rect(15, 0, 15, 15, 4)
        c.rect(14, 0, 14, 15, 7)
    if kind == 'r':
        c.rect(0, 0, 0, 15, 4)
        c.rect(1, 0, 1, 15, 7)
    if kind == 'b':
        c.rect(0, 0, 15, 0, 4)
        c.rect(0, 1, 15, 1, 7)
    if kind == 'bl':
        c.rect(15, 0, 15, 15, 4)
        c.rect(0, 0, 15, 0, 4)
    if kind == 'br':
        c.rect(0, 0, 0, 15, 4)
        c.rect(0, 0, 15, 0, 4)
    return c


def door_in():
    """bank 0: the doorway seen from inside: timber posts, daylight, a worn mat"""
    c = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            c.set(x, y, 11)
    for y in range(16):
        for x in range(16):
            if y < 3:
                c.set(x, y, 12 if False else 15)
    c.rect(0, 0, 15, 2, 15)
    c.rect(0, 0, 2, 15, 15)
    c.rect(13, 0, 15, 15, 15)
    c.rect(3, 3, 12, 15, 11)
    c.rect(3, 3, 12, 5, 4)
    c.rect(2, 3, 2, 15, 6)
    c.rect(13, 3, 13, 15, 6)
    c.rect(3, 12, 12, 15, 9)
    c.rect(4, 13, 11, 14, 8)
    for x in range(4, 12):
        c.set(x, 13, 7 if x % 2 else 8)
    return c


def rug_big():
    """3x3 cells (48x48), bank 0: red rug with a gold border"""
    c = Canvas(48, 48)
    for y in range(48):
        for x in range(48):
            d = min(x, y, 47 - x, 47 - y)
            if d < 2:
                col = 9
            elif d < 4:
                col = 10
            elif d < 5:
                col = 9
            else:
                cx, cy = x - 23.5, y - 23.5
                col = 7
                if (abs(cx) + abs(cy)) % 12 < 1.2:
                    col = 8
                if abs(cx) + abs(cy) < 4:
                    col = 10
            c.set(x, y, col)
    return c


# ------------------------------------------------------------------------------------------------ furniture (wood, bank 3)
def _floor(bank_floor, v=0):
    return floor_wood(v) if bank_floor == 'wood' else floor_stone(v)


def shadow_rect(c, x0, y0, x1, y1, col=3):
    for y in range(y0, y1 + 1):
        for x in range(x0, x1 + 1):
            if c.get(x, y) in (1, 2):
                c.set(x, y, col)


def bed(part):
    """1x2 bed, head at the top (bank 3)"""
    c = floor_wood(3)
    if part == 't':
        c.rect(1, 2, 14, 15, 4)
        c.rect(2, 3, 13, 15, 7)
        c.rect(2, 3, 13, 3, 5)
        c.rect(3, 4, 12, 9, 11)                           # pillow
        c.rect(3, 4, 12, 4, 12)
        c.rect(3, 9, 12, 9, 12)
        c.rect(2, 10, 13, 15, 8)                           # blanket
        c.rect(2, 10, 13, 10, 9)
    else:
        c.rect(1, 0, 14, 12, 4)
        c.rect(2, 0, 13, 11, 8)
        c.rect(2, 0, 13, 1, 9) if False else None
        for x in range(3, 13, 3):
            c.rect(x, 3, x, 4, 10)
        c.rect(2, 11, 13, 11, 10)
        c.rect(1, 12, 14, 14, 4)
        c.rect(2, 12, 13, 13, 7)
        shadow_rect(c, 2, 15, 15, 15)
    return c


def table(part):
    """2x1 table with a loaf, a jug and a candle (bank 3)"""
    c = floor_wood(1)
    x0, x1 = (0, 15) if part == 'm' else ((1, 15) if part == 'l' else (0, 14))
    c.rect(x0, 3, x1, 13, 4)
    c.rect(x0, 4, x1, 12, 6)
    c.rect(x0, 4, x1, 5, 5)
    c.rect(x0, 11, x1, 12, 7)
    if part == 'l':
        c.rect(5, 6, 8, 8, 15) if False else None
        c.rect(4, 5, 7, 7, 12)                         # plate
        c.rect(5, 5, 6, 6, 13)
        c.set(11, 6, 11)
        c.rect(11, 3, 11, 5, 11)
        c.set(11, 2, 13)
    else:
        c.rect(5, 4, 8, 8, 14) if False else None
        c.rect(5, 6, 8, 9, 7)
        c.rect(5, 6, 8, 6, 5)
        c.rect(11, 5, 12, 7, 12)
    shadow_rect(c, x0, 14, x1, 15)
    return c


def bookshelf():
    c = floor_wood(2)
    c.rect(0, 0, 15, 15, 4)
    c.rect(1, 1, 14, 14, 7)
    for (y0, y1) in ((2, 6), (9, 13)):
        c.rect(2, y0, 13, y1, 4)
        c.rect(3, y0 + 1, 12, y1, 7)
        x = 3
        k = 0
        while x < 12:
            w = 2 + (H(x, y0, 9) % 2)
            col = (8, 15, 14, 13, 10)[(k + y0) % 5]
            c.rect(x, y0 + 1 + (k % 2), x + w - 1, y1, col)
            c.rect(x, y0 + 1 + (k % 2), x, y1, 4) if False else None
            x += w
            k += 1
    c.rect(1, 7, 14, 8, 5)
    return c


def chair():
    c = floor_wood(4)
    c.rect(3, 3, 12, 6, 4)                               # backrest
    c.rect(4, 4, 11, 5, 6)
    c.rect(3, 7, 12, 12, 4)
    c.rect(4, 8, 11, 11, 5)
    c.rect(4, 8, 11, 8, 13) if False else None
    c.rect(3, 12, 4, 14, 4)
    c.rect(11, 12, 12, 14, 4)
    shadow_rect(c, 3, 14, 13, 15)
    return c


def stool():
    c = floor_wood(5)
    c.rect(4, 6, 11, 11, 4)
    c.rect(5, 7, 10, 10, 6)
    c.rect(5, 7, 10, 7, 5)
    c.rect(5, 11, 6, 13, 4)
    c.rect(9, 11, 10, 13, 4)
    shadow_rect(c, 4, 14, 12, 15)
    return c


def barrel_in(bank_floor='wood'):
    c = _floor(bank_floor, 6)
    for y in range(2, 14):
        w = 6 if y in (2, 13) else 7
        c.rect(8 - w, y, 7 + w, y, 4)
    c.rect(2, 3, 13, 12, 6)
    c.rect(2, 3, 5, 12, 5)
    c.rect(11, 3, 13, 12, 7)
    c.rect(2, 5, 13, 5, 4)
    c.rect(2, 10, 13, 10, 4)
    c.rect(4, 2, 11, 3, 5)
    shadow_rect(c, 3, 14, 14, 15)
    return c


def crate_in(bank_floor='wood'):
    c = _floor(bank_floor, 7)
    c.rect(2, 3, 13, 14, 4)
    c.rect(3, 4, 12, 13, 6)
    c.rect(3, 4, 12, 5, 5)
    c.rect(3, 12, 12, 13, 7)
    c.rect(3, 4, 4, 13, 7)
    c.rect(11, 4, 12, 13, 7)
    for i in range(3, 13):
        c.set(i, 4 + (i - 3) * 9 // 9 if 4 + (i - 3) <= 13 else 13, 4)
    shadow_rect(c, 3, 15, 14, 15)
    return c


def plant_pot():
    c = floor_wood(8)
    c.rect(5, 9, 10, 14, 4)
    c.rect(6, 10, 9, 13, 8)
    c.rect(6, 10, 7, 13, 9)
    for (x, y, col) in ((8, 4, 14), (6, 5, 14), (10, 5, 14), (5, 7, 14), (11, 7, 14), (8, 6, 14), (7, 8, 14), (9, 8, 14)):
        c.rect(x - 1, y, x + 1, y + 1, col)
    c.set(8, 3, 14)
    c.set(8, 5, 4)
    shadow_rect(c, 5, 15, 11, 15)
    return c


def candle_stand():
    c = floor_wood(9)
    c.rect(7, 6, 8, 12, 4)
    c.rect(6, 12, 9, 13, 4)
    c.rect(7, 7, 7, 12, 6)
    c.rect(6, 4, 9, 6, 4)
    c.rect(7, 4, 8, 6, 12)
    c.set(7, 2, 13)
    c.set(8, 3, 13)
    c.set(8, 2, 13)
    c.set(7, 3, 9)
    shadow_rect(c, 6, 14, 10, 15)
    return c


def hearth(part):
    """2x2 fireplace in a stone breast, drawn over a stone floor colour set (bank 4): tl tr (mantle) / bl br (fire)"""
    c = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            c.set(x, y, 2 if H(x, y, 11) % 9 else 1)
    left = part in ('tl', 'bl')
    top = part in ('tl', 'tr')
    if top:
        c.rect(0, 0, 15, 15, 7)
        c.rect(0, 0, 15, 3, 6)
        c.rect(0, 12, 15, 15, 5)
        c.rect(0, 12, 15, 12, 14)
        for x in range(0, 16, 5):
            c.rect(x, 4, x, 11, 4)
        for y in (6, 9):
            c.rect(0, y, 15, y, 4)
        if left:
            c.rect(0, 0, 0, 15, 4)
        else:
            c.rect(15, 0, 15, 15, 4)
    else:
        c.rect(0, 0, 15, 15, 7)
        x0, x1 = (3, 15) if left else (0, 12)
        c.rect(x0, 2, x1, 15, 4)
        c.rect(x0 + 1, 3, x1 - 1, 15, 11) if False else None
        c.rect(x0 + 1, 3, x1 - 1, 15, 4)
        c.rect(x0 + 2, 4, x1 - 2, 15, 10)
        for x in range(x0 + 3, x1 - 2):
            h = 5 + (H(x, 3, 21) % 5)
            c.rect(x, 15 - h, x, 15, 9)
            c.rect(x, 15 - h + 3, x, 15, 8)
        c.rect(x0 + 5, 12, x1 - 5, 15, 11) if False else None
        c.rect(x0 + 2, 14, x1 - 2, 15, 4)
        for x in range(x0 + 2, x1 - 1, 2):
            c.set(x, 14, 10)
        if left:
            c.rect(0, 0, 2, 15, 6)
            c.rect(0, 0, 0, 15, 4)
        else:
            c.rect(13, 0, 15, 15, 6)
            c.rect(15, 0, 15, 15, 4)
    return c


# ------------------------------------------------------------------------------------------------ forge (bank 4)
def _stone_bg(seed):
    return floor_stone(seed)


def anvil():
    c = _stone_bg(1)
    c.rect(2, 4, 13, 7, 4)                               # top face and horn
    c.rect(3, 4, 12, 6, 14)
    c.rect(3, 4, 12, 4, 11)
    c.rect(0, 5, 3, 6, 4)
    c.rect(0, 5, 3, 5, 5)
    c.rect(4, 7, 11, 9, 4)                               # waist
    c.rect(5, 7, 10, 9, 6)
    c.rect(3, 10, 12, 13, 4)                             # base
    c.rect(4, 10, 11, 12, 7)
    c.rect(4, 10, 11, 10, 6)
    c.set(8, 3, 8)                                       # a spark
    c.set(10, 2, 9)
    shadow_rect(c, 4, 14, 14, 15)
    return c


def grindstone():
    c = _stone_bg(2)
    c.rect(2, 12, 13, 13, 4)                             # frame
    c.rect(3, 12, 4, 15, 13)
    c.rect(11, 12, 12, 15, 13)
    for y in range(2, 13):
        for x in range(3, 13):
            if (x - 7.5) ** 2 + (y - 7.5) ** 2 <= 22:
                edge = (x - 7.5) ** 2 + (y - 7.5) ** 2 > 16
                c.set(x, y, 4 if edge else (14 if x < 7 else 5))
    c.set(7, 7, 4)
    c.set(8, 7, 4)
    shadow_rect(c, 3, 15, 13, 15)
    return c


def weapon_rack(variant):
    c = _stone_bg(3)
    c.rect(0, 0, 15, 15, 2)
    c.rect(0, 0, 15, 15, 7)
    c.rect(1, 1, 14, 14, 13)
    c.rect(1, 1, 14, 2, 12)
    c.rect(1, 12, 14, 14, 12)
    if variant == 0:                                     # three swords
        for x in (3, 7, 11):
            c.rect(x, 3, x + 1, 9, 14)
            c.rect(x, 3, x, 9, 11)
            c.rect(x - 1, 9, x + 2, 9, 8)
            c.rect(x, 10, x + 1, 11, 15)
    else:                                                # axe, spear and shield
        c.rect(3, 3, 4, 12, 13) if False else None
        c.rect(3, 3, 3, 11, 12)
        c.rect(1, 3, 5, 5, 14)
        c.rect(1, 3, 2, 5, 11)
        c.rect(8, 3, 8, 12, 12)
        c.rect(7, 3, 9, 4, 14)
        c.set(8, 2, 11)
        c.rect(11, 4, 14, 10, 4)
        c.rect(11, 5, 13, 9, 15)
        c.rect(11, 5, 12, 6, 8)
    return c


def bellows():
    c = _stone_bg(4)
    c.rect(2, 5, 13, 12, 4)
    c.rect(3, 6, 12, 11, 12)
    c.rect(3, 6, 12, 7, 15)
    for x in (5, 8, 11):
        c.rect(x, 6, x, 11, 4)
    c.rect(0, 7, 2, 9, 4)
    c.rect(0, 8, 2, 8, 14)
    c.rect(6, 2, 8, 5, 13)
    shadow_rect(c, 3, 13, 14, 14)
    return c


def coal_pile():
    c = _stone_bg(5)
    for (cx, cy, r) in ((5, 11, 4), (10, 10, 4), (8, 7, 3)):
        for y in range(cy - r, cy + r + 1):
            for x in range(cx - r, cx + r + 1):
                if (x - cx) ** 2 + (y - cy) ** 2 <= r * r and 0 <= x < 16 and 0 <= y < 16:
                    c.set(x, y, 7 if (x + y) % 3 else 4)
    for (x, y) in ((5, 9), (11, 9), (8, 5), (7, 12)):
        c.set(x, y, 10)
    c.set(9, 6, 9)
    shadow_rect(c, 2, 15, 14, 15)
    return c


def workbench(part):
    c = _stone_bg(6)
    x0, x1 = (1, 15) if part == 'l' else (0, 14)
    c.rect(x0, 4, x1, 13, 4)
    c.rect(x0, 5, x1, 12, 12)
    c.rect(x0, 5, x1, 6, 14) if False else c.rect(x0, 5, x1, 6, 12)
    c.rect(x0, 11, x1, 12, 13)
    if part == 'l':
        c.rect(4, 3, 9, 4, 14)                          # a hammer and tongs
        c.rect(4, 2, 6, 3, 6)
        c.rect(11, 5, 12, 9, 5)
    else:
        c.rect(3, 4, 8, 5, 14)
        c.rect(10, 3, 12, 4, 15)
    shadow_rect(c, x0, 14, x1, 15)
    return c


# ------------------------------------------------------------------------------------------------ shop (bank 6)
def shop_counter(part):
    c = floor_wood(7)
    x0, x1 = (1, 15) if part == 'l' else ((0, 14) if part == 'r' else (0, 15))
    c.rect(x0, 2, x1, 14, 4)
    c.rect(x0, 3, x1, 7, 5)                                # top
    c.rect(x0, 3, x1, 3, 12) if False else None
    c.rect(x0, 8, x1, 13, 6)                               # front panels
    c.rect(x0, 8, x1, 8, 7)
    for x in range(x0 + 3, x1, 6):
        c.rect(x, 9, x, 12, 7)
    c.rect(x0, 13, x1, 13, 7)
    if part == 'l':                                        # potions
        for (x, col) in ((4, 8), (7, 9), (10, 10)):
            c.rect(x, 4, x + 1, 6, col)
            c.set(x, 3, 12)
            c.set(x + 1, 3, 4)
    elif part == 'm':
        c.rect(3, 5, 6, 6, 11)                             # coins
        c.rect(4, 4, 5, 4, 11)
        c.rect(9, 4, 12, 6, 12)                            # a ledger
        c.rect(9, 4, 12, 4, 4)
    else:
        c.rect(3, 4, 5, 6, 14)                             # a sack on the counter
        c.rect(3, 4, 5, 4, 15)
        c.rect(8, 3, 9, 6, 13)
        c.set(8, 3, 12)
    shadow_rect(c, x0, 15, x1, 15)
    return c


def shelf_goods(variant):
    c = floor_wood(2)
    c.rect(0, 0, 15, 15, 4)
    c.rect(1, 1, 14, 14, 7)
    for (y0, y1) in ((2, 6), (9, 13)):
        c.rect(2, y0, 13, y1, 4)
        c.rect(3, y0 + 1, 12, y1, 7)
        c.rect(3, y1, 12, y1, 5)
        for k, x in enumerate(range(3, 12, 3)):
            col = (8, 9, 10, 13, 11, 12)[(k + variant * 2 + y0) % 6]
            if (k + y0 + variant) % 3 == 0:               # a flat box
                c.rect(x, y1 - 3, x + 2, y1 - 1, 15)
                c.rect(x, y1 - 3, x + 2, y1 - 3, 14)
            else:                                           # a round bottle
                c.rect(x, y1 - 3, x + 2, y1 - 1, col)
                c.rect(x + 1, y1 - 4, x + 1, y1 - 4, 12)
                c.set(x, y1 - 3, 12)
    c.rect(1, 7, 14, 8, 5)
    return c


def sacks():
    c = floor_wood(9)
    for (cx, cy, r) in ((5, 11, 4), (11, 11, 4), (8, 7, 4)):
        for y in range(cy - r, cy + r + 1):
            for x in range(cx - r, cx + r + 1):
                if (x - cx) ** 2 + (y - cy) ** 2 <= r * r + 1:
                    edge = (x - cx) ** 2 + (y - cy) ** 2 > (r - 1) ** 2
                    c.set(x, y, 4 if edge else (14 if x <= cx else 15))
    c.rect(7, 3, 9, 4, 15)
    c.set(8, 3, 4)
    shadow_rect(c, 2, 15, 14, 15)
    return c


# ------------------------------------------------------------------------------------------------ chapel (bank 5)
def altar(part):
    c = _stone_bg(7)
    x0, x1 = (1, 15) if part == 'l' else ((0, 14) if part == 'r' else (0, 15))
    c.rect(x0, 3, x1, 14, 4)
    c.rect(x0, 4, x1, 13, 6)
    c.rect(x0, 4, x1, 6, 5)                                # white cloth
    c.rect(x0, 7, x1, 7, 8)                                # gold trim
    c.rect(x0, 8, x1, 13, 7)
    for x in range(x0 + 2, x1 - 1, 4):
        c.set(x, 10, 9)
        c.set(x, 11, 8)
    if part == 'm':                                        # a golden cross on the altar
        c.rect(7, 0, 8, 6, 8)
        c.rect(5, 2, 10, 3, 8)
        c.rect(7, 0, 7, 6, 9) if False else None
        c.set(8, 1, 9)
    elif part == 'l':
        c.rect(10, 2, 11, 6, 5)                            # candle
        c.set(10, 1, 8)
        c.set(11, 1, 8)
        c.set(10, 0, 8)
    else:
        c.rect(4, 2, 5, 6, 5)
        c.set(4, 1, 8)
        c.set(5, 1, 8)
        c.set(5, 0, 8)
    shadow_rect(c, x0, 14, x1, 15)
    return c


def pew(part):
    c = _stone_bg(8)
    x0, x1 = (1, 15) if part == 'l' else (0, 14)
    c.rect(x0, 3, x1, 6, 4)                                # backrest
    c.rect(x0, 4, x1, 5, 14)
    c.rect(x0, 7, x1, 12, 4)                               # seat
    c.rect(x0, 8, x1, 11, 15) if False else c.rect(x0, 8, x1, 11, 14)
    c.rect(x0, 8, x1, 8, 15) if False else None
    c.rect(x0, 11, x1, 11, 15)
    if part == 'l':
        c.rect(1, 3, 2, 13, 15)
    else:
        c.rect(13, 3, 14, 13, 15)
    shadow_rect(c, x0, 13, x1, 15)
    return c


def aisle():
    """red runner carpet with gold edges, walkable (bank 5)"""
    c = _stone_bg(9)
    c.rect(3, 0, 12, 15, 13)
    c.rect(3, 0, 3, 15, 8)
    c.rect(12, 0, 12, 15, 8)
    c.rect(4, 0, 4, 15, 9)
    c.rect(11, 0, 11, 15, 9)
    for y in range(2, 16, 5):
        c.rect(6, y, 9, y + 1, 8)
    return c


def candelabra():
    c = _stone_bg(10)
    c.rect(7, 5, 8, 12, 9)
    c.rect(5, 12, 10, 13, 8)
    c.rect(4, 4, 11, 5, 8)
    for x in (4, 7, 11):
        c.rect(x, 1, x, 3, 5)
        c.set(x, 0, 8)
    shadow_rect(c, 4, 14, 11, 15)
    return c


def cauldron():
    """iron pot of green brew on a tripod over embers (bank 3)"""
    c = floor_wood(11)
    c.rect(3, 10, 12, 13, 4)                              # embers
    c.rect(4, 11, 11, 12, 8)
    c.set(6, 11, 13)
    c.set(9, 12, 13)
    for (x0, x1, y) in ((3, 5, 4), (10, 12, 4)):          # tripod legs
        c.rect(x0, y, x0, 11, 4)
        c.rect(x1, y, x1, 11, 4)
    c.rect(2, 4, 13, 10, 4)                               # pot
    c.rect(3, 5, 12, 9, 12)
    c.rect(3, 5, 5, 8, 11) if False else c.rect(3, 5, 5, 8, 12)
    c.rect(3, 4, 12, 4, 4)
    c.rect(4, 3, 11, 5, 14)                               # brew
    c.rect(5, 3, 10, 3, 14)
    c.set(6, 2, 14)
    c.set(9, 1, 14)
    c.set(8, 3, 11)
    c.set(5, 4, 11)
    shadow_rect(c, 3, 14, 13, 15)
    return c


def herbs():
    """bunches of herbs and peppers hanging on a beam (bank 3), covers the whole tile"""
    c = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            c.set(x, y, 5 if H(x, y, 31) % 11 else 6)
    c.rect(0, 0, 15, 2, 7)
    c.rect(0, 0, 15, 0, 4)
    for (x, col, ln) in ((2, 14, 9), (6, 8, 7), (10, 14, 10), (13, 9, 6)):
        c.rect(x, 3, x, 4, 4)
        c.rect(x - 1, 5, x + 1, 5 + ln - 3, col)
        c.rect(x, 5 + ln - 2, x, 5 + ln, col)
        c.set(x - 1, 6, 11)
    return c


def globe():
    """a small globe on a stand (bank 3)"""
    c = floor_wood(12)
    for y in range(3, 11):
        for x in range(3, 13):
            if (x - 7.5) ** 2 + (y - 6.5) ** 2 <= 17:
                c.set(x, y, 4 if (x - 7.5) ** 2 + (y - 6.5) ** 2 > 12 else (15 if (x + y) % 5 else 14))
    c.rect(7, 11, 8, 12, 4)
    c.rect(5, 13, 10, 13, 4)
    c.rect(6, 12, 9, 12, 6)
    shadow_rect(c, 5, 14, 11, 15)
    return c


# ------------------------------------------------------------------------------------------------ tileset + rooms
def build_interior_tileset():
    ts = Tileset('interior', IN_BANKS)
    ts.add('blank', Canvas(16, 16), 7, SOLID, mm=1)
    A = ts.add
    for v in range(4):
        A('floor_wood%d' % v, floor_wood(v), 0, 0, mm=2)
        A('floor_stone%d' % v, floor_stone(v), 1, 0, mm=2)
    A('wp_top', wall_plaster('top'), 2, SOLID, mm=8)
    A('wp_base', wall_plaster('base'), 2, SOLID, mm=8)
    A('wp_win', wall_plaster('win'), 2, SOLID, mm=8)
    A('ws_top', wall_stone('top'), 1, SOLID, mm=8)
    A('ws_base', wall_stone('base'), 1, SOLID, mm=8)
    A('ws_win', wall_stone('win'), 1, SOLID, mm=8)
    for k in ('l', 'r', 'b', 'bl', 'br'):
        A('wd_' + k, wall_dark(k), 2, SOLID, mm=8)
    A('door_in', door_in(), 0, 0, mm=10)
    rug = rug_big()
    for ry, row in enumerate('tmb'):
        for rx, col in enumerate('lmr'):
            sub = Canvas(16, 16)
            sub.a[:, :] = rug.a[ry * 16:ry * 16 + 16, rx * 16:rx * 16 + 16]
            A('rug_' + row + col, sub, 0, 0, mm=9)
    A('bed_t', bed('t'), 3, SOLID, mm=8)
    A('bed_b', bed('b'), 3, SOLID, mm=8)
    A('table_l', table('l'), 3, SOLID, mm=8)
    A('table_r', table('r'), 3, SOLID, mm=8)
    A('bookshelf', bookshelf(), 3, SOLID, mm=8)
    A('chair', chair(), 3, SOLID, mm=8)
    A('stool', stool(), 3, SOLID, mm=8)
    A('barrel_in', barrel_in(), 3, SOLID, mm=8)
    A('crate_in', crate_in(), 3, SOLID, mm=8)
    A('plant_pot', plant_pot(), 3, SOLID, mm=8)
    A('candle_stand', candle_stand(), 3, SOLID, mm=8)
    A('cauldron', cauldron(), 3, SOLID, mm=8)
    A('herbs', herbs(), 3, SOLID, mm=8)
    A('globe', globe(), 3, SOLID, mm=8)
    for p in ('tl', 'tr', 'bl', 'br'):
        A('hearth_' + p, hearth(p), 4, SOLID, mm=14 if p in ('bl', 'br') else 8)
    A('anvil', anvil(), 4, SOLID, mm=8)
    A('grindstone', grindstone(), 4, SOLID, mm=8)
    A('rack_a', weapon_rack(0), 4, SOLID, mm=8)
    A('rack_b', weapon_rack(1), 4, SOLID, mm=8)
    A('bellows', bellows(), 4, SOLID, mm=8)
    A('coal_pile', coal_pile(), 4, SOLID, mm=8)
    A('bench_l', workbench('l'), 4, SOLID, mm=8)
    A('bench_r', workbench('r'), 4, SOLID, mm=8)
    A('counter_l', shop_counter('l'), 6, SOLID, mm=8)
    A('counter_m', shop_counter('m'), 6, SOLID, mm=8)
    A('counter_r', shop_counter('r'), 6, SOLID, mm=8)
    A('shelf_a', shelf_goods(0), 6, SOLID, mm=8)
    A('shelf_b', shelf_goods(1), 6, SOLID, mm=8)
    A('sacks', sacks(), 6, SOLID, mm=8)
    A('barrel_s', barrel_in(), 6, SOLID, mm=8)
    A('crate_s', crate_in(), 6, SOLID, mm=8)
    A('altar_l', altar('l'), 5, SOLID, mm=8)
    A('altar_m', altar('m'), 5, SOLID, mm=8)
    A('altar_r', altar('r'), 5, SOLID, mm=8)
    A('pew_l', pew('l'), 5, SOLID, mm=8)
    A('pew_r', pew('r'), 5, SOLID, mm=8)
    A('aisle', aisle(), 5, 0, mm=9)
    A('candelabra', candelabra(), 5, SOLID, mm=8)
    return ts


class Room:
    """one 15x10 screen; everything outside it stays blank (black)"""

    def __init__(self, ts, name):
        self.ts, self.name = ts, name
        self.g = {}
        self.o = {}
        self.spawns, self.npcs, self.chests, self.warps, self.signs = [], [], [], [], []
        self.start = ENTRY[name]
        self.biome = 'interior'

    def put(self, x, y, g, o=None):
        self.g[(x, y)] = g
        if o:
            self.o[(x, y)] = o

    def frame(self, wall):
        """back wall (two rows), dark timber sides and bottom, the doorway in the bottom middle"""
        top, base, win = ('wp_top', 'wp_base', 'wp_win') if wall == 'plaster' else ('ws_top', 'ws_base', 'ws_win')
        for x in range(1, 14):
            self.put(x, 0, top)
            self.put(x, 1, base)
        for y in range(0, 9):
            self.put(0, y, 'wd_l')
            self.put(14, y, 'wd_r')
        for x in range(1, 14):
            self.put(x, 9, 'wd_b')
        self.put(0, 9, 'wd_bl')
        self.put(14, 9, 'wd_br')
        self.put(DOOR[0], DOOR[1], 'door_in')
        return win

    def floor(self, style):
        for y in range(2, 9):
            for x in range(1, 14):
                self.put(x, y, 'floor_%s%d' % (style, H(x, y, 3) % 4))

    def rug(self, x0, y0, x1, y1):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                r = 't' if y == y0 else ('b' if y == y1 else 'm')
                c = 'l' if x == x0 else ('r' if x == x1 else 'm')
                self.put(x, y, 'rug_' + r + c)

    def finalize(self):
        ts = self.ts
        g = [[0] * 32 for _ in range(32)]
        o = [[0] * 32 for _ in range(32)]
        for (x, y), n in self.g.items():
            g[y][x] = ts.id(n)
        for (x, y), n in self.o.items():
            o[y][x] = ts.id(n)
        self.ground, self.overlay = g, o


def room_elder(ts):
    r = Room(ts, 'elder')
    win = r.frame('plaster')
    r.floor('wood')
    for x in (4, 10):
        r.put(x, 0, win)
    r.put(6, 0, 'hearth_tl')
    r.put(7, 0, 'hearth_tr')
    r.put(6, 1, 'hearth_bl')
    r.put(7, 1, 'hearth_br')
    for x in (1, 2, 11, 12):
        r.put(x, 1, 'bookshelf')
    r.rug(6, 4, 8, 6)
    r.put(13, 3, 'bed_t')
    r.put(13, 4, 'bed_b')
    r.put(1, 2, 'plant_pot')
    r.put(1, 7, 'candle_stand')
    r.put(13, 7, 'barrel_in')
    r.put(3, 5, 'table_l')
    r.put(4, 5, 'table_r')
    r.put(3, 6, 'chair')
    r.put(4, 6, 'stool')
    r.npcs = [('NPC_ELDER', 7, 3, 0)]
    return r


def room_chapel(ts):
    r = Room(ts, 'chapel')
    win = r.frame('stone')
    r.floor('stone')
    for x in (3, 7, 11):
        r.put(x, 0, win)
    r.put(6, 1, 'altar_l')
    r.put(7, 1, 'altar_m')
    r.put(8, 1, 'altar_r')
    r.put(5, 1, 'candelabra')
    r.put(9, 1, 'candelabra')
    for y in range(3, 9):
        r.put(7, y, 'aisle')
    for y in (5, 7):
        r.put(3, y, 'pew_l')
        r.put(4, y, 'pew_r')
        r.put(10, y, 'pew_l')
        r.put(11, y, 'pew_r')
    r.put(1, 2, 'candelabra')
    r.put(13, 2, 'candelabra')
    r.npcs = [('NPC_HEALER', 7, 2, 0)]
    return r


def room_shop(ts):
    r = Room(ts, 'shop')
    win = r.frame('plaster')
    r.floor('wood')
    r.put(7, 0, win)
    for i, x in enumerate((2, 3, 4, 5)):
        r.put(x, 1, 'shelf_a' if i % 2 == 0 else 'shelf_b')
    for i, x in enumerate((9, 10, 11, 12)):
        r.put(x, 1, 'shelf_b' if i % 2 == 0 else 'shelf_a')
    r.put(3, 4, 'counter_l')
    for x in range(4, 10):
        r.put(x, 4, 'counter_m')
    r.put(10, 4, 'counter_r')
    r.rug(6, 6, 8, 8)
    r.put(1, 2, 'barrel_s')
    r.put(1, 3, 'crate_s')
    r.put(2, 2, 'sacks')
    r.put(13, 2, 'barrel_s')
    r.put(13, 3, 'sacks')
    r.put(12, 6, 'table_l')
    r.put(13, 6, 'table_r')
    r.npcs = [('NPC_MERCHANT', 7, 3, 0)]
    return r


def room_forge(ts):
    r = Room(ts, 'forge')
    win = r.frame('stone')
    r.floor('stone')
    r.put(6, 0, 'hearth_tl')
    r.put(7, 0, 'hearth_tr')
    r.put(6, 1, 'hearth_bl')
    r.put(7, 1, 'hearth_br')
    r.put(2, 1, 'rack_a')
    r.put(3, 1, 'rack_b')
    r.put(11, 1, 'rack_a')
    r.put(12, 1, 'rack_b')
    r.put(5, 1, 'coal_pile')
    r.put(8, 1, 'bellows')
    r.put(9, 4, 'anvil')
    r.put(4, 4, 'grindstone')
    r.put(11, 6, 'bench_l')
    r.put(12, 6, 'bench_r')
    r.put(1, 7, 'barrel_in') if False else None
    r.put(1, 6, 'barrel_s') if False else None
    r.put(1, 2, 'bellows') if False else None
    r.npcs = [('NPC_SMITH', 7, 3, 0)]
    return r


def room_inn(ts):
    r = Room(ts, 'inn')
    win = r.frame('plaster')
    r.floor('wood')
    for x in (3, 11):
        r.put(x, 0, win)
    r.put(6, 0, 'hearth_tl')
    r.put(7, 0, 'hearth_tr')
    r.put(6, 1, 'hearth_bl')
    r.put(7, 1, 'hearth_br')
    r.put(4, 3, 'counter_l')
    for x in range(5, 10):
        r.put(x, 3, 'counter_m')
    r.put(10, 3, 'counter_r')
    for (x, y) in ((1, 2), (2, 2), (12, 2), (13, 2)):               # four beds against the side walls
        r.put(x, y, 'bed_t')
        r.put(x, y + 1, 'bed_b')
    r.rug(6, 6, 8, 8)
    r.put(10, 6, 'table_l')
    r.put(11, 6, 'table_r')
    r.put(10, 7, 'stool')
    r.put(11, 7, 'chair')
    r.put(1, 7, 'barrel_in')
    r.put(13, 7, 'plant_pot')
    r.npcs = [('NPC_INNKEEPER', 7, 2, 0)]
    return r


def room_alchemist(ts):
    r = Room(ts, 'alchemist')
    win = r.frame('plaster')
    r.floor('wood')
    r.put(7, 0, win)
    for i, x in enumerate((1, 2, 3, 4, 10, 11, 12, 13)):
        r.put(x, 1, 'shelf_a' if i % 2 == 0 else 'shelf_b')
    r.put(5, 1, 'herbs')
    r.put(9, 1, 'herbs')
    r.put(7, 3, 'cauldron')
    r.rug(5, 5, 9, 7)
    r.put(2, 5, 'table_l')
    r.put(3, 5, 'table_r')
    r.put(12, 5, 'bookshelf')
    r.put(1, 7, 'plant_pot')
    r.put(13, 7, 'plant_pot')
    r.put(12, 7, 'barrel_in')
    r.npcs = [('NPC_ALCHEMIST', 6, 3, 0)]
    return r


def room_study(ts):
    r = Room(ts, 'study')
    win = r.frame('plaster')
    r.floor('wood')
    r.put(7, 0, win)
    for x in (1, 2, 3, 4, 5, 9, 10, 11, 12, 13):
        r.put(x, 1, 'bookshelf')
    r.put(6, 4, 'table_l')
    r.put(7, 4, 'table_r')
    r.put(6, 5, 'chair')
    r.put(10, 4, 'globe')
    r.rug(5, 6, 9, 8)
    r.put(1, 3, 'bookshelf')
    r.put(13, 3, 'bookshelf')
    r.put(1, 7, 'candle_stand')
    r.put(13, 7, 'plant_pot')
    r.npcs = [('NPC_SCHOLAR', 8, 4, 2)]
    return r


def build_rooms(ts):
    """exit warps lead back to the village, in front of each door"""
    rooms = {'elder': room_elder(ts), 'chapel': room_chapel(ts), 'shop': room_shop(ts), 'forge': room_forge(ts),
             'inn': room_inn(ts), 'alchemist': room_alchemist(ts), 'study': room_study(ts)}
    return rooms


def set_exits(rooms, door_front):
    """door_front: {'elder': (x, y) outside the door, ...}: the hero steps out facing down"""
    for name, r in rooms.items():
        fx, fy = door_front[name]
        r.warps = [(DOOR[0], DOOR[1], DOOR[0], DOOR[1], 7 if name in ('inn', 'alchemist', 'study') else 0, fx, fy, 0)]
        r.finalize()
