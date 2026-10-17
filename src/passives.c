/* passives.c - passive skills. The hero earns a passive point every second level and spends it in the Skills tab.
   Stat passives are applied inside stats_calc() (via pass_apply_stats) so every preview agrees with the game;
   the rest are small hooks called from combat, kills, pickups and damage. Slot 7 is the class passive. */
#include "game.h"
#include "assets.h"
#include "audio_data.h"

typedef struct { const char *name, *desc; } PassiveText;

static const PassiveText common_text[7] = {
    { "Vitality", "+6% max HP per rank." },
    { "Iron Skin", "+2 DEF per rank." },
    { "Keen Edge", "+3 CRIT per rank." },
    { "Quick Feet", "+1 DEX per rank." },
    { "Scavenger", "+12% gold, better loot odds." },
    { "Lifedrinker", "Kills heal you a little." },
    { "Spirit Link", "Hits sometimes restore MP." },
};
static const PassiveText class_text[CLS_COUNT] = {
    { "Bulwark", "-6% damage taken per rank." },
    { "Arcane Mind", "+3 INT per rank." },
    { "Fleetfoot", "+6% attack speed per rank." },
    { "Shadow Veil", "6% dodge chance per rank." },
};

const char *pass_name(int i) { return i < 7 ? common_text[i].name : class_text[pl.cls].name; }
const char *pass_desc(int i) { return i < 7 ? common_text[i].desc : class_text[pl.cls].desc; }
int pass_max(int i) { (void)i; return 3; }

int pass_spend(int i) {
    if (i < 0 || i >= PASS_COUNT || !pl.pass_pts || pl.pass[i] >= pass_max(i)) return 0;
    pl.pass[i]++;
    pl.pass_pts--;
    player_recalc();
    return 1;
}

/* a point at every even level; returns how many a jump from `from` to `to` grants */
int pass_points_for(int from, int to) {
    int n = 0;
    for (int l = from + 1; l <= to; l++) n += (l & 1) == 0;
    return n;
}

void pass_apply_stats(int cls, Stats *o) {
    const u8 *p = pl.pass;
    if (p[0]) o->maxhp = (s16)(o->maxhp + o->maxhp * 6 * p[0] / 100);
    if (p[1]) { o->def = (s16)(o->def + 2 * p[1]); }
    if (p[2]) o->crit = (s16)iclamp(o->crit + 3 * p[2], 0, 60);
    if (p[3]) o->spd = (s16)(o->spd + p[3]);
    if (cls == CLS_MAGE && p[7]) { o->mag = (s16)(o->mag + 3 * p[7]); o->atk = (s16)(o->atk + 3 * p[7]); }
    if (cls == CLS_RANGER && p[7]) o->aspd = (s16)iclamp(o->aspd + o->aspd * 6 * p[7] / 100, 256, 460);
}

int pass_gold(int g) { return pl.pass[4] ? g + (g * 12 * pl.pass[4] + 50) / 100 + 1 : g; }
int pass_loot_bonus(void) { return pl.pass[4] * 7; }

/* Shadow Veil: the rogue slips some blows entirely */
int pass_dodge(void) {
    if (pl.cls != CLS_ROGUE || !pl.pass[7]) return 0;
    if (rnd_n(100) >= 6 * pl.pass[7]) return 0;
    dmgnum((int)(pl.x >> 8), (int)(pl.y >> 8) - 18, 0, 1, 0);
    toast_show("Dodged!");
    return 1;
}

/* Bulwark: the knight shrugs off a share of every blow */
int pass_reduce(int dmg) {
    if (pl.cls == CLS_KNIGHT && pl.pass[7]) dmg = dmg * (100 - 6 * pl.pass[7]) / 100;
    return dmg < 1 ? 1 : dmg;
}

void pass_on_kill(void) {
    if (pl.pass[5]) player_heal(pl.pass[5] * (2 + pl.level / 4), 0);
}

void pass_on_hit(void) {
    if (pl.pass[6] && pl.mp < pl.maxmp && rnd_n(100) < 14 + 8 * pl.pass[6]) {
        pl.mp++;
        part_spawn(PK_SPARKLE, (int)(pl.x >> 8), (int)(pl.y >> 8) - 12, 0, -30, 14, 0);
    }
}
