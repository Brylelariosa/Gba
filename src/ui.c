#include "game.h"
#include "assets.h"
#include "audio_data.h"

/* text colours (palette bank 13 indices) */
#define C_WHITE 1
#define C_YELLOW 3
#define C_RED 4
#define C_GREEN 5
#define C_CYAN 6
#define C_GRAY 7

static TextArea ta_lv, ta_hp, ta_mp, ta_xp, ta_gold, ta_pot, ta_boss, ta_bossname, ta_banner, ta_toast, ta_pts, ta_combo, ta_track, ta_mm;
/* HUD layout (tile coordinates): the coin counter sits at the end of the EXP bar, the minimap owns the top-right corner,
   and the combo counter hangs to the minimap's left. */
#define GOLD_TX 11
#define GOLD_TY 2
#define MM_TX 24
#define MM_TY 0
#define MM_TW 6
#define COMBO_TX 15
static char track_txt[40];
static int track_cls = -1;
static int mm_ox = -1, mm_oy = -1, mm_chests = -1;
static int mm_hidden;                                   /* the minimap is blanked while the hero is inside a house */
int hud_minimap = 1;                 /* SELECT in the menu's System tab toggles it */
static int hud_on, hud_pool_end;
static int c_hp = -1, c_mp = -1, c_xp = -1, c_lv = -1, c_gold = -1, c_pot = -1, c_maxhp = -1, c_maxmp = -1, c_trail = -1, c_boss = -1;
static int trail_delay;
static int banner_t, toast_t;
static int boss_bar_on, boss_bar_type;
static int c_pts = -1, c_chain = -1, c_chain_col = -1;

void ui_init(void) {
    text_pool_reset(UT_POOL);
    bg0_clear();
}

static void thin_bar(TextArea *t, int fill_px) { bar_draw_thin(t, fill_px); }

void hud_init(void) {
    text_pool_reset(UT_POOL);
    bg0_clear();
    ta_init(&ta_lv, 0, 0, 4, 1, 13, 0);
    ta_init(&ta_hp, 4, 0, 9, 1, 12, 0);
    ta_init(&ta_mp, 4, 1, 7, 1, 12, 0);
    ta_init(&ta_xp, 4, 2, 7, 1, 12, 0);
    ta_init(&ta_gold, GOLD_TX + 1, GOLD_TY, 4, 1, 13, 0);
    ta_init(&ta_pot, 1, 18, 3, 1, 13, 0);
    ta_init(&ta_bossname, 7, 17, 16, 1, 13, 0);
    ta_init(&ta_boss, 7, 18, 16, 1, 12, 0);
    ta_init(&ta_banner, 4, 6, 22, 2, 13, 0);
    ta_init(&ta_toast, 2, 16, 26, 1, 13, 0);
    ta_init(&ta_pts, 0, 1, 4, 1, 13, 0);
    ta_init(&ta_combo, COMBO_TX, 1, 9, 1, 13, 0);
    ta_init(&ta_track, 0, 3, 22, 2, 13, 0);                 /* two tiles high: the outlined text needs a pixel above and below */
    ta_init(&ta_mm, MM_TX, MM_TY, MM_TW, MM_TW, 15, 0);
    bg0_set(GOLD_TX, GOLD_TY, (u16)(UT_COIN | (13 << 12)));
    bg0_set(0, 18, (u16)(UT_POTION | (13 << 12)));
    hud_pool_end = text_pool_mark();
    c_hp = c_mp = c_xp = c_lv = c_gold = c_pot = c_maxhp = c_maxmp = c_trail = c_boss = -1;
    c_pts = c_chain = c_chain_col = -1;
    track_txt[0] = 0;
    track_cls = -1;
    mm_hidden = 0;
    mm_ox = mm_oy = mm_chests = -1;
    banner_t = toast_t = 0;
    boss_bar_on = 0;
    boss_bar_type = 0;
    hud_on = 1;
    /* hide boss widgets until needed */
    for (int x = 7; x < 23; x++) { bg0_set(x, 17, 0); bg0_set(x, 18, 0); }
    if (!hud_minimap)                                  /* minimap off: the cells would otherwise stay a solid square */
        for (int y = MM_TY; y < MM_TY + MM_TW; y++)
            for (int x = MM_TX; x < MM_TX + MM_TW; x++) bg0_set(x, y, 0);
    hud_update();
}

void hud_remap(void) {
    ta_remap(&ta_lv); ta_remap(&ta_hp); ta_remap(&ta_mp); ta_remap(&ta_xp); ta_remap(&ta_gold); ta_remap(&ta_pot);
    ta_remap(&ta_banner); ta_remap(&ta_toast); ta_remap(&ta_pts); ta_remap(&ta_combo);
    ta_remap(&ta_track);
    if (hud_minimap) ta_remap(&ta_mm);
    bg0_set(GOLD_TX, GOLD_TY, (u16)(UT_COIN | (13 << 12)));
    bg0_set(0, 18, (u16)(UT_POTION | (13 << 12)));
    if (boss_bar_on) {
        ta_remap(&ta_bossname);
        ta_remap(&ta_boss);
    }
}

void hud_show(int on) { hud_on = on; }


/* ------------------------------------------------------------------ quest tracker + minimap */
static void track_update(void) {
    char b[40];
    int cls = quest_tracker_line(b);
    int same = cls == track_cls;
    for (int i = 0; same && i < 40; i++) { if (b[i] != track_txt[i]) same = 0; if (!b[i]) break; }
    if (same) return;
    track_cls = cls;
    for (int i = 0; i < 40; i++) { track_txt[i] = b[i]; if (!b[i]) break; }
    ta_clear(&ta_track);
    if (cls != QT_NONE) {
        /* yellow = main quest, cyan = side quest, green = ready to hand in, white = where the story goes next.
           No plate behind it: a dark outline keeps it readable on any ground. */
        static const u8 col[5] = { 0, C_YELLOW, C_CYAN, C_GREEN, C_WHITE };
        ta_print_ol(&ta_track, 3, 1, b, col[cls]);
    }
}

#define MMC 15                       /* the HUD minimap shows a 15x15 tile window, three pixels per tile */
#define MM_IX 1                      /* the window's top-left pixel inside the 48x48 canvas (frame outside it) */
#define MM_IY 2
static int mm_chest_sig(void) {      /* changes when a chest is opened so the map redraws */
    return (int)((gchests ^ (gchests2 * 31u)) & 0x7FFFFFFF);
}

static u8 mm_cell[MMC][MMC];         /* colour class of every cell of the window, markers included */

/* 3x3 pixel pattern of every colour class (palette bank 15), two variants picked by cell parity so big areas get
   texture: grass flecks, leafy trees, tiled floors, shingled roofs, glowing lava. */
static const u8 mm_pat[16][2][9] = {
    { { 3,3,3,3,3,3,3,3,3 }, { 3,3,3,3,3,3,3,3,3 } },     /* 0 unknown */
    { { 1,1,1,1,1,1,1,1,1 }, { 1,1,1,1,1,1,1,1,1 } },     /* 1 frame */
    { { 2,2,2,2,2,2,2,2,2 }, { 2,2,2,2,2,2,2,2,1 } },     /* 2 dungeon floor */
    { { 3,3,3,3,3,3,3,3,3 }, { 3,15,3,3,3,3,3,3,3 } },    /* 3 grass */
    { { 4,4,4,4,4,4,4,4,4 }, { 4,4,4,4,4,4,4,4,4 } },     /* 4 path */
    { { 5,5,5,5,5,5,5,5,5 }, { 5,5,5,5,5,5,5,5,2 } },     /* 5 plaza */
    { { 6,6,6,6,6,6,6,6,6 }, { 6,6,6,6,6,6,6,6,6 } },     /* 6 water */
    { { 7,7,7,7,7,15,7,7,7 }, { 7,7,7,15,7,7,7,7,7 } },   /* 7 foliage */
    { { 5,5,5,8,8,8,8,8,8 }, { 5,5,5,8,8,8,8,8,8 } },     /* 8 rock / wall */
    { { 9,9,9,9,9,9,8,8,8 }, { 9,9,9,9,9,9,8,8,8 } },     /* 9 roof */
    { { 10,10,10,10,10,10,10,10,10 }, { 10,10,10,10,10,10,10,10,10 } },   /* 10 door / warp / quest giver */
    { { 11,11,11,11,1,11,11,11,11 }, { 11,11,11,11,1,11,11,11,11 } },     /* 11 chest */
    { { 12,12,12,12,12,12,12,12,12 }, { 12,12,12,12,12,12,12,12,12 } },   /* 12 villager */
    { { 13,13,13,13,13,13,13,13,13 }, { 13,13,13,13,13,13,13,13,13 } },   /* 13 sign */
    { { 14,14,14,14,14,14,14,14,14 }, { 14,14,14,14,10,14,14,14,14 } },   /* 14 lava */
    { { 3,3,3,3,3,3,3,3,3 }, { 3,3,3,3,3,3,3,3,3 } },     /* 15 spare */
};

static int cell_class(int idx) {
    int o = map_o[idx];
    if (o && cur_ts->mmo[o]) return cur_ts->mmo[o];       /* canopies and roofs hide the ground under them */
    return cur_ts->mm[map_g[idx]];
}

/* Colour class of every tile in the window (ox, oy, w, h) of the current area, points of interest included:
   doors/warps (gold), closed chests (orange), signs (white), people (cyan, gold = quest). Shared by the HUD
   minimap and the menu's Map tab. */
static void map_cells(u8 *cell, int stride, int ox, int oy, int w, int h) {
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) cell[y * stride + x] = (u8)cell_class((oy + y) * 32 + ox + x);
    for (int i = 0; i < cur_area->nwarps; i++) {
        const WarpDef *wd = &cur_area->warps[i];
        if (wd->area == AREA_ROAD && cur_area_id == AREA_VILLAGE && !(gflags & GF_TYRANT_DEAD)) continue;   /* sealed gate */
        for (int ty = wd->y0; ty <= wd->y1; ty++)
            for (int tx = wd->x0; tx <= wd->x1; tx++)
                if (tx >= ox && tx < ox + w && ty >= oy && ty < oy + h) cell[(ty - oy) * stride + tx - ox] = 10;
    }
    for (int i = 0; i < cur_area->nsigns; i++) {
        int tx = cur_area->signs[i].tx - ox, ty = cur_area->signs[i].ty - oy;
        if (tx >= 0 && tx < w && ty >= 0 && ty < h) cell[ty * stride + tx] = 13;
    }
    for (int i = 0; i < cur_area->nchests; i++) {
        const ChestDef *c = &cur_area->chests[i];
        int tx = c->tx - ox, ty = c->ty - oy;
        if (!chest_taken(c->id) && tx >= 0 && tx < w && ty >= 0 && ty < h) cell[ty * stride + tx] = 11;
    }
    for (int i = 0; i < MAX_NPC; i++) {
        if (!npcs[i].active) continue;
        int tx = (npcs[i].x >> 4) - ox, ty = ((npcs[i].y - 4) >> 4) - oy;
        if (tx < 0 || tx >= w || ty < 0 || ty >= h) continue;
        int qm = quest_marker(npcs[i].id);
        cell[ty * stride + tx] = (qm >= 0 && qm != QM_BUSY) ? 10 : 12;
    }
}

/* Paints a cw x ch window of cells (three pixels each, framed) into a text area. The picture is built one pixel row
   at a time in a small RAM line and written to VRAM a word per 8 pixels: a pixel-by-pixel redraw of the whole map
   costs far more cycles than the vertical blank allows. (ix, iy) is where the cells start inside the canvas. */
static void mm_paint(TextArea *t, const u8 *cell, int stride, int cw, int ch, int ix, int iy) {
    static u8 line[112];
    int W = t->w * 8, H = t->h * 8, iw = cw * 3, ih = ch * 3;
    for (int py = 0; py < H; py++) {
        int dy = py - iy;
        for (int x = 0; x < W; x++) line[x] = (u8)t->bg;
        if (dy == -1 || dy == ih) {                                  /* top / bottom frame: corners left out = rounded */
            for (int x = ix; x < ix + iw; x++) line[x] = 1;
        } else if (dy >= 0 && dy < ih) {
            line[ix - 1] = 1;
            line[ix + iw] = 1;
            const u8 *crow = cell + (dy / 3) * stride;
            int sy = (dy % 3) * 3, cy = dy / 3;
            for (int c = 0; c < cw; c++) {
                const u8 *pt = &mm_pat[crow[c] & 15][(c + cy) & 1][sy];
                line[ix + c * 3] = pt[0];
                line[ix + c * 3 + 1] = pt[1];
                line[ix + c * 3 + 2] = pt[2];
            }
        }
        for (int tx = 0; tx < t->w; tx++) {
            u32 w = 0;
            for (int i = 0; i < 8; i++) w |= (u32)line[tx * 8 + i] << (i * 4);
            ta_row(t, tx, py >> 3, py & 7, w);
        }
    }
}

static void mm_redraw(int ox, int oy) {
    map_cells(&mm_cell[0][0], MMC, ox, oy, MMC, MMC);
    mm_paint(&ta_mm, &mm_cell[0][0], MMC, MMC, MMC, MM_IX, MM_IY);
    mm_ox = ox;
    mm_oy = oy;
    mm_chests = mm_chest_sig();
}

/* The whole area (32x32 tiles) on a 13x13-tile canvas, three pixels per tile, for the menu's Map tab.
   The canvas must use BG palette bank 15 (the minimap colours). */
void map_paint(TextArea *t) {
    static u8 cells[32 * 32];
    if (!cur_ts || !cur_area) { ta_clear(t); return; }
    map_cells(cells, 32, 0, 0, 32, 32);
    mm_paint(t, cells, 32, 32, 32, 4, 4);
}

static int mm_hx, mm_hy;             /* the hero's pixel on the minimap, for hud_draw() */
static void mm_update(void) {
    if (!hud_minimap || !cur_ts || !cur_area) return;
    if (cur_area->camw < 32) {                                    /* inside a house there is nothing to chart */
        if (!mm_hidden) { ta_clear(&ta_mm); mm_hidden = 1; }
        return;
    }
    if (mm_hidden) { mm_hidden = 0; mm_ox = mm_oy = -1; }
    int ptx = iclamp(player_body_x() >> 4, 0, 31), pty = iclamp(player_body_y() >> 4, 0, 31);
    int ox = iclamp(ptx - MMC / 2, 0, 32 - MMC), oy = iclamp(pty - MMC / 2, 0, 32 - MMC);
    if (ox != mm_ox || oy != mm_oy || mm_chests != mm_chest_sig()) mm_redraw(ox, oy);
    mm_hx = MM_TX * 8 + MM_IX + (ptx - ox) * 3 + 1 - 3;           /* the blip's art is centred on pixel 3 of its tile */
    mm_hy = MM_TY * 8 + MM_IY + (pty - oy) * 3 + 1 - 3;
}

/* sprites of the HUD (called after the world's sprites each frame): the hero's blip on the minimap */
void hud_draw(void) {
    if (hud_on && hud_minimap && !mm_hidden && ((frame_count >> 3) & 1)) spr_ui8(mm_hx, mm_hy, OT_BLIP, PB_ICON);
}

void hud_update(void) {
    if (!hud_on) return;
    track_update();
    mm_update();
    if (pl.hp_trail > pl.hp) {
        if (trail_delay > 0) trail_delay--;
        else pl.hp_trail--;
    } else pl.hp_trail = pl.hp;
    if (c_hp > pl.hp) trail_delay = 22;
    if (pl.level != c_lv) {
        c_lv = pl.level;
        ta_clear(&ta_lv);
        ta_print(&ta_lv, 1, 0, "Lv", C_YELLOW);
        ta_num(&ta_lv, 14, 0, pl.level, C_WHITE);
    }
    if (pl.hp != c_hp || pl.maxhp != c_maxhp || pl.hp_trail != c_trail) {
        c_hp = pl.hp; c_maxhp = pl.maxhp; c_trail = pl.hp_trail;
        int f = pl.maxhp ? (pl.hp * 70) / pl.maxhp : 0;
        int t = pl.maxhp ? (pl.hp_trail * 70) / pl.maxhp : 0;
        if (pl.hp > 0 && f < 1) f = 1;
        bar_draw(&ta_hp, f, t, 3, 4, 5, 11);
    }
    if (pl.mp != c_mp || pl.maxmp != c_maxmp) {
        c_mp = pl.mp; c_maxmp = pl.maxmp;
        int f = pl.maxmp ? (pl.mp * 54) / pl.maxmp : 0;
        bar_draw(&ta_mp, f, f, 6, 7, 8, 6);
    }
    {
        int need = xp_for_level(pl.level);
        int xv = pl.level >= LEVEL_CAP ? 54 : (int)((pl.xp * 54) / (u32)need);
        if (xv != c_xp) { c_xp = xv; thin_bar(&ta_xp, xv); }
    }
    if (pl.gold != c_gold) {
        c_gold = pl.gold;
        ta_clear(&ta_gold);
        int w = 0;
        char tmp[8];
        int v = pl.gold, m = 0;
        do { tmp[m++] = (char)('0' + v % 10); v /= 10; } while (v);
        (void)w;
        char buf[8];
        for (int i = 0; i < m; i++) buf[i] = tmp[m - 1 - i];
        buf[m] = 0;
        ta_print(&ta_gold, 1, 0, buf, C_YELLOW);
    }
    int pots = pl.inv[ITEM_POTION] + pl.inv[ITEM_HIPOTION];
    if (pots != c_pot) {
        c_pot = pots;
        ta_clear(&ta_pot);
        ta_print(&ta_pot, 1, 0, "x", C_GRAY);
        ta_num(&ta_pot, 7, 0, pots, pots ? C_WHITE : C_RED);
    }
    /* unspent stat points: a "+N" under the level, blinking */
    {
        int pv = pl.stat_pts ? (pl.stat_pts | (((frame_count >> 5) & 1) << 8)) : 0;
        if (pv != c_pts) {
            c_pts = pv;
            ta_clear(&ta_pts);
            if (pl.stat_pts && ((frame_count >> 5) & 1) == 0) {
                ta_print(&ta_pts, 1, 0, "+", C_GREEN);
                ta_num(&ta_pts, 7, 0, pl.stat_pts, C_GREEN);
            }
        }
    }
    /* combo counter: "N HITS", right aligned, hotter colours as the chain grows */
    {
        int n = pl.chain_n >= 2 ? pl.chain_n : 0;
        int col = n >= 8 ? C_RED : n >= 5 ? C_YELLOW : C_WHITE;
        if (n != c_chain || col != c_chain_col) {
            c_chain = n;
            c_chain_col = col;
            ta_clear(&ta_combo);
            if (n) {
                char buf[12];
                int m = 0;
                if (n >= 10) buf[m++] = (char)('0' + n / 10);
                buf[m++] = (char)('0' + n % 10);
                const char *t = " HITS";
                while (*t) buf[m++] = *t++;
                buf[m] = 0;
                ta_print(&ta_combo, 70 - ta_width(buf), 0, buf, col);
            }
        }
    }
    /* boss bar */
    Enemy *b = enemy_big();
    int show = b != 0;
    if (show && b->type != boss_bar_type) { boss_bar_type = b->type; boss_bar_on = 0; }
    if (show != boss_bar_on) {
        boss_bar_on = show;
        c_boss = -1;
        for (int x = 7; x < 23; x++) {
            bg0_set(x, 17, show ? (u16)((ta_bossname.base + (x - 7)) | (13 << 12)) : 0);
            bg0_set(x, 18, show ? (u16)((ta_boss.base + (x - 7)) | (12 << 12)) : 0);
        }
        if (show) {
            ta_clear(&ta_bossname);
            const char *bn = b->type == EN_TYRANT ? "Cinder Tyrant" : (b->type == EN_WARDEN ? "Bone Warden" : "Hollow King");
            ta_print(&ta_bossname, (128 - ta_width(bn)) / 2, 0, bn, C_RED);
        }
    }
    if (show && b->hp != c_boss) {
        c_boss = b->hp;
        int f = (b->hp * 126) / b->maxhp;
        bar_draw(&ta_boss, f, f, 3, 4, 5, 11);
    }
    /* banner / toast timers */
    if (banner_t > 0 && --banner_t == 0) ta_clear(&ta_banner);
    if (toast_t > 0 && --toast_t == 0) ta_clear(&ta_toast);
}

void banner_show(const char *text) {
    ta_clear(&ta_banner);
    int w = ta_width(text);
    ta_print(&ta_banner, (176 - w) / 2, 4, text, C_YELLOW);
    banner_t = 120;
}

void toast_show(const char *text) {
    ta_clear(&ta_toast);
    int w = ta_width(text);
    ta_print(&ta_toast, (208 - w) / 2, 0, text, C_WHITE);
    toast_t = 150;
}

void ui_overlay_update(void) {}

/* ------------------------------------------------------------------ level up & item get */
void levelup_fx(void) {
    int x = (int)(pl.x >> 8), y = (int)(pl.y >> 8) - 6;
    snd_sfx(SFX_LEVELUP);
    for (int i = 0; i < 12; i++) {
        int a = i * 21;
        part_spawn(PK_SPARKLE, x, y, (isin(a) * 260) >> 8, (-icos(a) * 260) >> 8, 26, 0);
    }
    for (int i = 0; i < 8; i++) part_spawn(PK_STAR, x - 10 + rnd_n(20), y + 8 - rnd_n(6), 0, -50 - rnd_n(40), 40, 0);
    pl.flash = 8;
    banner_show("LEVEL UP!");
    char buf[40];
    int n = 0;
    const char *s = "Lv ";
    while (*s) buf[n++] = *s++;
    if (pl.level >= 10) buf[n++] = (char)('0' + pl.level / 10);
    buf[n++] = (char)('0' + pl.level % 10);
    const char *t = "  +3 stat points! (START)";
    while (*t) buf[n++] = *t++;
    buf[n] = 0;
    toast_show(buf);
    shake_add(1);
}

void item_get_show(int item, int qty) {
    char buf[48];
    int n = 0;
    const char *s = "Got ";
    while (*s) buf[n++] = *s++;
    s = item_defs[item].name;
    while (*s && n < 36) buf[n++] = *s++;
    if (qty > 1) { buf[n++] = ' '; buf[n++] = 'x'; buf[n++] = (char)('0' + qty % 10); }
    buf[n++] = '!';
    buf[n] = 0;
    toast_show(buf);
}

/* ------------------------------------------------------------------ dialogue */
static TextArea dlg_ta;
static char dlg_speaker[20];
static const char *dlg_src;
static char page[100];
static int page_len, line_brk, shown, dlg_mode, pen_x, dlg_tick;
static int dlg_pool_mark;
static int dlg_open_flag;

static int width_of(const char *s, int n) {
    int w = 0;
    for (int i = 0; i < n; i++) {
        int ci = (u8)s[i] - 32;
        if (ci < 0 || ci > 94) ci = '?' - 32;
        w += font_w[ci] + 1;
    }
    return w;
}

static int next_page(void) {
    if (!dlg_src || !*dlg_src) return 0;
    while (*dlg_src == '|' || *dlg_src == ' ') dlg_src++;
    if (!*dlg_src) return 0;
    int n = 0;
    while (dlg_src[n] && dlg_src[n] != '|' && n < 98) n++;
    const char *src = dlg_src;
    const int maxw = 200;
    /* wrap into at most two lines; split extra text into following pages */
    int used = n;
    line_brk = n;
    if (width_of(src, n) > maxw) {
        int last = -1;
        for (int i = 0; i < n; i++) {
            if (src[i] == ' ') {
                if (width_of(src, i) <= maxw) last = i;
                else break;
            }
        }
        if (last < 0) last = n > 28 ? 28 : n;
        line_brk = last;
        int rest = last + 1;
        if (width_of(src + rest, n - rest) > maxw) {
            int last2 = -1;
            for (int i = rest; i < n; i++) {
                if (src[i] == ' ') {
                    if (width_of(src + rest, i - rest) <= maxw) last2 = i;
                    else break;
                }
            }
            if (last2 < 0) last2 = n;
            used = last2;
        }
    }
    for (int i = 0; i < used; i++) page[i] = src[i];
    page[used] = 0;
    page_len = used;
    dlg_src += used;
    return 1;
}

static void page_start(void) {
    ta_clear(&dlg_ta);
    shown = 0;
    pen_x = 0;
    dlg_tick = 0;
    dlg_mode = 1;
    if (dlg_speaker[0]) ta_print(&dlg_ta, 0, 1, dlg_speaker, C_YELLOW);
}

void dlg_open(const char *speaker, const char *text) {
    int i = 0;
    while (speaker[i] && i < 18) { dlg_speaker[i] = speaker[i]; i++; }
    dlg_speaker[i] = 0;
    dlg_src = text;
    dlg_pool_mark = text_pool_mark();
    ui_panel(1, 13, 28, 6);
    ta_init(&dlg_ta, 2, 14, 26, 4, 13, 8);
    dlg_open_flag = 1;
    if (next_page()) page_start();
    else dlg_mode = 0;
}

int dlg_active(void) { return dlg_open_flag; }

static void dlg_close(void) {
    for (int y = 13; y < 19; y++)
        for (int x = 1; x < 29; x++) bg0_set(x, y, 0);
    text_pool_reset(dlg_pool_mark);
    dlg_open_flag = 0;
    hud_remap();
}

static void type_one(int quiet) {
    if (shown >= page_len) return;
    if (line_brk < page_len && shown == line_brk) { shown++; pen_x = 0; return; }
    int line = (line_brk < page_len && shown > line_brk) ? 1 : 0;
    char c = page[shown++];
    pen_x += ta_putc(&dlg_ta, pen_x, line ? 22 : 12, c, C_WHITE);
    if (!quiet && c != ' ' && (shown & 1)) snd_sfx(SFX_TEXT + (page_len % 3));
}

int dlg_update(void) {
    if (!dlg_open_flag) return 1;
    if (dlg_mode == 1) {
        dlg_tick++;
        int per = (keys_held & KEY_B) ? 1 : 2;
        if (dlg_tick >= per && shown < page_len) {
            dlg_tick = 0;
            type_one(0);
        }
        if (keys_down & KEY_A) {
            while (shown < page_len) type_one(1);
        }
        if (shown >= page_len) dlg_mode = 2;
    } else if (dlg_mode == 2) {
        if (keys_down & (KEY_A | KEY_B)) {
            snd_sfx(SFX_MENU_OK);
            if (next_page()) page_start();
            else { dlg_mode = 0; dlg_close(); return 1; }
        }
        if ((frame_count >> 4) & 1) spr_add(222, 143, SH_SQUARE, 0, OT_STAR + 1, PB_UI, 0, 0, 30000);
    } else {
        dlg_close();
        return 1;
    }
    return 0;
}
