#include "game.h"
#include "assets.h"
#include "audio_data.h"

#include "ui_util.h"

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

/* ================================================================== REST (healer) */
static TextArea r_box;
static int rest_sel, rest_state, rest_t, rest_mark;

static void rest_close(void) {
    for (int y = 7; y < 12; y++)
        for (int x = 9; x < 21; x++) bg0_set(x, y, 0);
    text_pool_reset(rest_mark);
    hud_remap();
}

int rest_inn;                       /* 1 when the innkeeper is the one tucking you in */

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
            toast_show(rest_inn ? "A warm bed restores you." : "Sister Lyra's blessing restores you.");
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
