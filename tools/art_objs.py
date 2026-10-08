"""Weapons, enemies, boss, projectiles, FX, items, UI sprites."""
import math
from common import Canvas, pal16

# ------------------------------------------------------------------ palettes
PAL_WEAPON = pal16(['#2a1a2e', '#f4f8ff', '#b8c4dc', '#6c7894', '#f0c840', '#a87818', '#9a6a38', '#5c3a1c',
                    '#c8f0ff', '#58a8f0', '#2a58b8', '#e8ffff', '#a02830', '#e8e0c8', '#ffffff'])
PAL_SLIME_G = pal16(['#1c3a20', '#58d060', '#98f088', '#2c9040', '#ffffff', '#e04060', '#ffd0d0'])
PAL_SLIME_B = pal16(['#1a2a50', '#5090e8', '#90c8ff', '#2858b8', '#ffffff', '#e04060', '#ffd0d0'])
PAL_BAT = pal16(['#1c1030', '#7a4ab8', '#a878e0', '#4a2880', '#5a3898', '#8a60c8', '#ff4040', '#ffffff'])
PAL_WOLF = pal16(['#241c28', '#8c8c9c', '#b8b8c8', '#5c5c70', '#d8d8e4', '#f8d040', '#e04848', '#ffffff'])
PAL_BOSS = pal16(['#1a0a24', '#e0dcc8', '#a8a090', '#5a2a8a', '#8a4ac0', '#321458', '#f0c840', '#a87818',
                  '#60ff90', '#c8ffd8', '#b080ff', '#7a5a30', '#e03050', '#ffffff', '#2a1040'])
PAL_FX = pal16(['#00ff00', '#005000', '#000000', '#ffffff', '#fff098', '#d8d8e0', '#80808c', '#ff9020',
                '#ffe040', '#e03018', '#58d058', '#2c7c34', '#60b0ff', '#b060f0', '#a07848'])
PAL_ITEM = pal16(['#ffe040', '#a07000', '#f8c820', '#fff8a0', '#c88010', '#e83048', '#ff8090', '#901830',
                  '#40a0ff', '#a0d8ff', '#ff70c0', '#e8f4ff', '#d8c060', '#806810', '#241830'])
PAL_UI = pal16(['#ffffff', '#303048', '#f0c040', '#a07018', '#fff4d0', '#e04848', '#4890e0', '#58c860',
                '#c0c8e0', '#686880', '#ff8020', '#101020', '#ffe880', '#d8a028', '#888898'])
PAL_FLASH = pal16(['#ffffff'] * 15)


def circle_pts(cx, cy, r, a0, a1, n=200):
    pts = []
    for i in range(n + 1):
        a = math.radians(a0 + (a1 - a0) * i / n)
        pts.append((cx + r * math.sin(a), cy - r * math.cos(a)))
    return pts


# ------------------------------------------------------------------ weapons (32x32, pivot 16,16, pointing up)
def weapon(kind):
    c = Canvas(32, 32)
    O, SL, SM, SD, GO, GD, WO, WD, OL_, OM, OD, GL, LE, ST, WH = range(1, 16)
    if kind == 'sword':
        for y in range(3, 15):
            c.set(15, y, SM)
            c.set(16, y, SL)
            c.set(17, y, SD)
        c.set(16, 2, SL)
        c.set(15, 3, SL)
        c.set(16, 1, SL)
        c.rect(11, 15, 21, 16, GO)
        c.rect(11, 16, 21, 16, GD)
        c.set(10, 15, GD)
        c.set(22, 15, GD)
        c.rect(15, 17, 17, 20, WO)
        c.rect(17, 17, 17, 20, WD)
        c.rect(15, 21, 17, 21, GO)
        c.set(16, 22, GD)
    elif kind == 'staff':
        c.rect(15, 7, 16, 25, WO)
        c.rect(16, 7, 16, 25, WD)
        c.rect(14, 12, 17, 12, GO)
        c.rect(14, 22, 17, 22, GO)
        c.ellipse(16, 4.5, 3.4, 3.4, OM)
        c.ellipse(15.5, 3.5, 1.5, 1.5, OL_)
        c.set(18, 6, OD)
        c.set(17, 7, OD)
        c.set(14, 7, GD)
        c.set(18, 7, GD)
        for p in ((12, 4), (20, 4), (16, 0), (13, 1), (19, 1)):
            c.set(p[0], p[1], GL)
        c.rect(15, 26, 16, 26, GO)
    elif kind == 'bow':
        for x, y in circle_pts(16, 27, 11, -78, 78):
            c.set(round(x), round(y), WO)
            c.set(round(x) + 1, round(y), WD)
        c.rect(15, 15, 17, 18, GD)
        c.line(6, 21, 16, 28, ST)
        c.line(26, 21, 16, 28, ST)
        c.rect(16, 7, 16, 28, WO)
        c.set(16, 5, SL)
        c.set(15, 6, SM)
        c.set(17, 6, SM)
        c.set(16, 6, SM)
        c.set(15, 27, LE)
        c.set(17, 27, LE)
        c.set(15, 28, LE)
        c.set(17, 28, LE)
    elif kind == 'dagger':
        for y in range(8, 15):
            c.set(15, y, SM)
            c.set(16, y, SL)
            c.set(17, y, SD)
        c.set(16, 7, SL)
        c.set(16, 6, SL)
        c.rect(13, 15, 19, 15, GO)
        c.rect(13, 16, 19, 16, GD)
        c.rect(15, 17, 17, 20, LE)
        c.rect(16, 21, 16, 21, GD)
    c.outline(O)
    return c


def arc_sprite():
    """crescent trailing behind the blade (blade at angle 0 = up, trail to the left/ccw)."""
    c = Canvas(32, 32)
    for r in range(8, 16):
        for i in range(0, 241):
            a = -85 + 85 * i / 240.0
            rr = r
            # taper: thinner toward the trailing end
            t = (a + 85) / 85.0
            lo = 15 - 7 * t
            if r < lo - 0.5:
                continue
            x = 16 + rr * math.sin(math.radians(a))
            y = 16 - rr * math.cos(math.radians(a))
            core = r >= 13 and t > 0.3
            c.set(int(round(x)), int(round(y)), 15 if core else 12)
    return c


def shadow(wide=True):
    if wide:
        c = Canvas(16, 8)
        c.ellipse(7.5, 4.0, 6.6, 2.7, 3)
    else:
        c = Canvas(8, 8)
        c.ellipse(3.5, 4.0, 3.4, 1.8, 3)
    return c


# ------------------------------------------------------------------ enemies
def slime(frame):
    c = Canvas(16, 16)
    OLn, M, L, D, W, MO, T = range(1, 8)
    specs = [(8, 10, 6.4, 5.0), (8, 11, 7.2, 4.0), (8, 8, 5.0, 6.6), (8, 11, 7.0, 4.2)]
    cx, cy, rx, ry = specs[frame]
    c.ellipse(cx - 0.5, cy + 0.5, rx, ry, M)
    # shading: darker lower part
    for y in range(16):
        for x in range(16):
            if c.get(x, y) == M:
                if y > cy + ry * 0.35:
                    c.set(x, y, D)
                elif y < cy - ry * 0.35 and x < cx:
                    c.set(x, y, L)
    # shine
    c.set(int(cx - rx * 0.5), int(cy - ry * 0.55), W)
    c.set(int(cx - rx * 0.5) + 1, int(cy - ry * 0.55), W)
    c.set(int(cx - rx * 0.5), int(cy - ry * 0.55) + 1, W)
    ey = int(cy - 0.5)
    c.set(5, ey, OLn)
    c.set(5, ey + 1, OLn)
    c.set(10, ey, OLn)
    c.set(10, ey + 1, OLn)
    c.set(5, ey, W) if frame == 2 else None
    c.set(10, ey, W) if frame == 2 else None
    c.set(7, ey + 2, OLn)
    c.set(8, ey + 2, OLn)
    c.outline(OLn)
    return c


def bat(frame):
    c = Canvas(16, 16)
    O, B, L, D, WM, WL, EY, FG = range(1, 9)
    tips = [(0, 2), (0, 7), (1, 12)]
    tx, ty = tips[frame]
    # wings (left), mirrored
    for side in (0, 1):
        pts = []
        sx = 6 if side == 0 else 9
        for i in range(0, 21):
            t = i / 20.0
            x = sx + (tx - sx) * t if side == 0 else sx + ((15 - tx) - sx) * t
            top = 8 + (ty - 8) * t - 2 * math.sin(math.pi * t) * 0
            bot = 10 + (ty + 3 - 10) * t + (2.2 * math.sin(math.pi * t) if frame != 1 else 1.2)
            for y in range(int(round(top)), int(round(bot)) + 1):
                c.set(int(round(x)), y, WM if (y + int(x)) % 3 else WL)
    c.ellipse(7.5, 9, 3.0, 3.4, B)
    c.rect(5, 5, 6, 6, B)
    c.rect(9, 5, 10, 6, B)
    c.set(5, 4, D)
    c.set(10, 4, D)
    c.ellipse(7.5, 7.5, 2.6, 2.2, B)
    c.set(6, 7, EY)
    c.set(9, 7, EY)
    c.set(6, 10, FG)
    c.set(9, 10, FG)
    for y in range(9, 12):
        c.set(7, y, D)
        c.set(8, y, D)
    c.outline(O)
    return c


def wolf(frame):
    c = Canvas(16, 16)
    O, F, L, D, BL, EY, MO, TH = range(1, 9)
    low = 2 if frame == 3 else 0
    # body
    c.ellipse(9, 8.5 + low, 5.2, 3.2, F)
    c.rect(5, 7 + low, 12, 8 + low, F)
    for x in range(5, 14):
        c.set(x, 6 + low, L)
        c.set(x, 11 + low, BL if x < 12 else D)
    # head & snout
    c.ellipse(4, 6.5 + low + (1 if frame == 3 else 0), 3, 2.8, F)
    c.rect(0, 7 + low + (1 if frame == 3 else 0), 3, 8 + low + (1 if frame == 3 else 0), L)
    c.set(0, 7 + low + (1 if frame == 3 else 0), O)
    c.set(2, 6 + low + (1 if frame == 3 else 0), EY)
    c.set(3, 3 + low + (1 if frame == 3 else 0), D)
    c.set(5, 3 + low + (1 if frame == 3 else 0), D)
    c.set(4, 4 + low + (1 if frame == 3 else 0), D)
    c.set(1, 9 + low + (1 if frame == 3 else 0), TH)
    # tail
    tl = [(14, 5), (14, 6), (13, 7), (14, 6)]
    c.line(13, 8 + low, 15, tl[frame][1] + low - 2, F)
    c.set(15, tl[frame][1] + low - 3, L)
    # legs
    legs = {0: [(5, 0), (7, 0), (11, 0), (13, 0)],
            1: [(4, -1), (8, 1), (10, -1), (14, 1)],
            2: [(6, 1), (7, -1), (12, 1), (12, -1)],
            3: [(5, 0), (7, 0), (11, 0), (13, 0)]}[frame]
    for i, (lx, off) in enumerate(legs):
        top = 11 + low
        for y in range(top, 14 + low + (0 if frame != 3 else -1) + (off if off else 0)):
            c.set(lx, y, D if i % 2 else F)
        c.set(lx, 14 + low + off, D)
    c.outline(O)
    return c


def boss(frame):
    """32x32 Hollow King. frames: 0 idle,1 idle-bob,2 cast,3 slam-up,4 slam-down"""
    c = Canvas(32, 32)
    O, BN, BS, CK, CL_, CD, CR, CRD, EY, EL, AU, WO, GM, WH, DK = range(1, 16)
    bob = [0, 1, -1, -3, 3][frame]
    top = 3 + bob
    # cloak body
    for y in range(11 + bob, 31):
        t = (y - (11 + bob)) / 19.0
        half = 5 + 9 * t
        for x in range(int(16 - half), int(16 + half) + 1):
            col = CK
            if x < 16 - half * 0.55:
                col = CD
            elif x > 16 + half * 0.4:
                col = CL_ if (x + y) % 5 else CK
            c.set(x, y, col)
    # ragged hem
    for x in range(1, 31):
        if (x * 7) % 5 < 2:
            for y in range(28, 32):
                if y > 28 + (x % 3):
                    c.set(x, y, 0)
    # shoulders / arms
    arm_up = frame in (2, 3)
    for side, sx in ((-1, 8), (1, 24)):
        ay = 14 + bob
        if arm_up:
            for i in range(0, 9):
                c.set(sx + side * (1 + i // 3), ay - i, BN)
                c.set(sx + side * (2 + i // 3), ay - i, BS)
            c.rect(sx + side * 3 - (1 if side < 0 else 0), ay - 11, sx + side * 3 + (1 if side > 0 else 0), ay - 9, BN)
            if frame == 2:
                c.ellipse(sx + side * 3, ay - 13, 2.4, 2.4, AU)
                c.set(sx + side * 3, ay - 13, WH)
        else:
            for i in range(0, 8):
                c.set(sx + side * (i // 3), ay + i, BN)
                c.set(sx + side * (i // 3) + 1, ay + i, BS)
            c.rect(sx + side * 2 - 1, ay + 8, sx + side * 2 + 1, ay + 9, BN)
    # staff (right hand)
    sx = 28
    for y in range(8 + bob, 29):
        c.set(sx, y, WO)
    c.ellipse(sx, 6 + bob, 2.2, 2.2, GM)
    c.set(sx - 1, 5 + bob, WH)
    # skull
    c.ellipse(15.5, top + 5, 6.2, 5.6, BN)
    c.rect(11, top + 9, 20, top + 11, BN)
    for x in range(11, 21):
        c.set(x, top + 11, BS)
        if x % 2:
            c.set(x, top + 10, O)
    c.rect(12, top + 4, 14, top + 7, O)
    c.rect(17, top + 4, 19, top + 7, O)
    eyc = EL if frame in (2, 3) else EY
    c.rect(13, top + 5, 14, top + 6, eyc)
    c.rect(17, top + 5, 18, top + 6, eyc)
    c.set(16, top + 8, O)
    c.set(15, top + 8, O)
    # crown
    c.rect(10, top - 1, 21, top + 1, CR)
    for x in (10, 13, 16, 19, 21):
        c.set(x, top - 2, CR)
        c.set(x, top - 3, CR) if x in (13, 19) else None
    c.set(16, top, GM)
    c.rect(10, top + 1, 21, top + 1, CRD)
    # aura sparkles
    if frame == 2:
        for p in ((3, 6), (28, 10), (5, 20), (25, 3)):
            c.set(p[0], p[1], AU)
    c.outline(O)
    return c


# ------------------------------------------------------------------ projectiles / fx
def arrow():
    c = Canvas(16, 16)
    O, SL, SM, SD, GO, GD, WO, WD = 1, 2, 3, 4, 5, 6, 7, 8
    c.rect(7, 4, 7, 12, WO)
    c.rect(8, 4, 8, 12, WD)
    c.set(7, 2, SL)
    c.set(8, 2, SL)
    c.rect(6, 3, 9, 3, SM)
    c.rect(7, 1, 8, 2, SL)
    c.set(5, 12, 12)
    c.set(10, 12, 12)
    c.set(6, 13, 14)
    c.set(9, 13, 14)
    c.set(6, 11, 14)
    c.set(9, 11, 14)
    c.outline(O)
    return c


def bolt(frame):
    c = Canvas(8, 8)
    r = 2.6 if frame else 2.0
    c.ellipse(3.5, 3.5, r, r, 13)
    c.ellipse(3.5, 3.5, 1.3, 1.3, 4)
    c.outline(14)
    return c


def fireball(frame):
    c = Canvas(16, 16)
    r = 5.6 if frame == 0 else 6.4
    c.ellipse(7.5, 7.5, r, r, 8)
    c.ellipse(7.5, 7.5, r - 1.6, r - 1.6, 9)
    c.ellipse(7.0, 7.0, r - 3.4, r - 3.4, 4)
    for p in ((2, 3), (13, 12), (1, 9)) if frame else ((13, 4), (2, 12), (14, 9)):
        c.set(p[0], p[1], 10)
    c.outline(10)
    return c


def orb(frame):
    c = Canvas(8, 8)
    r = 2.8 if frame else 2.3
    c.ellipse(3.5, 3.5, r, r, 14)
    c.ellipse(3.5, 3.5, 1.3, 1.3, 4 if frame else 13)
    c.outline(7)
    return c


def explosion(frame):
    c = Canvas(16, 16)
    rs = [3.5, 6.0, 7.5, 7.8]
    r = rs[frame]
    if frame < 3:
        c.ellipse(7.5, 7.5, r, r, 10 if frame < 2 else 7)
        c.ellipse(7.5, 7.5, r * 0.78, r * 0.78, 8 if frame < 2 else 6)
        c.ellipse(7.5, 7.5, r * 0.48, r * 0.48, 9 if frame < 2 else 8)
        if frame == 0:
            c.ellipse(7.5, 7.5, 1.6, 1.6, 4)
    else:
        for (x, y) in ((2, 3), (12, 2), (13, 11), (3, 12), (7, 1), (7, 14), (1, 8), (14, 7)):
            c.set(x, y, 7)
            c.set(x + (1 if x < 8 else -1), y, 6)
    return c


def particle(kind, frame):
    c = Canvas(8, 8)
    if kind == 'sparkle':
        arm = [1, 2, 3, 1][frame]
        for p in ((3, 3), (4, 4), (3, 4), (4, 3)):
            c.set(p[0], p[1], 4)
        for i in range(1, arm + 1):
            col = 4 if i == 1 else 5
            c.set(3 - i, 3, col)
            c.set(4 + i, 4, col)
            c.set(4, 3 - i, col)
            c.set(3, 4 + i, col)
    elif kind == 'smoke':
        r = [1.6, 2.4, 3.0, 3.4][frame]
        c.ellipse(3.5, 3.5, r, r, 6 if frame < 3 else 7)
        if frame < 3:
            c.set(2, 2, 4)
    elif kind == 'leaf':
        if frame == 0:
            c.art([".AB.", "ABBA", ".AB.", "..A."], 2, 2, {'A': 12, 'B': 11})
        else:
            c.art(["..A.", ".BBA", "ABB.", ".A.."], 2, 2, {'A': 12, 'B': 11})
    elif kind == 'hit':
        if frame == 0:
            c.art(["...A....", "...A....", ".AA.AA..", "AA.A.AA.", ".AA.AA..", "...A....", "...A...."], 0, 0, {'A': 4})
            c.set(3, 3, 9)
        elif frame == 1:
            c.art(["A..A..A.", ".A.A.A..", "..AAA...", "AAA.AAA.", "..AAA...", ".A.A.A..", "A..A..A."], 0, 0, {'A': 9})
        else:
            for p in ((1, 1), (6, 1), (1, 6), (6, 6), (3, 0), (3, 7), (0, 3), (7, 3)):
                c.set(p[0], p[1], 8)
    elif kind == 'dust':
        r = [1.2, 2.0, 2.6][frame]
        c.ellipse(3.5, 4.5, r, r * 0.8, 15 if frame < 2 else 6)
    elif kind == 'ember':
        c.set(3, 3, 8 if frame != 1 else 9)
        c.set(4, 3, 10)
        c.set(3, 4, 9)
        if frame == 0:
            c.set(4, 4, 9)
    elif kind == 'heal':
        c.art(["..AA..", "..AA..", "AAAAAA", "AAAAAA", "..AA..", "..AA.."], 1, 1, {'A': 11 if frame != 1 else 4})
        c.outline(12)
    return c


DIG = {
    '0': ("###", "#.#", "#.#", "#.#", "###"), '1': (".#.", "##.", ".#.", ".#.", "###"),
    '2': ("###", "..#", "###", "#..", "###"), '3': ("###", "..#", "###", "..#", "###"),
    '4': ("#.#", "#.#", "###", "..#", "..#"), '5': ("###", "#..", "###", "..#", "###"),
    '6': ("###", "#..", "###", "#.#", "###"), '7': ("###", "..#", "..#", "..#", "..#"),
    '8': ("###", "#.#", "###", "#.#", "###"), '9': ("###", "#.#", "###", "..#", "###"),
}


def digit(ch):
    c = Canvas(8, 8)
    for y, r in enumerate(DIG[ch]):
        for x, p in enumerate(r):
            if p == '#':
                c.set(2 + x, 1 + y, 1)
    c.outline(2, diag=True)
    return c


def item(kind, frame=0):
    """PAL_ITEM idx: 3 gold,4 gold light,5 gold dark(orange),6 red,7 pink,8 dark red,9 blue,10 light blue,
    11 magenta,12 white,13 pale gold,14 dark gold,15 outline"""
    c = Canvas(8, 8)
    if kind == 'coin':
        w = [3, 2, 1, 2][frame]
        c.ellipse(3.5, 3.5, w + 0.6 if w > 1 else 0.9, 3.4, 3)
        if w >= 2:
            c.set(3, 2, 4)
            c.set(3, 3, 4)
            c.set(4, 5, 5)
        c.outline(14)
    elif kind == 'heart':
        c.art([".AA.AA..", "ABBABBA.", "ABBBBBA.", ".ABBBA..", "..ABA...", "...A...."], 0, 1, {'A': 8, 'B': 6})
        c.set(1, 2, 7)
        c.set(2, 2, 7)
        c.outline(15)
    elif kind == 'mana':
        c.art(["..A...", ".ABA..", "ABCBA.", "ABBBA.", ".ABA..", "..A..."], 1, 1, {'A': 9, 'B': 10, 'C': 12})
        c.outline(15)
    elif kind == 'potion':
        c.art(["..AA..", "..BB..", ".CCCC.", "CDDDDC", "CDEDDC", ".CCCC."], 1, 1, {'A': 13, 'B': 12, 'C': 12, 'D': 6, 'E': 7})
        c.outline(15)
    elif kind == 'ether':
        c.art(["..AA..", "..BB..", ".CCCC.", "CDDDDC", "CDEDDC", ".CCCC."], 1, 1, {'A': 13, 'B': 12, 'C': 12, 'D': 9, 'E': 10})
        c.outline(15)
    elif kind == 'key':
        c.art(["AAA.....", "A.A.....", "AAA.....", ".A......", ".AAA....", ".A......", ".AA....."], 0, 0, {'A': 3})
        c.set(1, 0, 4)
        c.outline(14)
    return c


def ui_sprite(kind, frame=0):
    if kind == 'cursor':
        c = Canvas(16, 16)
        off = frame
        c.art(["A.......", "AAA.....", "AAAAA...", "AAAAAAA.", "AAAAA...", "AAA.....", "A......."], 2 + off, 4, {'A': 3})
        for y in range(16):
            for x in range(16):
                if c.get(x, y) == 3 and (x + off) % 4 == 3:
                    c.set(x, y, 5)
        c.outline(4)
        return c
    if kind == 'prompt':
        c = Canvas(16, 16)
        bob = frame
        c.rect(3, 1 + bob, 12, 10 + bob, 5)
        c.rect(4, 0 + bob, 11, 11 + bob, 5)
        c.set(7, 12 + bob, 5)
        c.set(8, 12 + bob, 5)
        c.set(7, 13 + bob, 5)
        c.art([".##.", "#..#", "####", "#..#", "#..#"], 6, 3 + bob, {'#': 6})
        c.outline(7)
        return c
    if kind == 'alert':
        c = Canvas(16, 16)
        bob = frame
        c.rect(6, 1 + bob, 9, 8 + bob, 6)
        c.rect(6, 10 + bob, 9, 12 + bob, 6)
        c.rect(7, 1 + bob, 7, 7 + bob, 5)
        c.rect(7, 10 + bob, 7, 11 + bob, 5)
        c.outline(12)
        return c
    if kind == 'star':
        c = Canvas(8, 8)
        arm = [0, 1, 2, 1][frame]
        c.set(3, 3, 5)
        c.set(4, 4, 5)
        c.set(3, 4, 5)
        c.set(4, 3, 5)
        for i in range(1, arm + 1):
            c.set(3 - i, 3, 5)
            c.set(4 + i, 4, 5)
            c.set(4, 3 - i, 5)
            c.set(3, 4 + i, 5)
        return c


def preview(path):
    from common import sheet
    fr = []
    pals = []
    for k in ('sword', 'staff', 'bow', 'dagger'):
        fr.append(weapon(k))
    fr.append(arc_sprite())
    sheet(fr, PAL_WEAPON, 5, scale=3).save(path + '_weapons.png')
    sheet([slime(i) for i in range(4)], PAL_SLIME_G, 4, scale=6).save(path + '_slime.png')
    sheet([bat(i) for i in range(3)], PAL_BAT, 4, scale=6).save(path + '_bat.png')
    sheet([wolf(i) for i in range(4)], PAL_WOLF, 4, scale=6).save(path + '_wolf.png')
    sheet([boss(i) for i in range(5)], PAL_BOSS, 5, scale=4).save(path + '_boss.png')
    f = [arrow(), bolt(0), bolt(1), fireball(0), fireball(1), orb(0), orb(1)] + [explosion(i) for i in range(4)]
    f += [particle('sparkle', i) for i in range(4)] + [particle('smoke', i) for i in range(4)] + [particle('leaf', i) for i in range(2)]
    f += [particle('hit', i) for i in range(3)] + [particle('dust', i) for i in range(3)] + [particle('ember', i) for i in range(3)] + [particle('heal', i) for i in range(3)]
    sheet(f, PAL_FX, 12, scale=6).save(path + '_fx.png')
    f = [item('coin', i) for i in range(4)] + [item(k) for k in ('heart', 'mana', 'potion', 'ether', 'key')]
    sheet(f, PAL_ITEM, 9, scale=6).save(path + '_items.png')
    f = [digit(str(i)) for i in range(10)]
    sheet(f, PAL_UI, 10, scale=6).save(path + '_digits.png')
    f = [ui_sprite('cursor', 0), ui_sprite('cursor', 1), ui_sprite('prompt', 0), ui_sprite('prompt', 1), ui_sprite('alert', 0), ui_sprite('star', 1), ui_sprite('star', 2)]
    sheet(f, PAL_UI, 8, scale=6).save(path + '_ui.png')


if __name__ == '__main__':
    preview('/tmp/obj')
