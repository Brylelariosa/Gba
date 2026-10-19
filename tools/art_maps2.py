"""Maps for the road south of Embervale: Sunmeadow Road, Amberwood and the town of Amberfall (32x32 metatiles each).

Warp tuples are (x0, y0, x1, y1, dest_area, dest_tx, dest_ty, dest_dir); dir 0 down, 1 up, 2 left, 3 right.
Area ids: 0 village, 1 woods, 2 crypt, 3 depths, 4 ember, 5 road, 6 amberwood, 7 amberfall.
"""
import random
from art_maps import MapB, W, H, house_details

AREA_VILLAGE, AREA_WOODS, AREA_ROAD, AREA_AMBERWOOD, AREA_AMBERFALL = 0, 1, 5, 6, 7


def border(m, gaps, first='oak', second='pine'):
    """a ring of complete 2x2 trees around the map with exits (gaps must sit on the 2x2 grid); `first` is the species"""
    m.border_units(first, gaps)


def path_points(path):
    pts = []
    for (x0, y0), (x1, y1) in zip(path, path[1:]):
        n = max(abs(x1 - x0), abs(y1 - y0), 1)
        for i in range(n + 1):
            pts.append((round(x0 + (x1 - x0) * i / n), round(y0 + (y1 - y0) * i / n)))
    return pts


def thin_trees(m, rng, density, clear_boxes, kinds=('pine', 'oak'), mix=0.8):
    """scatter trees on free grass except inside the clearings: a few giant oaks first (when `oak` is among the kinds), then
    pines everywhere else, so every tree is a whole tree"""
    if 'oak' in kinds:
        m.scatter_big_oaks(int(round(28 * density * (1.0 - mix) * 9)), (3, 4, 28, 28), clear_boxes)
    for y in range(2, 30):
        for x in range(2, 30):
            if m.kind[y][x] != 'grass':
                continue
            if any(c[0] <= x <= c[2] and c[1] <= y <= c[3] for c in clear_boxes):
                continue
            if rng.random() < density:
                m.tree(x, y, 'pine' if rng.random() < 0.95 else 'dead')


def open_corridor(m, pts, r=2):
    for (px, py) in pts:
        for dy in range(-r, r + 2):
            for dx in range(-r, r + 2):
                x, y = px + dx, py + dy
                if 2 <= x < 30 and 2 <= y < 30 and m.kind[y][x] in ('tree_pine', 'tree_dead'):
                    m.kind[y][x] = 'grass'


def clear_around(m, x, y, r=1):
    for dx in range(-r, r + 1):
        for dy in range(-r, r + 1):
            xx, yy = x + dx, y + dy
            if 0 <= xx < W and 0 <= yy < H and m.kind[yy][xx] in ('tree_pine', 'tree_dead'):
                m.kind[yy][xx] = 'grass'


def put_chest(m, x, y, item, qty, cid):
    clear_around(m, x, y, 1)
    m.fix(x, y, 'chest')
    m.chests.append((x, y, item, qty, cid))
    m.reserve(x, y, 1)


def put_sign(m, x, y, text):
    clear_around(m, x, y, 1)
    m.fix(x, y, 'sign')
    m.signs.append((x, y, text))
    m.reserve(x, y, 1)


def lamps(m, pts):
    for (x, y) in pts:
        m.deco(x, y, 'lamp_base', 'lamp_top')


def reach_from(m, ts, start):
    """tiles the hero can walk to from `start` (4-connected), by the tileset's solid/water flags"""
    from collections import deque
    import art_world as AW
    block = AW.SOLID | AW.WATERF

    def walk(x, y):
        return 0 <= x < W and 0 <= y < H and not ((ts.flags[m.ground[y][x]] & block) == block or (ts.flags[m.ground[y][x]] & AW.SOLID and not ts.flags[m.ground[y][x]] & AW.HALF) or ts.flags[m.ground[y][x]] & AW.WATERF)
    seen = {start}
    dq = deque([start])
    while dq:
        x, y = dq.popleft()
        for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
            n = (x + dx, y + dy)
            if n not in seen and walk(*n):
                seen.add(n)
                dq.append(n)
    return seen, walk


def snap_spawns(m, ts):
    """Spawns must stand on a tile the hero can reach AND have all eight neighbours walkable, so an enemy is never born
    boxed in between trees (the cause of 'stuck' enemies). Anything else moves to the nearest tile that qualifies."""
    seen, walk = reach_from(m, ts, m.start)

    def roomy(x, y):
        return (x, y) in seen and all(walk(x + dx, y + dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1))
    taken = set()
    out = []
    for sp in m.spawns:
        x, y = sp[1], sp[2]
        if roomy(x, y) and (x, y) not in taken:
            taken.add((x, y))
            out.append(sp)
            continue
        best = None
        for r in range(1, 10):
            for dy in range(-r, r + 1):
                for dx in range(-r, r + 1):
                    c = (x + dx, y + dy)
                    if roomy(*c) and c not in taken and m.kind[c[1]][c[0]] == 'grass' and 3 <= c[0] <= 28 and 3 <= c[1] <= 28:
                        best = c
                        break
                if best:
                    break
            if best:
                break
        if best:
            taken.add(best)
            sp = (sp[0], best[0], best[1], sp[3])
        out.append(sp)
    m.spawns = out


# =========================================================================== SUNMEADOW ROAD
def build_road(ts):
    m = MapB(ts, 303)
    m.biome = 'village'
    rng = random.Random(3030)
    border(m, [(14, 0, 17, 1), (30, 20, 31, 23)])
    main = [(15, 0), (15, 5), (11, 9), (9, 14), (11, 19), (17, 22), (24, 21), (31, 21)]
    m.road(main, 2)
    m.road([(9, 14), (5, 17), (4, 20)], 2)            # down to the pond
    m.road([(15, 5), (21, 6), (26, 8)], 2)            # up to the old waystone
    m.road([(17, 22), (19, 26), (22, 27)], 2)         # to the southern camp
    # pond in the south-west
    m.rect('water', 4, 24, 9, 27)
    m.rect('water', 5, 23, 8, 23)
    m.rect('water', 5, 28, 8, 28)
    # the old waystone: a ring of rocks on a little rise
    for (x, y) in ((25, 4), (27, 4), (24, 6), (28, 6), (25, 8), (27, 8)):
        m.setk(x, y, 'rock')
    m.npcs = [('NPC_TRAVELER', 19, 18, 0)]
    for n in m.npcs:
        m.reserve(n[1], n[2], 1)
    put_sign(m, 13, 2, 'SUNMEADOW ROAD. EAST THROUGH AMBERWOOD LIES AMBERFALL. RECOMMENDED LEVEL 22.')
    put_sign(m, 26, 10, 'THE OLD WAYSTONE. TRAVELERS LEAVE AMBER HERE FOR LUCK. THEN THE WOLVES TAKE IT.')
    put_chest(m, 5, 21, 'ITEM_HIPOTION', 2, 23)
    # scatter: big oaks, shrubs, rocks, flower beds
    avoid = [(14, 0, 17, 5)]
    m.scatter_big_oaks(7, (4, 4, 27, 27), avoid)
    m.scatter('bush', 18, (3, 3, 28, 28), 0, avoid)
    m.scatter('rock', 7, (3, 3, 28, 28), 0, avoid)
    # camp props around Kess
    for (x, y, nm) in ((17, 17, 'hay'), (18, 17, 'hay'), (21, 18, 'crate'), (21, 19, 'barrel'), (17, 19, 'log_pile')):
        m.deco(x, y, nm)
    for (x, y) in ((6, 23), (9, 23), (10, 25), (3, 25), (3, 28), (10, 27)):
        m.deco(x, y, 'reeds')
    sp = []
    for (x, y) in ((7, 10), (12, 14), (20, 24), (26, 15), (7, 19)):
        sp.append(('EN_SLIME', x, y, 0))
    for (x, y) in ((13, 18), (22, 12), (18, 10), (25, 24)):
        sp.append(('EN_BAT', x, y, 0))
    for (x, y) in ((24, 18), (11, 24), (21, 8)):
        sp.append(('EN_WOLF', x, y, 0))
    m.spawns = sp
    m.warps = [(14, 0, 17, 0, AREA_VILLAGE, 15, 28, 1), (31, 20, 31, 23, AREA_AMBERWOOD, 1, 21, 3)]
    m.start = (15, 2)
    m.finalize()
    snap_spawns(m, ts)
    return m


# =========================================================================== AMBERWOOD
def build_amberwood(ts):
    m = MapB(ts, 404)
    m.biome = 'forest'
    rng = random.Random(4040)
    border(m, [(0, 20, 1, 23), (30, 8, 31, 11)], 'pine', 'oak')
    path = [(0, 21), (7, 21), (11, 17), (9, 12), (14, 9), (22, 10), (31, 9)]
    m.road(path, 2)
    clear = [(3, 23, 10, 29), (16, 16, 27, 25), (20, 3, 28, 8), (3, 4, 9, 9)]
    thin_trees(m, rng, 0.40, clear, ('oak', 'pine'), 0.7)
    open_corridor(m, path_points(path))
    m.road([(11, 17), (17, 19), (22, 21)], 2)         # to the lakeside camp
    m.road([(7, 21), (6, 25)], 2)                     # to the hunters' camp
    m.road([(22, 10), (24, 6)], 2)                    # to the shrine ruin
    m.road([(9, 12), (6, 7)], 2)                      # to the north-west hollow
    # lake
    m.rect('water', 21, 20, 25, 23)
    m.rect('water', 22, 19, 24, 19)
    m.rect('water', 22, 24, 24, 24)
    for y in range(17, 27):
        for x in range(19, 28):
            if m.kind[y][x] in ('tree_pine', 'tree_dead') and 20 <= x <= 26 and 18 <= y <= 25:
                m.kind[y][x] = 'grass'
    put_chest(m, 24, 6, 'ITEM_HIPOTION', 3, 27)
    put_chest(m, 5, 6, 'ITEM_ELIXIR', 1, 29)
    put_sign(m, 3, 19, 'AMBERWOOD. THE LEAVES TURN GOLD HERE EVEN IN SPRING. AMBERFALL IS EAST. RECOMMENDED LEVEL 26.')
    put_sign(m, 12, 15, 'HUNTERS WARNING: WOLVES RUN IN PACKS HERE. AMBER SHARDS WASH UP BY THE LAKE.')
    m.scatter('bush', 14, (3, 5, 28, 28), 0)
    m.scatter('rock', 7, (3, 5, 28, 28), 0)
    m.scatter('grass_f', 5, (3, 5, 28, 28), 0)
    for sp in ((24, 6), (5, 6), (3, 19), (12, 15)):
        m.reserve(sp[0], sp[1], 1)
    # fallen logs, dead trees, mushrooms, camp props
    placed, tries = 0, 0
    while placed < 4 and tries < 400:
        tries += 1
        x, y = rng.randint(3, 27), rng.randint(5, 27)
        if m.k(x, y) == 'grass' and m.k(x + 1, y) == 'grass' and (x, y) not in m.reserved and (x + 1, y) not in m.reserved:
            m.fix(x, y, 'log_l')
            m.fix(x + 1, y, 'log_r')
            placed += 1
    placed, tries = 0, 0
    while placed < 4 and tries < 400:
        tries += 1
        if m.deco(rng.randint(3, 28), rng.randint(6, 27), 'dead2b_trunk', 'dead2b_top'):
            placed += 1
    placed, tries = 0, 0
    while placed < 7 and tries < 400:
        tries += 1
        if m.deco(rng.randint(3, 28), rng.randint(5, 28), 'mushroom_big'):
            placed += 1
    for (x, y, nm) in ((5, 25, 'hay'), (7, 27, 'crate'), (4, 27, 'log_pile'), (18, 21, 'barrel'), (17, 23, 'crate'),
                       (23, 7, 'log_pile'), (4, 5, 'barrel')):
        m.deco(x, y, nm)
    for (x, y) in ((20, 21), (20, 22), (26, 21), (26, 22), (22, 25), (24, 25), (21, 19), (25, 19)):
        m.deco(x, y, 'reeds')
    sp = []
    for (x, y) in ((8, 14), (13, 12), (5, 9), (10, 25), (19, 14)):
        sp.append(('EN_SLIME', x, y, 0))
    for (x, y) in ((16, 11), (20, 9), (13, 20), (24, 14), (7, 17), (27, 12)):
        sp.append(('EN_BAT', x, y, 0))
    for (x, y) in ((10, 23), (8, 8), (19, 17), (25, 10), (15, 25), (27, 17)):
        sp.append(('EN_WOLF', x, y, 0))
    m.spawns = sp
    m.warps = [(0, 20, 0, 23, AREA_ROAD, 30, 21, 2), (31, 8, 31, 11, AREA_AMBERFALL, 1, 15, 3)]
    m.start = (2, 21)
    m.finalize()
    snap_spawns(m, ts)
    return m


# =========================================================================== AMBERFALL (town)
def build_amberfall(ts):
    m = MapB(ts, 505)
    m.biome = 'village'
    border(m, [(0, 14, 1, 17)])
    m.rect('plaza', 11, 12, 20, 19)
    m.road([(0, 15), (10, 15)], 2)
    m.road([(15, 4), (15, 11)], 2)
    m.road([(15, 20), (15, 28)], 2)
    m.road([(21, 15), (28, 15)], 2)
    m.road([(6, 10), (25, 10)], 2)
    m.road([(9, 7), (9, 10)], 1)
    m.road([(22, 7), (22, 10)], 1)
    m.road([(7, 25), (7, 26)], 1)
    m.road([(25, 25), (25, 26)], 1)
    m.road([(7, 26), (26, 26)], 2)
    # buildings: inn (north-west), alchemist (north-east), two homes (south)
    m.house(6, 4, 6, 9, 'r')                       # Mara's inn
    m.house(20, 4, 5, 22, 'b')                     # Wyn's alchemy
    m.house(5, 22, 5, 7, 'b', enter=False)         # a private home
    m.house(23, 22, 5, 25, 'r')                    # Orin's study
    # market stall west of the fountain
    for i, s in enumerate('lmr'):
        m.fix(11 + i, 12, 'awning_' + s)
        m.fix(11 + i, 13, 'stallback_' + s)
        m.fix(11 + i, 14, 'counter_' + s)
    # fountain
    m.fix(15, 15, 'fountain_tl')
    m.fix(16, 15, 'fountain_tr')
    m.fix(15, 16, 'fountain_bl')
    m.fix(16, 16, 'fountain_br')
    # ring of inlay stones, planters and benches around the fountain
    for x in range(13, 19):
        for y in (13, 18):
            if (x, y) not in m.fixed:
                m.fix(x, y, 'plaza_inlay')
    for y in range(14, 18):
        for x in (13, 18):
            if (x, y) not in m.fixed:
                m.fix(x, y, 'plaza_inlay')
    for (x, y) in ((17, 13), (18, 13), (13, 18), (18, 18)):
        m.fix(x, y, 'planter_inlay')
    for (x, y) in ((13, 19), (17, 19)):
        m.fix(x, y, 'bench_plaza_l')
        m.fix(x + 1, y, 'bench_plaza_r')
    # a small walled garden and a well
    for x in range(3, 8):
        m.fix(x, 13, 'fence_h')
    m.deco(4, 14, 'garden_a')
    m.deco(5, 14, 'garden_b')
    m.deco(6, 14, 'garden_a')
    m.deco(24, 12, 'well_base', 'well_roof')
    # east end: the road is closed by a rock fall
    for y in (14, 15, 16):
        m.fix(28, y, 'rock')
        m.fix(29, y, 'rock')
    # npcs outdoors: the stall keeper and townsfolk. Mara (inn), Wyn (alchemy) and Orin (study) are at home.
    m.npcs = [('NPC_VENDOR', 12, 13, 0), ('NPC_TOWNSMAN', 3, 14, 0), ('NPC_TOWNSWOMAN', 13, 21, 0), ('NPC_TOWNSMAN', 8, 13, 2)]
    for n in m.npcs:
        m.reserve(n[1], n[2], 1)
    put_sign(m, 9, 17, 'AMBERFALL. MARKET: BREA. INN: MARA. ALCHEMIST: WYN. STUDY: ORIN.')
    put_sign(m, 26, 14, 'THE EAST ROAD IS CLOSED BY A ROCK FALL. WORKERS WILL CLEAR IT SOON. TO BE CONTINUED.')
    put_chest(m, 28, 24, 'ITEM_ETHER', 3, 30)
    for (hx, hy, hw, door, roof) in ((6, 4, 6, 9, 'r'), (20, 4, 5, 22, 'b'), (5, 22, 5, 7, 'b'), (23, 22, 5, 25, 'r')):
        house_details(m, hx, hy, hw, door, roof)
    lamps(m, [(13, 9), (18, 9), (14, 21), (17, 23), (10, 17), (21, 14), (5, 19), (27, 19), (9, 12), (23, 12)])
    for (x, y, nm) in ((10, 13, 'barrel'), (10, 14, 'crate'), (14, 23, 'hay'), (26, 24, 'hay'), (27, 24, 'hay'), (12, 25, 'log_pile'),
                       (18, 25, 'log_pile'), (11, 8, 'crate'), (12, 8, 'barrel')):
        m.deco(x, y, nm)
    m.scatter('bush', 12, (3, 3, 28, 28), 0, [(10, 9, 22, 21), (3, 3, 12, 9), (19, 3, 26, 9), (3, 20, 10, 29), (22, 20, 29, 29)])
    m.scatter_big_oaks(5, (3, 11, 27, 28), [(10, 9, 22, 21), (14, 20, 17, 29), (3, 14, 9, 17), (20, 14, 29, 17), (3, 20, 11, 29), (21, 20, 29, 29)])
    m.warps = [(0, 14, 0, 17, AREA_AMBERWOOD, 30, 9, 2)]
    import art_interior as AIN
    for (door, y, room) in ((9, 6, 'inn'), (22, 6, 'alchemist'), (25, 24, 'study')):      # door tiles
        ex, ey = AIN.ENTRY[room]
        m.warps.append((door, y, door, y, AIN.AREA_OF[room], ex, ey, 1))
    m.start = (3, 15)
    m.finalize()
    return m
