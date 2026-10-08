"""Title scene: 240x160 indexed image -> unique 8x8 tiles + 30x20 map (bank 0)."""
import math
import random
import numpy as np
from common import hx
from art_font import glyph_rows

TITLE_PAL = ['#0a0a28', '#141446', '#26286a', '#4a3a86', '#8a4a8c', '#d87a6a', '#f0e8c8', '#c0b898',
             '#2a2858', '#1a1840', '#0e0e24', '#ffd860', '#e89828', '#6a2810', '#ffffff', '#a05830']

BAYER = [[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]


def draw_text(img, text, x, y, scale, fill_top, fill_bot, outline, shadow=None, spacing=1):
    pen = x
    glyphs = []
    for ch in text:
        w, rows = glyph_rows(ch)
        glyphs.append((w, rows))
    # first pass: collect pixel set
    px = []
    for (w, rows) in glyphs:
        for gy in range(8):
            for gx in range(w):
                if rows[gy] & (0x80 >> gx):
                    for sy in range(scale):
                        for sx in range(scale):
                            px.append((pen + gx * scale + sx, y + gy * scale + sy, gy))
        pen += (w + spacing) * scale
    S = set((a, b) for a, b, _ in px)
    if shadow is not None:
        for (a, b, g) in px:
            for dx, dy in ((1, 1), (2, 2), (1, 2), (2, 1)):
                if 0 <= a + dx < 240 and 0 <= b + dy < 160 and (a + dx, b + dy) not in S:
                    img[b + dy, a + dx] = shadow
    for (a, b) in S:
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                if (a + dx, b + dy) not in S and 0 <= a + dx < 240 and 0 <= b + dy < 160:
                    img[b + dy, a + dx] = outline
    for (a, b, g) in px:
        if 0 <= a < 240 and 0 <= b < 160:
            img[b, a] = fill_top if g < 4 else fill_bot
    return pen - x


def text_width(text, scale, spacing=1):
    return sum((glyph_rows(ch)[0] + spacing) * scale for ch in text) - spacing * scale


def build_title(logo=True):
    img = np.zeros((160, 240), np.uint8)
    # sky gradient with dithering
    bands = [(0, 0), (30, 1), (54, 2), (78, 3), (98, 4), (112, 5)]
    for y in range(160):
        for x in range(240):
            # find band
            b = 0
            for i, (y0, idx) in enumerate(bands):
                if y >= y0:
                    b = i
            idx = bands[b][1]
            if b + 1 < len(bands):
                y1 = bands[b + 1][0]
                span = 12
                if y > y1 - span:
                    t = (y - (y1 - span)) / span
                    if BAYER[y % 4][x % 4] / 16.0 < t:
                        idx = bands[b + 1][1]
            img[y, x] = idx
    rng = random.Random(3)
    # stars
    for _ in range(90):
        x, y = rng.randint(0, 239), rng.randint(0, 80)
        if rng.random() < 0.8 - y / 120.0:
            img[y, x] = 14 if rng.random() < 0.5 else 7
    # moon
    mx, my, mr = 208, 70, 16
    for y in range(160):
        for x in range(240):
            d = math.hypot(x - mx, y - my)
            if d <= mr:
                img[y, x] = 6
                if math.hypot(x - (mx - 6), y - (my + 5)) < mr - 4 and d > mr - 5 and (x + y) % 2 == 0:
                    pass
                if math.hypot(x - (mx + 7), y - (my - 6)) < 4:
                    img[y, x] = 7
                if math.hypot(x - (mx - 6), y - (my + 6)) < 3:
                    img[y, x] = 7
                if math.hypot(x - (mx + 4), y - (my + 8)) < 2.2:
                    img[y, x] = 7
                if x - mx + (y - my) > 14:
                    img[y, x] = 7 if (x + y) % 2 else 6
    # far mountains
    ridge = {}
    for x in range(240):
        y = 112 - 14 * math.sin(x / 31.0 + 1.0) - 8 * math.sin(x / 13.0 + 0.4) - 5 * abs(math.sin(x / 7.0))
        ridge[x] = int(y)
    for x in range(240):
        for y in range(ridge[x], 160):
            img[y, x] = 8
    # tower (ruined crypt spire) on far mountains
    tx = 58
    for y in range(78, 124):
        for x in range(tx, tx + 10):
            img[y, x] = 9
    for x in range(tx - 1, tx + 11):
        img[78, x] = 9
    for i in range(0, 10, 3):
        for y in range(74, 78):
            img[y, tx + i] = 9
    img[92, tx + 4] = 11
    img[93, tx + 4] = 11
    img[92, tx + 5] = 11
    # near hills
    for x in range(240):
        y = 128 - 8 * math.sin(x / 23.0 + 2.2) - 4 * math.sin(x / 9.0)
        for yy in range(int(y), 160):
            img[yy, x] = 9
    # pine silhouettes
    for _ in range(46):
        x = rng.randint(-4, 243)
        base = rng.randint(138, 160)
        h = rng.randint(14, 26)
        for i in range(h):
            half = (i * 5) // (h) + 0
            w = 1 + int(i * 0.42)
            for xx in range(x - w, x + w + 1):
                if 0 <= xx < 240 and 0 <= base - h + i < 160:
                    img[base - h + i, xx] = 10
        for yy in range(base, min(160, base + 4)):
            if 0 <= x < 240:
                img[yy, x] = 10
    for y in range(150, 160):
        for x in range(240):
            img[y, x] = 10
    if not logo:
        return img
    # logo
    title = "EMBERVALE"
    sc = 3
    tw = text_width(title, sc, 1)
    x0 = (240 - tw) // 2
    draw_text(img, title, x0, 16, sc, 11, 12, 13, shadow=15)
    sub = "THE HOLLOW CRYPT"
    tw2 = text_width(sub, 1)
    # subtitle with letter spacing 2
    sw = sum((glyph_rows(ch)[0] + 2) for ch in sub) - 2
    draw_text(img, sub, (240 - sw) // 2, 44, 1, 6, 6, 9, shadow=None, spacing=2)
    return img


def tileize(img, tiles=None, tmap=None):
    if tiles is None:
        tiles = [[0] * 8]
        tmap = {tuple([0] * 8): 0}
    ents = []
    for ty in range(20):
        for tx in range(30):
            a = img[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8]
            words = []
            for y in range(8):
                v = 0
                for x in range(8):
                    v |= int(a[y, x]) << (4 * x)
                words.append(v)
            k = tuple(words)
            if k not in tmap:
                tmap[k] = len(tiles)
                tiles.append(words)
            ents.append(tmap[k])
    return tiles, ents


if __name__ == '__main__':
    from PIL import Image
    img = build_title()
    tiles, ents = tileize(img)
    print('unique tiles', len(tiles))
    pal = [hx(c) for c in TITLE_PAL]
    rgb = np.zeros((160, 240, 3), np.uint8)
    for y in range(160):
        for x in range(240):
            rgb[y, x] = pal[img[y, x]]
    Image.fromarray(rgb).resize((720, 480), Image.NEAREST).save('/tmp/title.png')
