"""Close-up preview of the village props (dev tool, not part of the ROM build).
usage: python3 tools/preview_props.py out.png [scale]
Draws the fountain, benches, lampposts, well, barrels and trees from the real outdoor tileset on plaza / grass."""
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from PIL import Image
import art_world as AW
from common import hx


def build():
    return AW.build_outdoor()


def meta_rgba(ts, mid, pal):
    im = Image.new('RGBA', (16, 16), (0, 0, 0, 0))
    ents = ts.meta[mid]
    for i, e in enumerate(ents):
        t, bank = e & 0xFFF, e >> 12
        if t == 0:
            continue
        words = ts.tiles[t]
        ox, oy = (i % 2) * 8, (i // 2) * 8
        for y in range(8):
            for x in range(8):
                v = (words[y] >> (4 * x)) & 15
                if v:
                    r, g, b = hx(pal[bank * 16 + v])
                    im.putpixel((ox + x, oy + y), (r, g, b, 255))
    return im


def scene(ts, ground, over, w, h, scale):
    pal = ts.palette()
    im = Image.new('RGBA', (w * 16, h * 16), (0, 0, 0, 255))
    for y in range(h):
        for x in range(w):
            n = ground.get((x, y), 'grass_a')
            im.alpha_composite(meta_rgba(ts, ts.id(n), pal), (x * 16, y * 16))
    for (x, y), n in over.items():
        im.alpha_composite(meta_rgba(ts, ts.id(n), pal), (x * 16, y * 16))
    return im.resize((w * 16 * scale, h * 16 * scale), Image.NEAREST)


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else '/tmp/props.png'
    scale = int(sys.argv[2]) if len(sys.argv) > 2 else 4
    ts = build()
    W, H = 16, 11
    g, o = {}, {}
    for y in range(H):
        for x in range(W):
            g[(x, y)] = 'plaza' if x < 9 else 'grass_a'
    # fountain
    for (dx, dy, q) in ((0, 0, 'tl'), (1, 0, 'tr'), (0, 1, 'bl'), (1, 1, 'br')):
        g[(4 + dx, 2 + dy)] = 'fountain_' + q
    # benches
    if 'bench_plaza_l' in ts.names:
        g[(1, 3)], g[(2, 3)] = 'bench_plaza_l', 'bench_plaza_r'
        g[(7, 3)], g[(8, 3)] = 'bench_plaza_l', 'bench_plaza_r'
    else:
        g[(2, 3)] = 'bench_plaza'
        g[(7, 3)] = 'bench_plaza'
    g[(1, 0)], g[(8, 0)] = 'planter_plaza', 'planter_plaza'
    # lamps, well, barrels, crates
    for x, y in ((9, 2), (15, 4)):
        g[(x, y)] = 'lamp_base'
        o[(x, y - 1)] = 'lamp_top'
    g[(10, 4)] = 'well_base'
    o[(10, 3)] = 'well_roof'
    g[(12, 2)], g[(13, 2)], g[(12, 3)] = 'barrel', 'barrel', 'crate'
    g[(14, 5)] = 'barrel'
    g[(11, 6)] = 'hay'
    # trees: giant oak 3x3 and a tall oak column
    for dx, side in ((0, 'l'), (1, 'm'), (2, 'r')):
        o[(10 + dx, 7)] = 'goak_top_' + side
        o[(10 + dx, 8)] = 'goak_mid_' + side
    g[(11, 9)] = 'goak_trunk'
    for x in (14, 15):
        g[(x, 10)] = 'oak_trunk'
        o[(x, 9)] = 'oak_canopy'
        o[(x, 8)] = 'oak_top'
    for x in (7, 8):
        g[(x, 10)] = 'pine_trunk'
        o[(x, 9)] = 'pine_canopy'
        o[(x, 8)] = 'pine_top'
    img = scene(ts, g, o, W, H, scale)
    img.convert('RGB').save(out)
    print('saved', out, img.size)


if __name__ == '__main__':
    main()
