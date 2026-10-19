"""The Cinder Hollows' own bestiary. These used to be the forest monsters with an orange tint; now every creature has its
own silhouette.  They replace the slime / bat / wolf / skeleton sheets and the Warden's boss sheet (same tile layouts, so
the AI and drawing code are untouched) while the hero is inside the Hollows.

  slime slot   -> Fire Imp     (hops: idle, squash, stretch, land)
  bat slot     -> Ember Wisp   (flies: 3 flicker frames)
  wolf slot    -> Cinder Boar  (charges: idle, run a, run b, lunge)  - faces left, the engine flips it
  skeleton slot-> Slag Golem   (walk x4 down/up/side, attack down/up/side)
  warden slot  -> Cinder Tyrant (9 frames of 32x32, same pose list as the Warden)
"""
import math
from common import Canvas, pal16

# ----------------------------------------------------------------------------------------------- palettes (15 colours each)
IMP_PAL = pal16(['#2a0a0a', '#8a2210', '#d83c18', '#ff7a2c', '#f4e8c0', '#ffb020', '#fff080', '#ffffff', '#3a0808',
                 '#ffd070', '#b02a14', '#601410', '#ffe8a0', '#ff5a20', '#c0a060'])
WISP_PAL = pal16(['#3a0c08', '#ff3a10', '#ff8a18', '#ffd040', '#fff4a0', '#2a0808', '#ffc060', '#5a2a28', '#ff5a20',
                  '#b81808', '#ffe890', '#7a3a30', '#ffffff', '#ff7a30', '#401810'])
BOAR_PAL = pal16(['#1a0c0c', '#3a2a2a', '#5a403c', '#7a5850', '#ff7a18', '#ffd040', '#f4e8c0', '#ffe060', '#b02a14',
                  '#2a1818', '#9a7868', '#ff4a10', '#fff4c0', '#c0a888', '#6a4a44'])
GOLEM_PAL = pal16(['#1c0c0c', '#3a2420', '#5c3a30', '#845648', '#d83410', '#ff7a18', '#ffd030', '#fff4c0', '#fff080',
                   '#6a5a58', '#2a1814', '#a07868', '#ff5a20', '#4a302a', '#c89880'])
TYRANT_PAL = pal16(['#1a0a10', '#2e1c24', '#4c2e38', '#6e4652', '#d83010', '#ff7a18', '#ffd030', '#fff4c0', '#e0d0a8',
                    '#a89468', '#ff4a1c', '#ffa030', '#6a5a64', '#fff8a0', '#503038'])


# ----------------------------------------------------------------------------------------------- Fire Imp
def imp(frame):
    c = Canvas(16, 16)
    O, DK, MD, LT, HN, FL, FB, WH, MO, BG, SH, DD = range(1, 13)
    squash = frame in (1, 3)
    jump = frame == 2
    hy = 6.5 + (2 if squash else (-2 if jump else 0))       # head centre
    by = 12.0 + (1.6 if squash else (-2.5 if jump else 0))   # body centre
    # flame tail (right side)
    tail = {0: [(12, 12), (13, 11), (14, 9), (14, 7)], 1: [(12, 13), (13, 13), (14, 12), (14, 10)],
            2: [(12, 9), (13, 8), (14, 6), (13, 4)], 3: [(12, 13), (13, 13), (14, 12), (15, 11)]}[frame]
    for i, (x, y) in enumerate(tail):
        c.set(x, y, MD)
        c.set(x, y - 1, LT) if i < 3 else None
    tx, ty = tail[-1]
    c.set(tx, ty - 1, FL)
    c.set(tx, ty - 2, FB)
    c.set(tx + 1 if tx < 15 else tx, ty - 1, FL)
    # body
    brx, bry = (4.6, 2.5) if squash else ((3.4, 3.6) if jump else (3.8, 3.2))
    c.ellipse(8, by, brx, bry, MD)
    for y in range(16):
        for x in range(16):
            if c.get(x, y) == MD and ((x - 8) ** 2 / (brx * brx) + (y - by) ** 2 / (bry * bry)) <= 1:
                if y > by + bry * 0.3:
                    c.set(x, y, DK)
                elif x < 8 - brx * 0.35 and y < by:
                    c.set(x, y, LT)
    c.ellipse(8, by + 0.3, 1.8, 1.4, BG)                    # glowing belly
    c.set(8, int(by), FB)
    # legs / arms
    if not squash:
        for lx in (6, 10):
            c.rect(lx, int(by + bry - 0.5), lx + 1, 14 if not jump else 13, DK)
            c.set(lx, 14 if not jump else 13, HN)
    if jump:
        for (ax, sgn) in ((3, -1), (12, 1)):                  # arms thrown up
            c.line(ax + (1 if sgn < 0 else 0), int(by) - 1, ax + sgn, int(hy) - 4, MD)
            c.set(ax + sgn, int(hy) - 5, HN)
    else:
        c.set(4, int(by), MD)
        c.set(3, int(by) + 1, DK)
        c.set(11, int(by), MD)
        c.set(12, int(by) + 1, DK)
    # head
    c.ellipse(8, hy, 4.6, 4.0, MD)
    for y in range(16):
        for x in range(16):
            if c.get(x, y) == MD and ((x - 8) ** 2 / 21.2 + (y - hy) ** 2 / 16.0) <= 1.0 and y > hy + 1.5:
                c.set(x, y, SH)
            elif c.get(x, y) == MD and x < 6 and y < hy - 0.5:
                c.set(x, y, LT)
    c.set(3, int(hy), MD)                                    # pointy ears
    c.set(2, int(hy) - 1, MD)
    c.set(13, int(hy), MD)
    c.set(14, int(hy) - 1, MD)
    for (hx, sgn) in ((5, -1), (10, 1)):                     # horns
        c.set(hx, int(hy) - 4, HN)
        c.set(hx + sgn, int(hy) - 5, HN)
    # face
    ey = int(hy) - 1
    for ex in (5, 9):
        c.rect(ex, ey, ex + 1, ey + 1, WH)
        c.set(ex + (1 if jump else 0), ey + 1, O)
    c.set(5, ey - 1, O)
    c.set(10, ey - 1, O)
    if jump or frame == 3:
        c.rect(6, int(hy) + 2, 9, int(hy) + 3, MO)              # open mouth, shouting
        c.set(6, int(hy) + 2, HN)
        c.set(9, int(hy) + 2, HN)
    else:
        for x in range(6, 10):
            c.set(x, int(hy) + 2, MO)
        c.set(6, int(hy) + 3, HN)
        c.set(9, int(hy) + 3, HN)
    c.outline(O)
    return c


# ----------------------------------------------------------------------------------------------- Ember Wisp
def wisp(frame):
    c = Canvas(16, 16)
    O, F1, F2, F3, F4, EY, SP, SM = range(1, 9)
    lean = {0: 0, 1: -1, 2: 1}[frame]
    for y in range(2, 14):
        t = (y - 2) / 11.0                                    # 0 top .. 1 bottom
        hw = 1.0 + 4.6 * math.sin(min(1.0, t * 1.15) * math.pi / 2) if t < 0.62 else 5.4 * math.sqrt(max(0.0, 1 - ((t - 0.62) / 0.40) ** 2))
        cx = 8 + lean * (1 - t) * 1.6
        for x in range(16):
            d = abs(x + 0.5 - cx)
            if d <= hw:
                k = d / max(hw, 0.6)
                col = F4 if (k < 0.34 and 0.25 < t < 0.85) else (F3 if k < 0.66 else F2)
                if t < 0.12:
                    col = F2
                c.set(x, y, col)
    for (ox, oy, h) in (((4, 4, 3), (11, 3, 2), (8, 0, 2)) if frame == 0 else ((3, 5, 2), (12, 4, 3), (7, 0, 2)) if frame == 1 else ((5, 3, 2), (12, 5, 2), (9, 0, 3))):
        for i in range(h):                                    # flame tongues licking up
            c.set(ox + lean, oy - i, F2 if i < h - 1 else F3)
    c.rect(5, 8, 6, 10, EY)                                   # eyes
    c.rect(9, 8, 10, 10, EY)
    c.set(5, 8, F4)
    c.set(9, 8, F4)
    c.rect(7, 12, 8, 12, EY)                                  # small round mouth
    for (sx, sy) in ((2, 11), (13, 12), (1, 6), (14, 8)):    # drifting sparks
        c.set(sx, sy + (frame % 2), SP)
    c.set(8, 14, SM)
    c.outline(O)
    return c


# ----------------------------------------------------------------------------------------------- Cinder Boar
def boar(frame):
    c = Canvas(16, 16)
    O, CK, CM, CL, LV, LY, TK, EY, RD = range(1, 10)
    low = 1 if frame == 3 else 0
    # body: charcoal hide, lit from above
    c.ellipse(9.5, 8.5 + low, 5.6, 3.7, CM)
    for y in range(16):
        for x in range(16):
            if c.get(x, y) == CM:
                if y < 6 + low:
                    c.set(x, y, CL)
                elif y > 10 + low:
                    c.set(x, y, CK)
    for x in range(6, 14):                                    # glowing cracks across the flank
        if (x * 3) % 5 in (0, 1):
            c.set(x, 8 + low + (x % 2), LV)
    c.set(9, 9 + low, LY)
    c.set(12, 8 + low, LY)
    for x in (7, 9, 11, 13):                                  # flame bristles along the spine
        c.set(x, 4 + low, LV)
        c.set(x, 3 + low, LY if x == 9 else LV)
    # head
    hy = 9 + low + (1 if frame == 3 else 0)
    c.ellipse(4, hy, 3.3, 3.0, CM)
    c.rect(0, hy, 3, hy + 2, CL)
    c.set(0, hy + 1, LV)                                      # glowing snout
    c.set(0, hy, LY)
    c.set(4, hy - 1, EY)
    c.set(5, hy - 4, CK)                                      # ear
    c.set(6, hy - 4, CK)
    for (tx, ty) in ((1, hy + 3), (3, hy + 3)):               # tusks
        c.set(tx, ty, TK)
        c.set(tx, ty - 1, TK)
    c.set(1, hy + 4, TK) if frame == 3 else None
    # tail tuft
    c.set(15, 6 + low, LV)
    c.set(15, 5 + low, LY)
    c.set(14, 7 + low, CM)
    # legs
    legs = {0: [(5, 0), (7, 0), (11, 0), (13, 0)], 1: [(4, -1), (8, 1), (10, -1), (14, 1)],
            2: [(6, 1), (7, -1), (12, 1), (12, -1)], 3: [(4, 0), (7, 0), (11, 0), (13, 0)]}[frame]
    for i, (lx, off) in enumerate(legs):
        for y in range(11 + low, 14 + low + off):
            c.set(lx, y, CK if i % 2 else CM)
        c.set(lx, 14 + low + off, RD)
    c.outline(O)
    return c


# ----------------------------------------------------------------------------------------------- Slag Golem
def golem(d, step, atk=False):
    """d: 0 down, 1 up, 2 side (faces left)."""
    c = Canvas(16, 16)
    O, RD, RM, RL, LR, LO, LY, WH, EY, AS = range(1, 11)
    bob = (0, -1, 0, -1)[step % 4] if not atk else -1
    sw = (0, 1, 0, -1)[step % 4]                               # arm / leg swing
    if d in (0, 1):
        # legs
        ll, rl = 13 - (1 if sw > 0 else 0), 13 - (1 if sw < 0 else 0)
        c.rect(5, 11 + bob, 7, ll, RM)
        c.rect(8, 11 + bob, 10, rl, RM)
        c.rect(5, ll, 7, ll + 1, RD)
        c.rect(8, rl, 10, rl + 1, RD)
        # torso
        c.rect(4, 6 + bob, 11, 11 + bob, RM)
        c.rect(4, 6 + bob, 5, 11 + bob, RL)
        c.rect(10, 6 + bob, 11, 11 + bob, RD)
        if d == 0:
            c.ellipse(7.5, 9 + bob, 2.4, 2.2, LR)                # furnace core
            c.ellipse(7.5, 9 + bob, 1.4, 1.2, LY)
            c.set(7, 9 + bob, WH)
            c.set(5, 10 + bob, LO)                               # side cracks
            c.set(10, 7 + bob, LO)
        else:
            for y in range(6 + bob, 12 + bob):                   # glowing spine
                c.set(7, y, LO if y % 2 else LR)
                c.set(8, y, LO)
            c.set(7, 8 + bob, LY)
        # shoulders: boulders
        for sx in (3, 12):
            c.ellipse(sx, 6.5 + bob, 2.6, 2.4, RL if sx < 8 else RM)
            c.set(sx, 6 + bob, LO)
        # arms
        if atk:
            for ax in (2, 13):
                c.rect(ax, 1 + bob, ax + 1, 5 + bob, RM)
                c.ellipse(ax + 0.5, 1 + bob, 1.8, 1.6, LR)
                c.set(ax, bob + 1, LY)
        else:
            c.rect(1, 8 + bob + sw, 2, 11 + bob + sw, RM)
            c.ellipse(1.5, 12 + bob + sw, 1.8, 1.6, LR)
            c.rect(13, 8 + bob - sw, 14, 11 + bob - sw, RM)
            c.ellipse(13.5, 12 + bob - sw, 1.8, 1.6, LR)
        # head
        c.rect(5, 1 + bob, 10, 5 + bob, RD)
        c.rect(5, 1 + bob, 10, 1 + bob, RL)
        c.rect(4, 2 + bob, 4, 4 + bob, RD)
        c.rect(11, 2 + bob, 11, 4 + bob, RD)
        if d == 0:
            c.rect(6, 3 + bob, 7, 3 + bob, EY)
            c.rect(9, 3 + bob, 10, 3 + bob, EY)
            c.rect(6, 5 + bob, 9, 5 + bob, LO)
        else:
            c.rect(6, 2 + bob, 9, 2 + bob, LO)
            c.rect(6, 4 + bob, 9, 4 + bob, RM)
    else:
        # side view, facing left
        ll = 13 - (1 if sw > 0 else 0)
        rl = 13 - (1 if sw < 0 else 0)
        c.rect(5, 11 + bob, 7, ll, RM)
        c.rect(8, 11 + bob, 10, rl, RD)
        c.rect(5, ll, 7, ll + 1, RD)
        c.rect(8, rl, 10, rl + 1, RD)
        c.rect(4, 6 + bob, 10, 11 + bob, RM)
        c.rect(4, 6 + bob, 5, 11 + bob, RL)
        c.rect(9, 6 + bob, 10, 11 + bob, RD)
        c.set(4, 9 + bob, LR)                                    # core glows through the chest
        c.set(5, 9 + bob, LY)
        c.set(5, 8 + bob, LO)
        c.ellipse(6.5, 6.5 + bob, 2.6, 2.4, RL)
        c.set(6, 6 + bob, LO)
        if atk:
            c.rect(6, 1 + bob, 7, 5 + bob, RM)
            c.ellipse(6.5, 1 + bob, 1.8, 1.6, LR)
            c.set(6, 1 + bob, LY)
        else:
            c.rect(3, 7 + bob + sw, 4, 10 + bob + sw, RM)
            c.ellipse(3, 11 + bob + sw, 1.8, 1.6, LR)
        c.rect(3, 1 + bob, 8, 5 + bob, RD)
        c.rect(3, 1 + bob, 8, 1 + bob, RL)
        c.rect(3, 3 + bob, 4, 3 + bob, EY)
        c.rect(3, 5 + bob, 6, 5 + bob, LO)
        c.rect(10, 2 + bob, 12, 5 + bob, RD)                     # back of the head / hump
    for (fx, fy) in ((2, 14), (13, 13)):                            # ember specks at its feet
        c.set(fx, fy, LO) if c.get(fx, fy) == 0 else None
    c.outline(O)
    return c


def golem_frames():
    """same order as skel_frames: walk down x4, up x4, side x4, then attack down / up / side"""
    fr = []
    for d in (0, 1, 2):
        fr += [golem(d, s) for s in range(4)]
    fr += [golem(d, 0, True) for d in (0, 1, 2)]
    return fr


# ----------------------------------------------------------------------------------------------- Cinder Tyrant (32x32 x 9)
def _fill_ellipse_shaded(c, cx, cy, rx, ry, tones, light=(-0.6, -0.75)):
    """tones = (dark, mid, light); lit from the top-left"""
    for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
        for x in range(int(cx - rx) - 1, int(cx + rx) + 2):
            u, v = (x + 0.5 - cx) / rx, (y + 0.5 - cy) / ry
            if u * u + v * v <= 1.0:
                s = -(u * light[0] * -1) * 0  # keep a plain expression
                s = u * light[0] + v * light[1]
                c.set(x, y, tones[2] if s < -0.45 else (tones[1] if s < 0.25 else tones[0]))


def tyrant(frame):
    """A hulking magma colossus: boulder shoulders, a furnace in its chest, a mane of fire and curling bone horns.
    Pose list (same slots as the Warden): 0 idle 1 walk a 2 walk b 3 raised 4 smash 5 charge 6 toss 7 dazed 8 summon."""
    c = Canvas(32, 32)
    O, OB, OM, OL, MR, MO, MY, WH, HB, HD, FL, FI, SM, EY, DM = range(1, 16)
    P = {
        0: dict(bob=0, legs='stand', arms='side', eyes=EY, core=MO),
        1: dict(bob=-1, legs='walk_a', arms='side', eyes=EY, core=MO),
        2: dict(bob=-1, legs='walk_b', arms='side', eyes=EY, core=MO),
        3: dict(bob=-1, legs='stand', arms='raised', eyes=WH, core=MY, lean=-1),
        4: dict(bob=3, legs='lunge', arms='smash', eyes=WH, core=MY, lean=1),
        5: dict(bob=1, legs='lunge', arms='charge', eyes=WH, core=MY, lean=1),
        6: dict(bob=0, legs='stand', arms='toss', eyes=WH, core=MY),
        7: dict(bob=5, legs='kneel', arms='slump', eyes=DM, core=MR, tilt=True),
        8: dict(bob=-2, legs='stand', arms='summon', eyes=WH, core=MY, flames=True),
    }[frame]
    bob, lean = P['bob'], P.get('lean', 0)
    ox = lean
    # ---- legs: short thick obsidian pillars with lava seams
    ly = 22 + bob
    def leg(x, y0, y1):
        c.rect(x, y0, x + 4, y1, OM)
        c.rect(x, y0, x + 1, y1, OL)
        c.rect(x + 3, y0, x + 4, y1, OB)
        c.rect(x - 1, y1 - 2, x + 5, y1, OB)                      # boot
        c.rect(x - 1, y1 - 2, x + 5, y1 - 2, OM)
        c.set(x + 2, y0 + 2, MO)
        c.set(x + 2, y0 + 3, MR)
    lg = P['legs']
    if lg == 'stand':
        leg(9 + ox, ly, 29), leg(18 + ox, ly, 29)
    elif lg == 'walk_a':
        leg(8 + ox, ly, 29), leg(19 + ox, ly, 26)
    elif lg == 'walk_b':
        leg(10 + ox, ly, 26), leg(17 + ox, ly, 29)
    elif lg == 'lunge':
        leg(5 + ox, ly, 29), leg(21 + ox, ly, 27)
    else:                                                          # kneel
        c.rect(7 + ox, 26, 24 + ox, 29, OB)
        c.rect(7 + ox, 26, 24 + ox, 26, OM)
    # ---- torso: broad barrel, lit from the upper left, furnace window in the chest
    ty = 11 + bob
    for y in range(ty, ty + 12):
        t = (y - ty) / 11.0
        half = 9.5 - 2.2 * t
        for x in range(int(16 + ox - half), int(16 + ox + half) + 1):
            u = (x - (16 + ox)) / half
            c.set(x, y, OL if u < -0.55 else (OM if u < 0.35 else OB))
    for k in range(3):                                             # armour ridges
        c.rect(int(8 + ox) + k, ty + 2 + k * 3, int(12 + ox) + k, ty + 2 + k * 3, OB)
    c.ellipse(16 + ox, ty + 6, 3.6, 3.4, OB)
    c.ellipse(16 + ox, ty + 6, 2.8, 2.6, P['core'])
    c.ellipse(16 + ox, ty + 6, 1.6, 1.5, MY if P['core'] != MR else MO)
    c.set(int(16 + ox), ty + 6, WH if P['core'] != MR else MY)
    for (cx0, cy0, cx1, cy1) in ((10, ty + 2, 13, ty + 5), (21, ty + 3, 19, ty + 8), (13, ty + 10, 15, ty + 8)):   # lava cracks
        c.line(int(cx0 + ox), cy0, int(cx1 + ox), cy1, MR)
    # ---- shoulders: boulders crowned with glowing cracks
    for (sx, tone) in ((6, (OB, OM, OL)), (26, (OB, OM, OL))):
        _fill_ellipse_shaded(c, sx + ox, ty + 1.5, 5.4, 4.4, tone)
        c.set(int(sx + ox), ty, MO)
        c.set(int(sx + ox) + 1, ty + 1, MR)
        c.set(int(sx + ox) - 1, ty + 2, MR)
    # ---- arms
    def arm(pts, fist, glow=True):
        for (a, b) in zip(pts, pts[1:]):
            n = max(abs(b[0] - a[0]), abs(b[1] - a[1]), 1)
            for i in range(n + 1):
                x = round(a[0] + (b[0] - a[0]) * i / n)
                y = round(a[1] + (b[1] - a[1]) * i / n)
                c.rect(x - 1, y - 1, x + 2, y + 2, OM)
                c.set(x - 1, y - 1, OL)
                c.set(x + 2, y + 2, OB)
        fx, fy = fist
        _fill_ellipse_shaded(c, fx, fy, 3.6, 3.4, (MR, MO, MY) if glow else (OB, OM, OL))
        c.set(fx, fy - 1, WH)
    am = P['arms']
    if am == 'side':
        arm([(5 + ox, ty + 3), (3 + ox, ty + 9), (4 + ox, ty + 14)], (4 + ox, ty + 16))
        arm([(27 + ox, ty + 3), (29 + ox, ty + 9), (28 + ox, ty + 14)], (28 + ox, ty + 16))
    elif am == 'raised':
        arm([(5 + ox, ty + 2), (4 + ox, ty - 4), (8 + ox, ty - 9)], (10 + ox, ty - 10))
        arm([(27 + ox, ty + 2), (28 + ox, ty - 4), (24 + ox, ty - 9)], (22 + ox, ty - 10))
    elif am == 'smash':
        arm([(5 + ox, ty + 3), (6 + ox, ty + 10), (10 + ox, ty + 15)], (11 + ox, ty + 17))
        arm([(27 + ox, ty + 3), (26 + ox, ty + 10), (22 + ox, ty + 15)], (21 + ox, ty + 17))
        for fx in (4, 12, 20, 28):                                  # the floor cracks open
            c.set(fx, 30, MO)
            c.set(fx + 1, 29, MR)
    elif am == 'charge':
        arm([(5 + ox, ty + 3), (3 + ox, ty + 8), (2 + ox, ty + 12)], (2 + ox, ty + 14), False)
        arm([(27 + ox, ty + 2), (30, ty + 5), (31, ty + 9)], (30, ty + 11))
    elif am == 'toss':
        arm([(5 + ox, ty + 3), (3 + ox, ty + 9), (4 + ox, ty + 14)], (4 + ox, ty + 16))
        arm([(27 + ox, ty + 2), (30, ty - 1), (30, ty - 5)], (29, ty - 7))
        _fill_ellipse_shaded(c, 29, ty - 11, 3.0, 3.0, (MR, MO, MY))     # the fireball it throws
        c.set(29, ty - 12, WH)
        c.set(28, ty - 14, FL)
        c.set(30, ty - 14, FI)
    elif am == 'slump':
        arm([(5 + ox, ty + 3), (4 + ox, ty + 11), (7 + ox, ty + 17)], (8 + ox, ty + 18), False)
        arm([(27 + ox, ty + 3), (28 + ox, ty + 11), (25 + ox, ty + 17)], (24 + ox, ty + 18), False)
    else:                                                           # summon: arms flung out, fire in the palms
        arm([(5 + ox, ty + 2), (2 + ox, ty - 2), (3 + ox, ty - 7)], (3 + ox, ty - 9))
        arm([(27 + ox, ty + 2), (30, ty - 2), (29, ty - 7)], (29, ty - 9))
    # ---- head: small, sunk between the shoulders, with a fiery mane and bone horns
    hy = 3 + bob + (3 if P.get('tilt') else 0)
    hx = 16 + ox + (1 if P.get('tilt') else 0)
    mane = [(-5, 1, 5), (-3, -2, 7), (-1, -4, 8), (1, -4, 8), (3, -2, 7), (5, 1, 5)]
    for (dx, dy, h) in mane:                                         # flame mane behind the head
        for i in range(h - 3):
            c.set(int(hx + dx), hy + dy + 3 - i, FL if i < (h - 3) // 2 else FI)
        c.set(int(hx + dx), hy + dy - (h - 4), FI)
    _fill_ellipse_shaded(c, hx, hy + 5, 5.2, 4.8, (OB, OM, OL))
    c.rect(int(hx - 4), hy + 7, int(hx + 3), hy + 9, OB)              # heavy jaw
    c.rect(int(hx - 3), hy + 9, int(hx + 2), hy + 9, MR)               # glowing mouth line
    c.rect(int(hx - 4), hy + 4, int(hx - 2), hy + 5, P['eyes'])
    c.rect(int(hx + 1), hy + 4, int(hx + 3), hy + 5, P['eyes'])
    c.set(int(hx - 4), hy + 3, O)
    c.set(int(hx + 3), hy + 3, O)
    for (sx, sgn) in ((-1, -1), (1, 1)):                              # curling bone horns
        px, py = hx + sx * 5, hy + 3
        for i in range(6):
            c.set(int(px + sgn * (i * 0.9)), int(py - i * 0.8 + (i * i) * 0.06), HB if i < 4 else HD)
        c.set(int(px + sgn * 5.5), int(py - 4), HD)
    if P.get('flames'):
        for (gx, gy) in ((3, ty - 12), (29, ty - 12)):
            c.ellipse(gx + ox, gy, 3.0, 3.0, FL)
            c.ellipse(gx + ox, gy, 1.8, 1.8, FI)
            c.set(gx + ox, gy, WH)
    c.outline(O)
    return c


if __name__ == '__main__':
    from common import sheet
    sheet([imp(i) for i in range(4)], IMP_PAL, 4, scale=14).save('/tmp/prev/em_imp.png')
    sheet([wisp(i) for i in range(3)], WISP_PAL, 3, scale=14).save('/tmp/prev/em_wisp.png')
    sheet([boar(i) for i in range(4)], BOAR_PAL, 4, scale=14).save('/tmp/prev/em_boar.png')
    sheet(golem_frames(), GOLEM_PAL, 8, scale=10).save('/tmp/prev/em_golem.png')
    sheet([tyrant(i) for i in range(9)], TYRANT_PAL, 5, scale=8).save('/tmp/prev/em_tyrant.png')
    print('ok')
