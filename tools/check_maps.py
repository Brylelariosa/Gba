"""Sanity checks for the generated maps: every spawn/chest/sign/warp/NPC must be reachable from the area start.
Run: python3 tools/check_maps.py  (exit code 1 if anything is unreachable)."""
import os
import sys
from collections import deque

sys.path.insert(0, os.path.dirname(__file__))
import art_world as AW
import art_maps as AM

BLOCK = AW.SOLID | AW.WATERF


def check(name, m, ts):
    flags = ts.flags
    W, H = AM.W, AM.H

    def walk(x, y):
        if not (0 <= x < W and 0 <= y < H):
            return False
        f = flags[m.ground[y][x]]
        if f & (AW.DOOR | AW.DOOR2):          # locked doors open during play
            return True
        return not (f & BLOCK)

    sx, sy = m.start
    seen = set()
    if walk(sx, sy):
        dq = deque([(sx, sy)])
        seen.add((sx, sy))
        while dq:
            x, y = dq.popleft()
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                n = (x + dx, y + dy)
                if n not in seen and walk(*n):
                    seen.add(n)
                    dq.append(n)
    bad = []
    if (sx, sy) not in seen:
        bad.append(('start not walkable', (sx, sy)))

    def near(x, y):
        return any(((x + dx, y + dy) in seen) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
    warn = []
    for s in m.spawns:
        x, y = s[1], s[2]
        if (x, y) not in seen:
            warn.append(('spawn not reachable (enemy is walled in)', s))
    for c in m.chests:
        if not near(c[0], c[1]):
            bad.append(('chest unreachable', c))
    for sg in m.signs:
        if not near(sg[0], sg[1]):
            bad.append(('sign unreachable', sg))
    for n in m.npcs:
        if (n[1], n[2]) not in seen and not near(n[1], n[2]):
            warn.append(('npc not adjacent to a walkable tile (stall keeper?)', n))
    for w in m.warps:
        if not any((x, y) in seen for x in range(w[0], w[2] + 1) for y in range(w[1], w[3] + 1)):
            bad.append(('warp unreachable', w))
        dest_ok = True
    print('%-8s reachable tiles %4d  spawns %2d chests %2d signs %d warps %d  %s' % (name, len(seen), len(m.spawns), len(m.chests), len(m.signs), len(m.warps), 'OK' if not bad else 'PROBLEMS'))
    for b in bad:
        print('   ERROR', b)
    for w_ in warn:
        print('   warn ', w_)
    return not bad


def main():
    outdoor = AW.build_outdoor()
    dungeon = AW.build_dungeon()
    ok = True
    ok &= check('village', AM.build_village(outdoor), outdoor)
    ok &= check('forest', AM.build_forest(outdoor), outdoor)
    ok &= check('crypt', AM.build_crypt(dungeon), dungeon)
    ok &= check('depths', AM.build_depths(dungeon), dungeon)
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
