"""Procedural humanoid sprite builder. 16x16 frames, feet baseline y=14."""
from common import Canvas, pal16

OL, SK, SKS, HA, HAL, CL, CLL, CLD, TR, TRL, PA, PAD, AC, ACL, WH = range(1, 16)

DOWN, UP, SIDE = 0, 1, 2

# palettes: idx1..15 =
#  outline, skin, skin shade, hair, hair light, cloth, cloth light, cloth dark,
#  trim(belt), trim light, pants, boots/pants dark, accent, accent light, white
STYLES = {
    'knight': dict(
        pal=pal16(['#2a1a2e', '#f8c8a0', '#d89878', '#7a4a28', '#a8703c', '#3a6ad8', '#6a9af0', '#2a46a0',
                   '#6a4020', '#f0d050', '#4a5a8a', '#2a3050', '#9aa4b8', '#d8e0f0', '#ffffff']),
        hair='short', robe=False, pads=True),
    'mage': dict(
        pal=pal16(['#2a1a2e', '#f8c8a0', '#d89878', '#e8e0f8', '#ffffff', '#8a3ad0', '#b878f0', '#5a2090',
                   '#d8a028', '#f8e060', '#6a2ca8', '#3a1860', '#7a30c0', '#b070f0', '#ffffff']),
        hair='hat', robe=True, pads=False),
    'ranger': dict(
        pal=pal16(['#2a1a2e', '#f8c8a0', '#d89878', '#c89048', '#e8b868', '#48a840', '#78d060', '#2c7430',
                   '#6a4020', '#e0c058', '#7a5a30', '#3a2814', '#3c8838', '#68b858', '#ffffff']),
        hair='hood', robe=False, pads=False),
    'rogue': dict(
        pal=pal16(['#2a1a2e', '#f8c8a0', '#d89878', '#2c2c3c', '#5a5a74', '#484860', '#6c6c8c', '#2a2a3c',
                   '#6a4020', '#c0c0d0', '#383850', '#1c1c2c', '#d83040', '#f87080', '#ffffff']),
        hair='band', robe=False, pads=False),
    'elder': dict(
        pal=pal16(['#2a1a2e', '#f0c098', '#d09070', '#f0f0f0', '#ffffff', '#8a6a48', '#b09068', '#5a4028',
                   '#4a3018', '#e0c050', '#6a4a30', '#3a2814', '#48689a', '#7898c8', '#ffffff']),
        hair='bald', robe=True, pads=False, beard=True),
    'merchant': dict(
        pal=pal16(['#2a1a2e', '#f8c8a0', '#d89878', '#5a3a20', '#7a5a38', '#e8a830', '#f8d060', '#a87018',
                   '#6a4020', '#f8f8f0', '#4a6a9a', '#2a3a5a', '#c04838', '#e87860', '#ffffff']),
        hair='merchant', robe=False, pads=False),
    'healer': dict(
        pal=pal16(['#2a1a2e', '#f8c8a0', '#d89878', '#e8a0b0', '#f8c8d0', '#f0f0f8', '#ffffff', '#b8c0d8',
                   '#d8b048', '#f8e070', '#5898d8', '#3868a8', '#78b0f0', '#a8d0ff', '#ffffff']),
        hair='long', robe=True, pads=False),
    'skeleton': dict(
        pal=pal16(['#201830', '#e8e4d0', '#b0a890', '#e8e4d0', '#ffffff', '#d0ccb8', '#f0ecdc', '#8c8670',
                   '#5a4a38', '#c8c0a0', '#d0ccb8', '#8c8670', '#60e090', '#a0ffc0', '#ffffff']),
        hair='skull', robe=False, pads=False),
}


def head(c, st, d, ox, oy, dark=False):
    """Draw head in canvas c at offset (ox,oy). Head box ~ x3..12,y1..9 relative."""
    s = STYLES[st]
    hs = s['hair']
    cx, cy, rx, ry = 7.5, 5.0, 5.0, 4.6
    for y in range(0, 11):
        for x in range(0, 16):
            if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1.0:
                px, py = ox + x, oy + y
                hair = False
                if d == UP:
                    hair = y <= 7
                elif d == DOWN:
                    hair = y <= 3 or (y == 4 and x not in (7, 8)) or (y in (5, 6) and (x <= 4 or x >= 11))
                else:  # SIDE facing left
                    hair = y <= 3 or (y == 4 and x >= 6) or (y in (5, 6, 7) and x >= 9) or (x >= 10 and y <= 8)
                col = HA if hair else SK
                if hs == 'bald' and hair and d != UP:
                    col = SK if y > 2 else SK
                    hair = False
                if hs == 'skull':
                    col = SK
                    hair = False
                if col == SK and (y >= 9 or (d == SIDE and x >= 9) or (d == DOWN and (x <= 3 or x >= 12))):
                    col = SKS
                if col == HA and (y <= 2 and x < 8 and x > 3):
                    col = HAL
                c.set(px, py, col)
    # face features
    if d == DOWN:
        if hs == 'skull':
            c.rect(ox + 5, oy + 5, ox + 6, oy + 7, OL)
            c.rect(ox + 9, oy + 5, ox + 10, oy + 7, OL)
            c.set(ox + 5, oy + 6, 12)
            c.set(ox + 10, oy + 6, 12)
            c.set(ox + 7, oy + 8, OL)
            c.set(ox + 8, oy + 8, OL)
            for x in (6, 7, 8, 9):
                c.set(ox + x, oy + 9, SKS if x % 2 else OL)
        else:
            c.set(ox + 6, oy + 6, OL)
            c.set(ox + 6, oy + 7, OL)
            c.set(ox + 9, oy + 6, OL)
            c.set(ox + 9, oy + 7, OL)
            if s.get('beard'):
                for y in range(7, 10):
                    for x in range(4, 12):
                        if c.get(ox + x, oy + y) in (SK, SKS):
                            c.set(ox + x, oy + y, HA if y > 7 or x < 6 or x > 9 else SK)
                c.set(ox + 7, oy + 7, SKS)
                c.set(ox + 8, oy + 7, SKS)
            else:
                c.set(ox + 5, oy + 8, SKS)  # blush hint
                c.set(ox + 10, oy + 8, SKS)
    elif d == SIDE:
        if hs == 'skull':
            c.rect(ox + 4, oy + 5, ox + 5, oy + 7, OL)
            c.set(ox + 4, oy + 6, 12)
            c.rect(ox + 3, oy + 8, ox + 5, oy + 8, OL)
        else:
            c.set(ox + 5, oy + 6, OL)
            c.set(ox + 5, oy + 7, OL)
            c.set(ox + 3, oy + 7, SKS)  # nose
            if s.get('beard'):
                for y in range(7, 10):
                    for x in range(3, 9):
                        if c.get(ox + x, oy + y) in (SK, SKS):
                            c.set(ox + x, oy + y, HA)
    # headgear
    if hs == 'hat':
        for x in range(ox + 2, ox + 14):
            c.set(x, oy + 3, ACL if x % 3 else AC)
            c.set(x, oy + 4, AC)
        for y, (a, b) in enumerate([(7, 8), (6, 9), (5, 10), (4, 11)]):
            for x in range(ox + a, ox + b + 1):
                c.set(x, oy + y - 0, AC if y else ACL)
        c.set(ox + 8, oy - 1, AC)
        c.set(ox + 9, oy - 1, AC)
        c.set(ox + 9, oy, ACL)
        for x in range(ox + 4, ox + 12):
            c.set(x, oy + 3, ACL if x < 7 else AC)
        # band
        for x in range(ox + 4, ox + 12):
            c.set(x, oy + 2, 9 if x % 2 else 10)
    elif hs == 'merchant':
        for x in range(ox + 2, ox + 14):
            c.set(x, oy + 3, CLL if x % 2 else CL)
            c.set(x, oy + 4, CL if d != UP else CLD)
        for y in range(0, 3):
            for x in range(ox + 4 + y // 2, ox + 12 - y // 2):
                c.set(x, oy + y + 0, CLL if y == 0 else CL)
        c.rect(ox + 4, oy + 2, ox + 11, oy + 2, 13)
    elif hs == 'hood':
        for y in range(0, 5):
            for x in range(0, 16):
                if c.get(ox + x, oy + y) == HA:
                    c.set(ox + x, oy + y, AC if (x + y) % 5 else ACL)
        if d == DOWN:
            for x in range(3, 13):
                if c.get(ox + x, oy + 4) == HA or c.get(ox + x, oy + 4) == AC:
                    pass
        # feather
        c.set(ox + 12, oy, TRL)
        c.set(ox + 12, oy + 1, TRL)
        c.set(ox + 13, oy - 1, WH)
    elif hs == 'band':
        y = 3 if d != UP else 4
        for x in range(0, 16):
            if c.get(ox + x, oy + y) in (HA, HAL, SK, SKS):
                c.set(ox + x, oy + y, AC if x % 3 else ACL)
        if d == SIDE:
            c.set(ox + 12, oy + 4, AC)
            c.set(ox + 13, oy + 5, AC)
            c.set(ox + 13, oy + 6, AC)
        if d == UP:
            c.set(ox + 8, oy + 5, AC)
            c.set(ox + 9, oy + 6, AC)
    elif hs == 'long':
        # hair falling to shoulders
        if d == DOWN:
            for y in range(5, 11):
                c.set(ox + 3, oy + y, HA)
                c.set(ox + 4, oy + y, HA)
                c.set(ox + 11, oy + y, HA)
                c.set(ox + 12, oy + y, HA)
        elif d == UP:
            for y in range(8, 12):
                for x in range(4, 12):
                    c.set(ox + x, oy + y, HA if y < 11 else HAL)
        else:
            for y in range(6, 11):
                for x in range(9, 13):
                    c.set(ox + x, oy + y, HA)
    elif hs == 'short' and s.get('pads') is not None:
        pass


def torso(c, st, d, ox, oy, arm_l=0, arm_r=0, ext=None):
    """Torso rows oy..oy+3 ; arm_l/arm_r = vertical arm offsets (down/up views) or side swing."""
    s = STYLES[st]
    robe = s['robe']
    cl_main = CL
    if d in (DOWN, UP):
        for y in range(0, 4):
            for x in range(5, 11):
                col = CL
                if x <= 5:
                    col = CLD if d == DOWN else CLD
                if y == 0 and x in (6, 7):
                    col = CLL
                c.set(ox + x, oy + y, col)
        # belt
        if st == 'skeleton':
            for y in range(0, 4):
                for x in range(5, 11):
                    c.set(ox + x, oy + y, SK if (y % 2 == 0) else SKS)
            for y in range(0, 4):
                c.set(ox + 7, oy + y, CLL)
                c.set(ox + 8, oy + y, CLL)
        elif st == 'merchant' and d == DOWN:
            for y in range(0, 4):
                for x in range(6, 10):
                    c.set(ox + x, oy + y, TR if False else 9 if False else CLL)
            c.rect(ox + 5, oy + 2, ox + 10, oy + 2, TR)
        else:
            c.rect(ox + 5, oy + 2, ox + 10, oy + 2, TR)
            if d == DOWN:
                c.set(ox + 7, oy + 2, TRL)
                c.set(ox + 8, oy + 2, TRL)
        # arms
        if st == 'skeleton':
            for yy in range(0, 4):
                c.set(ox + 4, oy + yy + arm_l, SKS if yy % 2 else SK)
                c.set(ox + 11, oy + yy + arm_r, SKS if yy % 2 else SK)
        else:
            for yy in range(0, 3):
                c.set(ox + 4, oy + yy + arm_l, CL if yy < 2 else CLD)
                c.set(ox + 11, oy + yy + arm_r, CL if yy < 2 else CLD)
            c.set(ox + 4, oy + 3 + arm_l, SK)
            c.set(ox + 11, oy + 3 + arm_r, SK)
            if s['pads']:
                c.set(ox + 4, oy + arm_l, AC)
                c.set(ox + 11, oy + arm_r, ACL)
                c.set(ox + 5, oy, ACL)
                c.set(ox + 10, oy, AC)
        if d == UP and st in ('ranger',):
            for y in range(0, 5):
                for x in range(5, 11):
                    c.set(ox + x, oy + y, AC if (x + y) % 4 else ACL)
        if robe:
            for y in range(4, 6):
                for x in range(4 - (y - 3) // 2 * 0, 12):
                    c.set(ox + x, oy + y, CL if x > 5 else CLD)
                    if y == 5:
                        c.set(ox + x, oy + y, CLD if x % 2 else CL)
    else:  # SIDE (facing left)
        for y in range(0, 4):
            for x in range(6, 11):
                c.set(ox + x, oy + y, CL if x < 9 else CLD)
        c.rect(ox + 6, oy + 2, ox + 10, oy + 2, TR)
        c.set(ox + 6, oy + 2, TRL)
        if st == 'skeleton':
            for y in range(0, 4):
                for x in range(6, 11):
                    c.set(ox + x, oy + y, SK if y % 2 == 0 else SKS)
        # near arm swings
        ax = 8 + arm_l
        for yy in range(0, 3):
            c.set(ox + ax, oy + yy, CLL if st != 'skeleton' else SK)
            c.set(ox + ax + 1, oy + yy, CL if st != 'skeleton' else SK)
        c.set(ox + ax, oy + 3, SK)
        c.set(ox + ax + 1, oy + 3, SK)
        if robe:
            for y in range(4, 6):
                for x in range(5, 12):
                    c.set(ox + x, oy + y, CL if x < 9 else CLD)
        if s['pads']:
            c.set(ox + 8, oy, AC)
            c.set(ox + 9, oy, ACL)


def legs(c, st, d, ox, oy, la=0, ra=0, spread=0, robe=False):
    """oy = first leg row. la/ra: lift (px up) of left/right foot. spread: side-view stride"""
    s = STYLES[st]
    if robe:
        # robe hides legs except toes
        for x, lift in ((6, la), (9, ra)):
            c.set(ox + x, oy + 1 - lift, TR)
            c.set(ox + x + 1, oy + 1 - lift, TR)
        return
    if d in (DOWN, UP):
        for x0, lift in ((5, la), (9, ra)):
            for y in range(0, 2 - (1 if lift else 0)):
                c.rect(ox + x0, oy + y, ox + x0 + 1, oy + y, PA if st != 'skeleton' else SK)
            fy = oy + 2 - lift - 0
            c.rect(ox + x0, fy - 1 if lift else fy, ox + x0 + 1, fy if not lift else fy - 1 + 1, PAD if st != 'skeleton' else SKS)
            c.rect(ox + x0, oy + 1 - lift + 1, ox + x0 + 1, oy + 1 - lift + 1, PAD if st != 'skeleton' else SKS)
    else:
        # side view: front leg at x-spread, back leg at x+spread
        for x0, lift, sp in ((6, la, -spread), (8, ra, spread)):
            for y in range(0, 2):
                c.rect(ox + x0 + sp, oy + y - (lift if y == 1 else 0), ox + x0 + sp + 1, oy + y - (lift if y == 1 else 0), PA if st != 'skeleton' else SK)
            c.rect(ox + x0 + sp - (1 if sp <= 0 else 0), oy + 2 - lift, ox + x0 + sp + 1, oy + 2 - lift, PAD if st != 'skeleton' else SKS)


WALK_LEG = [(0, 0, 0), (1, 0, 0), (0, 0, 0), (0, 0, 0), (0, 1, 0), (0, 0, 0)]
WALK_BOB = [0, 1, 0, 0, 1, 0]
WALK_ARM = [(0, 0), (1, -1), (0, 0), (0, 0), (-1, 1), (0, 0)]
SIDE_SPREAD = [0, 2, 1, 0, 2, 1]


def base_frame(st, d, step=0, arm=(0, 0), lean=(0, 0), ext=None, bob=None):
    s = STYLES[st]
    c = Canvas(16, 16)
    bobv = WALK_BOB[step] if bob is None else bob
    robe = s['robe']
    la, ra, _ = WALK_LEG[step]
    if d == SIDE:
        spread = SIDE_SPREAD[step]
        la, ra = (1 if step in (1,) else 0), (1 if step in (4,) else 0)
        legs(c, st, d, 0, 12 + lean[1], la, ra, spread, robe)
        body_y = 8 + bobv + lean[1]
        torso(c, st, d, lean[0], body_y, arm_l=(-2 if step in (1,) else 2 if step in (4,) else 0) if arm == (0, 0) else arm[0], ext=ext)
        head(c, st, d, lean[0], -1 + bobv + lean[1])
    else:
        legs(c, st, d, 0, 12 + lean[1], la, ra, 0, robe)
        torso(c, st, d, lean[0], 8 + bobv + lean[1], arm_l=WALK_ARM[step][0] if arm == (0, 0) else arm[0],
              arm_r=WALK_ARM[step][1] if arm == (0, 0) else arm[1], ext=ext)
        head(c, st, d, lean[0], -1 + bobv + lean[1])
    return c


def finish(c, st):
    c.outline(OL)
    return c


def hero_frames(st):
    """Return dict of lists: walk[d][6], atk[d][3], hurt[d], roll[4], dead[3]."""
    walk = {d: [finish(base_frame(st, d, i), st) for i in range(6)] for d in (DOWN, UP, SIDE)}
    atk = {}
    for d in (DOWN, UP, SIDE):
        lean = {DOWN: (0, 1), UP: (0, -1), SIDE: (-1, 0)}[d]
        wl = {DOWN: (0, -1), UP: (0, 1), SIDE: (1, 0)}[d]
        atk[d] = [
            finish(base_frame(st, d, 0, arm=(-2, -2), lean=wl, bob=0), st),
            finish(base_frame(st, d, 0, arm=(1, 1), lean=lean, bob=1), st),
            finish(base_frame(st, d, 0, arm=(0, 0), lean=(0, 0), bob=0), st),
        ]
    hurt = {d: finish(base_frame(st, d, 0, arm=(-1, -1), lean=({DOWN: (0, -1), UP: (0, 1), SIDE: (1, 0)}[d]), bob=0), st) for d in (DOWN, UP, SIDE)}
    # roll: curled ball built from head + torso discs
    roll = []
    for k in range(4):
        c = Canvas(16, 16)
        c.ellipse(7.5, 9.5, 5.2, 4.6, CL)
        c.ellipse(7.5, 8.0, 4.0, 3.0, HA if k % 2 == 0 else CLL)
        # skin patch rotates
        ang = [(5, 12), (11, 10), (10, 6), (5, 7)][k]
        c.rect(ang[0], ang[1] - 1, ang[0] + 2, ang[1] + 1, SK)
        c.rect(6 + (k % 2) * 3, 12, 8 + (k % 2) * 3, 13, PAD)
        c.set(3 + k * 3 % 7, 11, CLD)
        c.outline(OL)
        roll.append(c)
    dead = []
    f0 = finish(base_frame(st, DOWN, 0, arm=(-1, -1), lean=(0, -1), bob=0), st)
    dead.append(f0)
    c = Canvas(16, 16)
    # lying on side: rotate frame
    src = base_frame(st, SIDE, 0, bob=0)
    rot = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            v = src.a[y, x]
            if v:
                rot.set(15 - y + 0, x - 0 + 0, int(v))
    # lower the lying body
    low = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            v = rot.a[y, x]
            if v:
                low.set(x, y + 3, int(v))
    low.outline(OL)
    dead.append(low)
    small = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            v = low.a[y, x]
            if v and x % 2 == 0 and y % 2 == 0:
                small.set(x // 2 + 4, y // 2 + 8, int(v))
    dead.append(small)
    return dict(walk=walk, atk=atk, hurt=hurt, roll=roll, dead=dead)


def npc_frames(st):
    a = finish(base_frame(st, DOWN, 0, bob=0), st)
    b = finish(base_frame(st, DOWN, 0, bob=1), st)
    return [a, b]


def skel_frames():
    st = 'skeleton'
    walk = {d: [finish(base_frame(st, d, i), st) for i in (0, 1, 3, 4)] for d in (DOWN, UP, SIDE)}
    atk = {}
    for d in (DOWN, UP, SIDE):
        atk[d] = finish(base_frame(st, d, 0, arm=(-2, -2), lean={DOWN: (0, -1), UP: (0, 1), SIDE: (1, 0)}[d], bob=0), st)
    return dict(walk=walk, atk=atk)


if __name__ == '__main__':
    from common import sheet
    frames = []
    for st in ('knight', 'mage', 'ranger', 'rogue'):
        h = hero_frames(st)
        for d in (DOWN, UP, SIDE):
            frames += h['walk'][d][:6]
        frames += [h['atk'][d][i] for d in (DOWN, SIDE) for i in range(3)]
        frames += h['roll'] + h['dead']
    im = sheet(frames, STYLES['knight']['pal'], 24, scale=3)
    # palette differs per style: render separately
    from PIL import Image
    for st in ('knight', 'mage', 'ranger', 'rogue', 'elder', 'merchant', 'healer'):
        if st in ('elder', 'merchant', 'healer'):
            fr = npc_frames(st)
        else:
            h = hero_frames(st)
            fr = []
            for d in (DOWN, UP, SIDE):
                fr += h['walk'][d]
            fr += [h['atk'][d][i] for d in (DOWN, SIDE) for i in range(3)] + h['roll'] + h['dead']
        sheet(fr, STYLES[st]['pal'], 12 if len(fr) > 4 else 2, scale=5).save('/tmp/chr_%s.png' % st)
    k = skel_frames()
    fr = k['walk'][DOWN] + k['walk'][UP] + k['walk'][SIDE] + [k['atk'][d] for d in (DOWN, UP, SIDE)]
    sheet(fr, STYLES['skeleton']['pal'], 8, scale=5).save('/tmp/chr_skeleton.png')
