#!/usr/bin/env python3
"""Turn host_test frame dumps (pal+oam+vram+dispcnt) into PNG screenshots."""
import struct, sys, glob, os
from PIL import Image

def rgb(c):
    r = (c & 31) << 3; g = ((c >> 5) & 31) << 3; b = ((c >> 10) & 31) << 3
    return (r | r >> 5, g | g >> 5, b | b >> 5)

def render(path, scale=3):
    d = open(path, 'rb').read()
    pal = struct.unpack('<512H', d[0:0x400])
    oam = d[0x400:0x800]
    vram = d[0x800:0x800 + 0x18000]
    disp = struct.unpack('<H', d[0x800 + 0x18000:0x800 + 0x18002])[0]
    page = 0xA000 if disp & 0x10 else 0
    img = Image.new('RGB', (240, 160))
    px = img.load()
    for y in range(160):
        for x in range(240):
            idx = vram[page + y * 240 + x]
            px[x, y] = rgb(pal[0] if idx == 0 else pal[idx])
    # sprites (lowest OAM index on top => draw from the back)
    for i in range(127, -1, -1):
        a0, a1, a2 = struct.unpack('<HHH', oam[i * 8:i * 8 + 6])
        if a0 & 0x300 == 0x200:
            continue
        sy = a0 & 0xFF
        if sy >= 160: sy -= 256
        sx = a1 & 0x1FF
        if sx >= 256: sx -= 512
        hflip = a1 & 0x1000
        tile = a2 & 0x3FF
        bank = a2 >> 12
        base = 0x10000 + tile * 32
        for ty in range(16):
            for tx in range(16):
                t = (ty // 8) * 2 + (tx // 8)
                off = base + t * 32 + (ty % 8) * 4 + (tx % 8) // 2
                byte = vram[off]
                nib = (byte >> 4) if (tx % 8) & 1 else (byte & 15)
                if nib == 0: continue
                dx = 15 - tx if hflip else tx
                X, Y = sx + dx, sy + ty
                if 0 <= X < 240 and 0 <= Y < 160:
                    px[X, Y] = rgb(pal[256 + bank * 16 + nib])
    return img.resize((240 * scale, 160 * scale), Image.NEAREST)

if __name__ == '__main__':
    src = sys.argv[1] if len(sys.argv) > 1 else 'out'
    names = sys.argv[2:] or None
    for f in sorted(glob.glob(os.path.join(src, '*.bin'))):
        n = os.path.splitext(os.path.basename(f))[0]
        if names and n not in names: continue
        render(f).save(os.path.join(src, n + '.png'))
        print('wrote', n)
