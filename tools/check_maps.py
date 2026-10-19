"""Sanity checks for the generated maps: every spawn/chest/sign/warp/NPC must be reachable from the area start.
Run: python3 tools/check_maps.py  (exit code 1 if anything is unreachable)."""
import os
import sys
from collections import deque

sys.path.insert(0, os.path.dirname(__file__))
import art_world as AW
import art_maps as AM
import art_maps2 as AM2

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


def roomy(m, ts, x, y):
    """every one of the eight neighbours is walkable: an enemy born here cannot be pinned between obstacles"""
    for dy in (-1, 0, 1):
        for dx in (-1, 0, 1):
            xx, yy = x + dx, y + dy
            if not (0 <= xx < AM.W and 0 <= yy < AM.H) or ts.flags[m.ground[yy][xx]] & BLOCK:
                return False
    return True


def escapable(m, ts, x, y, need=12, steps=3):
    """a spawn is not pinned if at least `need` walkable tiles lie within `steps` moves of it (walls beside it are fine)"""
    seen = {(x, y)}
    frontier = [(x, y)]
    for _ in range(steps):
        nxt = []
        for (cx, cy) in frontier:
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                n = (cx + dx, cy + dy)
                if n in seen or not (0 <= n[0] < AM.W and 0 <= n[1] < AM.H) or ts.flags[m.ground[n[1]][n[0]]] & BLOCK:
                    continue
                seen.add(n)
                nxt.append(n)
        frontier = nxt
    return len(seen) >= need


def check_spawns(name, m, ts, strict=True):
    """outdoors (trees, rocks, bushes) a spawn needs all eight neighbours free; in the dungeons walls beside it are normal,
    but it must not sit in a pocket (fewer than a dozen walkable tiles within three steps)"""
    bad = []
    for s in m.spawns:
        if strict and not roomy(m, ts, s[1], s[2]):
            bad.append(('spawn has no room around it (enemy would be pinned)', s))
        elif not strict and not escapable(m, ts, s[1], s[2]):
            bad.append(('spawn sits in a pocket (enemy would be pinned)', s))
    for b in bad:
        print('   ERROR', b)
    return not bad


def check_gate(m, ts):
    """with the south gate sealed (the state the game starts in) everything else in the village must still be reachable"""
    import copy
    closed = copy.deepcopy(m)
    for i, nm in enumerate(m.gate_shut):
        closed.ground[AM.GATE[1] + (i >> 2)][AM.GATE[0] + (i & 3)] = ts.id(nm)
    ok = True
    sx, sy = m.start
    flags = ts.flags
    seen = {(sx, sy)}
    dq = deque([(sx, sy)])
    while dq:
        x, y = dq.popleft()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            n = (x + dx, y + dy)
            if n not in seen and 0 <= n[0] < AM.W and 0 <= n[1] < AM.H and not (flags[closed.ground[n[1]][n[0]]] & BLOCK):
                seen.add(n)
                dq.append(n)
    south = m.warps[1]
    if any((x, y) in seen for x in range(south[0], south[2] + 1) for y in range(south[1], south[3] + 1)):
        print('   ERROR the sealed gate leaves the south exit reachable')
        ok = False
    for w in m.warps:
        if w is south:
            continue
        if not any((x, y) in seen for x in range(w[0], w[2] + 1) for y in range(w[1], w[3] + 1)):
            print('   ERROR warp unreachable while the gate is sealed', w)
            ok = False
    print('   sealed gate: village still connected, south exit blocked' if ok else '   sealed gate check FAILED')
    return ok


def main():
    import art_world3 as W3
    import art_interior as AIN

    def outdoor_ts(banks=None):
        t = AW.build_outdoor(banks)
        W3.upgrade(t)
        return t
    vts = outdoor_ts(AW.village_banks())
    fts = outdoor_ts()
    dungeon = AW.build_dungeon()
    ok = True
    village = AM.build_village(vts)
    ok &= check('village', village, vts)
    ok &= check_gate(village, vts)
    forest = AM.build_forest(fts)
    ok &= check('forest', forest, fts)
    ok &= check_spawns('forest', forest, fts)
    crypt = AM.build_crypt(dungeon)
    ok &= check('crypt', crypt, dungeon)
    ok &= check_spawns('crypt', crypt, dungeon, strict=False)
    depths = AM.build_depths(dungeon)
    ok &= check('depths', depths, dungeon)
    ok &= check_spawns('depths', depths, dungeon, strict=False)
    ember = AW.build_ember()
    embermap = AM.build_ember(ember)
    ok &= check('ember', embermap, ember)
    ok &= check_spawns('ember', embermap, ember, strict=False)
    amber = outdoor_ts()
    road = AM2.build_road(amber)
    ok &= check('road', road, amber)
    ok &= check_spawns('road', road, amber)
    aw = AM2.build_amberwood(amber)
    ok &= check('amberwd', aw, amber)
    ok &= check_spawns('amberwd', aw, amber)
    town = outdoor_ts()
    ok &= check('amberfal', AM2.build_amberfall(town), town)
    its = AIN.build_interior_tileset()
    rooms = AIN.build_rooms(its)
    fr = {n: (d, hy + 3) for (hx, hy, hw, d, r), n in zip(AM.HOUSES, ('elder', 'chapel', 'shop', 'forge'))}
    fr.update({'inn': (9, 7), 'alchemist': (22, 7), 'study': (25, 25)})
    AIN.set_exits(rooms, fr)
    for n, r in rooms.items():
        ok &= check('in:' + n, r, its)
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
