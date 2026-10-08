"""Procedural map construction (32x32 metatiles each)."""
import random
from art_world import (build_outdoor, build_dungeon, path_tile, water_tile, SOLID)

W = H = 32


class MapB:
    def __init__(self, ts, seed):
        self.ts = ts
        self.rng = random.Random(seed)
        self.kind = [['grass'] * W for _ in range(H)]
        self.fixed = {}        # (x,y) -> metatile name for ground
        self.over = {}         # (x,y) -> metatile name for overlay
        self.spawns, self.npcs, self.chests, self.warps, self.signs = [], [], [], [], []
        self.var = {}

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
        weights = [('grass_a', 34), ('grass_b', 30), ('grass_c', 7), ('grass_d', 12), ('grass_e', 5), ('grass_f', 2)]
        pool = [n for n, w in weights for _ in range(w)]
        for y in range(H):
            for x in range(W):
                k = self.kind[y][x]
                name = None
                if k == 'grass':
                    h = (x * 7919 + y * 104729 + 13) % len(pool)
                    r = self.rng.random()
                    name = pool[int(r * len(pool))]
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
                    key = (style,) + tuple(nb)
                    if key not in cache:
                        cache[key] = ts.add('p_%s_%d' % (style, len(cache)), path_tile(nb[0], nb[1], nb[2], nb[3], nb[4], nb[5], nb[6], nb[7], style=style), 1, 0)
                    ground[y][x] = cache[key]
                    continue
                elif k == 'water':
                    nb = [self.conn(x, y - 1, 'water'), self.conn(x + 1, y, 'water'), self.conn(x, y + 1, 'water'), self.conn(x - 1, y, 'water'),
                          self.conn(x + 1, y - 1, 'water'), self.conn(x + 1, y + 1, 'water'), self.conn(x - 1, y + 1, 'water'), self.conn(x - 1, y - 1, 'water')]
                    key = ('w',) + tuple(nb)
                    if key not in cache:
                        cache[key] = ts.add('w_%d' % len(cache), water_tile(*nb), 2, 3)
                    ground[y][x] = cache[key]
                    continue
                elif k == 'tree_oak':
                    name = 'oak_trunk'
                    if y > 0:
                        over[y - 1][x] = ts.id('oak_canopy')
                elif k == 'tree_pine':
                    name = 'pine_trunk'
                    if y > 0:
                        over[y - 1][x] = ts.id('pine_canopy')
                elif k == 'bush':
                    name = 'bush'
                elif k == 'rock':
                    name = 'rock'
                elif k == 'wall':
                    name = 'wall'
                else:
                    name = k
                ground[y][x] = ts.id(name)
        for (x, y), n in self.over.items():
            over[y][x] = ts.id(n)
        self.ground, self.overlay = ground, over

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
    m.scatter('tree_oak', 6, (3, 11, 28, 28), 1, [(8, 10, 24, 21), (14, 21, 17, 29)])
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
    m.finalize()
    return m


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
    m.finalize()
    return m


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
    carve(7, 2, 24, 10)       # boss chamber
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
    for (x, y) in ((9, 16), (9, 22), (22, 16), (22, 22), (13, 16), (18, 16), (13, 22), (18, 22), (9, 4), (9, 8), (22, 4), (22, 8)):
        m.kind[y][x] = 'fixed'
        m.fixed[(x, y)] = 'pillar'
    for (x, y) in ((8, 15), (23, 15)):
        m.kind[y][x] = 'fixed'
        m.fixed[(x, y)] = 'coffin'
    for (x, y) in ((11, 3), (20, 3)):
        m.kind[y][x] = 'fixed'
        m.fixed[(x, y)] = 'brazier'
    # carpet in boss chamber
    for y in range(3, 11):
        m.kind[y][14] = 'fixed'
        m.fixed[(14, y)] = 'carpet_l'
        for x in (15, 16):
            m.kind[y][x] = 'fixed'
            m.fixed[(x, y)] = 'carpet_c'
        m.kind[y][17] = 'fixed'
        m.fixed[(17, y)] = 'carpet_r'
    for (x, y) in ((15, 11), (16, 11), (15, 12), (16, 12)):
        m.kind[y][x] = 'fixed'
        m.fixed[(x, y)] = 'carpet_c'
    # stairs (exit)
    for x in (15, 16):
        m.kind[30][x] = 'fixed'
        m.fixed[(x, 30)] = 'stairs'
    # chests
    for (x, y, it, q, i) in ((2, 17, 'ITEM_HIPOTION', 2, 5), (29, 17, 'CH_WEAPON3', 1, 6), (7, 15, 'ITEM_ARMOR2', 1, 7), (12, 28, 'ITEM_ELIXIR', 1, 8)):
        m.kind[y][x] = 'fixed'
        m.fixed[(x, y)] = 'chest'
        m.chests.append((x, y, it, q, i))
    m.fix(14, 29, 'sign')
    m.signs.append((14, 29, 'THE DEAD STIR BEYOND THE IRON DOOR. FIND THE KEY.'))
    sp = []
    for (x, y) in ((10, 18), (21, 18), (10, 21), (21, 21), (15, 19), (16, 22)):
        sp.append(('EN_SKELETON', x, y, 0))
    for (x, y) in ((3, 19), (3, 21), (28, 19), (29, 21)):
        sp.append(('EN_SKELETON', x, y, 0))
    for (x, y) in ((12, 19), (19, 20), (14, 15), (17, 23)):
        sp.append(('EN_BAT', x, y, 0))
    sp.append(('EN_BOSS', 15, 5, 0))
    m.spawns = sp
    m.warps = [(15, 30, 16, 30, 1, 14, 4, 0)]
    m.start = (15, 29)
    # finalize with dungeon rules
    ts = m.ts
    ground = [[0] * W for _ in range(H)]
    over = [[0] * W for _ in range(H)]
    rng = m.rng
    floors = ['floor_a'] * 8 + ['floor_b'] * 8 + ['floor_crack', 'floor_moss', 'floor_bones', 'floor_blood']
    torch_cols = {(10, 2), (21, 2), (8, 13), (13, 13), (18, 13), (23, 13), (15, 25), (12, 26), (19, 26), (2, 15), (29, 15), (12, 26)}
    for y in range(H):
        for x in range(W):
            k = m.kind[y][x]
            if k == 'floor':
                nm = rng.choice(floors)
                if 7 <= x <= 24 and y <= 10:
                    nm = 'floor_rune' if (x in (12, 13, 18, 19) and y in (6, 7)) else rng.choice(['floor_a', 'floor_b', 'floor_b'])
            elif k == 'fixed':
                nm = m.fixed[(x, y)]
            elif k == 'wall':
                below = m.kind[y + 1][x] if y + 1 < H else 'wall'
                if below != 'wall':
                    nm = 'wall_front'
                    r = rng.random()
                    if (x, y) in torch_cols:
                        nm = 'wall_front_torch'
                    elif r < 0.12:
                        nm = 'wall_front_moss'
                    elif r < 0.2:
                        nm = 'wall_front_skull'
                else:
                    nm = 'wall_top'
            else:
                nm = 'floor_a'
            ground[y][x] = ts.id(nm)
    m.ground, m.overlay = ground, over
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
