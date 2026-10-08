"""Tiny GBA PPU emulator for verifying the game visually on a PC (mode 0, 4bpp BGs, sprites, alpha/brightness)."""
import sys
import numpy as np
from PIL import Image

W, H = 240, 160


def u16(b, o):
    return b[o] | (b[o + 1] << 8)


def s16(b, o):
    v = u16(b, o)
    return v - 65536 if v >= 32768 else v


def rgb555(v):
    return ((v & 31) << 3 | (v & 31) >> 2, ((v >> 5) & 31) << 3 | ((v >> 5) & 31) >> 2, ((v >> 10) & 31) << 3 | ((v >> 10) & 31) >> 2)


def load(path):
    d = open(path, 'rb').read()
    return d[0:0x400], d[0x400:0x800], d[0x800:0x800 + 0x18000], d[0x800 + 0x18000:0x800 + 0x18000 + 0x400]


SIZES = {(0, 0): (8, 8), (0, 1): (16, 16), (0, 2): (32, 32), (0, 3): (64, 64),
         (1, 0): (16, 8), (1, 1): (32, 8), (1, 2): (32, 16), (1, 3): (64, 32),
         (2, 0): (8, 16), (2, 1): (8, 32), (2, 2): (16, 32), (2, 3): (32, 64)}


def render(path, scale=3):
    io, pal, vram, oam = load(path)
    vram = np.frombuffer(vram, np.uint8)
    dispcnt = u16(io, 0)
    palc = np.zeros((512, 3), np.int32)
    for i in range(512):
        palc[i] = rgb555(u16(pal, i * 2))
    raw555 = [u16(pal, i * 2) for i in range(512)]
    palarr = np.array(raw555, np.int32)
    ys, xs = np.mgrid[0:H, 0:W]
    layers = []  # (prio, order, kind, mask, color(h,w,3), semi)
    # ---------------- backgrounds
    for bg in range(4):
        if not (dispcnt >> (8 + bg)) & 1:
            continue
        cnt = u16(io, 8 + bg * 2)
        prio = cnt & 3
        cbb = (cnt >> 2) & 3
        sbb = (cnt >> 8) & 31
        size = cnt >> 14
        hofs = u16(io, 0x10 + bg * 4) & 0x1FF
        vofs = u16(io, 0x12 + bg * 4) & 0x1FF
        bw = 512 if size in (1, 3) else 256
        bh = 512 if size in (2, 3) else 256
        bx = (xs + hofs) % bw
        by = (ys + vofs) % bh
        tx, ty = bx >> 3, by >> 3
        if size == 0:
            sbi = np.zeros_like(tx)
        elif size == 1:
            sbi = tx >> 5
        elif size == 2:
            sbi = ty >> 5
        else:
            sbi = (tx >> 5) + ((ty >> 5) << 1)
        addr = (sbb + sbi) * 0x800 + (((ty & 31) * 32) + (tx & 31)) * 2
        ent = vram[addr].astype(np.int32) | (vram[addr + 1].astype(np.int32) << 8)
        tile = ent & 0x3FF
        hf = (ent >> 10) & 1
        vf = (ent >> 11) & 1
        pb = ent >> 12
        px = bx & 7
        py = by & 7
        px = np.where(hf == 1, 7 - px, px)
        py = np.where(vf == 1, 7 - py, py)
        taddr = cbb * 0x4000 + tile * 32 + py * 4 + (px >> 1)
        taddr = np.minimum(taddr, 0x17FFF)
        byte = vram[taddr].astype(np.int32)
        idx = np.where((px & 1) == 1, byte >> 4, byte & 15)
        mask = idx != 0
        col = palc[pb * 16 + idx]
        layers.append((prio, bg, bg, mask, col, np.zeros((H, W), bool)))
    # ---------------- sprites
    obj_col = np.zeros((H, W, 3), np.int32)
    obj_mask = np.zeros((H, W), bool)
    obj_prio = np.full((H, W), 9, np.int32)
    obj_semi = np.zeros((H, W), bool)
    if (dispcnt >> 12) & 1:
        for i in range(127, -1, -1):
            a0 = u16(oam, i * 8)
            a1 = u16(oam, i * 8 + 2)
            a2 = u16(oam, i * 8 + 4)
            rs = (a0 >> 8) & 1
            dbl = (a0 >> 9) & 1
            if not rs and dbl:
                continue
            mode = (a0 >> 10) & 3
            shape = a0 >> 14
            size = a1 >> 14
            if shape == 3:
                continue
            w, h = SIZES[(shape, size)]
            y = a0 & 255
            if y >= 160:
                y -= 256
            x = a1 & 511
            if x >= 256:
                x -= 512
            tile = a2 & 0x3FF
            prio = (a2 >> 10) & 3
            pb = a2 >> 12
            bw, bh = (w * 2, h * 2) if (rs and dbl) else (w, h)
            for dy in range(bh):
                sy = y + dy
                if sy < 0 or sy >= H:
                    continue
                for dx in range(bw):
                    sx = x + dx
                    if sx < 0 or sx >= W:
                        continue
                    if rs:
                        grp = (a1 >> 9) & 31
                        pa = s16(oam, grp * 32 + 6)
                        pbb = s16(oam, grp * 32 + 14)
                        pc = s16(oam, grp * 32 + 22)
                        pd = s16(oam, grp * 32 + 30)
                        cx, cy = dx - bw // 2, dy - bh // 2
                        tx_ = ((pa * cx + pbb * cy) >> 8) + w // 2
                        ty_ = ((pc * cx + pd * cy) >> 8) + h // 2
                    else:
                        tx_, ty_ = dx, dy
                        if (a1 >> 12) & 1:
                            tx_ = w - 1 - dx
                        if (a1 >> 13) & 1:
                            ty_ = h - 1 - dy
                    if tx_ < 0 or ty_ < 0 or tx_ >= w or ty_ >= h:
                        continue
                    t = tile + (ty_ >> 3) * (w >> 3) + (tx_ >> 3)
                    ad = 0x10000 + t * 32 + (ty_ & 7) * 4 + ((tx_ & 7) >> 1)
                    if ad >= 0x18000:
                        continue
                    byte = int(vram[ad])
                    idx = (byte >> 4) if (tx_ & 1) else (byte & 15)
                    if idx == 0:
                        continue
                    if prio <= obj_prio[sy, sx]:
                        obj_col[sy, sx] = palc[256 + pb * 16 + idx]
                        obj_mask[sy, sx] = True
                        obj_prio[sy, sx] = prio
                        obj_semi[sy, sx] = (mode == 1)
    # ---------------- composite
    backdrop = palc[0]
    top = np.tile(backdrop, (H, W, 1)).astype(np.int32)
    top_tag = np.full((H, W), 5, np.int32)
    top_semi = np.zeros((H, W), bool)
    sec = np.tile(backdrop, (H, W, 1)).astype(np.int32)
    sec_tag = np.full((H, W), 5, np.int32)
    got1 = np.zeros((H, W), bool)
    got2 = np.zeros((H, W), bool)
    order = []
    for p in range(4):
        order.append(('obj', p))
        for bg in range(4):
            order.append(('bg', p, bg))
    bgl = {l[1]: l for l in layers}
    for item in order:
        if item[0] == 'obj':
            m = obj_mask & (obj_prio == item[1])
            col, tag, semi = obj_col, 4, obj_semi
        else:
            l = bgl.get(item[2])
            if not l or l[0] != item[1]:
                continue
            m, col, tag, semi = l[3], l[4], item[2], np.zeros((H, W), bool)
        n1 = m & ~got1
        top[n1] = col[n1]
        top_tag[n1] = tag
        top_semi[n1] = semi[n1] if isinstance(semi, np.ndarray) else False
        got1 |= n1
        n2 = m & got1 & ~n1 & ~got2
        sec[n2] = col[n2]
        sec_tag[n2] = tag
        got2 |= n2
    # blending
    bld = u16(io, 0x50)
    mode = (bld >> 6) & 3
    first = bld & 63
    second = (bld >> 8) & 63
    alpha = u16(io, 0x52)
    eva = min(alpha & 31, 16)
    evb = min((alpha >> 8) & 31, 16)
    bldy = min(u16(io, 0x54) & 31, 16)
    out = top.copy()
    second_ok = np.zeros((H, W), bool)
    for t in range(6):
        second_ok |= (sec_tag == t) & (((second >> t) & 1) == 1)
    # semi-transparent obj
    sm = top_semi & (top_tag == 4) & second_ok
    out[sm] = np.minimum(255, (top[sm] * eva + sec[sm] * evb) >> 4)
    first_ok = np.zeros((H, W), bool)
    for t in range(6):
        first_ok |= (top_tag == t) & (((first >> t) & 1) == 1)
    if mode == 3:
        m = first_ok & ~sm
        out[m] = top[m] - ((top[m] * bldy) >> 4)
    elif mode == 2:
        m = first_ok & ~sm
        out[m] = top[m] + (((255 - top[m]) * bldy) >> 4)
    elif mode == 1:
        m = first_ok & second_ok & ~sm
        out[m] = np.minimum(255, (top[m] * eva + sec[m] * evb) >> 4)
    if dispcnt & 0x80:
        out[:] = 255
    im = Image.fromarray(np.clip(out, 0, 255).astype(np.uint8))
    if scale != 1:
        im = im.resize((W * scale, H * scale), Image.NEAREST)
    return im


if __name__ == '__main__':
    for p in sys.argv[1:]:
        render(p).save(p.replace('.bin', '.png'))
