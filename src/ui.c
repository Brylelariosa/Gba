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

static TextArea ta_lv, ta_hp, ta_mp, ta_xp, ta_gold, ta_pot, ta_boss, ta_bossname, ta_banner, ta_toast;
static int hud_on, hud_pool_end;
static int c_hp = -1, c_mp = -1, c_xp = -1, c_lv = -1, c_gold = -1, c_pot = -1, c_maxhp = -1, c_maxmp = -1, c_trail = -1, c_boss = -1;
static int trail_delay;
static int banner_t, toast_t;
static int boss_bar_on;

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
    ta_init(&ta_gold, 27, 0, 3, 1, 13, 0);
    ta_init(&ta_pot, 1, 18, 3, 1, 13, 0);
    ta_init(&ta_bossname, 7, 17, 16, 1, 13, 0);
    ta_init(&ta_boss, 7, 18, 16, 1, 12, 0);
    ta_init(&ta_banner, 4, 6, 22, 2, 13, 0);
    ta_init(&ta_toast, 2, 16, 26, 1, 13, 0);
    bg0_set(26, 0, (u16)(UT_COIN | (13 << 12)));
    bg0_set(0, 18, (u16)(UT_POTION | (13 << 12)));
    hud_pool_end = text_pool_mark();
    c_hp = c_mp = c_xp = c_lv = c_gold = c_pot = c_maxhp = c_maxmp = c_trail = c_boss = -1;
    banner_t = toast_t = 0;
    boss_bar_on = 0;
    hud_on = 1;
    /* hide boss widgets until needed */
    for (int x = 7; x < 23; x++) { bg0_set(x, 17, 0); bg0_set(x, 18, 0); }
    hud_update();
}

void hud_remap(void) {
    ta_remap(&ta_lv); ta_remap(&ta_hp); ta_remap(&ta_mp); ta_remap(&ta_xp); ta_remap(&ta_gold); ta_remap(&ta_pot);
    ta_remap(&ta_banner); ta_remap(&ta_toast);
    bg0_set(26, 0, (u16)(UT_COIN | (13 << 12)));
    bg0_set(0, 18, (u16)(UT_POTION | (13 << 12)));
    if (boss_bar_on) {
        ta_remap(&ta_bossname);
        ta_remap(&ta_boss);
    }
}

void hud_show(int on) { hud_on = on; }

void hud_update(void) {
    if (!hud_on) return;
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
        int xv = pl.level >= 20 ? 54 : (int)((pl.xp * 54) / (u32)need);
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
        ta_print(&ta_gold, 24 - ta_width(buf) , 0, buf, C_YELLOW);
    }
    int pots = pl.inv[ITEM_POTION] + pl.inv[ITEM_HIPOTION];
    if (pots != c_pot) {
        c_pot = pots;
        ta_clear(&ta_pot);
        ta_print(&ta_pot, 1, 0, "x", C_GRAY);
        ta_num(&ta_pot, 7, 0, pots, pots ? C_WHITE : C_RED);
    }
    /* boss bar */
    Enemy *b = enemy_boss();
    int show = b && b->active && (gflags & GF_BOSS_SEEN) && b->state != 11;
    if (show != boss_bar_on) {
        boss_bar_on = show;
        c_boss = -1;
        for (int x = 7; x < 23; x++) {
            bg0_set(x, 17, show ? (u16)((ta_bossname.base + (x - 7)) | (13 << 12)) : 0);
            bg0_set(x, 18, show ? (u16)((ta_boss.base + (x - 7)) | (12 << 12)) : 0);
        }
        if (show) {
            ta_clear(&ta_bossname);
            ta_print(&ta_bossname, (128 - ta_width("Hollow King")) / 2, 0, "Hollow King", C_RED);
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
    const char *t = "  HP and MP restored!";
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
