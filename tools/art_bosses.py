"""Boss sprites (32x32, palette PAL_BOSS from art_objs).

  Hollow King frames:  0 idle, 1 idle-bob, 2 cast wind-up, 3 raised (slam up), 4 slam down, 5 charge, 6 dazed, 7 shield,
                       8 cast release
  Bone Warden frames:  0 idle, 1 walk A, 2 walk B, 3 club raised, 4 club smash, 5 shield bash, 6 bone toss, 7 dazed,
                       8 summon
Both are drawn facing the camera; the engine mirrors them for left/right.
"""
from common import Canvas

# palette indices (see PAL_BOSS)
O, BN, BS, CK, CL_, CD, CR, CRD, EY, EL, AU, WO, GM, WH, DK = range(1, 16)

KING_FRAMES = 9
WARDEN_FRAMES = 9


def _limb(c, pts, light=BN, shade=BS, thick=2):
    """a bone limb through a list of (x, y) points"""
    for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
        n = max(abs(x1 - x0), abs(y1 - y0), 1)
        for i in range(n + 1):
            x = round(x0 + (x1 - x0) * i / n)
            y = round(y0 + (y1 - y0) * i / n)
            c.set(x, y, light)
            if thick > 1:
                c.set(x + 1, y, shade)


def _hand(c, x, y):
    c.rect(x - 1, y, x + 1, y + 1, BN)
    c.set(x - 1, y + 2, BS)
    c.set(x + 1, y + 2, BS)


def _glow(c, x, y, r):
    c.ellipse(x, y, r, r, AU)
    c.ellipse(x, y, max(r - 1.4, 0.8), max(r - 1.4, 0.8), EL)
    c.set(int(x), int(y), WH)


# ======================================================================= Hollow King
def king(frame):
    c = Canvas(32, 32)
    pose = {
        0: dict(bob=0, arms='down', sway=0, eyes=EY),
        1: dict(bob=1, arms='down', sway=1, eyes=EY),
        2: dict(bob=-1, arms='up', sway=0, eyes=EL, glow=2.2),
        3: dict(bob=-3, arms='wide_up', sway=0, eyes=EL, glow=2.6),
        4: dict(bob=3, arms='wide_down', sway=0, eyes=EY, planted=True),
        5: dict(bob=0, arms='back', sway=2, eyes=EL),
        6: dict(bob=3, arms='slump', sway=0, eyes=DK, tilt=True),
        7: dict(bob=-1, arms='cross', sway=0, eyes=EL, aura=True),
        8: dict(bob=-2, arms='forward', sway=0, eyes=EL, glow=3.0),
    }[frame]
    bob, arms, sway = pose['bob'], pose['arms'], pose['sway']
    top = 3 + bob + (2 if pose.get('tilt') else 0)

    # ---- cloak: a tall bell with folds, a lit right edge and a ragged hem
    y0 = 11 + bob
    for y in range(y0, 32):
        t = (y - y0) / float(31 - y0 + 1)
        half = 5 + 9.5 * t
        shift = sway * t * (1 if arms != 'back' else -1.6)
        for x in range(int(16 + shift - half), int(16 + shift + half) + 1):
            col = CK
            u = (x - (16 + shift)) / max(half, 1)
            if u < -0.55:
                col = CD
            elif u > 0.5:
                col = CL_ if (x + y) % 5 else CK
            elif abs(u) < 0.04 or abs(u + 0.34) < 0.04 or abs(u - 0.3) < 0.04:
                col = CD if (y + x) % 3 else CK       # vertical folds
            c.set(x, y, col)
    for x in range(1, 31):                              # ragged hem
        if (x * 7) % 5 < 2:
            for y in range(28, 32):
                if y > 28 + (x % 3):
                    c.set(x, y, 0)
    # open cloak front: a glimpse of ribs and a purple sash
    for y in range(14 + bob, 21 + bob):
        for x in range(13, 19):
            c.set(x, y, DK)
    for y in (15 + bob, 17 + bob, 19 + bob):
        c.rect(13, y, 18, y, BN)
        c.set(15, y, O)
        c.set(16, y, O)
    c.rect(14, 21 + bob, 17, 22 + bob, CRD)             # belt buckle
    c.set(15, 21 + bob, CR)
    c.set(16, 21 + bob, GM)

    # ---- pauldrons
    for sx in (8, 24):
        c.ellipse(sx, 13 + bob, 3.6, 2.8, CR)
        c.ellipse(sx, 14 + bob, 3.2, 1.6, CRD)
        c.set(sx - 1, 12 + bob, 3 if False else CR)
        c.set(sx, 10 + bob, CR)                         # spike
        c.set(sx, 11 + bob, CR)

    # ---- arms (left arm free, right arm carries the staff)
    def arm(side, sx, pts, hand):
        _limb(c, [(sx + side * dx, y + bob) for dx, y in pts])
        _hand(c, sx + side * hand[0], hand[1] + bob)
    S = {'down': ([(0, 15), (-1, 20), (0, 24)], (0, 24)),
         'up': ([(0, 15), (-2, 10), (-1, 5)], (-1, 4)),
         'wide_up': ([(0, 15), (-4, 9), (-5, 4)], (-5, 3)),
         'wide_down': ([(0, 15), (-3, 21), (-5, 26)], (-5, 26)),
         'back': ([(0, 15), (-2, 20), (-5, 23)], (-5, 23)),
         'cross': ([(0, 15), (4, 17), (8, 19)], (8, 19)),
         'forward': ([(0, 15), (-3, 14), (-8, 12)], (-8, 11)),
         'slump': ([(0, 15), (0, 22), (1, 27)], (1, 27))}
    pl, hl = S[arms]
    arm(1, 8, pl, hl)
    # right arm mirrors, but may hold the staff
    pr, hr = S[arms]
    arm(-1, 24, pr, hr)
    hx = 24 - hr[0]                                     # right hand x, y
    hy = hr[1] + bob
    # ---- staff
    if pose.get('planted'):
        sx0 = 28
        for y in range(10 + bob, 32):
            c.set(sx0, y, WO)
        c.ellipse(sx0, 8 + bob, 2.2, 2.2, GM)
    elif arms in ('up', 'wide_up'):
        sx0 = hx + 1
        for y in range(max(0, hy - 8), hy + 14):
            c.set(sx0, y, WO)
        c.ellipse(sx0, max(1, hy - 9), 2.2, 2.2, GM)
    elif arms == 'cross':
        sx0 = 28
        for y in range(9 + bob, 30):
            c.set(sx0, y, WO)
        c.ellipse(sx0, 7 + bob, 2.2, 2.2, GM)
    elif arms == 'back':
        _limb(c, [(hx + 1, hy + 2), (hx + 6, hy - 8), (hx + 10, hy - 16)], WO, WO, 1)
        c.ellipse(min(31, hx + 10), max(1, hy - 17), 2.0, 2.0, GM)
    else:
        sx0 = min(30, hx + 3)
        for y in range(9 + bob, 29):
            c.set(sx0, y, WO)
        c.ellipse(sx0, 7 + bob, 2.2, 2.2, GM)
        c.set(sx0 - 1, 6 + bob, WH)

    # ---- head
    c.ellipse(15.5, top + 5, 6.2, 5.6, BN)
    c.rect(11, top + 9, 20, top + 11, BN)
    for x in range(11, 21):
        c.set(x, top + 11, BS)
        if x % 2:
            c.set(x, top + 10, O)
    c.rect(12, top + 4, 14, top + 7, O)
    c.rect(17, top + 4, 19, top + 7, O)
    eye = pose['eyes']
    c.rect(13, top + 5, 14, top + 6, eye)
    c.rect(17, top + 5, 18, top + 6, eye)
    c.set(15, top + 8, O)
    c.set(16, top + 8, O)
    c.set(8 if False else 12, top + 2, BS)              # cheek shading
    # crown
    c.rect(10, top - 1, 21, top + 1, CR)
    for x in (10, 13, 16, 19, 21):
        c.set(x, top - 2, CR)
        if x in (13, 19):
            c.set(x, top - 3, CR)
    c.set(16, top, GM)
    c.rect(10, top + 1, 21, top + 1, CRD)
    # ---- magic
    g = pose.get('glow')
    if g:
        if arms == 'up':
            _glow(c, 8, 3 + bob, g)
            _glow(c, 23, 3 + bob, g)
        elif arms == 'wide_up':
            _glow(c, 4, 2 + bob, g)
            _glow(c, 27, 2 + bob, g)
        elif arms == 'forward':
            _glow(c, 2, 11 + bob, g)
            _glow(c, 29, 11 + bob, g)
    if pose.get('aura'):
        import math
        for i in range(28):
            a = i / 28.0 * 2 * math.pi
            c.set(int(16 + 14.5 * math.cos(a)), int(17 + 14.5 * math.sin(a)), AU)
    if frame in (2, 3, 8):
        for p in ((2, 6), (29, 9), (4, 20), (27, 3), (1, 15), (30, 18)):
            c.set(p[0], p[1], AU)
    c.outline(O)
    return c


# ======================================================================= Bone Warden
def warden(frame):
    c = Canvas(32, 32)
    P = {
        0: dict(bob=0, legs='stand', club='side', shield=True, eyes=EY),
        1: dict(bob=-1, legs='walk_a', club='side', shield=True, eyes=EY),
        2: dict(bob=-1, legs='walk_b', club='side', shield=True, eyes=EY),
        3: dict(bob=-1, legs='stand', club='raised', shield=False, eyes=EL, lean=-1),
        4: dict(bob=2, legs='lunge', club='smash', shield=False, eyes=EL, lean=1),
        5: dict(bob=1, legs='lunge', club='side', shield=True, bash=True, eyes=EL, lean=1),
        6: dict(bob=0, legs='stand', club='toss', shield=True, eyes=EL),
        7: dict(bob=3, legs='stand', club='dropped', shield=False, eyes=DK, lean=0, tilt=True),
        8: dict(bob=-2, legs='stand', club='summon', shield=False, eyes=EL, glow=True),
    }[frame]
    bob = P['bob']
    lean = P.get('lean', 0)
    ox = lean * 1

    # ---- legs / greaves
    legs = P['legs']
    ly = 21 + bob
    def leg(x, y0, y1, fwd=0):
        c.rect(x, y0, x + 3, y1, BN)
        c.rect(x, y0, x, y1, BS)
        c.rect(x - 1, y1 - 3, x + 4, y1 - 2, CD)             # greave band
        c.rect(x - 1, y1 + 1, x + 4, y1 + 2, CD)             # boot
        c.rect(x - 1, y1 + 1, x + 4, y1 + 1, CK)
    if legs == 'stand':
        leg(11 + ox, ly, 28 + min(bob, 0)), leg(18 + ox, ly, 28 + min(bob, 0))
    elif legs == 'walk_a':
        leg(10 + ox, ly, 28), leg(19 + ox, ly, 25)
    elif legs == 'walk_b':
        leg(12 + ox, ly, 25), leg(17 + ox, ly, 28)
    else:                                                     # lunge: wide stance
        leg(8 + ox, ly, 28), leg(20 + ox, ly, 26)

    # ---- torso: dark plate armour with gold trim
    ty = 11 + bob
    for y in range(ty, ty + 11):
        t = (y - ty) / 10.0
        half = 7 - 1.2 * t
        for x in range(int(16 + ox - half), int(16 + ox + half) + 1):
            col = CK
            if x < 16 + ox - half * 0.5:
                col = CD
            elif x > 16 + ox + half * 0.45:
                col = CL_
            c.set(x, y, col)
    c.rect(int(10 + ox), ty + 5, int(21 + ox), ty + 5, CRD)       # belt
    c.rect(int(15 + ox), ty + 4, int(16 + ox), ty + 6, CR)
    c.rect(int(12 + ox), ty + 1, int(19 + ox), ty + 1, CR)        # gorget trim
    for y in (ty + 2, ty + 3):                                    # ribs through the breastplate gap
        c.rect(int(14 + ox), y, int(17 + ox), y, BN)
    # ---- pauldrons
    for sx in (8, 24):
        c.ellipse(sx + ox, ty + 1, 4.0, 3.0, CR)
        c.ellipse(sx + ox, ty + 2, 3.4, 1.8, CRD)
        c.set(sx + ox, ty - 3, CR)
        c.set(sx + ox, ty - 2, CR)
    # ---- head: horned bone helm
    hy = 2 + bob + (2 if P.get('tilt') else 0)
    c.ellipse(15.5 + ox, hy + 5, 5.8, 5.4, BN)
    c.rect(int(11 + ox), hy + 8, int(20 + ox), hy + 10, BN)
    for x in range(int(11 + ox), int(21 + ox)):
        c.set(x, hy + 10, BS)
        if x % 2:
            c.set(x, hy + 9, O)
    c.rect(int(12 + ox), hy + 4, int(14 + ox), hy + 6, O)
    c.rect(int(17 + ox), hy + 4, int(19 + ox), hy + 6, O)
    c.set(int(13 + ox), hy + 5, P['eyes'])
    c.set(int(14 + ox), hy + 5, P['eyes'])
    c.set(int(17 + ox), hy + 5, P['eyes'])
    c.set(int(18 + ox), hy + 5, P['eyes'])
    c.rect(int(10 + ox), hy + 1, int(21 + ox), hy + 2, CD)        # helm brim
    c.rect(int(10 + ox), hy + 1, int(21 + ox), hy + 1, CK)
    for (hx, sgn) in ((10, -1), (21, 1)):                         # horns
        for i in range(5):
            c.set(int(hx + ox + sgn * (i // 2)), hy + 1 - i, BN if i < 4 else BS)
    c.set(int(15 + ox), hy + 8, O)
    c.set(int(16 + ox), hy + 8, O)

    # ---- shield arm (left of the figure)
    if P['shield']:
        sx = 4 + (3 if P.get('bash') else 0) + ox
        sy = ty + (2 if P.get('bash') else 3)
        c.ellipse(sx + 2, sy + 5, 5.4, 6.4, CD)
        c.ellipse(sx + 2, sy + 5, 4.6, 5.6, BN)
        c.ellipse(sx + 2, sy + 5, 3.2, 4.2, BS)
        c.rect(int(sx + 1), sy + 3, int(sx + 3), sy + 7, CK)       # emblem
        c.set(int(sx + 2), sy + 5, CR)
        c.rect(int(sx + 1), sy + 11, int(sx + 3), sy + 12, BN)
    else:
        _limb(c, [(8 + ox, ty + 3), (6 + ox, ty + 8), (7 + ox, ty + 13)])
        _hand(c, int(7 + ox), ty + 13)

    # ---- club arm (right) -- spiked bone club
    club = P['club']
    def club_draw(x0, y0, x1, y1):
        n = max(abs(x1 - x0), abs(y1 - y0), 1)
        for i in range(n + 1):
            x = round(x0 + (x1 - x0) * i / n)
            y = round(y0 + (y1 - y0) * i / n)
            c.set(x, y, WO)
            c.set(x + 1, y, WO)
        hx, hy2 = x1, y1
        c.ellipse(hx + 0.5, hy2, 3.4, 3.4, BN)
        c.ellipse(hx + 0.5, hy2, 2.2, 2.2, BS)
        for (ax, ay) in ((-4, 0), (5, 0), (0, -4), (0, 4)):
            c.set(int(hx + 0.5 + ax), int(hy2 + ay), BN)
    if club == 'side':
        _limb(c, [(24 + ox, ty + 3), (26 + ox, ty + 8), (25 + ox, ty + 12)])
        _hand(c, int(25 + ox), ty + 12)
        club_draw(25 + ox, ty + 12, 28 + ox, ty + 19)
    elif club == 'raised':
        _limb(c, [(24 + ox, ty + 2), (27 + ox, ty - 2), (25 + ox, ty - 7)])
        _hand(c, int(25 + ox), ty - 8)
        club_draw(24 + ox, ty - 6, 22 + ox, 0 if ty - 14 < 3 else ty - 14)
    elif club == 'smash':
        _limb(c, [(24 + ox, ty + 3), (28 + ox, ty + 8), (27 + ox, ty + 13)])
        _hand(c, int(27 + ox), ty + 13)
        club_draw(26 + ox, ty + 14, 27 + ox, 29)
    elif club == 'toss':
        _limb(c, [(24 + ox, ty + 3), (28 + ox, ty + 4), (31, ty + 1)])
        _hand(c, 30, ty - 1)
        c.rect(30, ty - 5, 31, ty - 3, BN)
        club_draw(5 + ox, ty + 14, 3 + ox, 27) if False else None
    elif club == 'dropped':
        _limb(c, [(24 + ox, ty + 3), (26 + ox, ty + 9), (25 + ox, ty + 15)])
        _hand(c, int(25 + ox), ty + 15)
        club_draw(22 + ox, 29, 27 + ox, 30)
    elif club == 'summon':
        _limb(c, [(24 + ox, ty + 2), (27 + ox, ty - 3), (26 + ox, ty - 8)])
        _hand(c, int(26 + ox), ty - 9)
        _limb(c, [(8 + ox, ty + 2), (5 + ox, ty - 3), (6 + ox, ty - 8)])
        _hand(c, int(6 + ox), ty - 9)
    if P.get('glow'):
        for (gx, gy) in ((6, 1), (26, 1)):
            c.ellipse(gx + ox, gy, 3.0, 3.0, AU)
            c.ellipse(gx + ox, gy, 1.8, 1.8, EL)
            c.set(gx + ox, gy, WH)
    c.outline(O)
    return c
