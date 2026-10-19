/* craft.c - the blacksmith's forge and the alchemist's bench.
   Forge: make gear from trophies, or TEMPER a piece you own (+1..+3: more attack or defense).
   Bench: brew potions and charms. Everything is paid for with the trophies enemies drop, plus a little gold. */
#include "game.h"
#include "assets.h"
#include "audio_data.h"
#include "ui_util.h"

typedef struct { u8 result, qty, ing[3], n[3]; u16 gold; u8 min_level; } Recipe;

static const Recipe forge_recipes[] = {
    { ITEM_ARMOR_PELT, 1, { ITEM_PELT, ITEM_GEL, 0 }, { 5, 2, 0 }, 60, 3 },
    { ITEM_HELM_BONE, 1, { ITEM_BONE, ITEM_IRON, 0 }, { 6, 2, 0 }, 120, 6 },
    { ITEM_BOOTS_AMBER, 1, { ITEM_AMBER, ITEM_PELT, 0 }, { 4, 3, 0 }, 150, 6 },
};
static const Recipe brew_recipes[] = {
    { ITEM_POTION, 3, { ITEM_GEL, 0, 0 }, { 2, 0, 0 }, 5, 1 },
    { ITEM_HIPOTION, 1, { ITEM_GEL, ITEM_WING, 0 }, { 2, 2, 0 }, 15, 1 },
    { ITEM_ETHER, 2, { ITEM_WING, ITEM_GEL, 0 }, { 2, 1, 0 }, 10, 1 },
    { ITEM_ELIXIR, 1, { ITEM_WING, ITEM_BONE, ITEM_EMBER }, { 3, 3, 2 }, 60, 8 },
    { ITEM_CHARM_AMBER, 1, { ITEM_AMBER, ITEM_WING, 0 }, { 5, 2, 0 }, 120, 6 },
};
/* tempering a piece from level L-1 to L costs temper_cost[L-1] */
static const Recipe temper_cost[TEMPER_MAX] = {
    { 0, 0, { ITEM_IRON, ITEM_PELT, 0 }, { 2, 1, 0 }, 50, 1 },
    { 0, 0, { ITEM_IRON, ITEM_BONE, 0 }, { 4, 2, 0 }, 120, 1 },
    { 0, 0, { ITEM_IRON, ITEM_CINDER_HEART, 0 }, { 6, 1, 0 }, 300, 1 },
};

#ifndef LINE_BUF
#define LINE_BUF 48
#endif
#define CR_ROWS 7
static TextArea c_head, c_list, c_desc;
static int c_kind, c_mode, c_sel, c_top, c_n, c_msg_t;
static char c_msg[40];
static u8 c_items[40];                         /* temper mode: the item ids on offer */

static const Recipe *recipes(int *count) {
    if (c_kind == 0) { *count = sizeof(forge_recipes) / sizeof(forge_recipes[0]); return forge_recipes; }
    *count = sizeof(brew_recipes) / sizeof(brew_recipes[0]);
    return brew_recipes;
}

static void temper_build(void) {
    c_n = 0;
    for (int it = 1; it < ITEM_COUNT && c_n < 40; it++) {
        const ItemDef *d = &item_defs[it];
        if (!item_can_temper(it) || pl.temper[it] >= TEMPER_MAX) continue;
        if (d->slot == SLOT_WEAPON && d->cls != pl.cls) continue;
        c_items[c_n++] = (u8)it;
    }
}

static int has_all(const Recipe *r, int gold) {
    for (int i = 0; i < 3; i++)
        if (r->ing[i] && pl.inv[r->ing[i]] < r->n[i]) return 0;
    return pl.gold >= gold;
}

static int rec_result(int i) {
    int n;
    return recipes(&n)[i].result;
}

static void say(const char *s, int frames) {
    cat_str(c_msg, 0, s);
    c_msg_t = frames;
}

/* "Pelt 5/5" tokens: green when you have enough, red when short */
static int print_needs(TextArea *t, int y, const Recipe *r) {
    char b[24];
    int x = 4;
    for (int i = 0; i < 3; i++) {
        if (!r->ing[i]) continue;
        int have = pl.inv[r->ing[i]];
        int k = cat_str(b, 0, item_defs[r->ing[i]].name);
        k = cat_str(b, k, " ");
        k = cat_num(b, k, have > 99 ? 99 : have);
        k = cat_str(b, k, "/");
        cat_num(b, k, r->n[i]);
        x = ta_print(t, x, y, b, have >= r->n[i] ? C_GREEN : C_RED) + 8;
    }
    return x;
}

static void stat_text(char *b, int it, int temper_extra) {
    const ItemDef *d = &item_defs[it];
    int k = 0;
    int atk = d->atk + item_temper_atk(it), def = d->def + item_temper_def(it), hp = d->hp + item_temper_hp(it);
    if (temper_extra) {
        int t = pl.temper[it] + 1;
        atk = d->atk + (d->slot == SLOT_WEAPON ? t * imax(1, d->atk / 7) : 0);
        def = d->def + ((d->slot != SLOT_WEAPON && d->slot != SLOT_CHARM) ? t * imax(1, d->def / 6) : 0);
        hp = d->hp + ((d->slot == SLOT_ARMOR || d->slot == SLOT_HELM) ? t * 3 : 0);
    }
    b[0] = 0;
    if (atk) { k = cat_str(b, k, "ATK "); k = cat_num(b, k, atk); k = cat_str(b, k, " "); }
    if (def) { k = cat_str(b, k, "DEF "); k = cat_num(b, k, def); k = cat_str(b, k, " "); }
    if (d->spd) { k = cat_str(b, k, "SPD "); k = cat_num(b, k, d->spd); k = cat_str(b, k, " "); }
    if (d->lck) { k = cat_str(b, k, "LCK "); k = cat_num(b, k, d->lck); k = cat_str(b, k, " "); }
    if (d->mag) { k = cat_str(b, k, "MAG "); k = cat_num(b, k, d->mag); k = cat_str(b, k, " "); }
    if (hp) { k = cat_str(b, k, "HP "); k = cat_num(b, k, hp); k = cat_str(b, k, " "); }
    if (d->mp) { k = cat_str(b, k, "MP "); k = cat_num(b, k, d->mp); k = cat_str(b, k, " "); }
    if (d->crit) { k = cat_str(b, k, "CRIT "); k = cat_num(b, k, d->crit); }
}

static void craft_draw(void) {
    char b[LINE_BUF];
    ta_clear(&c_head);
    ta_clear(&c_list);
    ta_clear(&c_desc);
    int ncount = 0;
    const Recipe *rec = recipes(&ncount);
    int n = c_mode ? c_n : ncount;
    ta_print(&c_head, 4, 1, c_kind ? "Brew" : "Forge", c_mode == 0 ? C_YELLOW : C_GRAY);
    if (c_kind == 0) {
        ta_print(&c_head, 44, 1, "Temper", c_mode == 1 ? C_YELLOW : C_GRAY);
        ta_print(&c_head, 100, 1, "L/R: switch", C_GRAY);
    }
    int k = cat_num(b, 0, pl.gold);
    cat_str(b, k, " G");
    put_right(&c_head, 1, 204, b, C_YELLOW);
    if (c_sel < c_top) c_top = c_sel;
    if (c_sel >= c_top + CR_ROWS) c_top = c_sel - CR_ROWS + 1;
    if (!n) ta_print(&c_list, 14, 12, c_mode ? "Nothing left to temper." : "No recipes.", C_GRAY);
    for (int r = 0; r < CR_ROWS && c_top + r < n; r++) {
        int y = 1 + r * 12;
        int i = c_top + r;
        if (!c_mode) {
            const Recipe *q = &rec[i];
            int ok = has_all(q, q->gold) && pl.level >= q->min_level;
            int owned = item_defs[q->result].type == IT_EQUIP && pl.inv[q->result] > 0;
            ta_print(&c_list, 14, y, item_defs[q->result].name, owned ? C_GRAY : (ok ? C_WHITE : C_RED));
            if (owned) put_right(&c_list, y, 200, "Owned", C_GRAY);
            else if (q->qty > 1) { k = cat_str(b, 0, "x"); cat_num(b, k, q->qty); put_right(&c_list, y, 200, b, C_GRAY); }
        } else {
            int it = c_items[i];
            item_label(b, it);
            ta_print(&c_list, 14, y, b, C_WHITE);
            k = cat_str(b, 0, "to +");
            cat_num(b, k, pl.temper[it] + 1);
            put_right(&c_list, y, 200, b, C_YELLOW);
        }
    }
    if (c_top > 0) ta_print(&c_list, 204, 1, "^", C_GRAY);
    if (c_top + CR_ROWS < n) ta_print(&c_list, 204, 73, "v", C_GRAY);
    if (c_msg_t) { ta_print(&c_desc, 4, 4, c_msg, C_GREEN); return; }
    if (!n) return;
    const Recipe *q;
    int it;
    if (!c_mode) { q = &rec[c_sel]; it = q->result; }
    else { it = c_items[c_sel]; q = &temper_cost[pl.temper[it]]; }
    print_needs(&c_desc, 1, q);
    if (!c_mode && item_defs[it].type != IT_EQUIP) {
        ta_print(&c_desc, 4, 9, item_defs[it].desc, C_GRAY);
    } else {
        stat_text(b, it, c_mode);
        ta_print(&c_desc, 4, 9, b, C_ORANGE);
    }
    if (!c_mode && pl.level < q->min_level) {
        k = cat_str(b, 0, "Needs level ");
        cat_num(b, k, q->min_level);
        ta_print(&c_desc, 4, 17, b, C_RED);
    } else {
        k = cat_str(b, 0, "Cost ");
        k = cat_num(b, k, q->gold);
        k = cat_str(b, k, " G   A: ");
        cat_str(b, k, c_mode ? "temper" : (c_kind ? "brew" : "forge"));
        ta_print(&c_desc, 4, 17, b, pl.gold >= q->gold ? C_WHITE : C_RED);
    }
}

void craft_open(int kind) {
    text_pool_reset(UT_POOL);
    bg0_clear();
    ui_panel(0, 0, 30, 20);
    ta_init(&c_head, 1, 1, 28, 1, 13, 8);
    ta_init(&c_list, 2, 3, 26, 11, 13, 8);
    ta_init(&c_desc, 2, 15, 26, 3, 13, 8);
    c_kind = kind;
    c_mode = 0;
    c_sel = 0;
    c_top = 0;
    c_msg_t = 0;
    snd_sfx(SFX_MENU_OK);
    craft_draw();
}

static void spend(const Recipe *q) {
    for (int i = 0; i < 3; i++)
        if (q->ing[i]) pl.inv[q->ing[i]] = (u8)(pl.inv[q->ing[i]] - q->n[i]);
    pl.gold = (u16)(pl.gold - q->gold);
}

static void craft_press(void) {
    int ncount = 0;
    const Recipe *rec = recipes(&ncount);
    int n = c_mode ? c_n : ncount;
    if (!n) return;
    if (!c_mode) {
        const Recipe *q = &rec[c_sel];
        const ItemDef *d = &item_defs[q->result];
        if (d->type == IT_EQUIP && pl.inv[q->result] > 0) { snd_sfx(SFX_ERROR); say("You already own that.", 60); return; }
        if (pl.level < q->min_level) { snd_sfx(SFX_ERROR); say("Level too low.", 60); return; }
        if (!has_all(q, q->gold)) { snd_sfx(SFX_ERROR); say("Missing materials or gold.", 70); return; }
        spend(q);
        player_give(q->result, q->qty);
        snd_sfx(SFX_VICTORY);
        say(c_kind ? "Brewed!" : "Forged! Equip it in the menu.", 80);
    } else {
        int it = c_items[c_sel];
        const Recipe *q = &temper_cost[pl.temper[it]];
        if (!has_all(q, q->gold)) { snd_sfx(SFX_ERROR); say("Missing materials or gold.", 70); return; }
        spend(q);
        pl.temper[it]++;
        player_recalc();
        snd_sfx(SFX_LEVELUP);
        say("Tempered!", 80);
        temper_build();
        if (c_sel >= c_n) c_sel = c_n ? c_n - 1 : 0;
    }
    quest_refresh();
}

int craft_update(void) {
    int redraw = 0;
    if (c_msg_t && --c_msg_t == 0) redraw = 1;
    if (c_kind == 0 && (keys_down & (KEY_L | KEY_R | KEY_LEFT | KEY_RIGHT))) {
        c_mode ^= 1;
        c_sel = 0;
        c_top = 0;
        c_msg_t = 0;
        if (c_mode) temper_build();
        snd_sfx(SFX_MENU_MOVE);
        redraw = 1;
    }
    int ncount = 0;
    recipes(&ncount);
    int n = c_mode ? c_n : ncount;
    if (n && (keys_down & KEY_DOWN)) { c_sel = (c_sel + 1) % n; redraw = 1; snd_sfx(SFX_MENU_MOVE); }
    if (n && (keys_down & KEY_UP)) { c_sel = (c_sel + n - 1) % n; redraw = 1; snd_sfx(SFX_MENU_MOVE); }
    if (keys_down & KEY_A) { craft_press(); redraw = 1; }
    if (keys_down & KEY_B) { snd_sfx(SFX_MENU_BACK); return 1; }
    if (redraw) craft_draw();
    n = c_mode ? c_n : ncount;
    for (int r = 0; r < CR_ROWS && c_top + r < n; r++) {            /* an icon beside every row */
        int it = c_mode ? c_items[c_top + r] : rec_result(c_top + r);
        spr_ui8(22, 25 + r * 12, icon_tile(item_icon(it)), PB_ICON);
    }
    if (n > 0) cursor_at(14, 22 + (c_sel - c_top) * 12);
    return 0;
}
