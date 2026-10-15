#include "ui.h"
#include "config.h"
#include "craft.h"
#include "game.h"
#include "gfx.h"
#include "hw.h"
#include "inventory.h"
#include "items.h"
#include "palette.h"
#include "player.h"
#include "sfx.h"

/* ------------------------------------------------------------------ */
/* HUD                                                                 */
/* ------------------------------------------------------------------ */

static const u8 HEART[6] = { 0x6C, 0xFE, 0xFE, 0x7C, 0x38, 0x10 };
static const u8 MEAT[6]  = { 0x70, 0xF8, 0xF8, 0x78, 0x1C, 0x06 };

#define HOTBAR_X 56

static void draw_slot_count(int x, int y, const Slot *s)
{
    if (s->count > 1) {
        int digits = s->count >= 10 ? 2 : 1;
        gfx_number(x + 16 - digits * 4, y + 10, s->count, C_WHITE, 1);
    }
    int max = item_max_dur(s->id);
    if (max > 0) {
        int w = 14 * s->dur / max;
        u8 col = s->dur * 3 > max * 2 ? C_GREEN : (s->dur * 3 > max ? C_YELLOW : C_RED);
        gfx_rect(x + 1, y + 15, 14, 1, C_BLACK);
        gfx_rect(x + 1, y + 15, w > 0 ? w : 1, 1, col);
    }
}

/* The three HUD bars are painted into small off-screen panels only when their
 * contents change, then copied to the screen every frame (much cheaper than
 * redrawing every heart and icon at 60 fps). Panel x and w must be even. */
typedef struct {
    u16 *buf;          /* 240 pixels wide, h rows; drawn with screen x, local y */
    int x, y, w, h;
    u32 key;
} Panel;

static u16 s_buf_hp[10 * 120] EWRAM_BSS ALIGN4;
static u16 s_buf_food[10 * 120] EWRAM_BSS ALIGN4;
static u16 s_buf_bar[22 * 120] EWRAM_BSS ALIGN4;

static Panel s_hp   = { s_buf_hp,   2,   2,  82, 10, 0xFFFFFFFFu };
static Panel s_food = { s_buf_food, 156, 2,  82, 10, 0xFFFFFFFFu };
static Panel s_bar  = { s_buf_bar,  54,  138, 132, 22, 0xFFFFFFFFu };

static void paint_hearts(void)
{
    gfx_rect(0, 0, 240, 10, C_BLACK);
    for (int i = 0; i < 10; i++) {
        int x = 4 + i * 8;
        gfx_mask7(x, 2, HEART, 7, C_DRED);
        if (g_pl.hp >= 2 * (i + 1)) gfx_mask7(x, 2, HEART, 7, C_RED);
        else if (g_pl.hp == 2 * i + 1) gfx_mask7(x, 2, HEART, 4, C_RED);
    }
}

static void paint_food(void)
{
    gfx_rect(0, 0, 240, 10, C_BLACK);
    for (int i = 0; i < 10; i++) {
        int x = 230 - i * 8;
        gfx_mask7(x, 2, MEAT, 7, C_DGRAY);
        if (g_pl.food >= 2 * (i + 1)) gfx_mask7(x, 2, MEAT, 7, C_ORANGE);
        else if (g_pl.food == 2 * i + 1) gfx_mask7(x, 2, MEAT, 4, C_ORANGE);
    }
}

static void paint_bar(void)
{
    gfx_rect(0, 0, 240, 22, C_BLACK);
    gfx_rect(HOTBAR_X, 2, 128, 19, C_DGRAY);
    for (int i = 0; i < HOTBAR_SLOTS; i++) {
        int x = HOTBAR_X + i * 16;
        const Slot *s = &g_inv[i];
        gfx_item(x, 3, s->id);
        if (s->id) draw_slot_count(x, 3, s);
    }
    gfx_frame(HOTBAR_X + g_pl.sel * 16 - 1, 2, 18, 18, C_YELLOW);
}

static u32 bar_key(void)
{
    u32 k = g_pl.sel;
    for (int i = 0; i < HOTBAR_SLOTS; i++)
        k = k * 131u + g_inv[i].id + g_inv[i].count * 257u + g_inv[i].dur * 65537u;
    return k;
}

static void refresh_panel(Panel *p, u32 key, void (*paint)(void))
{
    if (p->key == key) return;
    p->key = key;
    gfx_begin(p->buf);
    paint();
}

static void blit_panel(u16 *page, const Panel *p)
{
    for (int r = 0; r < p->h; r++)
        hw_copy16(page + (p->y + r) * (SCREEN_W / 2) + p->x / 2,
                  p->buf + r * (SCREEN_W / 2) + p->x / 2, (u32)(p->w / 2));
}

void ui_hud(void)
{
    u16 *page = gfx_target();
    refresh_panel(&s_hp, (u32)g_pl.hp, paint_hearts);
    refresh_panel(&s_food, (u32)g_pl.food, paint_food);
    refresh_panel(&s_bar, bar_key(), paint_bar);
    gfx_begin(page);

    blit_panel(page, &s_hp);
    blit_panel(page, &s_food);
    blit_panel(page, &s_bar);

    /* selected item name */
    if (g_game.sel_t && g_inv[g_pl.sel].id)
        gfx_text_center(120, 130, item_name(g_inv[g_pl.sel].id), C_WHITE, 1);

    /* toast message */
    if (g_game.toast_t) {
        int w = gfx_text_w(g_game.toast, 1);
        gfx_rect(120 - w / 2 - 4, 15, w + 8, 11, C_BLACK);
        gfx_text(120 - w / 2, 18, g_game.toast, C_YELLOW, 1);
    }
}

/* ------------------------------------------------------------------ */
/* Inventory / crafting / goals                                        */
/* ------------------------------------------------------------------ */

enum { TAB_INV, TAB_CRAFT, TAB_GOALS, TAB_COUNT };

static int s_tab, s_cur, s_carry, s_csel, s_cscroll, s_rep;
static const char *s_msg = "";

static u16 nav_keys(u16 held, u16 pressed)
{
    const u16 dirs = KEY_UP | KEY_DOWN | KEY_LEFT | KEY_RIGHT;
    if (pressed & dirs) { s_rep = 0; return (u16)(pressed & dirs); }
    if (held & dirs) {
        s_rep++;
        if (s_rep > 14 && (s_rep & 3) == 0) return (u16)(held & dirs);
    } else {
        s_rep = 0;
    }
    return 0;
}

void ui_menu_open(void)
{
    s_tab = TAB_INV;
    s_cur = g_pl.sel;
    s_carry = -1;
    s_msg = "";
    s_rep = 0;
}

static void craft_press(int near_mask)
{
    const Recipe *r = &g_recipes[s_csel];
    if (!craft_station_ok(r, near_mask)) {
        s_msg = r->station == ST_TABLE ? "STAND NEAR A CRAFTING TABLE" : "STAND NEAR A FURNACE";
        sfx_play(SFX_HURT);
    } else if (!craft_has_items(r)) {
        s_msg = "MISSING INGREDIENTS";
        sfx_play(SFX_HURT);
    } else if (!craft_make(r, near_mask)) {
        s_msg = "INVENTORY FULL";
        sfx_play(SFX_HURT);
    } else {
        s_msg = "CRAFTED!";
        sfx_play(SFX_CRAFT);
    }
}

bool ui_menu_update(u16 held, u16 pressed, int near_mask)
{
    if (pressed & KEY_SELECT) return true;
    if (pressed & KEY_B) {
        if (s_tab == TAB_INV && s_carry >= 0) s_carry = -1;
        else return true;
    }
    if (pressed & KEY_R) { s_tab = (s_tab + 1) % TAB_COUNT; s_msg = ""; sfx_play(SFX_MENU); }
    if (pressed & KEY_L) { s_tab = (s_tab + TAB_COUNT - 1) % TAB_COUNT; s_msg = ""; sfx_play(SFX_MENU); }

    u16 nav = nav_keys(held, pressed);
    if (s_tab == TAB_INV) {
        int col = s_cur % 8, row = s_cur / 8;
        if (nav & KEY_LEFT)  col = (col + 7) % 8;
        if (nav & KEY_RIGHT) col = (col + 1) % 8;
        if (nav & KEY_UP)    row = (row + 2) % 3;
        if (nav & KEY_DOWN)  row = (row + 1) % 3;
        s_cur = row * 8 + col;
        if (pressed & KEY_A) {
            if (s_carry < 0) {
                if (g_inv[s_cur].id) s_carry = s_cur;
            } else {
                inv_move(s_carry, s_cur);
                s_carry = -1;
            }
            sfx_play(SFX_MENU);
        }
    } else if (s_tab == TAB_CRAFT) {
        if (nav & KEY_UP)   s_csel = (s_csel + g_recipe_count - 1) % g_recipe_count;
        if (nav & KEY_DOWN) s_csel = (s_csel + 1) % g_recipe_count;
        if (nav & (KEY_UP | KEY_DOWN)) s_msg = "";
        if (s_csel < s_cscroll) s_cscroll = s_csel;
        if (s_csel >= s_cscroll + 6) s_cscroll = s_csel - 5;
        if (pressed & KEY_A) craft_press(near_mask);
    }
    return false;
}

static void draw_tabs(void)
{
    static const char *const names[TAB_COUNT] = { "INVENTORY", "CRAFTING", "GOALS" };
    static const int xs[TAB_COUNT] = { 10, 92, 170 };
    for (int i = 0; i < TAB_COUNT; i++) {
        bool on = i == s_tab;
        gfx_text(xs[i], 3, names[i], on ? C_YELLOW : C_GRAY, 2);
        if (on) gfx_rect(xs[i], 14, gfx_text_w(names[i], 2), 1, C_YELLOW);
    }
    gfx_rect(0, 16, SCREEN_W, 1, C_GRAY);
}

static void draw_inventory_tab(void)
{
    for (int i = 0; i < INV_SLOTS; i++) {
        int x = 24 + (i % 8) * 24, y = 22 + (i / 8) * 20;
        gfx_rect(x + 1, y, 22, 19, i < HOTBAR_SLOTS ? C_GRAY : C_BLACK);
        gfx_rect(x + 2, y + 1, 20, 17, C_DGRAY);
        if (g_inv[i].id) {
            gfx_item(x + 4, y + 2, g_inv[i].id);
            draw_slot_count(x + 4, y + 2, &g_inv[i]);
        }
        if (i == s_carry) gfx_frame(x + 1, y, 22, 19, C_CYAN);
        if (i == s_cur) gfx_frame(x, y - 1, 24, 21, C_YELLOW);
    }
    gfx_text(24, 84, "TOP ROW = HOTBAR", C_GRAY, 1);

    const Slot *s = &g_inv[s_cur];
    if (s->id) {
        gfx_text(24, 96, item_name(s->id), C_WHITE, 2);
        if (item_max_dur(s->id)) {
            gfx_text(24, 112, "DURABILITY", C_GRAY, 1);
            gfx_number(70, 112, s->dur, C_WHITE, 1);
        } else {
            gfx_text(24, 112, "AMOUNT", C_GRAY, 1);
            gfx_number(52, 112, s->count, C_WHITE, 1);
        }
    } else {
        gfx_text(24, 100, "EMPTY SLOT", C_GRAY, 1);
    }
    if (s_carry >= 0) gfx_text(24, 128, "A: DROP ITEM IN THIS SLOT", C_CYAN, 1);
    else gfx_text(24, 128, "A: PICK UP / SWAP ITEMS", C_LGRAY, 1);
}

static void draw_craft_tab(int near_mask)
{
    for (int i = 0; i < 6; i++) {
        int idx = s_cscroll + i;
        if (idx >= g_recipe_count) break;
        const Recipe *r = &g_recipes[idx];
        int y = 20 + i * 18;
        bool ok = craft_station_ok(r, near_mask) && craft_has_items(r);
        if (idx == s_csel) gfx_rect(2, y, 138, 17, C_BLUE);
        gfx_item(4, y + 1, r->out);
        const char *name = item_name(r->out);
        gfx_text(24, y + 6, name, ok ? C_WHITE : C_GRAY, 1);
        if (r->out_n > 1) {
            gfx_text(24 + gfx_text_w(name, 1) + 4, y + 6, "X", ok ? C_YELLOW : C_GRAY, 1);
            gfx_number(24 + gfx_text_w(name, 1) + 9, y + 6, r->out_n, ok ? C_YELLOW : C_GRAY, 1);
        }
    }
    /* scroll position */
    gfx_rect(139, 20, 2, 107, C_DGRAY);
    gfx_rect(139, 20 + s_cscroll * 107 / g_recipe_count, 2, 107 * 6 / g_recipe_count, C_LGRAY);

    /* ingredients */
    const Recipe *r = &g_recipes[s_csel];
    gfx_text(146, 22, "NEEDS", C_YELLOW, 1);
    for (int i = 0; i < 3; i++) {
        if (!r->in_id[i]) continue;
        int have = inv_count(r->in_id[i]);
        bool enough = have >= r->in_n[i];
        int y = 32 + i * 16;
        gfx_text(146, y, item_name(r->in_id[i]), enough ? C_GREEN : C_RED, 1);
        int nx = 146;
        gfx_number(nx, y + 7, have, enough ? C_LGRAY : C_RED, 1);
        nx += (have >= 10 ? 8 : 4);
        gfx_text(nx, y + 7, "/", C_GRAY, 1);
        gfx_number(nx + 4, y + 7, r->in_n[i], C_LGRAY, 1);
    }
    gfx_text(146, 86, "AT", C_YELLOW, 1);
    bool st_ok = craft_station_ok(r, near_mask);
    const char *st = r->station == ST_HAND ? "BY HAND" : (r->station == ST_TABLE ? "CRAFT TABLE" : "FURNACE");
    gfx_text(146, 94, st, st_ok ? C_GREEN : C_RED, 1);
}

static void draw_goals_tab(void)
{
    int done = 0;
    for (int i = 0; i < ADV_COUNT; i++) {
        bool has = game_has(i);
        if (has) done++;
        int y = 20 + i * 8;
        gfx_text(6, y, has ? "[X]" : "[ ]", has ? C_GREEN : C_GRAY, 1);
        gfx_text(24, y, g_adv_name[i], has ? C_GREEN : C_WHITE, 1);
        gfx_text(124, y, g_adv_hint[i], C_GRAY, 1);
    }
    int sy = 20 + ADV_COUNT * 8 + 3;
    gfx_text(6, sy, "COMPLETE", C_YELLOW, 1);
    gfx_number(42, sy, done, C_YELLOW, 1);
    gfx_text(done >= 10 ? 54 : 50, sy, "/", C_YELLOW, 1);
    gfx_number(done >= 10 ? 58 : 54, sy, ADV_COUNT, C_YELLOW, 1);
}

void ui_menu_draw(int near_mask)
{
    gfx_clear(C_BLACK);
    gfx_rect(0, 0, SCREEN_W, 16, C_DGRAY);
    draw_tabs();
    if (s_tab == TAB_INV) draw_inventory_tab();
    else if (s_tab == TAB_CRAFT) draw_craft_tab(near_mask);
    else draw_goals_tab();

    gfx_rect(0, 142, SCREEN_W, 18, C_DGRAY);
    gfx_text(4, 146, s_msg[0] ? s_msg : "L R: TABS   SELECT OR B: CLOSE", s_msg[0] ? C_YELLOW : C_LGRAY, 1);
    if (s_tab == TAB_CRAFT) gfx_text(4, 153, "A: CRAFT   UP DOWN: CHOOSE", C_GRAY, 1);
}

/* ------------------------------------------------------------------ */
/* Pause menu                                                          */
/* ------------------------------------------------------------------ */

static int s_pause_sel;
static bool s_help;

static const char *const HELP_LINES[] = {
    "D-PAD: WALK, TAP TO TURN",
    "(UP = UP-RIGHT ON SCREEN)",
    "A: MINE / ATTACK (HOLD)",
    "B: PLACE BLOCK / EAT",
    "HOLD B: 2 BLOCKS HIGH",
    "L R: CHANGE HOTBAR SLOT",
    "SELECT: INVENTORY + CRAFT",
    "START: PAUSE / SAVE",
    "CHOP TREES, CRAFT A TABLE,",
    "MAKE TOOLS, MINE ORES AND",
    "SMELT THEM IN A FURNACE.",
    "ZOMBIES COME OUT AT NIGHT:",
    "2-HIGH WALLS + A DOOR.",
    "LIGHT THE EMBER BEACON!",
};
#define HELP_CONTROL_LINES 8

void ui_pause_open(void)
{
    s_pause_sel = 0;
    s_help = false;
}

PauseAction ui_pause_update(u16 pressed)
{
    if (s_help) {
        if (pressed & (KEY_B | KEY_A | KEY_START)) s_help = false;
        return PAUSE_NONE;
    }
    if (pressed & KEY_START) return PAUSE_RESUME;
    if (pressed & KEY_UP)   { s_pause_sel = (s_pause_sel + 3) % 4; sfx_play(SFX_MENU); }
    if (pressed & KEY_DOWN) { s_pause_sel = (s_pause_sel + 1) % 4; sfx_play(SFX_MENU); }
    if (pressed & KEY_B) return PAUSE_RESUME;
    if (pressed & KEY_A) {
        switch (s_pause_sel) {
        case 0: return PAUSE_RESUME;
        case 1: return PAUSE_SAVE;
        case 2: s_help = true; return PAUSE_NONE;
        default: return PAUSE_QUIT;
        }
    }
    return PAUSE_NONE;
}

void ui_pause_draw(void)
{
    static const char *const items[4] = { "RESUME", "SAVE GAME", "HOW TO PLAY", "QUIT TO TITLE" };
    if (s_help) {
        gfx_rect(14, 12, 212, 136, C_BLACK);
        gfx_frame(14, 12, 212, 136, C_LGRAY);
        gfx_text_center(120, 18, "HOW TO PLAY", C_YELLOW, 2);
        for (int i = 0; i < (int)ARRAY_LEN(HELP_LINES); i++)
            gfx_text(24, 31 + i * 7, HELP_LINES[i], i < HELP_CONTROL_LINES ? C_WHITE : C_LGRAY, 1);
        gfx_text_center(120, 138, "PRESS A OR B", C_GRAY, 1);
        return;
    }
    gfx_rect(60, 34, 120, 92, C_BLACK);
    gfx_frame(60, 34, 120, 92, C_LGRAY);
    gfx_text_center(120, 40, "PAUSED", C_YELLOW, 2);
    for (int i = 0; i < 4; i++) {
        bool on = i == s_pause_sel;
        gfx_text(80, 62 + i * 14, on ? ">" : " ", C_YELLOW, 1);
        gfx_text(90, 62 + i * 14, items[i], on ? C_YELLOW : C_WHITE, 1);
    }
}

/* ------------------------------------------------------------------ */
/* Title and death screens                                             */
/* ------------------------------------------------------------------ */

static int s_title_sel;
static bool s_title_save;

void ui_title_open(bool has_save)
{
    s_title_save = has_save;
    s_title_sel = has_save ? 1 : 0;
}

TitleAction ui_title_update(u16 pressed)
{
    if (s_title_save) {
        if (pressed & (KEY_UP | KEY_DOWN)) { s_title_sel ^= 1; sfx_play(SFX_MENU); }
    } else {
        s_title_sel = 0;
    }
    if (pressed & (KEY_A | KEY_START)) {
        sfx_play(SFX_ADV);
        return s_title_sel == 1 ? TITLE_CONTINUE : TITLE_NEW;
    }
    return TITLE_NONE;
}

void ui_title_draw(void)
{
    const char *logo = GAME_NAME;
    int scale = 4;
    while (scale > 1 && gfx_text_w(logo, scale) > SCREEN_W - 16) scale--; /* long names shrink to fit */
    int w = gfx_text_w(logo, scale);
    int ly = 15 + (4 - scale) * 3;
    gfx_text(120 - w / 2 + 3, ly + 3, logo, C_BLACK, scale);
    gfx_text(120 - w / 2 + 1, ly + 1, logo, C_DRED, scale);
    gfx_text(120 - w / 2, ly, logo, C_ORANGE, scale);
    gfx_text_center(120, 40, "A BLOCKY ISOMETRIC ADVENTURE", C_WHITE, 1);

    gfx_rect(76, 100, 88, 36, C_BLACK);
    gfx_frame(76, 100, 88, 36, C_LGRAY);
    gfx_text(88, 108, s_title_sel == 0 ? ">" : " ", C_YELLOW, 1);
    gfx_text(96, 108, "NEW GAME", s_title_sel == 0 ? C_YELLOW : C_WHITE, 1);
    gfx_text(88, 122, s_title_sel == 1 ? ">" : " ", C_YELLOW, 1);
    gfx_text(96, 122, "CONTINUE", s_title_save ? (s_title_sel == 1 ? C_YELLOW : C_WHITE) : C_DGRAY, 1);
    gfx_text_center(120, 148, "PRESS A OR START", C_LGRAY, 1);
}

void ui_dead_draw(void)
{
    gfx_rect(50, 52, 140, 52, C_BLACK);
    gfx_frame(50, 52, 140, 52, C_RED);
    gfx_text_center(120, 60, "YOU DIED", C_RED, 2);
    gfx_text_center(120, 84, "PRESS A TO RESPAWN", C_WHITE, 1);
}
