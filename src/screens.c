#include "game.h"
#include "assets.h"
#include "audio_data.h"

#define C_WHITE 1
#define C_YELLOW 3
#define C_RED 4
#define C_GREEN 5
#define C_CYAN 6
#define C_GRAY 7
#define C_ORANGE 12
#define C_PURPLE 13

static void cursor_at(int x, int y) {
    spr_ui16(x, y, OT_CURSOR + ((frame_count >> 4) & 1) * 4, PB_UI);
}

static void put_centered(TextArea *t, int y, const char *s, int col) {
    int w = ta_width(s);
    ta_print(t, (t->w * 8 - w) / 2, y, s, col);
}

static void put_right(TextArea *t, int y, int rx, const char *s, int col) {
    ta_print(t, rx - ta_width(s), y, s, col);
}

static void num_str(char *buf, int v) {
    char tmp[12];
    int m = 0, n = 0;
    if (v < 0) { buf[n++] = '-'; v = -v; }
    do { tmp[m++] = (char)('0' + v % 10); v /= 10; } while (v && m < 10);
    while (m) buf[n++] = tmp[--m];
    buf[n] = 0;
}

static int cat_str(char *d, int n, const char *s) {
    while (*s) d[n++] = *s++;
    d[n] = 0;
    return n;
}

static int cat_num(char *d, int n, int v) {
    char b[12];
    num_str(b, v);
    return cat_str(d, n, b);
}

/* ================================================================== TITLE */
static TextArea t_press, t_menu;
static int title_t, title_sel, title_menu_mode;
static u8 star_x[6], star_y[6], star_ph[6];

void title_enter(void) {
    gfx_blank(1);
    snd_stop_music();
    text_pool_reset(UT_POOL);
    bg0_clear();
    gfx_clear_world_maps();
    gfx_load_title(1);
    gfx_mode_ui();
    bg2_darken(0);
    cam_x = cam_y = 0;
    ta_init(&t_press, 9, 15, 12, 1, 13, 0);
    ta_init(&t_menu, 10, 13, 10, 4, 13, 0);
    put_centered(&t_press, 0, "PRESS START", C_WHITE);
    title_t = 0;
    title_sel = 0;
    title_menu_mode = 0;
    for (int i = 0; i < 6; i++) {
        star_x[i] = (u8)(10 + rnd_n(220));
        star_y[i] = (u8)(54 + rnd_n(40));
        star_ph[i] = (u8)rnd_n(40);
    }
    gfx_blank(0);
    snd_play_song(SONG_TITLE);
    gfx_set_fade(16, 0);
}

static void title_menu_draw(void) {
    ta_clear(&t_menu);
    ta_print(&t_menu, 12, 0, "New Game", title_sel == 0 ? C_YELLOW : C_WHITE);
    ta_print(&t_menu, 12, 12, "Continue", title_sel == 1 ? C_YELLOW : C_WHITE);
}

int title_update(void) {
    title_t++;
    if (title_t < 20) gfx_set_fade(16 - title_t * 16 / 20, 0);
    else gfx_set_fade(0, 0);
    rnd();
    for (int i = 0; i < 6; i++) {
        int f = ((title_t + star_ph[i]) >> 3) & 7;
        f = f > 3 ? 7 - f : f;
        spr_add(star_x[i], star_y[i], SH_SQUARE, 0, OT_STAR + f, PB_UI, 2, 0, 100);
    }
    if (!title_menu_mode) {
        if ((title_t >> 5) & 1) ta_clear(&t_press);
        else { ta_clear(&t_press); put_centered(&t_press, 0, "PRESS START", C_WHITE); }
        if (keys_down & (KEY_START | KEY_A)) {
            rnd_seed(title_t * 977 + frame_count);
            snd_sfx(SFX_MENU_OK);
            if (has_save()) {
                title_menu_mode = 1;
                ta_clear(&t_press);
                title_menu_draw();
            } else return 1;
        }
    } else {
        if (keys_down & (KEY_UP | KEY_DOWN)) { title_sel ^= 1; snd_sfx(SFX_MENU_MOVE); title_menu_draw(); }
        cursor_at(74, 101 + title_sel * 12);
        if (keys_down & KEY_A) { snd_sfx(SFX_MENU_OK); return title_sel == 0 ? 1 : 2; }
    }
    return 0;
}

/* ================================================================== CLASS SELECT */
static TextArea c_head, c_info, c_stats;
static int cls_sel, cls_t;
#define CLS_TILES 700

static void class_info_draw(void) {
    const ClassDef *c = &class_defs[cls_sel];
    ta_clear(&c_info);
    ta_print(&c_info, 0, 1, c->name, C_YELLOW);
    /* simple word wrap of the description over two lines */
    char l1[64], l2[64];
    int n = 0;
    const char *d = c->desc;
    int len = 0;
    while (d[len]) len++;
    int cut = len;
    if (ta_width(d) > 200) {
        cut = 0;
        for (int i = 0; i < len; i++) {
            if (d[i] == ' ') {
                char tmp[64];
                int k = 0;
                for (; k < i; k++) tmp[k] = d[k];
                tmp[k] = 0;
                if (ta_width(tmp) <= 200) cut = i; else break;
            }
        }
    }
    for (int i = 0; i < cut; i++) l1[n++] = d[i];
    l1[n] = 0;
    n = 0;
    for (int i = cut + 1; i < len; i++) l2[n++] = d[i];
    l2[n] = 0;
    ta_print(&c_info, 0, 12, l1, C_WHITE);
    ta_print(&c_info, 0, 22, l2, C_WHITE);
    ta_clear(&c_stats);
    char b[48];
    int k = cat_str(b, 0, "HP ");
    k = cat_num(b, k, c->hp);
    ta_print(&c_stats, 0, 1, b, C_RED);
    k = cat_str(b, 0, "MP ");
    k = cat_num(b, k, c->mp);
    ta_print(&c_stats, 52, 1, b, C_CYAN);
    k = cat_str(b, 0, "ATK ");
    k = cat_num(b, k, c->str > c->mag ? c->str : c->mag);
    ta_print(&c_stats, 104, 1, b, C_ORANGE);
    k = cat_str(b, 0, "DEF ");
    k = cat_num(b, k, c->def);
    ta_print(&c_stats, 156, 1, b, C_WHITE);
    k = cat_str(b, 0, "SPD ");
    k = cat_num(b, k, c->spd);
    ta_print(&c_stats, 0, 11, b, C_GREEN);
    k = cat_str(b, 0, "LCK ");
    k = cat_num(b, k, c->lck);
    ta_print(&c_stats, 52, 11, b, C_YELLOW);
    (void)k;
}

void class_enter(void) {
    text_pool_reset(UT_POOL);
    bg0_clear();
    gfx_blank(1);
    /* preview sprites: every class's walk-down frames in spare OBJ tiles + palettes in banks 0,8,9,10 */
    static const u8 banks[4] = { 0, 8, 9, 10 };
    for (int c = 0; c < 4; c++) {
        copy32(OBJ_VRAM32 + (CLS_TILES + c * 24) * 8, hero_tiles[c], 24 * 8);
        copy16(PAL_OBJ + banks[c] * 16, hero_pal[c], 16);
    }
    gfx_blank(0);
    gfx_blank(1);
    gfx_load_title(0);
    gfx_blank(0);
    bg2_darken(4);
    ta_init(&c_head, 4, 1, 22, 1, 13, 0);
    put_centered(&c_head, 0, "Choose your class", C_YELLOW);
    ui_panel(1, 10, 28, 10);
    ta_init(&c_info, 2, 11, 26, 4, 13, 8);
    ta_init(&c_stats, 2, 15, 26, 3, 13, 8);
    cls_sel = 0;
    cls_t = 0;
    class_info_draw();
    snd_sfx(SFX_MENU_OK);
}

int class_update(void) {
    static const u8 banks[4] = { 0, 8, 9, 10 };
    cls_t++;
    if (keys_down & KEY_LEFT) { cls_sel = (cls_sel + 3) & 3; snd_sfx(SFX_MENU_MOVE); class_info_draw(); }
    if (keys_down & KEY_RIGHT) { cls_sel = (cls_sel + 1) & 3; snd_sfx(SFX_MENU_MOVE); class_info_draw(); }
    for (int c = 0; c < 4; c++) {
        int cx = 32 + c * 48, cy = 56;
        int sel = c == cls_sel;
        int f = sel ? (cls_t / 7) % 6 : ((cls_t >> 5) & 1 ? 2 : 0);
        int tile = CLS_TILES + c * 24 + f * 4;
        if (sel) {
            int bob = (cls_t >> 3) & 1;
            spr_add_aff(cx - 16, cy - 16 - bob, SH_SQUARE, 1, tile, banks[c], 0, SF_DOUBLE, 50, 0, 128);
            spr_add(cx - 4, cy - 30 - bob, SH_SQUARE, 0, OT_STAR + ((cls_t >> 3) & 3), PB_UI, 0, 0, 51);
        } else {
            spr_add(cx - 8, cy - 8, SH_SQUARE, 1, tile, banks[c], 0, 0, 50);
        }
    }
    if (keys_down & KEY_A) { snd_sfx(SFX_MENU_OK); return cls_sel + 1; }
    if (keys_down & KEY_B) { snd_sfx(SFX_MENU_BACK); return -1; }
    return 0;
}

/* ================================================================== MENU */
static TextArea m_tab, m_body, m_side, m_foot;
static int menu_tab, menu_sel, menu_sub, menu_msg_t, menu_top, menu_slot;
static int menu_list[ITEM_COUNT], menu_n;
static int menu_quit_title;
static char menu_msg[48];

static const char *tab_names[4] = { "Status", "Items", "Equip", "System" };

static void menu_tabs_draw(void) {
    ta_clear(&m_tab);
    for (int i = 0; i < 4; i++) {
        int x = 6 + i * 54;
        ta_print(&m_tab, x, 1, tab_names[i], i == menu_tab ? C_YELLOW : C_GRAY);
    }
}

static void footer(const char *s, int col) {
    ta_clear(&m_foot);
    ta_print(&m_foot, 2, 1, s, col);
}

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

static void menu_draw_status(void) {
    char b[48];
    int k;
    ta_print(&m_body, 0, 1, class_defs[pl.cls].name, C_YELLOW);
    k = cat_str(b, 0, "Level ");
    cat_num(b, k, pl.level);
    ta_print(&m_body, 0, 12, b, C_WHITE);
    k = cat_str(b, 0, "HP ");
    k = cat_num(b, k, pl.hp);
    k = cat_str(b, k, " / ");
    cat_num(b, k, pl.maxhp);
    ta_print(&m_body, 0, 24, b, C_RED);
    k = cat_str(b, 0, "MP ");
    k = cat_num(b, k, pl.mp);
    k = cat_str(b, k, " / ");
    cat_num(b, k, pl.maxmp);
    ta_print(&m_body, 0, 35, b, C_CYAN);
    if (pl.level < 20) {
        k = cat_str(b, 0, "EXP ");
        k = cat_num(b, k, (int)pl.xp);
        k = cat_str(b, k, " / ");
        cat_num(b, k, xp_for_level(pl.level));
    } else cat_str(b, 0, "EXP MAX");
    ta_print(&m_body, 0, 46, b, C_YELLOW);
    ta_print(&m_body, 0, 62, "Weapon", C_GRAY);
    ta_print(&m_body, 44, 62, item_defs[pl.equip[0]].name, C_WHITE);
    ta_print(&m_body, 0, 73, "Armor", C_GRAY);
    ta_print(&m_body, 44, 73, item_defs[pl.equip[1]].name, C_WHITE);
    ta_print(&m_body, 0, 84, "Charm", C_GRAY);
    ta_print(&m_body, 44, 84, item_defs[pl.equip[2]].name, pl.equip[2] ? C_WHITE : C_GRAY);
    static const char *nm[6] = { "ATK", "DEF", "SPD", "LCK", "STR", "MAG" };
    int v[6] = { pl.atk, pl.def, pl.spd, pl.lck, pl.str, pl.mag };
    for (int i = 0; i < 6; i++) {
        ta_print(&m_side, 0, 1 + i * 11, nm[i], C_GRAY);
        num_str(b, v[i]);
        put_right(&m_side, 1 + i * 11, 80, b, i == 0 ? C_ORANGE : C_WHITE);
    }
    ta_print(&m_side, 0, 70, "Gold", C_YELLOW);
    num_str(b, pl.gold);
    put_right(&m_side, 70, 80, b, C_YELLOW);
    ta_print(&m_side, 0, 81, "Time", C_GRAY);
    int secs = (int)(pl.playtime / 60);
    k = cat_num(b, 0, secs / 3600);
    k = cat_str(b, k, ":");
    if (((secs / 60) % 60) < 10) k = cat_str(b, k, "0");
    k = cat_num(b, k, (secs / 60) % 60);
    put_right(&m_side, 81, 80, b, C_WHITE);
}

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

static void menu_draw_equip(void) {
    static const char *slotn[3] = { "Weapon", "Armor", "Charm" };
    if (!menu_sub) {
        for (int s = 0; s < 3; s++) {
            ta_print(&m_body, 14, 1 + s * 14, slotn[s], C_GRAY);
            ta_print(&m_body, 14, 8 + s * 14 + 1, "", C_WHITE);
        }
        for (int s = 0; s < 3; s++) {
            ta_print(&m_body, 64, 1 + s * 14, item_defs[pl.equip[s]].name, pl.equip[s] ? C_WHITE : C_GRAY);
        }
    } else {
        ta_print(&m_body, 14, 1, slotn[menu_slot], C_YELLOW);
        if (!menu_n) ta_print(&m_body, 14, 14, "Nothing to equip.", C_GRAY);
        for (int r = 0; r < 8 && menu_top + r < menu_n; r++) {
            int it = menu_list[menu_top + r];
            ta_print(&m_body, 14, 14 + r * 11, item_defs[it].name, C_WHITE);
            if (pl.equip[item_defs[it].slot] == it) ta_print(&m_body, 104, 14 + r * 11, "E", C_GREEN);
        }
    }
    char b[24];
    static const char *nm[4] = { "ATK", "DEF", "SPD", "LCK" };
    int v[4] = { pl.atk, pl.def, pl.spd, pl.lck };
    for (int i = 0; i < 4; i++) {
        ta_print(&m_side, 0, 1 + i * 11, nm[i], C_GRAY);
        num_str(b, v[i]);
        put_right(&m_side, 1 + i * 11, 80, b, C_WHITE);
    }
    ta_print(&m_side, 0, 50, "HP", C_GRAY);
    num_str(b, pl.maxhp);
    put_right(&m_side, 50, 80, b, C_WHITE);
    ta_print(&m_side, 0, 61, "MP", C_GRAY);
    num_str(b, pl.maxmp);
    put_right(&m_side, 61, 80, b, C_WHITE);
}

static void menu_draw_system(void) {
    ta_print(&m_body, 14, 1, "Save Game", C_WHITE);
    ta_print(&m_body, 14, 14, music_on ? "Music: On" : "Music: Off", C_WHITE);
    ta_print(&m_body, 14, 27, "Title Screen", C_WHITE);
    ta_print(&m_body, 14, 40, "Back", C_WHITE);
    ta_print(&m_body, 4, 66, "Controls:", C_YELLOW);
    ta_print(&m_body, 4, 77, "A attack/talk   B skill (MP)", C_GRAY);
    ta_print(&m_body, 4, 88, "R roll   L quick potion", C_GRAY);
    ta_print(&m_body, 4, 99, "START menu", C_GRAY);
}

static void menu_rebuild(void) {
    ta_clear(&m_body);
    ta_clear(&m_side);
    menu_tabs_draw();
    switch (menu_tab) {
    case 0: menu_draw_status(); footer("L/R: switch tab   B: close", C_GRAY); break;
    case 1: menu_draw_items();
        if (menu_n) footer(item_defs[menu_list[menu_sel]].desc, C_WHITE);
        else footer("L/R: switch tab", C_GRAY);
        break;
    case 2: menu_draw_equip();
        if (!menu_sub) footer("A: choose slot", C_GRAY);
        else if (menu_n) footer(item_defs[menu_list[menu_sel]].desc, C_WHITE);
        else footer("B: back", C_GRAY);
        break;
    case 3: menu_draw_system(); if (menu_msg_t) footer(menu_msg, C_GREEN); else footer("A: select", C_GRAY); break;
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
    menu_tab = 0;
    menu_sel = 0;
    menu_sub = 0;
    menu_top = 0;
    menu_msg_t = 0;
    menu_quit_title = 0;
    menu_rebuild();
}

int menu_update(void) {
    int redraw = 0;
    if (menu_msg_t && --menu_msg_t == 0) redraw = 1;
    if (keys_down & KEY_R) { menu_tab = (menu_tab + 1) & 3; menu_sel = 0; menu_sub = 0; menu_top = 0; redraw = 1; snd_sfx(SFX_MENU_MOVE); }
    if (keys_down & KEY_L) { menu_tab = (menu_tab + 3) & 3; menu_sel = 0; menu_sub = 0; menu_top = 0; redraw = 1; snd_sfx(SFX_MENU_MOVE); }
    int n = 0;
    if (menu_tab == 1) n = menu_n;
    else if (menu_tab == 2) n = menu_sub ? menu_n : 3;
    else if (menu_tab == 3) n = 4;
    if (n > 0) {
        if (keys_down & KEY_DOWN) { menu_sel = (menu_sel + 1) % n; redraw = 1; snd_sfx(SFX_MENU_MOVE); }
        if (keys_down & KEY_UP) { menu_sel = (menu_sel + n - 1) % n; redraw = 1; snd_sfx(SFX_MENU_MOVE); }
    }
    if (keys_down & KEY_A) {
        if (menu_tab == 1 && menu_n) {
            int it = menu_list[menu_sel];
            if (item_defs[it].type == IT_USE) {
                if (!player_use_item(it)) snd_sfx(SFX_ERROR);
                redraw = 1;
            }
        } else if (menu_tab == 2) {
            if (!menu_sub) {
                menu_slot = menu_sel;
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
        } else if (menu_tab == 3) {
            if (menu_sel == 0) {
                save_write();
                snd_sfx(SFX_SAVE);
                int k = cat_str(menu_msg, 0, "Game saved.");
                (void)k;
                menu_msg_t = 120;
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
        if (menu_tab == 2 && menu_sub) { menu_sub = 0; menu_sel = menu_slot; redraw = 1; snd_sfx(SFX_MENU_BACK); }
        else { snd_sfx(SFX_MENU_BACK); return 1; }
    }
    if (keys_down & KEY_START) { snd_sfx(SFX_MENU_BACK); return 1; }
    if (redraw) menu_rebuild();
    /* cursor sprite */
    if (menu_tab == 1 && menu_n) cursor_at(6, 14 + (menu_sel - menu_top) * 12);
    if (menu_tab == 2) {
        if (!menu_sub) cursor_at(6, 14 + menu_sel * 14);
        else if (menu_n) cursor_at(6, 27 + (menu_sel - menu_top) * 11);
    }
    if (menu_tab == 3) cursor_at(6, 14 + menu_sel * 13);
    if (menu_tab == 0) {
        int f = (frame_count / 6) % 6;
        spr_ui16(206, 118, f * 4, PB_HERO);
    }
    return 0;
}

/* ================================================================== SHOP */
static TextArea s_head, s_list, s_desc;
static int shop_stock[7], shop_n, shop_sel;
static char shop_msg[40];
static int shop_msg_t;

static int shop_price(int item) { return item_defs[item].price; }

static void shop_draw(void) {
    ta_clear(&s_list);
    ta_clear(&s_head);
    ta_print(&s_head, 4, 1, "Tobin's Wares", C_YELLOW);
    char b[16];
    int k = cat_num(b, 0, pl.gold);
    cat_str(b, k, " G");
    put_right(&s_head, 1, 204, b, C_YELLOW);
    for (int r = 0; r < shop_n; r++) {
        int it = shop_stock[r];
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
    ta_clear(&s_desc);
    if (shop_msg_t) ta_print(&s_desc, 4, 4, shop_msg, C_GREEN);
    else {
        ta_print(&s_desc, 4, 1, item_defs[shop_stock[shop_sel]].desc, C_WHITE);
        char c2[24];
        int kk = cat_str(c2, 0, "You have ");
        kk = cat_num(c2, kk, pl.inv[shop_stock[shop_sel]]);
        ta_print(&s_desc, 4, 12, c2, C_GRAY);
    }
}

void shop_open(void) {
    text_pool_reset(UT_POOL);
    bg0_clear();
    ui_panel(0, 0, 30, 20);
    ta_init(&s_head, 1, 1, 28, 1, 13, 8);
    ta_init(&s_list, 2, 3, 26, 11, 13, 8);
    ta_init(&s_desc, 2, 15, 26, 3, 13, 8);
    int n = 0;
    shop_stock[n++] = ITEM_POTION;
    shop_stock[n++] = ITEM_HIPOTION;
    shop_stock[n++] = ITEM_ETHER;
    shop_stock[n++] = ITEM_SWORD1 + pl.cls * 3 + 1;
    shop_stock[n++] = ITEM_ARMOR2;
    shop_stock[n++] = ITEM_CHARM_SWIFT;
    shop_stock[n++] = ITEM_CHARM_LUCK;
    shop_n = n;
    shop_sel = 0;
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
            cat_str(shop_msg, 0, item_defs[it].type == IT_EQUIP ? "Purchased and equipped!" : "Purchased!");
            shop_msg_t = 60;
        }
        redraw = 1;
    }
    if (keys_down & KEY_B) { snd_sfx(SFX_MENU_BACK); return 1; }
    if (redraw) shop_draw();
    cursor_at(14, 22 + shop_sel * 12);
    return 0;
}

/* ================================================================== REST (healer) */
static TextArea r_box;
static int rest_sel, rest_state, rest_t, rest_mark;

static void rest_close(void) {
    for (int y = 7; y < 12; y++)
        for (int x = 9; x < 21; x++) bg0_set(x, y, 0);
    text_pool_reset(rest_mark);
    hud_remap();
}

void rest_open(void) {
    rest_mark = text_pool_mark();
    ui_panel(9, 7, 12, 5);
    ta_init(&r_box, 10, 8, 10, 3, 13, 8);
    ta_print(&r_box, 14, 3, "Rest", C_WHITE);
    ta_print(&r_box, 14, 14, "Leave", C_WHITE);
    rest_sel = 0;
    rest_state = 0;
    rest_t = 0;
}

int rest_update(void) {
    if (rest_state == 0) {
        if (keys_down & (KEY_UP | KEY_DOWN)) { rest_sel ^= 1; snd_sfx(SFX_MENU_MOVE); }
        cursor_at(78, 64 + rest_sel * 11);
        if (keys_down & KEY_B) { snd_sfx(SFX_MENU_BACK); rest_close(); return 1; }
        if (keys_down & KEY_A) {
            snd_sfx(SFX_MENU_OK);
            if (rest_sel == 1) { rest_close(); return 1; }
            rest_state = 1;
            rest_t = 0;
            pl.hp = pl.maxhp;
            pl.mp = pl.maxmp;
            snd_sfx(SFX_POTION);
            player_heal(0, 0);
        }
    } else {
        rest_t++;
        if (rest_t == 2) {
            rest_close();
            toast_show("Sister Lyra's blessing restores you.");
        }
        if (rest_t % 6 == 0) part_spawn(PK_HEAL, (int)(pl.x >> 8) - 8 + rnd_n(16), (int)(pl.y >> 8) - 4, 0, -50, 24, 0);
        if (rest_t > 40) return 1;
    }
    return 0;
}

/* ================================================================== GAME OVER */
static TextArea g_head, g_menu;
static int go_t, go_sel;

void gameover_enter(void) {
    text_pool_reset(UT_POOL);
    bg0_clear();
    ta_init(&g_head, 5, 5, 20, 2, 14, 0);
    ta_init(&g_menu, 10, 11, 10, 4, 14, 0);
    put_centered(&g_head, 3, "You have fallen...", C_RED);
    go_t = 0;
    go_sel = 0;
    gfx_set_fade_mask(0x16);
    snd_stop_music();
    snd_sfx(SFX_GAMEOVER);
}

int gameover_update(void) {
    go_t++;
    if (go_t < 50) gfx_set_fade(go_t * 16 / 50, 0);
    else gfx_set_fade(16, 0);
    if (go_t == 70) {
        ta_clear(&g_menu);
        ta_print(&g_menu, 12, 0, "Continue", C_WHITE);
        ta_print(&g_menu, 12, 12, "Title", C_WHITE);
    }
    if (go_t > 70) {
        if (keys_down & (KEY_UP | KEY_DOWN)) { go_sel ^= 1; snd_sfx(SFX_MENU_MOVE); }
        ta_clear(&g_menu);
        ta_print(&g_menu, 12, 0, "Continue", go_sel == 0 ? C_YELLOW : C_WHITE);
        ta_print(&g_menu, 12, 12, "Title", go_sel == 1 ? C_YELLOW : C_WHITE);
        cursor_at(74, 85 + go_sel * 12);
        if (keys_down & KEY_A) { snd_sfx(SFX_MENU_OK); gfx_set_fade_mask(0x3F); return go_sel + 1; }
    }
    return 0;
}

/* ================================================================== ENDING */
static TextArea e_a, e_b;
static int end_t;

void ending_enter(void) {
    gfx_blank(1);
    text_pool_reset(UT_POOL);
    bg0_clear();
    gfx_clear_world_maps();
    gfx_load_title(0);
    gfx_mode_ui();
    bg2_darken(5);
    ta_init(&e_a, 3, 3, 24, 4, 13, 0);
    ta_init(&e_b, 3, 11, 24, 6, 13, 0);
    gfx_blank(0);
    put_centered(&e_a, 1, "THE END", C_YELLOW);
    put_centered(&e_a, 14, "The Hollow King is no more.", C_WHITE);
    put_centered(&e_a, 24, "Embervale sleeps in peace.", C_WHITE);
    char b[40];
    int k = cat_str(b, 0, class_defs[pl.cls].name);
    k = cat_str(b, k, "  Level ");
    cat_num(b, k, pl.level);
    put_centered(&e_b, 2, b, C_GREEN);
    int secs = (int)(pl.playtime / 60);
    k = cat_str(b, 0, "Time ");
    k = cat_num(b, k, secs / 3600);
    k = cat_str(b, k, ":");
    if (((secs / 60) % 60) < 10) k = cat_str(b, k, "0");
    k = cat_num(b, k, (secs / 60) % 60);
    k = cat_str(b, k, "   Gold ");
    cat_num(b, k, pl.gold);
    put_centered(&e_b, 14, b, C_YELLOW);
    put_centered(&e_b, 32, "Thanks for playing!", C_WHITE);
    end_t = 0;
    snd_play_song(SONG_TITLE);
}

int ending_update(void) {
    end_t++;
    if (end_t < 30) gfx_set_fade(16 - end_t * 16 / 30, 0);
    else gfx_set_fade(0, 0);
    for (int i = 0; i < 6; i++) {
        int f = ((end_t + i * 13) >> 3) & 7;
        f = f > 3 ? 7 - f : f;
        spr_add(20 + i * 38, 60 + (i * 29) % 60, SH_SQUARE, 0, OT_STAR + f, PB_UI, 2, 0, 100);
    }
    if (end_t > 120 && (keys_down & (KEY_START | KEY_A))) return 1;
    return 0;
}
