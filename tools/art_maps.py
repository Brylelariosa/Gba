"""Procedural map construction (32x32 metatiles each)."""
import math
import random
from art_world import (build_outdoor, build_dungeon, path_tile, water_tile, SOLID, H as HASH)

W = H = 32


def vnoise(x, y, seed):
    """smooth value noise in [0,1): drives sunny / shaded patches of grass so the ground is not uniformly speckled."""
    def octave(scale, sd):
        fx, fy = x / scale, y / scale
        ix, iy = int(math.floor(fx)), int(math.floor(fy))
        tx, ty = fx - ix, fy - iy
        tx, ty = tx * tx * (3 - 2 * tx), ty * ty * (3 - 2 * ty)

        def lat(a, b):
            return (HASH(a, b, sd) & 255) / 255.0
        top = lat(ix, iy) * (1 - tx) + lat(ix + 1, iy) * tx
        bot = lat(ix, iy + 1) * (1 - tx) + lat(ix + 1, iy + 1) * tx
        return top * (1 - ty) + bot * ty
    return 0.62 * octave(5.0, seed) + 0.38 * octave(2.3, seed + 17)


def make_pool(spec):
    return [n for n, w in spec for _ in range(w)]


POOLS = {
    'village': {
        'mid': make_pool([('grass_a', 26), ('grass_b', 24), ('grass_c', 7), ('grass_d', 9), ('grass_e', 4), ('grass_clover', 5), ('grass_dand', 4), ('grass_peb', 2), ('grass_tall', 3)]),
        'light': make_pool([('grass_light', 36), ('grass_a', 14), ('grass_b', 10), ('grass_c', 8), ('grass_e', 6), ('grass_dand', 6), ('grass_clover', 3)]),
        'dark': make_pool([('grass_dark', 34), ('grass_a', 12), ('grass_b', 10), ('grass_tall', 10), ('grass_d', 8)]),
    },
    'forest': {
        'mid': make_pool([('grass_a', 18), ('grass_b', 16), ('floor_leaves_a', 14), ('floor_leaves_b', 8), ('grass_d', 8), ('grass_tall', 8), ('grass_dark', 8),
                          ('fern_a', 4), ('fern_b', 3), ('grass_f', 3), ('glow_flowers', 2), ('grass_clover', 3), ('grass_peb', 2)]),
        'light': make_pool([('grass_light', 22), ('grass_a', 14), ('grass_b', 10), ('grass_c', 8), ('glow_flowers', 4), ('fern_a', 4), ('grass_dand', 5)]),
        'dark': make_pool([('grass_dark', 24), ('floor_leaves_b', 20), ('floor_leaves_a', 12), ('fern_b', 8), ('grass_tall', 10), ('grass_f', 4)]),
    },
}


class MapB:
    def __init__(self, ts, seed):
        self.ts = ts
        self.rng = random.Random(seed)
        self.kind = [['grass'] * W for _ in range(H)]
        self.fixed = {}        # (x,y) -> metatile name for ground
        self.over = {}         # (x,y) -> metatile name for overlay
        self.spawns, self.npcs, self.chests, self.warps, self.signs = [], [], [], [], []
        self.var = {}
        self.biome = 'village'
        self.noise_seed = seed
        self.reserved = set()   # cells decorations must keep clear (chests, signs, npcs, warps)

    def k(self, x, y):
        if 0 <= x < W and 0 <= y < H:
            return self.kind[y][x]
        return 'edge'

    def setk(self, x, y, kind):
        if 0 <= x < W and 0 <= y < H:
            self.kind[y][x] = kind

    def rect(self, kind, x0, y0, x1, y1):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.setk(x, y, kind)

    def fix(self, x, y, name, over=None):
        self.kind[y][x] = 'fixed'
        self.fixed[(x, y)] = name
        if over:
            self.over[(x, y)] = over

    def road(self, pts, width=2, kind='path'):
        for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
            n = max(abs(x1 - x0), abs(y1 - y0), 1)
            for i in range(n + 1):
                x = round(x0 + (x1 - x0) * i / n)
                y = round(y0 + (y1 - y0) * i / n)
                for dx in range(width):
                    for dy in range(width):
                        self.setk(x + dx, y + dy, kind)

    def house(self, x, y, w, door_x, roof='r'):
        """roof rows y,y+1 ; wall row y+2"""
        for i in range(w):
            s = 'l' if i == 0 else ('r' if i == w - 1 else 'm')
            self.fix(x + i, y, 'roof%s_%s0' % (roof, s))
            self.fix(x + i, y + 1, 'roof%s_%s1' % (roof, s))
            if x + i == door_x:
                nm = 'wall_door'
            elif i == 0 or i == w - 1:
                nm = 'wall_edge'
            elif (i % 2) == 1:
                nm = 'wall_window'
            else:
                nm = 'wall_plain'
            self.fix(x + i, y + 2, nm)

    def reserve(self, x, y, r=1):
        for dy in range(-r, r + 1):
            for dx in range(-r, r + 1):
                self.reserved.add((x + dx, y + dy))

    def deco(self, x, y, name, over=None):
        """put a decoration on a free grass cell (optionally with an overlay tile above it); False if not possible."""
        if self.k(x, y) != 'grass' or (x, y) in self.reserved:
            return False
        if over:
            if self.k(x, y - 1) not in ('grass', 'path', 'plaza') or (x, y - 1) in self.over or (x, y - 1) in self.reserved:
                return False
            self.over[(x, y - 1)] = over
        self.fix(x, y, name)
        return True

    def tree(self, x, y, kind='oak'):
        self.kind[y][x] = 'tree_' + kind

    # ------------------------------------------------------------------
    def conn(self, x, y, style):
        k = self.k(x, y)
        if style == 'path':
            return k in ('path', 'plaza', 'step')
        if style == 'plaza':
            return k in ('path', 'plaza', 'step')
        if style == 'water':
            return k in ('water', 'edge')
        return False

    def finalize(self):
        ts = self.ts
        ground = [[0] * W for _ in range(H)]
        over = [[0] * W for _ in range(H)]
        cache = {}
        pools = POOLS[self.biome]
        self.clear_trees_near_interactables()
        tree_over = {}
        for y in range(H):
            for x in range(W):
                k = self.kind[y][x]
                name = None
                if k == 'grass':
                    nz = vnoise(x, y, self.noise_seed)
                    pool = pools['light'] if nz > 0.60 else (pools['dark'] if nz < 0.38 else pools['mid'])
                    name = pool[int(self.rng.random() * len(pool))]
                elif k == 'fixed':
                    name = self.fixed[(x, y)]
                elif k in ('path', 'plaza', 'step'):
                    style = 'stone' if k == 'plaza' else 'dirt'
                    nb = [self.conn(x, y - 1, 'path'), self.conn(x + 1, y, 'path'), self.conn(x, y + 1, 'path'), self.conn(x - 1, y, 'path'),
                          self.conn(x + 1, y - 1, 'path'), self.conn(x + 1, y + 1, 'path'), self.conn(x - 1, y + 1, 'path'), self.conn(x - 1, y - 1, 'path')]
                    # blocked kinds count as connected to avoid grass fringes against buildings
                    for i, (dx, dy) in enumerate(((0, -1), (1, 0), (0, 1), (-1, 0), (1, -1), (1, 1), (-1, 1), (-1, -1))):
                        kk = self.k(x + dx, y + dy)
                        if kk in ('fixed', 'edge'):
                            nb[i] = True
                    if style == 'stone' and all(nb):
                        v = HASH(x, y, 5) % 12
                        if v in (0, 1):
                            ground[y][x] = ts.id('plaza_crack')
                            continue
                        if v == 2:
                            ground[y][x] = ts.id('plaza_moss')
                            continue
                    key = (style,) + tuple(nb)
                    if key not in cache:
                        cache[key] = ts.add('p_%s_%d' % (style, len(cache)), path_tile(nb[0], nb[1], nb[2], nb[3], nb[4], nb[5], nb[6], nb[7], style=style), 1, 0)
                    ground[y][x] = cache[key]
                    continue
                elif k == 'water':
                    nb = [self.conn(x, y - 1, 'water'), self.conn(x + 1, y, 'water'), self.conn(x, y + 1, 'water'), self.conn(x - 1, y, 'water'),
                          self.conn(x + 1, y - 1, 'water'), self.conn(x + 1, y + 1, 'water'), self.conn(x - 1, y + 1, 'water'), self.conn(x - 1, y - 1, 'water')]
                    if all(nb) and HASH(x, y, 9) % 5 == 0:
                        ground[y][x] = ts.id('water_lily')
                        continue
                    key = ('w',) + tuple(nb)
                    if key not in cache:
                        cache[key] = ts.add('w_%d' % len(cache), water_tile(*nb), 2, 3)
                    ground[y][x] = cache[key]
                    continue
                elif k in ('tree_oak', 'tree_pine'):
                    t = k[5:]
                    name = t + '_trunk'
                    for (oy, prio, part) in ((y - 1, 2, 'canopy'), (y - 2, 1, 'top')):
                        cur = tree_over.get((x, oy))
                        if oy >= 0 and (cur is None or cur[0] < prio):
                            tree_over[(x, oy)] = (prio, t + '_' + part)
                elif k == 'bush':
                    name = 'bush'
                elif k == 'rock':
                    name = 'moss_rock' if (self.biome == 'forest' and HASH(x, y, 3) % 5 < 2) else 'rock'
                elif k == 'wall':
                    name = 'wall'
                else:
                    name = k
                ground[y][x] = ts.id(name)
        for (x, y), (prio, n) in tree_over.items():
            over[y][x] = ts.id(n)
        for (x, y), n in self.over.items():
            over[y][x] = ts.id(n)
        self.ground, self.overlay = ground, over

    def clear_trees_near_interactables(self):
        """tall canopies hang two rows above their trunk: keep chests, signs, npcs and warps from hiding under one"""
        pts = [(c[0], c[1]) for c in self.chests] + [(sg[0], sg[1]) for sg in self.signs] + [(n[1], n[2]) for n in self.npcs]
        for w in self.warps:
            pts += [(x, y) for x in range(w[0], w[2] + 1) for y in range(w[1], w[3] + 1)]
        for (cx, cy) in pts:
            for dy in range(0, 3):
                for dx in (-1, 0, 1):
                    if self.k(cx + dx, cy + dy) in ('tree_oak', 'tree_pine'):
                        self.kind[cy + dy][cx + dx] = 'grass'

    def big_oak(self, x, y):
        """giant oak: one trunk cell with a 3x2 canopy above it. The six canopy cells are solid ground so the hero
        is never hidden under the leaves. Returns False when the spot is not completely free."""
        cells = [(x + dx, y + dy) for dx in (-1, 0, 1) for dy in (-2, -1, 0)]
        for (cx, cy) in cells:
            if not (2 <= cx <= 29 and 2 <= cy <= 29) or self.k(cx, cy) != 'grass' or (cx, cy) in self.reserved or (cx, cy) in self.over:
                return False
        for dx, side in ((-1, 'l'), (0, 'm'), (1, 'r')):
            self.fix(x + dx, y - 2, 'grass_block', 'goak_top_' + side)
            self.fix(x + dx, y - 1, 'grass_block', 'goak_mid_' + side)
        self.fix(x, y, 'goak_trunk')
        return True

    def scatter_big_oaks(self, n, box, avoid=()):
        placed, tries = 0, 0
        while placed < n and tries < n * 120:
            tries += 1
            x, y = self.rng.randint(box[0], box[2]), self.rng.randint(box[1], box[3])
            if any(a[0] <= x <= a[2] and a[1] <= y <= a[3] for a in avoid):
                continue
            if self.big_oak(x, y):
                placed += 1
        return placed

    def free(self, x, y, margin=0):
        for dy in range(-margin, margin + 1):
            for dx in range(-margin, margin + 1):
                if self.k(x + dx, y + dy) not in ('grass',):
                    return False
        return True

    def scatter(self, kind, n, box, margin=0, avoid=()):
        placed = 0
        tries = 0
        while placed < n and tries < n * 80:
            tries += 1
            x = self.rng.randint(box[0], box[2])
            y = self.rng.randint(box[1], box[3])
            if any(ax0 <= x <= ax1 and ay0 <= y <= ay1 for (ax0, ay0, ax1, ay1) in avoid):
                continue
            if self.free(x, y, margin):
                self.setk(x, y, kind)
                placed += 1


# =========================================================================== VILLAGE
def build_village(ts):
    m = MapB(ts, 101)
    # border trees (two thick), exit gap east
    for y in range(H):
        for x in range(W):
            if x < 2 or y < 2 or y > 29 or x > 29:
                if x > 29 and 13 <= y <= 16:
                    continue
                m.tree(x, y, 'oak' if (x + y) % 3 else 'pine')
    # plaza + roads
    m.rect('plaza', 9, 12, 22, 20)
    m.road([(23, 14), (31, 14)], 2)
    m.road([(15, 21), (15, 29)], 2)
    m.road([(6, 10), (25, 10)], 2)
    m.road([(6, 7), (6, 10)], 1)
    m.road([(24, 7), (24, 10)], 1)
    m.rect('plaza', 15, 21, 16, 22)
    # buildings
    m.house(4, 4, 5, 6, 'r')       # elder house, door at x=6 (cell 2)
    m.house(22, 4, 5, 24, 'b')     # chapel
    m.house(4, 22, 4, 5, 'b')
    m.house(24, 23, 5, 26, 'r')
    # market stall
    for i, s in enumerate('lmr'):
        m.fix(9 + i, 12, 'awning_' + s)
        m.fix(9 + i, 13, 'stallback_' + s)
        m.fix(9 + i, 14, 'counter_' + s)
    # fountain
    m.fix(15, 16, 'fountain_tl')
    m.fix(16, 16, 'fountain_tr')
    m.fix(15, 17, 'fountain_bl')
    m.fix(16, 17, 'fountain_br')
    # pond
    m.rect('water', 3, 26, 8, 28)
    m.rect('water', 4, 25, 7, 25)
    m.rect('water', 4, 29, 7, 29)
    # fences
    for x in range(3, 8):
        m.fix(x, 20, 'fence_h')
    for x in range(23, 30):
        if x != 26:
            m.fix(x, 21, 'fence_h')
    # decor
    m.scatter('bush', 14, (3, 3, 28, 28), 0, [(8, 10, 24, 21), (4, 3, 10, 8), (21, 3, 28, 8)])
    m.scatter('rock', 5, (3, 3, 28, 28), 0, [(8, 10, 24, 21)])
    m.scatter_big_oaks(6, (4, 12, 27, 28), [(8, 10, 24, 21), (14, 21, 17, 29), (22, 22, 29, 29), (3, 20, 9, 29)])
    # signs
    m.fix(27, 13, 'sign')
    m.signs.append((27, 13, 'EAST: WHISPERING WOODS. BEWARE OF WOLVES.'))
    m.fix(13, 23, 'sign')
    m.signs.append((13, 23, 'WELCOME TO EMBERVALE VILLAGE. ELDER MAREN WAITS NEAR THE NORTH HOUSE.'))
    # chest
    m.fix(27, 19, 'chest')
    m.chests.append((27, 19, 'ITEM_POTION', 3, 0))
    # npcs
    m.npcs = [('NPC_ELDER', 6, 9, 0), ('NPC_MERCHANT', 10, 13, 0), ('NPC_HEALER', 24, 8, 0)]
    m.warps = [(31, 13, 31, 16, 1, 1, 15, 3)]
    m.start = (15, 24)
    village_details(m)
    m.finalize()
    return m


def house_details(m, hx, hy, hw, door, roof):
    """window boxes, a chimney with smoke, a doorstep with planters and cast shadows for one house."""
    for i in range(hw):
        if m.fixed.get((hx + i, hy + 2)) == 'wall_window':
            m.fixed[(hx + i, hy + 2)] = 'window_box'
    cx = hx + hw - 2
    if m.fixed.get((cx, hy)) == 'roof%s_m0' % roof and m.k(cx, hy - 1) in ('grass', 'path') and (cx, hy - 1) not in m.over:
        m.fixed[(cx, hy)] = 'chimney_roof_' + roof
        m.over[(cx, hy - 1)] = 'chimney_top_' + roof
    y = hy + 3
    m.deco(door, y, 'step_stone')
    for dx in (-1, 1):
        if (door + dx) in range(hx, hx + hw) and m.fixed.get((door + dx, hy + 2)) != 'wall_door':
            m.deco(door + dx, y, 'planter')
    for i in range(hw):
        m.deco(hx + i, y, 'shadow_eave')
    m.deco(hx + hw, hy + 2, 'shadow_side')
    m.deco(hx + hw, hy + 3, 'shadow_side')


def village_details(m):
    for (x, y) in ((27, 13), (13, 23), (27, 19)):
        m.reserve(x, y, 1)
    for n in m.npcs:
        m.reserve(n[1], n[2], 1)
    for (hx, hy, hw, door, roof) in ((4, 4, 5, 6, 'r'), (22, 4, 5, 24, 'b'), (4, 22, 4, 5, 'b'), (24, 23, 5, 26, 'r')):
        house_details(m, hx, hy, hw, door, roof)
    # stone inlay ring around the fountain, planters on its corners and benches facing it
    for x in range(13, 19):
        for y in (14, 19):
            m.fix(x, y, 'plaza_inlay')
    for y in range(15, 19):
        for x in (13, 18):
            m.fix(x, y, 'plaza_inlay')
    for (x, y) in ((13, 14), (18, 14), (13, 19), (18, 19)):
        m.fix(x, y, 'planter_inlay')
    for (x, y) in ((11, 16), (20, 16)):
        m.fix(x, y, 'bench_plaza')
    # lamp posts along the roads, a well, market goods, gardens, hay and wood
    for (x, y) in ((12, 9), (19, 9), (14, 24), (17, 26), (24, 13), (8, 17), (23, 18)):
        m.deco(x, y, 'lamp_base', 'lamp_top')
    m.deco(7, 15, 'well_base', 'well_roof')
    m.deco(8, 13, 'barrel')
    m.deco(8, 14, 'crate')
    m.deco(7, 13, 'barrel')
    for x in (9, 10, 11):
        m.deco(x, 23, 'garden_a')
        m.deco(x, 24, 'garden_b')
    for (x, y) in ((27, 17), (28, 17), (28, 18)):
        m.deco(x, y, 'hay')
    for (x, y) in ((10, 6), (20, 6), (12, 21)):
        m.deco(x, y, 'log_pile')
    for (x, y) in ((9, 26), (9, 28), (8, 25), (8, 29), (3, 25), (3, 29)):
        m.deco(x, y, 'reeds')


def forest_details(m):
    rng = random.Random(909)
    for (x, y) in ((25, 8), (6, 26), (27, 25), (4, 12), (5, 14), (12, 5)):
        m.reserve(x, y, 1)
    for sp in m.spawns:
        m.reserve(sp[1], sp[2], 0)
    # fallen logs (two tiles wide)
    placed, tries = 0, 0
    while placed < 5 and tries < 400:
        tries += 1
        x, y = rng.randint(3, 27), rng.randint(5, 27)
        if m.k(x, y) == 'grass' and m.k(x + 1, y) == 'grass' and (x, y) not in m.reserved and (x + 1, y) not in m.reserved:
            m.fix(x, y, 'log_l')
            m.fix(x + 1, y, 'log_r')
            placed += 1
    # dead trees, big mushrooms
    placed, tries = 0, 0
    while placed < 5 and tries < 400:
        tries += 1
        x, y = rng.randint(3, 28), rng.randint(6, 27)
        if m.deco(x, y, 'dead_trunk', 'dead_top'):
            placed += 1
    placed, tries = 0, 0
    while placed < 8 and tries < 400:
        tries += 1
        x, y = rng.randint(3, 28), rng.randint(5, 28)
        if m.deco(x, y, 'mushroom_big'):
            placed += 1
    # reeds around the pond
    for (x, y) in ((21, 16), (26, 16), (21, 21), (26, 21), (20, 18), (27, 19), (20, 19), (27, 18)):
        m.deco(x, y, 'reeds')


# =========================================================================== FOREST
def build_forest(ts):
    m = MapB(ts, 202)
    for y in range(H):
        for x in range(W):
            if x < 2 or y < 2 or y > 29 or x > 29:
                if x < 2 and 14 <= y <= 16:
                    continue
                m.tree(x, y, 'pine')
    path = [(0, 15), (7, 15), (11, 13), (13, 9), (14, 6), (14, 3)]
    m.road(path, 2)
    # clearings (kept free of trees)
    clear = [(3, 10, 12, 21), (12, 15, 22, 22), (19, 6, 27, 13), (3, 23, 11, 28), (23, 22, 28, 28)]
    # scatter forest trees everywhere except clearings and path
    rng = m.rng
    for y in range(2, 30):
        for x in range(2, 30):
            if m.kind[y][x] != 'grass':
                continue
            near_path = False
            for (px, py) in [(a, b) for (a, b) in ((p[0], p[1]) for p in path)]:
                pass
            inclr = any(c[0] <= x <= c[2] and c[1] <= y <= c[3] for c in clear)
            if inclr:
                if rng.random() < 0.05:
                    m.tree(x, y, 'pine')
                continue
            if rng.random() < 0.42:
                m.tree(x, y, 'pine' if rng.random() < 0.8 else 'oak')
    # re-clear a corridor around the path to keep it open
    pts = []
    for (x0, y0), (x1, y1) in zip(path, path[1:]):
        n = max(abs(x1 - x0), abs(y1 - y0), 1)
        for i in range(n + 1):
            pts.append((round(x0 + (x1 - x0) * i / n), round(y0 + (y1 - y0) * i / n)))
    for (px, py) in pts:
        for dy in range(-2, 4):
            for dx in range(-2, 4):
                x, y = px + dx, py + dy
                if 2 <= x < 30 and 2 <= y < 30 and m.kind[y][x] in ('tree_pine', 'tree_oak'):
                    m.kind[y][x] = 'grass'
    # branch paths to chests / clearings
    m.road([(11, 13), (18, 16), (24, 13), (25, 9)], 2)
    m.road([(8, 15), (7, 22), (6, 26)], 2)
    for y in range(4, 24):
        for x in range(16, 30):
            pass
    # pond
    m.rect('water', 21, 17, 26, 20)
    m.rect('water', 22, 16, 25, 16)
    m.rect('water', 22, 21, 25, 21)
    for y in range(14, 24):
        for x in range(19, 29):
            if m.kind[y][x] in ('tree_pine', 'tree_oak') and (20 <= x <= 27 and 15 <= y <= 22):
                m.kind[y][x] = 'grass'
    # decor
    m.scatter('bush', 16, (3, 5, 28, 28), 0)
    m.scatter('rock', 8, (3, 5, 28, 28), 0)
    m.scatter('grass_f', 6, (3, 5, 28, 28), 0)
    # crypt arch
    for i, nm in enumerate(('arch_tl', 'arch_tm', 'arch_tr')):
        m.fix(13 + i, 1, nm)
    for i, nm in enumerate(('arch_bl', 'arch_bm', 'arch_br')):
        m.fix(13 + i, 2, nm)
    m.rect('path', 13, 3, 15, 4)
    m.rect('path', 14, 3, 15, 5)
    # chests
    for (x, y, it, q, i) in ((25, 8, 'CH_WEAPON2', 1, 1), (6, 26, 'ITEM_CRYPT_KEY', 1, 2), (27, 25, 'ITEM_ETHER', 2, 3), (4, 12, 'ITEM_CHARM_VIGOR', 1, 4)):
        m.kind[y][x] = 'fixed'
        m.fixed[(x, y)] = 'chest'
        m.chests.append((x, y, it, q, i))
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                xx, yy = x + dx, y + dy
                if (dx or dy) and m.kind[yy][xx] in ('tree_pine', 'tree_oak') and dy >= 0:
                    pass
    for (x, y) in ((25, 8), (6, 26), (27, 25), (4, 12)):
        for dx in range(-1, 2):
            for dy in range(-1, 2):
                xx, yy = x + dx, y + dy
                if m.kind[yy][xx] in ('tree_pine', 'tree_oak'):
                    m.kind[yy][xx] = 'grass'
    m.fix(5, 14, 'sign')
    m.signs.append((5, 14, 'WHISPERING WOODS. THE CRYPT LIES NORTH. A CRYPT KEY IS SAID TO REST IN THE SOUTHWEST.'))
    m.fix(12, 5, 'sign')
    m.signs.append((12, 5, 'HOLLOW CRYPT. ONLY THE BRAVE RETURN.'))
    sp = []
    for (x, y) in ((5, 12), (8, 18), (10, 11), (6, 20), (11, 17)):
        sp.append(('EN_SLIME', x, y, 0))
    for (x, y) in ((16, 17), (20, 14), (14, 20), (22, 10), (18, 9)):
        sp.append(('EN_BAT', x, y, 0))
    for (x, y) in ((8, 25), (5, 24), (24, 8), (13, 7)):
        sp.append(('EN_WOLF', x, y, 0))
    m.spawns = sp
    m.warps = [(0, 14, 0, 16, 0, 28, 15, 2), (14, 2, 14, 2, 2, 15, 29, 1)]
    m.start = (2, 15)
    m.biome = 'forest'
    forest_details(m)
    m.finalize()
    return m


def finalize_dungeon(m, floors, torch_cols, special=None, front_probs=(0.12, 0.08, 0.0)):
    """turn a carved dungeon (kinds 'wall' / 'floor' / 'fixed') into metatile ids.
    Walls get a lit brick face where floor lies south, a lit cap where floor lies north, ledges on their sides and a
    stone cap band behind the faces; floors under a wall face get an ambient-shadow variant."""
    ts, rng = m.ts, m.rng
    ground = [[0] * W for _ in range(H)]
    over = [[0] * W for _ in range(H)]
    shade_of = {'floor_a': 'floor_a_s', 'floor_b': 'floor_b_s', 'floor_c': 'floor_c_s', 'floor_d': 'floor_d_s', 'floor_e': 'floor_e_s'}

    def wall(x, y):
        return 0 <= x < W and 0 <= y < H and m.kind[y][x] == 'wall'

    def open_(x, y):
        return 0 <= x < W and 0 <= y < H and m.kind[y][x] != 'wall'
    p_moss, p_skull, p_web = front_probs
    for y in range(H):
        for x in range(W):
            k = m.kind[y][x]
            if k == 'floor':
                nm = (special(x, y) if special else None) or rng.choice(floors)
                if wall(x, y - 1) and nm in shade_of:
                    nm = shade_of[nm]
                elif nm == 'floor_a' and wall(x - 1, y):
                    nm = 'floor_a_sl'
                elif nm == 'floor_a' and wall(x + 1, y):
                    nm = 'floor_a_sr'
            elif k == 'fixed':
                nm = m.fixed[(x, y)]
            elif k == 'wall':
                if open_(x, y + 1):
                    nm = 'wall_front'
                    r = rng.random()
                    if (x, y) in torch_cols:
                        nm = 'wall_front_torch'
                    elif r < p_moss:
                        nm = 'wall_front_moss'
                    elif r < p_moss + p_skull:
                        nm = 'wall_front_skull'
                    elif r < p_moss + p_skull + p_web:
                        nm = 'wall_front_web'
                elif open_(x, y - 1):
                    nm = 'wall_cap_s'
                elif open_(x + 1, y):
                    nm = 'wall_edge_r'
                elif open_(x - 1, y):
                    nm = 'wall_edge_l'
                elif wall(x, y + 1) and open_(x, y + 2):
                    nm = 'wall_cap'
                else:
                    nm = 'wall_top'
            else:
                nm = 'floor_a'
            ground[y][x] = ts.id(nm)
    m.ground, m.overlay = ground, over


# =========================================================================== CRYPT
def build_crypt(ts):
    m = MapB(ts, 303)
    for y in range(H):
        for x in range(W):
            m.kind[y][x] = 'wall'

    def carve(x0, y0, x1, y1):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                m.kind[y][x] = 'floor'
    carve(3, 1, 28, 10)       # boss chamber (26x10: room to dodge)
    carve(15, 11, 16, 12)     # boss corridor
    carve(6, 14, 25, 24)      # main hall
    carve(14, 25, 17, 26)
    carve(11, 27, 20, 30)     # entrance room
    carve(1, 16, 4, 22)       # west room
    carve(5, 19, 5, 20)
    carve(27, 16, 30, 22)     # east room
    carve(26, 19, 26, 20)
    # doors
    m.kind[13][15] = 'fixed'
    m.fixed[(15, 13)] = 'door_l'
    m.kind[13][16] = 'fixed'
    m.fixed[(16, 13)] = 'door_r'
    # pillars
    for (x, y) in ((9, 16), (9, 22), (22, 16), (22, 22), (13, 16), (18, 16), (13, 22), (18, 22), (7, 3), (7, 8), (24, 3), (24, 8)):
        m.kind[y][x] = 'fixed'
        m.fixed[(x, y)] = 'pillar'
    for (x, y) in ((8, 15), (23, 15)):
        m.kind[y][x] = 'fixed'
        m.fixed[(x, y)] = 'coffin'
    for (x, y) in ((11, 2), (20, 2)):
        m.kind[y][x] = 'fixed'
        m.fixed[(x, y)] = 'brazier'
    # carpet in boss chamber
    for y in range(1, 11):
        m.kind[y][14] = 'fixed'
        m.fixed[(14, y)] = 'carpet_l'
        for x in (15, 16):
            m.kind[y][x] = 'fixed'
            m.fixed[(x, y)] = 'carpet_c'
        m.kind[y][17] = 'fixed'
        m.fixed[(17, y)] = 'carpet_r'
    for (x, y) in ((15, 12), (16, 12)):
        m.kind[y][x] = 'fixed'
        m.fixed[(x, y)] = 'carpet_c'
    # skull seal before the king's chamber (opened with the Skull Key from the depths)
    m.fix(15, 11, 'skull_door_l')
    m.fix(16, 11, 'skull_door_r')
    # stairway down to the depths at the bottom of the west room
    m.fix(2, 22, 'stairs_down_l')
    m.fix(3, 22, 'stairs_down_r')
    m.fix(4, 22, 'sign')
    m.signs.append((4, 22, 'A STAIRWAY DESCENDS INTO THE DEPTHS. RECOMMENDED LEVEL 11. THE BONE WARDEN GUARDS THE SKULL KEY.'))
    # stairs (exit)
    m.kind[30][15] = 'fixed'
    m.fixed[(15, 30)] = 'stairs_up_l'
    m.kind[30][16] = 'fixed'
    m.fixed[(16, 30)] = 'stairs_up_r'
    # chests
    for (x, y, it, q, i) in ((2, 17, 'ITEM_HIPOTION', 2, 5), (29, 17, 'CH_WEAPON3', 1, 6), (7, 15, 'ITEM_ARMOR2', 1, 7), (12, 28, 'ITEM_ELIXIR', 1, 8),
                             (1, 20, 'ITEM_HELM2', 1, 11), (30, 21, 'ITEM_BOOTS2', 1, 12)):
        m.kind[y][x] = 'fixed'
        m.fixed[(x, y)] = 'chest'
        m.chests.append((x, y, it, q, i))
    m.fix(14, 29, 'sign')
    m.signs.append((14, 29, 'THE DEAD STIR BEYOND THE IRON DOOR. FIND THE KEY. RECOMMENDED LEVEL 7.'))
    sp = []
    for (x, y) in ((10, 18), (21, 18), (10, 21), (21, 21), (15, 19), (16, 22), (7, 17), (24, 17), (7, 23), (24, 23), (12, 20), (19, 23), (15, 16), (17, 24)):
        sp.append(('EN_SKELETON', x, y, 0))
    for (x, y) in ((28, 19), (29, 21)):
        sp.append(('EN_SKELETON', x, y, 0))
    for (x, y) in ((12, 19), (19, 20), (14, 15), (17, 23), (8, 20), (23, 19), (11, 24), (20, 15)):
        sp.append(('EN_BAT', x, y, 0))
    sp.append(('EN_BOSS', 15, 4, 0))
    m.spawns = sp
    m.warps = [(15, 30, 16, 30, 1, 14, 4, 0), (2, 22, 3, 22, 3, 4, 27, 1)]
    m.warps.append((15, 1, 16, 1, 4, 15, 28, 1))      # the Cinder Stair: only usable once the Hollow King is dead (see world_check_warp)
    m.start = (15, 29)
    floors = ['floor_a'] * 5 + ['floor_b'] * 5 + ['floor_c'] * 3 + ['floor_d'] * 3 + ['floor_e'] * 2 + ['floor_crack', 'floor_moss', 'floor_bones', 'floor_blood']
    torch_cols = {(8, 0), (14, 0), (17, 0), (23, 0), (8, 13), (13, 13), (18, 13), (23, 13), (15, 25), (12, 26), (19, 26), (2, 15), (29, 15)}

    def special(x, y):
        if 3 <= x <= 28 and y <= 10:                  # the king's chamber: regal, mostly clean flagstones
            if x in (11, 12, 19, 20) and y in (5, 6):
                return 'floor_rune'
            return m.rng.choice(['floor_a', 'floor_b', 'floor_c', 'floor_d', 'floor_a', 'floor_b'])
        return None
    finalize_dungeon(m, floors, torch_cols, special, (0.12, 0.08, 0.0))
    return m


def build_ember(ts):
    """The Cinder Hollows: the forge beneath old Emberhold. Arrival room -> corridor -> a great hall split by a lava
    river (one bridge) -> the Cinder Tyrant's arena, with a treasure chamber to each side of the arrival room."""
    m = MapB(ts, 505)
    for y in range(H):
        for x in range(W):
            m.kind[y][x] = 'wall'

    def carve(x0, y0, x1, y1):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                m.kind[y][x] = 'floor'
    carve(10, 23, 21, 30)     # arrival room
    carve(15, 18, 16, 22)     # corridor north
    carve(4, 11, 27, 17)      # the great hall
    carve(15, 9, 16, 10)      # passage to the arena
    carve(4, 1, 27, 8)        # the tyrant's arena (24x8)
    carve(2, 20, 8, 27)       # west chamber
    carve(9, 25, 9, 26)
    carve(23, 20, 29, 27)     # east chamber
    carve(22, 25, 22, 26)
    m.fix(15, 30, 'stairs_up_l')
    m.fix(16, 30, 'stairs_up_r')
    # lava: a river across the hall with one bridge, pools in the corners
    for x in range(4, 28):
        if not 14 <= x <= 17:
            m.fix(x, 14, 'lava_' + 'abc'[x % 3])
    for (x, y) in ((5, 12), (6, 12), (5, 13), (6, 13), (24, 15), (25, 15), (24, 16), (25, 16),
                   (4, 1), (5, 1), (6, 1), (4, 2), (5, 2), (6, 2), (25, 1), (26, 1), (27, 1), (25, 2), (26, 2), (27, 2),
                   (3, 24), (4, 24), (3, 25), (4, 25), (27, 23), (28, 23), (27, 24), (28, 24)):
        m.fix(x, y, 'lava_' + 'abc'[(x + y) % 3])
    for (x, y) in ((7, 4), (24, 4), (10, 15), (21, 12), (13, 26), (18, 26)):
        m.fix(x, y, 'pillar')
    for (x, y) in ((11, 1), (20, 1), (12, 24), (19, 24)):
        m.fix(x, y, 'brazier')
    for (x, y) in ((8, 7), (23, 7), (6, 20), (26, 26), (18, 17)):
        m.fix(x, y, 'bone_pile')
    for (x, y, it, q, i) in ((3, 21, 'CH_WEAPON5', 1, 17), (7, 26, 'ITEM_ARMOR5', 1, 18), (28, 21, 'ITEM_HIPOTION', 3, 19),
                             (24, 26, 'ITEM_ELIXIR', 1, 20), (9, 2, 'ITEM_HELM5', 1, 21), (22, 2, 'ITEM_BOOTS5', 1, 22)):
        m.fix(x, y, 'chest')
        m.chests.append((x, y, it, q, i))
    for (x, y, txt) in ((13, 28, 'THE CINDER HOLLOWS. THE FORGE OF OLD EMBERHOLD STILL BURNS. RECOMMENDED LEVEL 14.'),
                        (5, 22, 'FORGE LOG: THE KING PAID THE FIRE-SPIRIT A SOUL FOR EVERY LINK OF THE CROWN. NO ONE WAS ASKED.'),
                        (26, 22, "CORVIN'S LAST ORDER: GUARD THE KEY. GUARD THE KING. FORGET THE FIRE. HE FORGOT NOTHING."),
                        (12, 13, "ALDRIC'S CROWN WAS QUENCHED HERE. THE FIRE NEVER FORGAVE HIM."),
                        (14, 11, 'THE TYRANT WAITS FOR A CROWN TO LIGHT. LEAVE IT DARK.')):
        m.fix(x, y, 'sign')
        m.signs.append((x, y, txt))
    sp = [('EN_TYRANT', 15, 4, 0)]
    for (t, x, y) in (('EN_SLIME', 13, 25), ('EN_SLIME', 18, 27), ('EN_BAT', 15, 24),
                      ('EN_SKELETON', 5, 23), ('EN_SKELETON', 7, 24), ('EN_WOLF', 4, 26),
                      ('EN_SKELETON', 25, 22), ('EN_SKELETON', 27, 25), ('EN_WOLF', 26, 21),
                      ('EN_WOLF', 8, 16), ('EN_WOLF', 22, 17), ('EN_SLIME', 11, 16), ('EN_SLIME', 20, 16),
                      ('EN_BAT', 7, 12), ('EN_BAT', 15, 12), ('EN_BAT', 24, 12), ('EN_SKELETON', 10, 13), ('EN_SKELETON', 22, 12)):
        sp.append((t, x, y, 0))
    m.spawns = sp
    m.warps = [(15, 30, 16, 30, 2, 15, 3, 0)]
    m.start = (15, 28)
    floors = ['floor_a'] * 4 + ['floor_b'] * 4 + ['floor_c'] * 3 + ['floor_d'] * 3 + ['floor_e'] * 2 + ['floor_crack'] * 3 + ['floor_bones', 'floor_blood']
    torch_cols = {(8, 0), (14, 0), (17, 0), (23, 0), (7, 10), (12, 10), (19, 10), (24, 10), (12, 22), (19, 22)}

    def special(x, y):
        if 4 <= x <= 27 and y <= 8:
            if x in (14, 15, 16, 17) and y in (3, 4, 5):
                return 'floor_rune'
            return m.rng.choice(['floor_a', 'floor_b', 'floor_c', 'floor_d', 'floor_crack'])
        return None
    finalize_dungeon(m, floors, torch_cols, special, (0.10, 0.06, 0.04))
    return m


def build_depths(ts):
    """Crypt Depths: a second dungeon floor. Arrival room -> long hall -> west rooms -> the Bone Warden's arena,
    with an optional treasure vault to the north. Enemies here use the toughest tier."""
    m = MapB(ts, 404)
    for y in range(H):
        for x in range(W):
            m.kind[y][x] = 'wall'

    def carve(x0, y0, x1, y1):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                m.kind[y][x] = 'floor'
    carve(2, 24, 8, 29)       # arrival room
    carve(9, 26, 13, 27)      # corridor east
    carve(14, 19, 27, 29)     # central hall
    carve(9, 21, 13, 22)      # corridor west
    carve(2, 12, 10, 22)      # west room
    carve(8, 11, 9, 11)       # doorway north
    carve(2, 1, 16, 10)       # the warden's arena (15x10)
    carve(20, 12, 21, 18)     # corridor north
    carve(19, 4, 29, 11)      # treasure vault
    # stairs back up
    m.fix(3, 29, 'stairs_up_l')
    m.fix(4, 29, 'stairs_up_r')
    # pillars, braziers, coffins, bone piles
    for (x, y) in ((17, 22), (17, 26), (24, 22), (24, 26), (4, 15), (7, 15), (4, 19), (7, 19), (4, 3), (4, 8), (14, 3), (14, 8)):
        m.fix(x, y, 'pillar')
    for (x, y) in ((6, 2), (12, 2), (20, 5), (28, 5), (3, 25), (8, 25)):
        m.fix(x, y, 'brazier')
    for (x, y) in ((22, 8), (27, 8), (24, 10)):
        m.fix(x, y, 'coffin')
    for (x, y) in ((19, 25), (22, 20), (14, 28), (26, 20), (6, 21), (3, 13), (23, 6)):
        m.fix(x, y, 'bone_pile')
    # carpet leading to the warden
    for y in range(1, 11):
        m.fix(7, y, 'carpet_l')
        m.fix(8, y, 'carpet_c')
        m.fix(9, y, 'carpet_c')
        m.fix(10, y, 'carpet_r')
    for (x, y) in ((8, 11), (9, 11)):
        m.fix(x, y, 'carpet_c')
    # chests: the vault holds the two best pieces of gear, the west room a restorative
    for (x, y, it, q, i) in ((22, 4, 'CH_WEAPON4', 1, 13), (25, 4, 'ITEM_ARMOR4', 1, 14), (28, 4, 'ITEM_HIPOTION', 3, 15), (2, 13, 'ITEM_ELIXIR', 1, 16)):
        m.fix(x, y, 'chest')
        m.chests.append((x, y, it, q, i))
    m.fix(6, 25, 'sign')
    m.signs.append((6, 25, 'THE AIR IS COLD AND THE BONES ARE RESTLESS. RECOMMENDED LEVEL 11. THE WARDEN WAITS IN THE NORTH-WEST.'))
    m.fix(4, 12, 'sign')
    m.signs.append((4, 12, 'BEYOND LIES THE BONE WARDEN. DEFEAT HIM TO CLAIM THE SKULL KEY.'))
    sp = [('EN_WARDEN', 9, 5, 0)]
    for (x, y) in ((16, 24), (19, 22), (22, 25), (25, 21), (18, 28), (23, 28), (26, 27)):
        sp.append(('EN_SKELETON', x, y, 0))
    for (x, y) in ((19, 21), (24, 24), (16, 20), (26, 28)):
        sp.append(('EN_BAT', x, y, 0))
    for (x, y) in ((22, 7), (26, 7), (24, 9)):
        sp.append(('EN_SKELETON', x, y, 0))
    for (x, y) in ((21, 10), (28, 10)):
        sp.append(('EN_BAT', x, y, 0))
    for (x, y) in ((3, 16), (6, 17), (3, 20), (7, 21)):
        sp.append(('EN_SKELETON', x, y, 0))
    for (x, y) in ((6, 14), (4, 18)):
        sp.append(('EN_BAT', x, y, 0))
    m.spawns = sp
    m.warps = [(3, 29, 4, 29, 2, 3, 20, 1)]
    m.start = (4, 27)
    floors = ['floor_a'] * 5 + ['floor_b'] * 5 + ['floor_c'] * 3 + ['floor_d'] * 3 + ['floor_e'] * 2 + ['floor_crack'] * 2 + ['floor_moss', 'floor_bones', 'floor_bones', 'floor_blood', 'floor_blood']
    torch_cols = {(4, 0), (9, 0), (14, 0), (3, 23), (7, 23), (16, 18), (24, 18), (22, 3), (27, 3), (4, 11), (13, 11), (14, 18), (27, 18)}

    def special(x, y):
        if 2 <= x <= 16 and y <= 10:                   # the warden's arena
            if x in (4, 5, 12, 13) and y in (5, 6):
                return 'floor_rune'
            return m.rng.choice(['floor_a', 'floor_b', 'floor_c', 'floor_d', 'floor_b', 'floor_blood'])
        return None
    finalize_dungeon(m, floors, torch_cols, special, (0.10, 0.10, 0.10))
    return m


def preview(path, m, ts, scale=2):
    from PIL import Image
    from common import hx
    import numpy as np
    pals = ts.palette()
    def tile_arr(t):
        a = np.zeros((8, 8), np.uint8)
        for y in range(8):
            for x in range(8):
                a[y, x] = (ts.tiles[t][y] >> (4 * x)) & 15
        return a
    img = np.zeros((H * 16, W * 16, 3), np.uint8)
    for layer in (m.ground, m.overlay):
        for y in range(H):
            for x in range(W):
                mid = layer[y][x]
                if layer is m.overlay and mid == 0:
                    continue
                for q, e in enumerate(ts.meta[mid]):
                    t = e & 0x3FF
                    bank = (e >> 12) & 15
                    a = tile_arr(t)
                    for yy in range(8):
                        for xx in range(8):
                            v = a[yy, xx]
                            if v == 0 and layer is m.overlay:
                                continue
                            if v == 0:
                                continue
                            img[y * 16 + (q // 2) * 8 + yy, x * 16 + (q % 2) * 8 + xx] = hx(pals[bank * 16 + v])
    im = Image.fromarray(img).resize((W * 16 * scale, H * 16 * scale), Image.NEAREST)
    im.save(path)


if __name__ == '__main__':
    o = build_outdoor()
    v = build_village(o)
    f = build_forest(o)
    d = build_dungeon()
    c = build_crypt(d)
    print('outdoor metatiles', len(o.meta), 'tiles', len(o.tiles))
    print('dungeon metatiles', len(d.meta), 'tiles', len(d.tiles))
    preview('/tmp/map_village.png', v, o, 1)
    preview('/tmp/map_forest.png', f, o, 1)
    preview('/tmp/map_crypt.png', c, d, 1)
