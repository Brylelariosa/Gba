"""Shared helpers for the Embervale asset generator."""
import numpy as np
from PIL import Image


def hx(s):
    s = s.lstrip('#')
    return (int(s[0:2], 16), int(s[2:4], 16), int(s[4:6], 16))


def c15(rgb):
    r, g, b = rgb if isinstance(rgb, tuple) else hx(rgb)
    return (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10)


def pal16(colors):
    """colors: list of hex strings for idx 1..15 (idx0 is transparent/black)."""
    out = ['#000000'] + list(colors)
    out += ['#000000'] * (16 - len(out))
    return out


class Canvas:
    def __init__(self, w, h):
        self.w, self.h = w, h
        self.a = np.zeros((h, w), np.uint8)

    def set(self, x, y, c):
        if 0 <= x < self.w and 0 <= y < self.h:
            self.a[y, x] = c

    def get(self, x, y):
        if 0 <= x < self.w and 0 <= y < self.h:
            return int(self.a[y, x])
        return 0

    def rect(self, x0, y0, x1, y1, c):
        for y in range(y0, y1 + 1):
            for x in range(x0, x1 + 1):
                self.set(x, y, c)

    def ellipse(self, cx, cy, rx, ry, c):
        for y in range(int(cy - ry - 1), int(cy + ry + 2)):
            for x in range(int(cx - rx - 1), int(cx + rx + 2)):
                if ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1.0:
                    self.set(x, y, c)

    def line(self, x0, y0, x1, y1, c):
        n = max(abs(x1 - x0), abs(y1 - y0), 1)
        for i in range(n + 1):
            self.set(round(x0 + (x1 - x0) * i / n), round(y0 + (y1 - y0) * i / n), c)

    def blit(self, o, ox, oy):
        for y in range(o.h):
            for x in range(o.w):
                v = o.a[y, x]
                if v:
                    self.set(ox + x, oy + y, int(v))

    def art(self, rows, ox, oy, key):
        """Draw ascii rows; key maps char->palette index ('.' transparent)."""
        for y, r in enumerate(rows):
            for x, ch in enumerate(r):
                if ch != '.' and ch != ' ':
                    self.set(ox + x, oy + y, key[ch])

    def outline(self, oc, diag=False):
        src = self.a.copy()
        for y in range(self.h):
            for x in range(self.w):
                if src[y, x] == 0:
                    n = False
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)) + (((1, 1), (-1, -1), (1, -1), (-1, 1)) if diag else ()):
                        xx, yy = x + dx, y + dy
                        if 0 <= xx < self.w and 0 <= yy < self.h and src[yy, xx] != 0 and src[yy, xx] != oc:
                            n = True
                    if n:
                        self.a[y, x] = oc

    def flip_h(self):
        c = Canvas(self.w, self.h)
        c.a = self.a[:, ::-1].copy()
        return c

    def copy(self):
        c = Canvas(self.w, self.h)
        c.a = self.a.copy()
        return c

    def remap(self, m):
        c = self.copy()
        for k, v in m.items():
            c.a[self.a == k] = v
        return c


def tiles_of(a):
    """array (h,w) of idx -> list of tiles (each 8 u32), row-major order of 8x8 tiles."""
    h, w = a.shape
    out = []
    for ty in range(h // 8):
        for tx in range(w // 8):
            t = a[ty * 8:ty * 8 + 8, tx * 8:tx * 8 + 8]
            words = []
            for y in range(8):
                v = 0
                for x in range(8):
                    v |= int(t[y, x]) << (4 * x)
                words.append(v)
            out.append(words)
    return out


def flat(tiles):
    return [w for t in tiles for w in t]


def render(a, pal, scale=1, bg=(255, 0, 255)):
    h, w = a.shape
    img = np.zeros((h, w, 3), np.uint8)
    for y in range(h):
        for x in range(w):
            v = a[y, x]
            img[y, x] = bg if v == 0 else hx(pal[v])
    im = Image.fromarray(img)
    if scale != 1:
        im = im.resize((w * scale, h * scale), Image.NEAREST)
    return im


def sheet(frames, pal, cols, scale=4, pad=2, bg=(60, 60, 80)):
    """frames: list of Canvas"""
    fw = max(f.w for f in frames)
    fh = max(f.h for f in frames)
    rows = (len(frames) + cols - 1) // cols
    W = cols * (fw + pad) + pad
    H = rows * (fh + pad) + pad
    im = Image.new('RGB', (W * scale, H * scale), bg)
    for i, f in enumerate(frames):
        r = render(f.a, pal, scale, bg=bg)
        im.paste(r, ((pad + (i % cols) * (fw + pad)) * scale, (pad + (i // cols) * (fh + pad)) * scale))
    return im


# ---------------- C emitters -----------------
def carr(ctype, name, vals, per=8, fmt='0x%x', qual='const', align=False, extra=''):
    s = []
    a = ' __attribute__((aligned(4)))' if align else ''
    s.append('%s %s %s[%d]%s%s = {' % (qual, ctype, name, len(vals), a, extra))
    for i in range(0, len(vals), per):
        s.append('  ' + ','.join(fmt % v for v in vals[i:i + per]) + ',')
    s.append('};')
    return '\n'.join(s) + '\n'


def u32arr(name, vals, per=8):
    return carr('u32', name, vals, per, '0x%08x', align=True)


def u16arr(name, vals, per=12):
    return carr('u16', name, vals, per, '0x%04x')


def u8arr(name, vals, per=24):
    return carr('u8', name, vals, per, '%d')


def s16arr(name, vals, per=16):
    return carr('s16', name, vals, per, '%d')
