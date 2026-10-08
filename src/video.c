#include "game.h"
#include "assets.h"

/* VRAM layout: CBB0 = world/title tiles, CBB1 = UI + dynamic text, SB16-19 BG2 ground, SB20-23 BG1 overlay, SB24 BG0 */
#define SB_GROUND 16
#define SB_OVER 20
#define SB_UI 24

typedef struct { u16 a0, a1, a2, fill; } OamE;
typedef struct { s16 key; u16 a0, a1, a2; } SprReq;

static OamE oam_buf[128] __attribute__((aligned(4)));
static SprReq sq[128];
static int nsq, aff_used;
static int bld_mode_fade;

int cam_x, cam_y, shake_t, hitstop_t;
int fade_level;
static int cam_fx, cam_fy;
static int mosaic_lv;
static int fade_mask = 0x3F;
static int disp_world = 1;
static u16 bgcnt_extra;
static int bg2_dark_amt;

static const u8 spr_w[3][4] = { { 8, 16, 32, 64 }, { 16, 32, 32, 64 }, { 8, 8, 16, 32 } };
static const u8 spr_h[3][4] = { { 8, 16, 32, 64 }, { 8, 8, 16, 32 }, { 16, 32, 32, 64 } };

void vsync(void) {
#ifndef HOST_BUILD
    while (REG_VCOUNT >= 160) {}
    while (REG_VCOUNT < 160) {}
#endif
}

void spr_begin(void) {
    nsq = 0;
    aff_used = 0;
}

void spr_add(int x, int y, int shape, int size, int tile, int pal, int prio, int flags, int key) {
    int w = spr_w[shape][size], h = spr_h[shape][size];
    if (nsq >= 120 || x <= -w || x >= 240 || y <= -h || y >= 160) return;
    SprReq *r = &sq[nsq++];
    r->key = (s16)key;
    r->a0 = (u16)((y & 0xFF) | ((flags & SF_ALPHA) ? (1 << 10) : 0) | (shape << 14));
    r->a1 = (u16)((x & 0x1FF) | ((flags & SF_HFLIP) ? (1 << 12) : 0) | ((flags & SF_VFLIP) ? (1 << 13) : 0) | (size << 14));
    r->a2 = (u16)((tile & 0x3FF) | (prio << 10) | (pal << 12));
}

void spr_add_aff(int x, int y, int shape, int size, int tile, int pal, int prio, int flags, int key, int angle, int inv_scale) {
    int w = spr_w[shape][size], h = spr_h[shape][size];
    if (nsq >= 120 || aff_used >= 30 || x <= -w || x >= 240 || y <= -h || y >= 160) return;
    int idx = aff_used++;
    int c = icos(angle), s = isin(angle);
    int pa = (c * inv_scale) >> 8, pb = (s * inv_scale) >> 8;
    int dbl = (flags & 16) ? 1 : 0;
    (void)dbl;
    oam_buf[idx * 4 + 0].fill = (u16)pa;
    oam_buf[idx * 4 + 1].fill = (u16)pb;
    oam_buf[idx * 4 + 2].fill = (u16)(-pb);
    oam_buf[idx * 4 + 3].fill = (u16)pa;
    SprReq *r = &sq[nsq++];
    r->key = (s16)key;
    r->a0 = (u16)((y & 0xFF) | 0x100 | ((flags & 16) ? 0x200 : 0) | ((flags & SF_ALPHA) ? (1 << 10) : 0) | (shape << 14));
    if (flags & SF_HFLIP) { oam_buf[idx * 4 + 0].fill = (u16)(-pa); oam_buf[idx * 4 + 1].fill = (u16)(-pb); oam_buf[idx * 4 + 2].fill = (u16)(-pb); oam_buf[idx * 4 + 3].fill = (u16)pa; }
    r->a1 = (u16)((x & 0x1FF) | (idx << 9) | (size << 14));
    r->a2 = (u16)((tile & 0x3FF) | (prio << 10) | (pal << 12));
}

void spr16(int x, int y, int tile, int pal, int flags, int key) { spr_add(x, y, SH_SQUARE, 1, tile, pal, 2, flags, key); }
void spr8(int x, int y, int tile, int pal, int flags, int key) { spr_add(x, y, SH_SQUARE, 0, tile, pal, 2, flags, key); }
void spr_ui8(int x, int y, int tile, int pal) { spr_add(x, y, SH_SQUARE, 0, tile, pal, 0, 0, 30000); }
void spr_ui16(int x, int y, int tile, int pal) { spr_add(x, y, SH_SQUARE, 1, tile, pal, 0, 0, 30000); }

/* Order: front group (key >= 20000, request order), characters sorted by key (highest key = nearest = lowest OAM index),
   back group (key <= -1000: shadows). Only the middle group needs sorting. */
static void spr_flush(void) {
    static u8 grp_f[128], grp_m[128], grp_b[128];
    int nf = 0, nm = 0, nb = 0;
    for (int i = 0; i < nsq; i++) {
        int k = sq[i].key;
        if (k >= 20000) grp_f[nf++] = (u8)i;
        else if (k <= -1000) grp_b[nb++] = (u8)i;
        else grp_m[nm++] = (u8)i;
    }
    for (int i = 1; i < nm; i++) {
        u8 t = grp_m[i];
        int tk = sq[t].key;
        int j = i - 1;
        while (j >= 0 && sq[grp_m[j]].key < tk) {
            grp_m[j + 1] = grp_m[j];
            j--;
        }
        grp_m[j + 1] = t;
    }
    int n = 0;
    for (int i = 0; i < nf; i++, n++) { const SprReq *r = &sq[grp_f[i]]; oam_buf[n].a0 = r->a0; oam_buf[n].a1 = r->a1; oam_buf[n].a2 = r->a2; }
    for (int i = 0; i < nm; i++, n++) { const SprReq *r = &sq[grp_m[i]]; oam_buf[n].a0 = r->a0; oam_buf[n].a1 = r->a1; oam_buf[n].a2 = r->a2; }
    for (int i = 0; i < nb; i++, n++) { const SprReq *r = &sq[grp_b[i]]; oam_buf[n].a0 = r->a0; oam_buf[n].a1 = r->a1; oam_buf[n].a2 = r->a2; }
    for (; n < 128; n++) oam_buf[n].a0 = 0x200;
}

void gfx_prepare(void) { spr_flush(); }

void gfx_set_fade_mask(int m) { fade_mask = m; }

void gfx_set_fade(int level, int mosaic) {
    fade_level = iclamp(level, 0, 16);
    mosaic_lv = iclamp(mosaic, 0, 15);
}

/* "blank" = instantly fade the screen to black (present() restores the registers); avoids a white forced-blank flash */
void gfx_blank(int on) {
    if (on) {
        REG_BLDCNT = 0x00FF;
        REG_BLDY = 16;
    }
}

void gfx_mode_world(void) {
    disp_world = 1;
    REG_DISPCNT = 0x1740;
}

void gfx_mode_ui(void) {
    disp_world = 0;
    REG_DISPCNT = 0x1540;
}

void shake_add(int n) {
    if (n > shake_t) shake_t = n;
}

void cam_snap(int px, int py) {
    int tx = iclamp(px - 120, 0, 512 - 240);
    int ty = iclamp(py - 88, 0, 512 - 160);
    cam_fx = tx << 8;
    cam_fy = ty << 8;
    cam_x = tx;
    cam_y = ty;
}

void cam_update(int px, int py) {
    int tx = iclamp(px - 120, 0, 512 - 240);
    int ty = iclamp(py - 88, 0, 512 - 160);
    cam_fx += ((tx << 8) - cam_fx) >> 3;
    cam_fy += ((ty << 8) - cam_fy) >> 3;
    cam_x = (cam_fx + 128) >> 8;
    cam_y = (cam_fy + 128) >> 8;
}

void gfx_present(void) {
    copy32(OAM32, (const u32 *)oam_buf, 256);
    int sx = 0, sy = 0;
    if (shake_t > 0) {
        sx = rnd_n(shake_t * 2 + 1) - shake_t;
        sy = rnd_n(shake_t * 2 + 1) - shake_t;
        if (frame_count & 1) shake_t--;
    }
    if (disp_world) {
        REG_BG1HOFS = (cam_x + sx) & 0x1FF;
        REG_BG1VOFS = (cam_y + sy) & 0x1FF;
        REG_BG2HOFS = (cam_x + sx) & 0x1FF;
        REG_BG2VOFS = (cam_y + sy) & 0x1FF;
    } else {
        REG_BG2HOFS = 0;
        REG_BG2VOFS = 0;
    }
    REG_BG0HOFS = 0;
    REG_BG0VOFS = 0;
    u16 mos = mosaic_lv ? 0x40 : 0;
    REG_BG1CNT = (u16)(1 | (0 << 2) | (SB_OVER << 8) | (3 << 14) | mos);
    REG_BG2CNT = (u16)(3 | (0 << 2) | (SB_GROUND << 8) | (3 << 14) | mos);
    REG_MOSAIC = (u16)(mosaic_lv | (mosaic_lv << 4));
    if (fade_level > 0) {
        REG_BLDCNT = (u16)(0x00C0 | fade_mask);
        REG_BLDY = (u16)fade_level;
    } else if (bg2_dark_amt > 0) {
        REG_BLDCNT = 0x00C4;
        REG_BLDY = (u16)bg2_dark_amt;
    } else {
        REG_BLDCNT = 0x2400;
        REG_BLDY = 0;
    }
    REG_BLDALPHA = (u16)(6 | (10 << 8));
}

void bg2_darken(int amount) { bg2_dark_amt = amount; }

void gfx_set_backdrop(u16 color) { PAL_BG[0] = color; }

void gfx_load_obj_static(void) {
    copy32(OBJ_VRAM32 + OT_COMMON * 8, obj_common_tiles, OT_COMMON_TILES * 8);
    copy16(PAL_OBJ, obj_pal_init, 256);
}

void gfx_load_hero(int cls) {
    copy32(OBJ_VRAM32, hero_tiles[cls], 37 * 4 * 8);
    copy16(PAL_OBJ, hero_pal[cls], 16);
}

void gfx_set_hero_pal(int cls) { copy16(PAL_OBJ, hero_pal[cls], 16); }

void gfx_load_ui(void) {
    copy32(VRAM32 + 0x4000 / 4, ui_tiles, 32 * 8);
    copy16(PAL_BG + 12 * 16, ui_pal, 64);
}

void gfx_clear_world_maps(void) {
    fill32(VRAM32 + (SB_GROUND * 0x800) / 4, 0, (8 * 0x800) / 4);
}

void gfx_load_title(int logo) {
    gfx_clear_world_maps();
    copy32(VRAM32, title_tiles, TITLE_NTILES * 8);
    copy16(PAL_BG, title_pal, 16);
    const u16 *map = logo ? title_map : scene_map;
    for (int y = 0; y < 20; y++)
        for (int x = 0; x < 30; x++)
            VRAM16[(SB_GROUND * 0x800) / 2 + y * 32 + x] = map[y * 30 + x];
}

void gfx_init(void) {
    REG_DISPCNT = 0x80;
    REG_WAITCNT = 0x4317;
    fill32(VRAM32, 0, 0x18000 / 4);
    fill16(PAL_BG, 0, 256);
    fill16(PAL_OBJ, 0, 256);
    for (int i = 0; i < 128; i++) {
        oam_buf[i].a0 = 0x200;
        oam_buf[i].a1 = 0;
        oam_buf[i].a2 = 0;
        oam_buf[i].fill = 0;
    }
    copy32(OAM32, (const u32 *)oam_buf, 256);
    gfx_load_ui();
    gfx_load_obj_static();
    REG_BG0CNT = (u16)(0 | (1 << 2) | (SB_UI << 8) | (0 << 14));
    REG_BG1CNT = (u16)(1 | (0 << 2) | (SB_OVER << 8) | (3 << 14));
    REG_BG2CNT = (u16)(3 | (0 << 2) | (SB_GROUND << 8) | (3 << 14));
    bgcnt_extra = 0;
    (void)bgcnt_extra;
    (void)bld_mode_fade;
    gfx_mode_ui();
}

/* ---------------------------------------------------------------- BG0 + text */
#define UI_TILE_WORDS (0x4000 / 4)
static int pool_next = UT_POOL;

int text_pool_mark(void) { return pool_next; }
void text_pool_reset(int mark) { pool_next = mark; }

void bg0_set(int tx, int ty, u16 entry) {
    if (tx < 0 || ty < 0 || tx >= 32 || ty >= 32) return;
    VRAM16[(SB_UI * 0x800) / 2 + ty * 32 + tx] = entry;
}

void bg0_clear(void) {
    fill32(VRAM32 + (SB_UI * 0x800) / 4, 0, 0x800 / 4);
}

void ui_panel(int tx, int ty, int w, int h) {
    u16 pb = 13 << 12;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int t;
            int top = (y == 0), bot = (y == h - 1), lef = (x == 0), rig = (x == w - 1);
            if (top) t = lef ? UT_FRAME_TL : (rig ? UT_FRAME_TR : UT_FRAME_T);
            else if (bot) t = lef ? UT_FRAME_BL : (rig ? UT_FRAME_BR : UT_FRAME_B);
            else t = lef ? UT_FRAME_L : (rig ? UT_FRAME_R : UT_FILL);
            bg0_set(tx + x, ty + y, (u16)(t | pb));
        }
    }
}

void ta_init(TextArea *t, int tx, int ty, int w, int h, int pal, int bg) {
    if (pool_next + w * h > 512) pool_next = 512 - w * h;
    t->base = (u16)pool_next;
    t->tx = (u8)tx; t->ty = (u8)ty; t->w = (u8)w; t->h = (u8)h; t->pal = (u8)pal; t->bg = (u8)bg;
    pool_next += w * h;
    for (int cy = 0; cy < h; cy++)
        for (int cx = 0; cx < w; cx++)
            bg0_set(tx + cx, ty + cy, (u16)((t->base + cy * w + cx) | (pal << 12)));
    ta_clear(t);
}

void ta_remap(TextArea *t) {
    for (int cy = 0; cy < t->h; cy++)
        for (int cx = 0; cx < t->w; cx++)
            bg0_set(t->tx + cx, t->ty + cy, (u16)((t->base + cy * t->w + cx) | (t->pal << 12)));
}

void ta_clear(TextArea *t) {
    u32 v = (u32)t->bg * 0x11111111u;
    fill32(VRAM32 + UI_TILE_WORDS + t->base * 8, v, t->w * t->h * 8);
}

static void put_px(TextArea *t, int x, int y, int c) {
    if (x < 0 || y < 0 || x >= t->w * 8 || y >= t->h * 8) return;
    int tile = t->base + (y >> 3) * t->w + (x >> 3);
    volatile u32 *p = VRAM32 + UI_TILE_WORDS + tile * 8 + (y & 7);
    int sh = (x & 7) * 4;
    u32 v = *p;
    *p = (v & ~(0xFu << sh)) | ((u32)c << sh);
}

int ta_putc(TextArea *t, int px, int py, char c, int col) {
    int ci = (u8)c - 32;
    if (ci < 0 || ci > 94) ci = '?' - 32;
    int w = font_w[ci];
    const u8 *rows = &font_rows[ci * 8];
    for (int y = 0; y < 8; y++) {
        u8 r = rows[y];
        if (!r) continue;
        for (int x = 0; x < w; x++)
            if (r & (0x80 >> x)) put_px(t, px + x + 1, py + y + 1, 2);
    }
    for (int y = 0; y < 8; y++) {
        u8 r = rows[y];
        if (!r) continue;
        for (int x = 0; x < w; x++)
            if (r & (0x80 >> x)) put_px(t, px + x, py + y, col);
    }
    return w + 1;
}

int ta_print(TextArea *t, int px, int py, const char *s, int col) {
    while (*s) px += ta_putc(t, px, py, *s++, col);
    return px;
}

int ta_width(const char *s) {
    int w = 0;
    while (*s) {
        int ci = (u8)*s++ - 32;
        if (ci < 0 || ci > 94) ci = '?' - 32;
        w += font_w[ci] + 1;
    }
    return w;
}

int ta_num(TextArea *t, int px, int py, int v, int col) {
    char buf[12];
    int n = 0;
    if (v < 0) { buf[n++] = '-'; v = -v; }
    char tmp[10];
    int m = 0;
    do { tmp[m++] = (char)('0' + (v % 10)); v /= 10; } while (v && m < 9);
    while (m) buf[n++] = tmp[--m];
    buf[n] = 0;
    return ta_print(t, px, py, buf, col);
}

/* bar: HUD bar drawn in a 1-tile-high text area (bg 0); palette bank 12 */
void bar_draw(TextArea *t, int fill_px, int trail_px, int cfill, int clight, int cdark, int ctrail) {
    int W = t->w * 8;
    int inner = W - 2;
    ta_clear(t);
    for (int x = 0; x < W; x++) {
        put_px(t, x, 1, 1);
        put_px(t, x, 6, 1);
    }
    put_px(t, 0, 2, 1); put_px(t, 0, 3, 1); put_px(t, 0, 4, 1); put_px(t, 0, 5, 1);
    put_px(t, W - 1, 2, 1); put_px(t, W - 1, 3, 1); put_px(t, W - 1, 4, 1); put_px(t, W - 1, 5, 1);
    for (int x = 0; x < inner; x++) {
        int c0, c1, c2, c3;
        if (x < fill_px) { c0 = clight; c1 = cfill; c2 = cfill; c3 = cdark; }
        else if (x < trail_px) { c0 = ctrail; c1 = ctrail; c2 = ctrail; c3 = ctrail; }
        else { c0 = 2; c1 = 2; c2 = 2; c3 = 2; }
        put_px(t, x + 1, 2, c0);
        put_px(t, x + 1, 3, c1);
        put_px(t, x + 1, 4, c2);
        put_px(t, x + 1, 5, c3);
    }
}

void bar_draw_thin(TextArea *t, int fill_px) {
    int W = t->w * 8;
    ta_clear(t);
    for (int x = 0; x < W; x++) {
        put_px(t, x, 2, 1);
        put_px(t, x, 5, 1);
    }
    put_px(t, 0, 3, 1); put_px(t, 0, 4, 1);
    put_px(t, W - 1, 3, 1); put_px(t, W - 1, 4, 1);
    for (int x = 0; x < W - 2; x++) {
        int on = x < fill_px;
        put_px(t, x + 1, 3, on ? 10 : 2);
        put_px(t, x + 1, 4, on ? 9 : 2);
    }
}
