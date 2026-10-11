/* menu.c - the pause menu (Status / Stats / Items / Equip / System) and the shop.
   Anything that shows numbers goes through stats_calc(), so "what if" previews (a stat point, a different
   weapon) can never disagree with what the game really does. */
#include "game.h"
#include "assets.h"
#include "audio_data.h"
#include "ui_util.h"

enum { TAB_STATUS, TAB_STATS, TAB_ITEMS, TAB_EQUIP, TAB_SYSTEM, TAB_COUNT };
static const char *const tab_names[TAB_COUNT] = { "Status", "Stats", "Items", "Equip", "System" };
/* order slots are listed on screen (the enum order is fixed by the save format) */
static const u8 equip_order[SLOT_COUNT] = { SLOT_WEAPON, SLOT_HELM, SLOT_ARMOR, SLOT_BOOTS, SLOT_CHARM };

static TextArea m_tab, m_body, m_side, m_foot;
static int menu_tab, menu_sel, menu_sub, menu_msg_t, menu_top, menu_slot;
static int menu_list[ITEM_COUNT], menu_n;
static int menu_quit_title, menu_a_hold, menu_reset_armed;
static char menu_msg[48];
#define LINE_BUF 64

#define STAT_ROW_Y(i) (14 + (i) * 12)
#define STAT_RESET_Y 79

/* ------------------------------------------------------------------ shared drawing */
static void menu_tabs_draw(void) {
    ta_clear(&m_tab);
    for (int i = 0; i < TAB_COUNT; i++) {
        int col = i == menu_tab ? C_YELLOW : (i == TAB_STATS && pl.stat_pts ? C_GREEN : C_GRAY);
        ta_print(&m_tab, 4 + i * 45, 1, tab_names[i], col);
    }
}

static void footer2(const char *a, int ca, const char *b, int cb) {
    ta_clear(&m_foot);
    ta_print(&m_foot, 2, 0, a, ca);
    if (b) ta_print(&m_foot, 2, 8, b, cb);
}

static void footer(const char *s, int col) { footer2(s, col, 0, 0); }

/* one "label  current > new" row of the right-hand panel */
static void side_row(int y, const char *label, int cur, int nw, int col) {
    char b[12];
    ta_print(&m_side, 0, y, label, C_GRAY);
    num_str(b, cur);
    put_right(&m_side, y, 46, b, col);
    if (nw != cur) {
        ta_print(&m_side, 49, y, ">", C_GRAY);
        num_str(b, nw);
        put_right(&m_side, y, 80, b, nw > cur ? C_GREEN : C_RED);
    }
}

static void stats_now(Stats *s) { stats_calc(pl.cls, pl.level, pl.pts, pl.equip, s); }

/* ------------------------------------------------------------------ lists */
static void build_item_list(void) {
    menu_n = 0;
    for (int i = 1; i < ITEM_COUNT; i++)
        if (pl.inv[i] && item_defs[i].type != IT_EQUIP) menu_list[menu_n++] = i;
}

static void build_equip_list(int slot) {
    menu_n = 0;
    for (int i = 1; i < ITEM_COUNT; i++) {
        const ItemDef *it = &item_defs[i];
        if (!pl.inv[i] || it->type != IT_EQUIP || it->slot != slot) continue;
        if (slot == SLOT_WEAPON && it->cls != pl.cls) continue;
        menu_list[menu_n++] = i;
    }
}

/* ------------------------------------------------------------------ Status tab */
static void menu_draw_status(void) {
    char b[LINE_BUF];
    int k;
    ta_print(&m_body, 0, 1, class_defs[pl.cls].name, C_YELLOW);
    k = cat_str(b, 0, "Level ");
    cat_num(b, k, pl.level);
    ta_print(&m_body, 0, 11, b, C_WHITE);
    k = cat_str(b, 0, "HP ");
    k = cat_num(b, k, pl.hp);
    k = cat_str(b, k, " / ");
    cat_num(b, k, pl.maxhp);
    ta_print(&m_body, 0, 21, b, C_RED);
    k = cat_str(b, 0, "MP ");
    k = cat_num(b, k, pl.mp);
    k = cat_str(b, k, " / ");
    cat_num(b, k, pl.maxmp);
    ta_print(&m_body, 0, 31, b, C_CYAN);
    if (pl.level < LEVEL_CAP) {
        k = cat_str(b, 0, "EXP ");
        k = cat_num(b, k, (int)pl.xp);
        k = cat_str(b, k, " / ");
        cat_num(b, k, xp_for_level(pl.level));
    } else cat_str(b, 0, "EXP MAX");
    ta_print(&m_body, 0, 41, b, C_YELLOW);
    static const char *const lab[SLOT_COUNT] = { "Weapon", "Helm", "Armor", "Boots", "Charm" };
    for (int r = 0; r < SLOT_COUNT; r++) {
        int sl = equip_order[r];
        int it = pl.equip[sl];
        ta_print(&m_body, 0, 58 + r * 11, lab[r], C_GRAY);
        ta_print(&m_body, 38, 58 + r * 11, item_defs[it].name, it ? C_WHITE : C_GRAY);
    }
    /* side panel */
    Stats s;
    stats_now(&s);
    side_row(1, "ATK", s.atk, s.atk, C_ORANGE);
    side_row(12, "DEF", s.def, s.def, C_WHITE);
    side_row(23, "CRIT", s.crit, s.crit, C_WHITE);
    side_row(34, "DEX", s.spd, s.spd, C_WHITE);
    side_row(45, "LUK", s.lck, s.lck, C_WHITE);
    ta_print(&m_side, 0, 60, "Points", C_GRAY);
    num_str(b, pl.stat_pts);
    put_right(&m_side, 60, 80, b, pl.stat_pts ? C_GREEN : C_GRAY);
    ta_print(&m_side, 0, 71, "Gold", C_YELLOW);
    num_str(b, pl.gold);
    put_right(&m_side, 71, 80, b, C_YELLOW);
    ta_print(&m_side, 0, 82, "Time", C_GRAY);
    int secs = (int)(pl.playtime / 60);
    k = cat_num(b, 0, secs / 3600);
    k = cat_str(b, k, ":");
    if (((secs / 60) % 60) < 10) k = cat_str(b, k, "0");
    k = cat_num(b, k, (secs / 60) % 60);
    put_right(&m_side, 82, 80, b, C_WHITE);
}

/* ------------------------------------------------------------------ Stats tab */
static int primary_stat(void) { return pl.cls == CLS_MAGE ? STAT_INT : STAT_STR; }

static void menu_draw_stats(void) {
    char b[24];
    Stats cur;
    stats_now(&cur);
    int tot[STAT_COUNT] = { cur.str, cur.spd, cur.vit, cur.mag, cur.lck };
    ta_print(&m_body, 0, 1, "Stat Points", C_GRAY);
    num_str(b, pl.stat_pts);
    put_right(&m_body, 1, 130, b, pl.stat_pts ? C_GREEN : C_GRAY);
    for (int i = 0; i < STAT_COUNT; i++) {
        int y = STAT_ROW_Y(i);
        ta_print(&m_body, 14, y, stat_names[i], i == menu_sel ? C_YELLOW : C_WHITE);
        num_str(b, tot[i]);
        put_right(&m_body, y, 78, b, C_WHITE);
        if (pl.pts[i]) {
            int k = cat_str(b, 0, "+");
            cat_num(b, k, pl.pts[i]);
            ta_print(&m_body, 84, y, b, C_GREEN);
        }
        if (i == primary_stat() || (i == STAT_DEX && pl.cls >= CLS_RANGER)) ta_print(&m_body, 122, y, "*", C_ORANGE);
    }
    int k = cat_str(b, 0, "Reset (");
    k = cat_num(b, k, stats_reset_cost());
    cat_str(b, k, "G)");
    ta_print(&m_body, 14, STAT_RESET_Y, b, menu_reset_armed ? C_RED : (stats_invested() ? C_WHITE : C_GRAY));
    ta_print(&m_body, 4, 95, "* = class focus", C_GRAY);

    /* right panel: current -> value after adding one point to the highlighted stat */
    Stats nw = cur;
    if (menu_sel < STAT_COUNT && pl.stat_pts) {
        u8 p[STAT_COUNT];
        for (int i = 0; i < STAT_COUNT; i++) p[i] = pl.pts[i];
        if (p[menu_sel] < STAT_MAX) p[menu_sel]++;
        stats_calc(pl.cls, pl.level, p, pl.equip, &nw);
    }
    side_row(1, "ATK", cur.atk, nw.atk, C_ORANGE);
    side_row(12, "DEF", cur.def, nw.def, C_WHITE);
    side_row(23, "HP", cur.maxhp, nw.maxhp, C_RED);
    side_row(34, "MP", cur.maxmp, nw.maxmp, C_CYAN);
    side_row(45, "CRIT", cur.crit, nw.crit, C_WHITE);
    side_row(56, "DEX", cur.spd, nw.spd, C_WHITE);
    side_row(67, "ASPD", cur.aspd * 100 / 256, nw.aspd * 100 / 256, C_WHITE);
}

/* ------------------------------------------------------------------ Items tab */
static void menu_draw_items(void) {
    build_item_list();
    if (menu_sel >= menu_n) menu_sel = menu_n ? menu_n - 1 : 0;
    if (!menu_n) { ta_print(&m_body, 4, 4, "No items.", C_GRAY); return; }
    if (menu_sel < menu_top) menu_top = menu_sel;
    if (menu_sel > menu_top + 8) menu_top = menu_sel - 8;
    for (int r = 0; r < 9 && menu_top + r < menu_n; r++) {
        int it = menu_list[menu_top + r];
        char b[8];
        ta_print(&m_body, 14, 1 + r * 12, item_defs[it].name, C_WHITE);
        if (item_defs[it].type != IT_KEY) {
            int k = cat_str(b, 0, "x");
            cat_num(b, k, pl.inv[it]);
            put_right(&m_body, 1 + r * 12, 128, b, C_GRAY);
        }
    }
}

/* ------------------------------------------------------------------ Equip tab */
static void menu_draw_equip(void) {
    Stats cur, nw;
    stats_now(&cur);
    nw = cur;
    if (!menu_sub) {
        for (int r = 0; r < SLOT_COUNT; r++) {
            int sl = equip_order[r], it = pl.equip[sl];
            ta_print(&m_body, 14, 1 + r * 22, slot_names[sl], r == menu_sel ? C_YELLOW : C_GRAY);
            ta_print(&m_body, 22, 11 + r * 22, item_defs[it].name, it ? C_WHITE : C_GRAY);
        }
    } else {
        ta_print(&m_body, 14, 1, slot_names[menu_slot], C_YELLOW);
        if (!menu_n) ta_print(&m_body, 14, 14, "Nothing to equip.", C_GRAY);
        if (menu_sel < menu_top) menu_top = menu_sel;
        if (menu_sel > menu_top + 7) menu_top = menu_sel - 7;
        for (int r = 0; r < 8 && menu_top + r < menu_n; r++) {
            int it = menu_list[menu_top + r];
            ta_print(&m_body, 14, 14 + r * 11, item_defs[it].name, C_WHITE);
            if (pl.equip[item_defs[it].slot] == it) ta_print(&m_body, 122, 14 + r * 11, "E", C_GREEN);
        }
        if (menu_n) {
            u8 eq[SLOT_COUNT];
            for (int i = 0; i < SLOT_COUNT; i++) eq[i] = pl.equip[i];
            eq[menu_slot] = (u8)menu_list[menu_sel];
            stats_calc(pl.cls, pl.level, pl.pts, eq, &nw);
        }
    }
    side_row(1, "ATK", cur.atk, nw.atk, C_ORANGE);
    side_row(12, "DEF", cur.def, nw.def, C_WHITE);
    side_row(23, "CRIT", cur.crit, nw.crit, C_WHITE);
    side_row(34, "DEX", cur.spd, nw.spd, C_WHITE);
    side_row(45, "LUK", cur.lck, nw.lck, C_WHITE);
    side_row(56, "HP", cur.maxhp, nw.maxhp, C_RED);
    side_row(67, "MP", cur.maxmp, nw.maxmp, C_CYAN);
}

/* ------------------------------------------------------------------ System tab */
static void menu_draw_system(void) {
    ta_print(&m_body, 14, 1, "Save Game", C_WHITE);
    ta_print(&m_body, 14, 14, music_on ? "Music: On" : "Music: Off", C_WHITE);
    ta_print(&m_body, 14, 27, "Title Screen", C_WHITE);
    ta_print(&m_body, 14, 40, "Back", C_WHITE);
    ta_print(&m_body, 4, 62, "Controls:", C_YELLOW);
    ta_print(&m_body, 4, 73, "A attack (tap to chain, hold)", C_GRAY);
    ta_print(&m_body, 4, 84, "B skill (MP)   R roll", C_GRAY);
    ta_print(&m_body, 4, 95, "L quick potion  START menu", C_GRAY);
}

static void menu_rebuild(void) {
    ta_clear(&m_body);
    ta_clear(&m_side);
    menu_tabs_draw();
    char b[LINE_BUF];
    switch (menu_tab) {
    case TAB_STATUS:
        menu_draw_status();
        if (pl.stat_pts) footer2("You have unspent stat points!", C_GREEN, "Open the Stats tab (L/R).", C_GRAY);
        else footer("L/R: switch tab   B: close", C_GRAY);
        break;
    case TAB_STATS:
        menu_draw_stats();
        if (menu_sel < STAT_COUNT) footer2(stat_blurb(menu_sel), C_WHITE, pl.stat_pts ? "A: add a point (hold to repeat)" : "No points left. Level up for more.", C_GRAY);
        else if (menu_reset_armed) footer2("Press A again to confirm.", C_RED, "All points are refunded.", C_GRAY);
        else {
            int k = cat_str(b, 0, "Refund all points for ");
            k = cat_num(b, k, stats_reset_cost());
            cat_str(b, k, "G.");
            footer2(b, C_WHITE, "Rearrange your build any time.", C_GRAY);
        }
        if (menu_msg_t) footer(menu_msg, C_GREEN);
        break;
    case TAB_ITEMS:
        menu_draw_items();
        if (menu_n) footer(item_defs[menu_list[menu_sel]].desc, C_WHITE);
        else footer("L/R: switch tab", C_GRAY);
        break;
    case TAB_EQUIP:
        menu_draw_equip();
        if (!menu_sub) footer("A: choose slot", C_GRAY);
        else if (menu_n) {
            item_stat_line(b, menu_list[menu_sel]);
            footer2(b, C_WHITE, item_defs[menu_list[menu_sel]].desc, C_GRAY);
        } else footer("B: back", C_GRAY);
        break;
    case TAB_SYSTEM:
        menu_draw_system();
        if (menu_msg_t) footer(menu_msg, C_GREEN);
        else footer("A: select", C_GRAY);
        break;
    }
}

void menu_open(void) {
    snd_sfx(SFX_MENU_OK);
    text_pool_reset(UT_POOL);
    bg0_clear();
    ui_panel(0, 0, 30, 20);
    ta_init(&m_tab, 1, 0, 28, 1, 13, 8);
    ta_init(&m_body, 1, 2, 17, 14, 13, 8);
    ta_init(&m_side, 19, 2, 10, 13, 13, 8);
    ta_init(&m_foot, 1, 17, 28, 2, 13, 8);
    for (int x = 1; x < 29; x++) bg0_set(x, 1, (u16)(UT_FRAME_T | (13 << 12)));
    menu_tab = TAB_STATUS;
    menu_sel = 0;
    menu_sub = 0;
    menu_top = 0;
    menu_msg_t = 0;
    menu_quit_title = 0;
    menu_a_hold = 0;
    menu_reset_armed = 0;
    menu_rebuild();
}

static void switch_tab(int d) {
    menu_tab = (menu_tab + d + TAB_COUNT) % TAB_COUNT;
    menu_sel = 0;
    menu_sub = 0;
    menu_top = 0;
    menu_reset_armed = 0;
    menu_msg_t = 0;
    snd_sfx(SFX_MENU_MOVE);
}

static void set_msg(const char *s, int frames) {
    cat_str(menu_msg, 0, s);
    menu_msg_t = frames;
}

/* A on the Stats tab: spend a point, or (on the last row) refund everything */
static int stats_press(void) {
    if (menu_sel < STAT_COUNT) {
        if (stats_spend(menu_sel)) { snd_sfx(SFX_EQUIP); return 1; }
        snd_sfx(SFX_ERROR);
        return 1;
    }
    if (!stats_invested()) { snd_sfx(SFX_ERROR); set_msg("No points to refund.", 70); return 1; }
    if (!menu_reset_armed) { menu_reset_armed = 180; snd_sfx(SFX_MENU_OK); return 1; }
    if (pl.gold < stats_reset_cost()) { menu_reset_armed = 0; snd_sfx(SFX_ERROR); set_msg("Not enough gold.", 70); return 1; }
    pl.gold = (u16)(pl.gold - stats_reset_cost());
    stats_reset();
    menu_reset_armed = 0;
    snd_sfx(SFX_COIN);
    set_msg("Points refunded!", 90);
    return 1;
}

int menu_update(void) {
    int redraw = 0;
    if (menu_msg_t && --menu_msg_t == 0) redraw = 1;
    if (menu_reset_armed && --menu_reset_armed == 0) redraw = 1;
    if (keys_down & KEY_R) { switch_tab(1); redraw = 1; }
    if (keys_down & KEY_L) { switch_tab(-1); redraw = 1; }
    int n = 0;
    if (menu_tab == TAB_STATS) n = STAT_COUNT + 1;
    else if (menu_tab == TAB_ITEMS) n = menu_n;
    else if (menu_tab == TAB_EQUIP) n = menu_sub ? menu_n : SLOT_COUNT;
    else if (menu_tab == TAB_SYSTEM) n = 4;
    if (n > 0) {
        if (keys_down & KEY_DOWN) { menu_sel = (menu_sel + 1) % n; menu_reset_armed = 0; redraw = 1; snd_sfx(SFX_MENU_MOVE); }
        if (keys_down & KEY_UP) { menu_sel = (menu_sel + n - 1) % n; menu_reset_armed = 0; redraw = 1; snd_sfx(SFX_MENU_MOVE); }
    }
    /* holding A on a stat row keeps adding points */
    int a_press = (keys_down & KEY_A) != 0;
    if (keys_held & KEY_A) {
        menu_a_hold++;
        if (menu_tab == TAB_STATS && menu_sel < STAT_COUNT && menu_a_hold > 24 && ((menu_a_hold - 24) % 5) == 0) a_press = 1;
    } else menu_a_hold = 0;
    if (a_press) {
        if (menu_tab == TAB_ITEMS && menu_n) {
            int it = menu_list[menu_sel];
            if (item_defs[it].type == IT_USE) {
                if (!player_use_item(it)) snd_sfx(SFX_ERROR);
                redraw = 1;
            }
        } else if (menu_tab == TAB_STATS) {
            redraw |= stats_press();
        } else if (menu_tab == TAB_EQUIP) {
            if (!menu_sub) {
                menu_slot = equip_order[menu_sel];
                build_equip_list(menu_slot);
                menu_sub = 1;
                menu_sel = 0;
                menu_top = 0;
                snd_sfx(SFX_MENU_OK);
                redraw = 1;
            } else if (menu_n) {
                player_equip(menu_list[menu_sel]);
                snd_sfx(SFX_EQUIP);
                redraw = 1;
            }
        } else if (menu_tab == TAB_SYSTEM) {
            if (menu_sel == 0) {
                save_write();
                snd_sfx(SFX_SAVE);
                set_msg("Game saved.", 120);
                redraw = 1;
            } else if (menu_sel == 1) {
                snd_set_music(!music_on);
                snd_sfx(SFX_MENU_OK);
                redraw = 1;
            } else if (menu_sel == 2) {
                menu_quit_title = 1;
                return 2;
            } else return 1;
        }
    }
    if (keys_down & KEY_B) {
        if (menu_tab == TAB_EQUIP && menu_sub) {
            menu_sub = 0;
            menu_sel = 0;
            for (int r = 0; r < SLOT_COUNT; r++) if (equip_order[r] == menu_slot) menu_sel = r;
            redraw = 1;
            snd_sfx(SFX_MENU_BACK);
        } else { snd_sfx(SFX_MENU_BACK); return 1; }
    }
    if (keys_down & KEY_START) { snd_sfx(SFX_MENU_BACK); return 1; }
    if (redraw) menu_rebuild();
    /* cursor sprite */
    if (menu_tab == TAB_STATS) cursor_at(6, 13 + (menu_sel < STAT_COUNT ? STAT_ROW_Y(menu_sel) : STAT_RESET_Y));
    if (menu_tab == TAB_ITEMS && menu_n) cursor_at(6, 14 + (menu_sel - menu_top) * 12);
    if (menu_tab == TAB_EQUIP) {
        if (!menu_sub) cursor_at(6, 14 + menu_sel * 22);
        else if (menu_n) cursor_at(6, 27 + (menu_sel - menu_top) * 11);
    }
    if (menu_tab == TAB_SYSTEM) cursor_at(6, 14 + menu_sel * 13);
    if (menu_tab == TAB_STATUS) {
        int f = (frame_count / 6) % 6;
        spr_ui16(206, 118, f * 4, PB_HERO);
    }
    return 0;
}

/* ================================================================== SHOP */
#define SHOP_ROWS 7
static TextArea s_head, s_list, s_desc;
static int shop_stock[32], shop_n, shop_sel, shop_top;
static char shop_msg[40];
static int shop_msg_t;

static int shop_price(int item) { return item_defs[item].price; }

static void shop_draw(void) {
    ta_clear(&s_list);
    ta_clear(&s_head);
    ta_print(&s_head, 4, 1, "Tobin's Wares", C_YELLOW);
    char b[LINE_BUF];
    int k = cat_num(b, 0, pl.gold);
    cat_str(b, k, " G");
    put_right(&s_head, 1, 204, b, C_YELLOW);
    if (shop_sel < shop_top) shop_top = shop_sel;
    if (shop_sel >= shop_top + SHOP_ROWS) shop_top = shop_sel - SHOP_ROWS + 1;
    for (int r = 0; r < SHOP_ROWS && shop_top + r < shop_n; r++) {
        int it = shop_stock[shop_top + r];
        int owned_equip = item_defs[it].type == IT_EQUIP && pl.inv[it] > 0;
        int col = owned_equip ? C_GRAY : (pl.gold >= shop_price(it) ? C_WHITE : C_RED);
        ta_print(&s_list, 14, 1 + r * 12, item_defs[it].name, col);
        if (owned_equip) put_right(&s_list, 1 + r * 12, 200, "Owned", C_GRAY);
        else {
            int kk = cat_num(b, 0, shop_price(it));
            cat_str(b, kk, " G");
            put_right(&s_list, 1 + r * 12, 200, b, col);
        }
    }
    if (shop_top > 0) ta_print(&s_list, 204, 1, "^", C_GRAY);
    if (shop_top + SHOP_ROWS < shop_n) ta_print(&s_list, 204, 73, "v", C_GRAY);
    ta_clear(&s_desc);
    if (shop_msg_t) ta_print(&s_desc, 4, 4, shop_msg, C_GREEN);
    else {
        int it = shop_stock[shop_sel];
        const ItemDef *d = &item_defs[it];
        if (d->type == IT_EQUIP) {
            item_stat_line(b, it);
            ta_print(&s_desc, 4, 1, b, C_WHITE);
            int eq = pl.equip[d->slot];
            int k2 = cat_str(b, 0, "Now: ");
            cat_str(b, k2, item_defs[eq].name);
            ta_print(&s_desc, 4, 12, b, C_GRAY);
        } else {
            ta_print(&s_desc, 4, 1, d->desc, C_WHITE);
            int k2 = cat_str(b, 0, "You have ");
            cat_num(b, k2, pl.inv[it]);
            ta_print(&s_desc, 4, 12, b, C_GRAY);
        }
    }
}

void shop_open(void) {
    text_pool_reset(UT_POOL);
    bg0_clear();
    ui_panel(0, 0, 30, 20);
    ta_init(&s_head, 1, 1, 28, 1, 13, 8);
    ta_init(&s_list, 2, 3, 26, 11, 13, 8);
    ta_init(&s_desc, 2, 15, 26, 3, 13, 8);
    shop_n = shop_build_stock(shop_stock, 32);
    shop_sel = 0;
    shop_top = 0;
    shop_msg_t = 0;
    snd_sfx(SFX_MENU_OK);
    shop_draw();
}

int shop_update(void) {
    int redraw = 0;
    if (shop_msg_t && --shop_msg_t == 0) redraw = 1;
    if (keys_down & KEY_DOWN) { shop_sel = (shop_sel + 1) % shop_n; redraw = 1; snd_sfx(SFX_MENU_MOVE); }
    if (keys_down & KEY_UP) { shop_sel = (shop_sel + shop_n - 1) % shop_n; redraw = 1; snd_sfx(SFX_MENU_MOVE); }
    if (keys_down & KEY_A) {
        int it = shop_stock[shop_sel];
        int owned_equip = item_defs[it].type == IT_EQUIP && pl.inv[it] > 0;
        if (owned_equip) { snd_sfx(SFX_ERROR); cat_str(shop_msg, 0, "Already owned."); shop_msg_t = 60; }
        else if (pl.gold < shop_price(it)) { snd_sfx(SFX_ERROR); cat_str(shop_msg, 0, "Not enough gold."); shop_msg_t = 60; }
        else {
            pl.gold = (u16)(pl.gold - shop_price(it));
            player_give(it, 1);
            snd_sfx(SFX_COIN);
            if (item_defs[it].type != IT_EQUIP) cat_str(shop_msg, 0, "Purchased!");
            else if (pl.equip[item_defs[it].slot] == it) cat_str(shop_msg, 0, "Purchased and equipped!");
            else cat_str(shop_msg, 0, "Purchased! Equip it in the menu.");
            shop_msg_t = 70;
        }
        redraw = 1;
    }
    if (keys_down & KEY_B) { snd_sfx(SFX_MENU_BACK); return 1; }
    if (redraw) shop_draw();
    cursor_at(14, 22 + (shop_sel - shop_top) * 12);
    return 0;
}
