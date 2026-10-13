/* stats.c - classes, derived stats and stat-point allocation.
   stats_calc() is a pure function (class, level, invested points, equipment -> Stats) so the menu can
   preview "what if" changes without touching the live player. */
#include "game.h"
#include "assets.h"

const char *const stat_names[STAT_COUNT] = { "STR", "DEX", "VIT", "INT", "LUK" };

static const char *const stat_text[STAT_COUNT] = {
    "STR: attack power and knockback.",
    "DEX: attack/move speed, crit chance.",
    "VIT: max HP and defense.",
    "INT: spell power (Mage ATK) and MP.",
    "LUK: crit chance and item drops.",
};
const char *stat_blurb(int stat) { return stat_text[stat]; }

/* ------------------------------------------------------------------ classes */
const ClassDef class_defs[CLS_COUNT] = {
    { "Knight", "Sturdy fighter. 4-hit sword chain, Whirlwind skill.", 42, 8, 8, 3, 6, 5, 4, 60, 14, 16, 4, 10, 3, 4 },
    { "Mage", "Frail but deadly. Bolt chain, Fireball skill.", 28, 26, 3, 11, 3, 5, 5, 34, 36, 5, 18, 6, 4, 5 },
    { "Ranger", "Swift archer. Arrow volleys, Multishot skill.", 34, 14, 7, 4, 4, 8, 6, 44, 20, 12, 6, 7, 6, 6 },
    { "Rogue", "Quick striker. Stab chain, Shadow Step skill.", 32, 14, 7, 4, 3, 9, 10, 40, 20, 13, 6, 6, 7, 9 },
};

/* ------------------------------------------------------------------ xp */
/* The curve is steep on purpose: levels 10+ have to be earned in the crypt. */
int xp_for_level(int l) { return 14 + 8 * l + 3 * l * l; }

/* ------------------------------------------------------------------ derived stats */
void stats_calc(int cls, int level, const u8 *pts, const u8 *equip, Stats *o) {
    const ClassDef *c = &class_defs[cls];
    int L = level - 1;
    int batk = 0, bdef = 0, bspd = 0, blck = 0, bmag = 0, bhp = 0, bmp = 0, bcrit = 0;
    for (int s = 0; s < SLOT_COUNT; s++) {
        const ItemDef *it = &item_defs[equip[s]];
        batk += it->atk; bdef += it->def; bspd += it->spd; blck += it->lck;
        bmag += it->mag; bhp += it->hp; bmp += it->mp; bcrit += it->crit;
    }
    /* class growth is automatic but gentle; the rest of a level's power comes from invested points */
    int hp = c->hp + ((c->ghp * L * 5) >> 7);
    int mp = c->mp + ((c->gmp * L * 5) >> 7);
    o->str = (s16)(c->str + ((c->gstr * L) >> 4) + pts[STAT_STR]);
    o->spd = (s16)(c->spd + ((c->gspd * L) >> 4) + bspd + pts[STAT_DEX]);
    o->vit = (s16)(c->def + ((c->gdef * L) >> 4) + pts[STAT_VIT]);
    o->mag = (s16)(c->mag + ((c->gmag * L) >> 4) + bmag + pts[STAT_INT]);
    o->lck = (s16)(c->lck + ((c->glck * L) >> 4) + blck + pts[STAT_LUK]);
    o->maxhp = (s16)(hp + bhp + pts[STAT_VIT] * 6);
    o->maxmp = (s16)(mp + bmp + pts[STAT_INT] * 2);
    o->def = (s16)(o->vit + bdef);
    o->atk = (s16)((cls == CLS_MAGE ? o->mag : o->str) + batk);
    o->crit = (s16)iclamp(4 + o->lck + (o->spd >> 2) + bcrit, 0, 60);
    o->aspd = (s16)iclamp(256 + imin(o->spd, 40) * 3, 256, 400);
    pass_apply_stats(cls, o);                    /* passive skills (Skills tab) */
}

void player_recalc(void) {
    Stats s;
    stats_calc(pl.cls, pl.level, pl.pts, pl.equip, &s);
    pl.maxhp = s.maxhp; pl.maxmp = s.maxmp;
    pl.str = s.str; pl.spd = s.spd; pl.vit = s.vit; pl.mag = s.mag; pl.lck = s.lck;
    pl.atk = s.atk; pl.def = s.def; pl.crit = s.crit; pl.aspd = s.aspd;
    if (pl.hp > pl.maxhp) pl.hp = pl.maxhp;
    if (pl.mp > pl.maxmp) pl.mp = pl.maxmp;
}

/* Shadow Strike (rogue B skill) fights with a +40 crit chance */
int player_roll_crit(void) { return rnd_n(100) < pl.crit + (pl.state == PS_SKILL && pl.cls == CLS_ROGUE ? 40 : 0); }

/* ------------------------------------------------------------------ stat points */
int stats_spend(int stat) {
    if (stat < 0 || stat >= STAT_COUNT || !pl.stat_pts || pl.pts[stat] >= STAT_MAX) return 0;
    int hp0 = pl.maxhp, mp0 = pl.maxmp;
    pl.pts[stat]++;
    pl.stat_pts--;
    player_recalc();
    /* the freshly gained capacity arrives filled, so investing never makes the bars look emptier */
    if (pl.hp > 0) pl.hp = (s16)imin(pl.maxhp, pl.hp + (pl.maxhp - hp0));
    pl.mp = (s16)imin(pl.maxmp, pl.mp + (pl.maxmp - mp0));
    return 1;
}

int stats_invested(void) {
    int n = 0;
    for (int i = 0; i < STAT_COUNT; i++) n += pl.pts[i];
    return n;
}

int stats_reset_cost(void) { return pl.level * 12; }

/* refunds every invested point (caller charges the gold) */
int stats_reset(void) {
    int n = stats_invested();
    for (int i = 0; i < STAT_COUNT; i++) pl.pts[i] = 0;
    pl.stat_pts = (u8)(pl.stat_pts + n);
    player_recalc();
    return n;
}
