#include "game.h"
#include "assets.h"
#include "audio_data.h"

Player pl;
int level_up_pending;

static const s8 dxv[4] = { 0, 0, -1, 1 };
static const s8 dyv[4] = { 1, -1, 0, 0 };

/* ------------------------------------------------------------------ tables */
const ClassDef class_defs[CLS_COUNT] = {
    { "Knight", "Sturdy blade fighter. 3-hit sword combo, Whirlwind skill.", 42, 8, 8, 3, 6, 5, 4, 60, 14, 16, 4, 10, 3, 4 },
    { "Mage", "Frail but deadly. Arcane bolts, Fireball skill.", 28, 26, 3, 11, 3, 5, 5, 34, 36, 5, 18, 6, 4, 5 },
    { "Ranger", "Swift archer. Fast arrows, Multishot skill.", 34, 14, 7, 4, 4, 8, 6, 44, 20, 12, 6, 7, 6, 6 },
    { "Rogue", "Quick striker. Rapid stabs, Shadow Step skill.", 32, 14, 7, 4, 3, 9, 10, 40, 20, 13, 6, 6, 7, 9 },
};

#define EQ(nm, ds, sl, cl, atk, def, spd, lck, mag, hp, mp, pr) { nm, ds, IT_EQUIP, sl, cl, atk, def, spd, lck, mag, hp, mp, pr }
#define USE(nm, ds, hp, mp, pr) { nm, ds, IT_USE, 0, 0xFF, 0, 0, 0, 0, 0, hp, mp, pr }
const ItemDef item_defs[ITEM_COUNT] = {
    [ITEM_NONE] = { "-", "", 0, 0, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0 },
    [ITEM_POTION] = USE("Potion", "Restores 40 HP.", 40, 0, 25),
    [ITEM_HIPOTION] = USE("Hi-Potion", "Restores 100 HP.", 100, 0, 70),
    [ITEM_ETHER] = USE("Ether", "Restores 20 MP.", 0, 20, 35),
    [ITEM_ELIXIR] = USE("Elixir", "Fully restores HP and MP.", 999, 999, 200),
    [ITEM_CRYPT_KEY] = { "Crypt Key", "Opens the iron door in the crypt.", IT_KEY, 0, 0xFF, 0, 0, 0, 0, 0, 0, 0, 0 },
    [ITEM_SWORD1] = EQ("Wooden Sword", "A training blade.", SLOT_WEAPON, CLS_KNIGHT, 4, 0, 0, 0, 0, 0, 0, 30),
    [ITEM_SWORD2] = EQ("Iron Blade", "Sturdy and sharp.", SLOT_WEAPON, CLS_KNIGHT, 9, 0, 0, 0, 0, 0, 0, 160),
    [ITEM_SWORD3] = EQ("Emberbrand", "Glows with inner fire.", SLOT_WEAPON, CLS_KNIGHT, 16, 0, 0, 0, 0, 0, 0, 600),
    [ITEM_STAFF1] = EQ("Oak Staff", "A simple focus.", SLOT_WEAPON, CLS_MAGE, 4, 0, 0, 0, 0, 0, 0, 30),
    [ITEM_STAFF2] = EQ("Crystal Rod", "Hums with magic.", SLOT_WEAPON, CLS_MAGE, 9, 0, 0, 0, 0, 0, 0, 160),
    [ITEM_STAFF3] = EQ("Arcane Scepter", "Crackling power.", SLOT_WEAPON, CLS_MAGE, 16, 0, 0, 0, 0, 0, 0, 600),
    [ITEM_BOW1] = EQ("Short Bow", "Light and quick.", SLOT_WEAPON, CLS_RANGER, 4, 0, 0, 0, 0, 0, 0, 30),
    [ITEM_BOW2] = EQ("Long Bow", "Longer reach, harder hits.", SLOT_WEAPON, CLS_RANGER, 9, 0, 0, 0, 0, 0, 0, 160),
    [ITEM_BOW3] = EQ("Elven Bow", "Sings when drawn.", SLOT_WEAPON, CLS_RANGER, 16, 0, 0, 0, 0, 0, 0, 600),
    [ITEM_DAGGER1] = EQ("Iron Dagger", "Small but nasty.", SLOT_WEAPON, CLS_ROGUE, 4, 0, 0, 0, 0, 0, 0, 30),
    [ITEM_DAGGER2] = EQ("Steel Dirk", "Balanced for speed.", SLOT_WEAPON, CLS_ROGUE, 9, 0, 0, 0, 0, 0, 0, 160),
    [ITEM_DAGGER3] = EQ("Shadow Fang", "Bites from the dark.", SLOT_WEAPON, CLS_ROGUE, 16, 0, 0, 0, 0, 0, 0, 600),
    [ITEM_ARMOR1] = EQ("Cloth Tunic", "DEF +2", SLOT_ARMOR, 0xFF, 0, 2, 0, 0, 0, 0, 0, 20),
    [ITEM_ARMOR2] = EQ("Leather Armor", "DEF +5", SLOT_ARMOR, 0xFF, 0, 5, 0, 0, 0, 0, 0, 120),
    [ITEM_ARMOR3] = EQ("Chain Mail", "DEF +9", SLOT_ARMOR, 0xFF, 0, 9, 0, 0, 0, 0, 0, 400),
    [ITEM_CHARM_VIGOR] = EQ("Vigor Charm", "Max HP +20", SLOT_CHARM, 0xFF, 0, 0, 0, 0, 0, 20, 0, 150),
    [ITEM_CHARM_MIGHT] = EQ("Might Ring", "ATK +3", SLOT_CHARM, 0xFF, 3, 0, 0, 0, 0, 0, 0, 180),
    [ITEM_CHARM_MANA] = EQ("Mana Pendant", "Max MP +10", SLOT_CHARM, 0xFF, 0, 0, 0, 0, 0, 0, 10, 150),
    [ITEM_CHARM_SWIFT] = EQ("Swift Charm", "SPD +2", SLOT_CHARM, 0xFF, 0, 0, 2, 0, 0, 0, 0, 120),
    [ITEM_CHARM_LUCK] = EQ("Lucky Clover", "LCK +5", SLOT_CHARM, 0xFF, 0, 0, 0, 5, 0, 0, 0, 120),
};

typedef struct { u8 windup, active, recover, reach, mult, lunge; } Atk;
static const Atk atk_tab[4][3] = {
    { { 4, 5, 8, 24, 16, 1 }, { 3, 5, 8, 24, 18, 1 }, { 5, 6, 14, 29, 28, 2 } },
    { { 6, 2, 10, 0, 18, 0 }, { 6, 2, 10, 0, 18, 0 }, { 6, 2, 10, 0, 18, 0 } },
    { { 5, 2, 7, 0, 17, 0 }, { 5, 2, 7, 0, 17, 0 }, { 5, 2, 7, 0, 17, 0 } },
    { { 2, 3, 5, 19, 11, 2 }, { 2, 3, 5, 19, 12, 2 }, { 3, 4, 7, 22, 17, 3 } },
};
static const u8 max_combo[4] = { 3, 1, 1, 3 };
static const u8 skill_cost[4] = { 4, 5, 4, 4 };

int xp_for_level(int l) { return 10 + 6 * l + 2 * l * l; }

int player_weapon_item(void) { return pl.equip[SLOT_WEAPON]; }

/* ------------------------------------------------------------------ stats */
void player_recalc(void) {
    const ClassDef *c = &class_defs[pl.cls];
    int L = pl.level - 1;
    int hp = c->hp + ((c->ghp * L) >> 3);
    int mp = c->mp + ((c->gmp * L) >> 3);
    int str = c->str + ((c->gstr * L) >> 3);
    int mag = c->mag + ((c->gmag * L) >> 3);
    int def = c->def + ((c->gdef * L) >> 3);
    int spd = c->spd + ((c->gspd * L) >> 3);
    int lck = c->lck + ((c->glck * L) >> 3);
    int batk = 0, bdef = 0, bspd = 0, blck = 0, bhp = 0, bmp = 0;
    for (int s = 0; s < 3; s++) {
        const ItemDef *it = &item_defs[pl.equip[s]];
        batk += it->atk; bdef += it->def; bspd += it->spd; blck += it->lck;
        if (it->type == IT_EQUIP) { bhp += it->hp; bmp += it->mp; }
    }
    pl.maxhp = (s16)(hp + bhp);
    pl.maxmp = (s16)(mp + bmp);
    pl.str = (s16)str;
    pl.mag = (s16)mag;
    pl.def = (s16)(def + bdef);
    pl.spd = (s16)(spd + bspd);
    pl.lck = (s16)(lck + blck);
    pl.atk = (s16)((pl.cls == CLS_MAGE ? mag : str) + batk);
    if (pl.hp > pl.maxhp) pl.hp = pl.maxhp;
    if (pl.mp > pl.maxmp) pl.mp = pl.maxmp;
}

void player_new(int cls) {
    u8 *p = (u8 *)&pl;
    for (unsigned i = 0; i < sizeof(pl); i++) p[i] = 0;
    pl.cls = (u8)cls;
    pl.level = 1;
    pl.gold = 20;
    pl.equip[SLOT_WEAPON] = (u8)(ITEM_SWORD1 + cls * 3);
    pl.equip[SLOT_ARMOR] = ITEM_ARMOR1;
    pl.inv[ITEM_POTION] = 3;
    pl.inv[ITEM_ETHER] = 1;
    player_recalc();
    pl.hp = pl.maxhp;
    pl.mp = pl.maxmp;
    pl.hp_trail = pl.hp;
    gflags = 0;
    gchests = 0;
}

void player_place(int tx, int ty, int dir) {
    /* if the requested tile is blocked, use the nearest free tile (guards against bad save positions) */
    if (!box_free(tx * 16 + 8, ty * 16 + 15, 5, 6)) {
        int found = 0;
        for (int r = 1; r <= 5 && !found; r++)
            for (int dy = -r; dy <= r && !found; dy++)
                for (int dx = -r; dx <= r && !found; dx++) {
                    if (iabs(dx) != r && iabs(dy) != r) continue;
                    if (box_free((tx + dx) * 16 + 8, (ty + dy) * 16 + 15, 5, 6)) { tx += dx; ty += dy; found = 1; }
                }
    }
    pl.x = (s32)(tx * 16 + 8) * 256;
    pl.y = (s32)(ty * 16 + 15) * 256;
    pl.dir = (u8)dir;
    pl.state = PS_IDLE;
    pl.t = 0;
    pl.kx = pl.ky = 0;
    pl.combo = 0;
    pl.invuln = 0;
}

void player_heal(int hp, int mp) {
    int px = (int)(pl.x >> 8) - cam_x, py = (int)(pl.y >> 8) - cam_y - 18;
    if (hp > 0) {
        int before = pl.hp;
        pl.hp = (s16)imin(pl.maxhp, pl.hp + hp);
        if (pl.hp > before) dmgnum((int)(pl.x >> 8), (int)(pl.y >> 8) - 18, pl.hp - before, 1, 0);
    }
    if (mp > 0) pl.mp = (s16)imin(pl.maxmp, pl.mp + mp);
    (void)px; (void)py;
    for (int i = 0; i < 4; i++)
        part_spawn(PK_HEAL, (int)(pl.x >> 8) - 6 + rnd_n(12), (int)(pl.y >> 8) - 4, 0, -40 - rnd_n(30), 26, 0);
}

void player_give(int item, int qty) {
    if (item <= ITEM_NONE || item >= ITEM_COUNT) return;
    int n = pl.inv[item] + qty;
    pl.inv[item] = (u8)imin(n, 99);
    const ItemDef *it = &item_defs[item];
    if (it->type == IT_KEY) gflags |= GF_GOT_KEY;
    if (it->type == IT_EQUIP) {
        const ItemDef *cur = &item_defs[pl.equip[it->slot]];
        int better = 0;
        if (it->slot == SLOT_WEAPON && it->cls == pl.cls) better = (pl.equip[0] == 0) || it->atk > cur->atk;
        else if (it->slot == SLOT_ARMOR) better = (pl.equip[1] == 0) || it->def > cur->def;
        else if (it->slot == SLOT_CHARM) better = (pl.equip[2] == 0);
        if (better) player_equip(item);
    }
}

void player_equip(int item) {
    const ItemDef *it = &item_defs[item];
    if (it->type != IT_EQUIP || pl.inv[item] == 0) return;
    if (it->slot == SLOT_WEAPON && it->cls != pl.cls) return;
    pl.equip[it->slot] = (u8)item;
    player_recalc();
}

int player_use_item(int item) {
    const ItemDef *it = &item_defs[item];
    if (it->type != IT_USE || pl.inv[item] == 0) return 0;
    if (it->hp && pl.hp >= pl.maxhp && !it->mp) return 0;
    if (it->mp && !it->hp && pl.mp >= pl.maxmp) return 0;
    if (it->hp >= 900) { pl.hp = pl.maxhp; pl.mp = pl.maxmp; player_heal(0, 0); }
    else player_heal(it->hp, it->mp);
    pl.inv[item]--;
    snd_sfx(SFX_POTION);
    return 1;
}

void player_add_gold(int g) {
    int v = pl.gold + g;
    pl.gold = (u16)iclamp(v, 0, 9999);
}

void player_add_xp(int xp) {
    if (pl.level >= 20) return;
    pl.xp += (u32)xp;
    int oldhp = pl.maxhp, oldatk = pl.atk;
    int lv = 0;
    while (pl.level < 20 && pl.xp >= (u32)xp_for_level(pl.level)) {
        pl.xp -= (u32)xp_for_level(pl.level);
        pl.level++;
        lv++;
    }
    if (lv) {
        player_recalc();
        pl.hp = pl.maxhp;
        pl.mp = pl.maxmp;
        (void)oldhp; (void)oldatk;
        level_up_pending = 1;
    }
}

int player_calc_dmg(int mult16, int *crit) {
    int base = (pl.atk * mult16) >> 4;
    int d = base + rnd_n(base / 4 + 1);
    int c = rnd_n(100) < (4 + pl.lck);
    if (crit) *crit = c;
    if (c) d = (d * 7) >> 2;
    return d < 1 ? 1 : d;
}

/* ------------------------------------------------------------------ hurt / death */
void player_hurt(int dmg, int fx, int fy) {
    if (pl.invuln || pl.state == PS_DEAD || pl.state == PS_ROLL || (pl.state == PS_SKILL && pl.cls == CLS_ROGUE)) return;
    dmg -= pl.def >> 1;
    if (dmg < 1) dmg = 1;
    pl.hp -= (s16)dmg;
    pl.flash = 6;
    pl.invuln = 60;
    int px = (int)(pl.x >> 8), py = (int)(pl.y >> 8);
    dmgnum(px, py - 18, dmg, 2, 0);
    int a = vec_angle(px - fx, py - 6 - fy);
    pl.kx = (s16)((isin(a) * 560) >> 8);
    pl.ky = (s16)((-icos(a) * 560) >> 8);
    part_burst(PK_HIT, px, py - 8, 1, 0, 10);
    shake_add(3);
    hitstop_t = imax(hitstop_t, 4);
    snd_sfx(SFX_HURT);
    if (pl.hp <= 0) {
        pl.hp = 0;
        pl.state = PS_DEAD;
        pl.t = 0;
        snd_stop_music();
        return;
    }
    pl.state = PS_HURT;
    pl.t = 0;
    pl.combo = 0;
}

/* ------------------------------------------------------------------ movement helpers */
static void move_axis(int vx, int vy) {
    int nx = (int)(pl.x + vx), ny = (int)(pl.y + vy);
    if (vx && box_free(nx >> 8, (int)(pl.y >> 8), 5, 6)) pl.x = nx;
    if (vy && box_free((int)(pl.x >> 8), ny >> 8, 5, 6)) pl.y = ny;
}

static void apply_knock(void) {
    if (pl.kx || pl.ky) {
        move_axis(pl.kx, pl.ky);
        pl.kx = (s16)((pl.kx * 14) >> 4);
        pl.ky = (s16)((pl.ky * 14) >> 4);
        if (iabs(pl.kx) < 16) pl.kx = 0;
        if (iabs(pl.ky) < 16) pl.ky = 0;
    }
}

static int read_dir(int *dx, int *dy) {
    *dx = ((keys_held & KEY_RIGHT) ? 1 : 0) - ((keys_held & KEY_LEFT) ? 1 : 0);
    *dy = ((keys_held & KEY_DOWN) ? 1 : 0) - ((keys_held & KEY_UP) ? 1 : 0);
    return *dx || *dy;
}

static void face(int dx, int dy) {
    if (dx && !dy) pl.dir = (u8)(dx < 0 ? DIR_LEFT : DIR_RIGHT);
    else if (dy && !dx) pl.dir = (u8)(dy < 0 ? DIR_UP : DIR_DOWN);
    else if (dx && dy) {
        int horiz = (pl.dir == DIR_LEFT || pl.dir == DIR_RIGHT);
        if (!horiz) pl.dir = (u8)(dx < 0 ? DIR_LEFT : DIR_RIGHT);
        else pl.dir = (u8)(dy < 0 ? DIR_UP : DIR_DOWN);
    }
}

static int move_speed(void) { return 226 + pl.spd * 12; }

/* ------------------------------------------------------------------ combat helpers */
static int body_x(void) { return (int)(pl.x >> 8); }
static int body_y(void) { return (int)(pl.y >> 8) - 7; }

static int in_reach(const Enemy *e, int reach) {
    int dx = (int)(e->x >> 8) - body_x(), dy = (int)(e->y >> 8) - 6 - body_y();
    int fx = dxv[pl.dir], fy = dyv[pl.dir];
    int along = dx * fx + dy * fy;
    int perp = iabs(dx * fy - dy * fx);
    int er = e->type == EN_BOSS ? 14 : 8;
    return along > -5 && along <= reach + er && perp <= (reach * 3 >> 2) + er;
}

static void melee_hit(int reach, int mult, int radial, int id) {
    for (int i = 0; i < MAX_EN; i++) {
        Enemy *e = &en[i];
        if (!e->active || e->dying || e->hitid == id) continue;
        int ok;
        if (radial) {
            int dx = (int)(e->x >> 8) - body_x(), dy = (int)(e->y >> 8) - 6 - body_y();
            ok = idist(dx, dy) <= reach + (e->type == EN_BOSS ? 14 : 8);
        } else ok = in_reach(e, reach);
        if (!ok) continue;
        e->hitid = (u8)id;
        int crit;
        int d = player_calc_dmg(mult, &crit);
        enemy_damage(e, d, (int)(e->x >> 8) - body_x(), (int)(e->y >> 8) - 6 - body_y(), crit);
    }
}

static void bush_cut(int reach) {
    int px = body_x() + dxv[pl.dir] * (reach - 6), py = (int)(pl.y >> 8) - 4 + dyv[pl.dir] * (reach - 6);
    for (int o = -6; o <= 6; o += 6) {
        int qx = px + (dyv[pl.dir] ? o : 0), qy = py + (dxv[pl.dir] ? o : 0);
        int tx = qx >> 4, ty = qy >> 4;
        if (tile_flags(tx, ty) & TF_CUT) {
            world_set_meta(0, tx, ty, 0);
            snd_sfx(SFX_BUSH);
            part_burst(PK_LEAF, tx * 16 + 8, ty * 16 + 8, 6, 60, 30);
            int r = rnd_n(100);
            if (r < 40) pickup_spawn(PK_COIN, tx * 16 + 8, ty * 16 + 10);
            else if (r < 52) pickup_spawn(PK_HEART, tx * 16 + 8, ty * 16 + 10);
            else if (r < 60) pickup_spawn(PK_MANA, tx * 16 + 8, ty * 16 + 10);
        }
    }
}

static int sweep_angle(int t_active_num, int t_active_den, int a0, int a1) {
    /* ease-out interpolation between a0 and a1 */
    int p = (t_active_num * 256) / (t_active_den ? t_active_den : 1);
    p = iclamp(p, 0, 256);
    int e = 256 - (((256 - p) * (256 - p)) >> 8);
    return a0 + (((a1 - a0) * e) >> 8);
}

static int swing_a0, swing_a1;

static void start_attack(void) {
    int c = pl.combo;
    if (c >= max_combo[pl.cls]) c = 0;
    pl.combo = (u8)c;
    pl.state = PS_ATTACK;
    pl.t = 0;
    pl.atk_id++;
    if (pl.atk_id == 0) pl.atk_id = 1;
    pl.buffered = 0;
    int center = dir_angle(pl.dir);
    int span = (c == 2) ? 76 : 58;
    int sgn = (c & 1) ? -1 : 1;
    if (pl.cls == CLS_ROGUE) span = 44;
    swing_a0 = center - span * sgn;
    swing_a1 = center + span * sgn;
    pl.swing_flip = (u8)(sgn < 0);
}

static void fire_arrow(int angle, int mult, int spd) {
    int crit;
    int d = player_calc_dmg(mult, &crit);
    proj_spawn(PJ_ARROW, 0, body_x() + dxv[pl.dir] * 8, body_y() + dyv[pl.dir] * 8, angle, spd, d);
}

static void start_roll(void);

static void attack_update(void) {
    const Atk *a = &atk_tab[pl.cls][pl.combo];
    int rec = a->recover - (pl.spd > 12 ? 2 : pl.spd > 8 ? 1 : 0);
    int total = a->windup + a->active + rec;
    pl.t++;
    int t = pl.t;
    if (t == a->windup) {
        switch (pl.cls) {
        case CLS_KNIGHT: snd_sfx(pl.combo == 2 ? SFX_SLASH2 : SFX_SLASH); break;
        case CLS_ROGUE: snd_sfx(SFX_SLASH2); break;
        case CLS_MAGE: {
            snd_sfx(SFX_MAGIC);
            int crit;
            int d = player_calc_dmg(a->mult, &crit);
            proj_spawn(PJ_BOLT, 0, body_x() + dxv[pl.dir] * 9, body_y() + dyv[pl.dir] * 9, dir_angle(pl.dir), 3 * 256 + 64, d);
            break;
        }
        case CLS_RANGER:
            snd_sfx(SFX_ARROW);
            fire_arrow(dir_angle(pl.dir), a->mult, 4 * 256);
            break;
        }
    }
    if (t >= a->windup && t < a->windup + a->active) {
        if (a->reach) {
            melee_hit(a->reach, a->mult, 0, pl.atk_id);
            bush_cut(a->reach);
        }
        if (a->lunge) move_axis(dxv[pl.dir] * a->lunge * 256 / 2, dyv[pl.dir] * a->lunge * 256 / 2);
    }
    /* chain / cancel */
    if (keys_down & KEY_A) pl.buffered = 1;
    if (t >= a->windup + a->active && (keys_down & KEY_R) && pl.roll_cd == 0) {
        pl.combo = 0;
        pl.buffered = 0;
        start_roll();
        return;
    }
    if (t >= total) {
        if (pl.buffered && max_combo[pl.cls] > 0) {
            pl.combo++;
            if (pl.combo >= max_combo[pl.cls]) pl.combo = 0;
            start_attack();
            return;
        }
        pl.state = PS_IDLE;
        pl.idle_t = 0;
        if (!(keys_held & KEY_A)) pl.combo = (pl.combo + 1) >= max_combo[pl.cls] ? 0 : pl.combo + 1;
    }
    (void)swing_a0;
}

/* skills ---------------------------------------------------------------- */
static int skill_t_total(void) {
    switch (pl.cls) {
    case CLS_KNIGHT: return 28;
    case CLS_MAGE: return 24;
    case CLS_RANGER: return 20;
    default: return 14;
    }
}

static void start_skill(void) {
    if (pl.skill_cd || pl.mp < skill_cost[pl.cls]) {
        if (pl.mp < skill_cost[pl.cls]) snd_sfx(SFX_ERROR);
        return;
    }
    pl.mp -= skill_cost[pl.cls];
    pl.state = PS_SKILL;
    pl.t = 0;
    pl.atk_id++;
    if (pl.atk_id == 0) pl.atk_id = 1;
    pl.skill_cd = 20;
    if (pl.cls == CLS_KNIGHT) snd_sfx(SFX_SPIN);
    if (pl.cls == CLS_MAGE) snd_sfx(SFX_MAGIC);
    if (pl.cls == CLS_RANGER) snd_sfx(SFX_ARROW);
    if (pl.cls == CLS_ROGUE) {
        snd_sfx(SFX_DASH);
        pl.invuln = imax(pl.invuln, 16);
        int dx, dy;
        if (read_dir(&dx, &dy)) face(dx, dy);
        int a = dx || dy ? vec_angle(dx, dy) : dir_angle(pl.dir);
        pl.dash_x = (s16)((isin(a) * 4 * 256) >> 8);
        pl.dash_y = (s16)((-icos(a) * 4 * 256) >> 8);
    }
}

static void skill_update(void) {
    pl.t++;
    int t = pl.t;
    switch (pl.cls) {
    case CLS_KNIGHT:
        if (t == 6 || t == 12 || t == 18) {
            pl.atk_id++;
            if (!pl.atk_id) pl.atk_id = 1;
        }
        if (t >= 4 && t < 22 && (t % 6) == 0) {
            melee_hit(32, 24, 1, pl.atk_id);
            part_burst(PK_DUST, body_x(), (int)(pl.y >> 8) - 2, 3, 70, 14);
        }
        if (t == 4) { pl.atk_id++; if (!pl.atk_id) pl.atk_id = 1; }
        break;
    case CLS_MAGE:
        if (t == 10) {
            int crit;
            int d = player_calc_dmg(40, &crit);
            snd_sfx(SFX_FIRE);
            proj_spawn(PJ_FIRE, 0, body_x() + dxv[pl.dir] * 10, body_y() + dyv[pl.dir] * 10, dir_angle(pl.dir), 2 * 256 + 128, d);
        }
        break;
    case CLS_RANGER:
        if (t == 8) {
            int base = dir_angle(pl.dir);
            for (int k = -2; k <= 2; k++) fire_arrow((base + k * 7) & 255, 12, 4 * 256);
            shake_add(1);
        }
        break;
    case CLS_ROGUE:
        if (t < 11) {
            move_axis(pl.dash_x, pl.dash_y);
            if (t & 1) ghost_spawn(body_x(), (int)(pl.y >> 8), pl.dir, pl.dir);
            melee_hit(14, 22, 1, pl.atk_id);
        }
        break;
    }
    if (t >= skill_t_total()) {
        pl.state = PS_IDLE;
        pl.idle_t = 0;
    }
}

/* roll ------------------------------------------------------------------ */
static void start_roll(void) {
    int dx, dy;
    if (read_dir(&dx, &dy)) {
        face(dx, dy);
        int a = vec_angle(dx, dy);
        pl.dash_x = (s16)((isin(a) * 256) >> 8);
        pl.dash_y = (s16)((-icos(a) * 256) >> 8);
    } else {
        pl.dash_x = (s16)(dxv[pl.dir] * 256);
        pl.dash_y = (s16)(dyv[pl.dir] * 256);
    }
    pl.state = PS_ROLL;
    pl.t = 0;
    pl.anim_t = 0;
    pl.roll_cd = (u8)(28 - imin(pl.spd, 14));
    pl.invuln = imax(pl.invuln, 16);
    snd_sfx(SFX_ROLL);
    part_burst(PK_DUST, body_x(), (int)(pl.y >> 8) - 1, 3, 40, 16);
}

static void roll_update(void) {
    int t = pl.t++;
    int sp = 3 * 256 + 100 - t * 22;
    if (sp < 120) sp = 120;
    if (pl.dash_x == 0 && pl.dash_y == 0) { pl.dash_x = (s16)(dxv[pl.dir] * 256); pl.dash_y = (s16)(dyv[pl.dir] * 256); }
    move_axis((pl.dash_x * sp) >> 8, (pl.dash_y * sp) >> 8);
    pl.anim_t++;
    if ((t & 3) == 0) part_spawn(PK_DUST, body_x() - 3 + rnd_n(6), (int)(pl.y >> 8) - 1, 0, -10, 12, 0);
    if (pl.t >= 14) {
        pl.state = PS_IDLE;
        pl.idle_t = 0;
    }
}

/* ------------------------------------------------------------------ interaction */
int player_try_interact(void) {
    int px = (int)(pl.x >> 8), py = (int)(pl.y >> 8);
    int fx = px + dxv[pl.dir] * 12, fy = py - 5 + dyv[pl.dir] * 12;
    Npc *n = world_npc_near(fx, fy - 4, 17);
    if (n) {
        n->talk_t = 60;
        npc_talk(n);
        return 1;
    }
    int f = pt_flags(fx, fy);
    int tx = fx >> 4, ty = fy >> 4;
    if (f & TF_SIGN) {
        const char *s = world_sign_at(tx, ty);
        if (s) { play_dialogue("Sign", s); return 1; }
    }
    if (f & TF_CHEST) {
        int ci = world_open_chest(tx, ty);
        if (ci >= 0) {
            const ChestDef *c = &cur_area->chests[ci];
            int item = c->item;
            if (item == CH_WEAPON2) item = ITEM_SWORD1 + pl.cls * 3 + 1;
            else if (item == CH_WEAPON3) item = ITEM_SWORD1 + pl.cls * 3 + 2;
            player_give(item, c->qty);
            snd_sfx(SFX_CHEST);
            item_get_show(item, c->qty);
            pl.state = PS_ITEM;
            pl.t = 0;
            return 1;
        }
    }
    if (f & TF_DOOR) {
        if (pl.inv[ITEM_CRYPT_KEY]) {
            pl.inv[ITEM_CRYPT_KEY]--;
            gflags |= GF_DOOR_OPEN;
            world_unlock_door();
            snd_sfx(SFX_DOOR);
            shake_add(2);
            toast_show("The iron door creaks open!");
        } else {
            snd_sfx(SFX_ERROR);
            play_dialogue("", "The iron door is locked. Perhaps a key lies in the woods.");
        }
        return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ idle */
static void use_quick_potion(void) {
    if (pl.hp < pl.maxhp) {
        if (pl.inv[ITEM_POTION] && player_use_item(ITEM_POTION)) return;
        if (pl.inv[ITEM_HIPOTION] && player_use_item(ITEM_HIPOTION)) return;
    }
    if (pl.mp < pl.maxmp && pl.inv[ITEM_ETHER] && pl.hp >= pl.maxhp) { player_use_item(ITEM_ETHER); return; }
    snd_sfx(SFX_ERROR);
}

static void idle_update(void) {
    int dx, dy;
    int moving = read_dir(&dx, &dy);
    if (moving) {
        face(dx, dy);
        int sp = move_speed();
        int vx = dx * sp, vy = dy * sp;
        if (dx && dy) { vx = (vx * 181) >> 8; vy = (vy * 181) >> 8; }
        move_axis(vx, vy);
        pl.anim_t++;
        pl.idle_t = 0;
        if ((pl.anim_t % 15) == 7 && (frame_count & 1)) part_spawn(PK_DUST, (int)(pl.x >> 8), (int)(pl.y >> 8) - 1, 0, -6, 10, 0);
    } else {
        pl.anim_t = 0;
        if (pl.idle_t < 250) pl.idle_t++;
    }
    if (keys_down & KEY_R && pl.roll_cd == 0) { start_roll(); return; }
    if (keys_down & KEY_B) { start_skill(); if (pl.state == PS_SKILL) return; }
    if (keys_down & KEY_L) use_quick_potion();
    if (keys_down & KEY_A) {
        if (player_try_interact()) return;
        start_attack();
    }
}

void player_update(void) {
    pl.playtime++;
    if (pl.invuln) pl.invuln--;
    if (pl.flash) pl.flash--;
    if (pl.roll_cd) pl.roll_cd--;
    if (pl.skill_cd) pl.skill_cd--;
    apply_knock();
    switch (pl.state) {
    case PS_IDLE: idle_update(); break;
    case PS_ATTACK: attack_update(); break;
    case PS_SKILL: skill_update(); break;
    case PS_ROLL: roll_update(); break;
    case PS_HURT:
        if (++pl.t > 12) { pl.state = PS_IDLE; }
        break;
    case PS_DEAD: pl.t++; break;
    case PS_ITEM:
        if (++pl.t > 64) pl.state = PS_IDLE;
        break;
    }
    /* mp regen */
    if (pl.state != PS_DEAD && pl.mp < pl.maxmp) {
        pl.mp_acc++;
        int period = 110 - imin(pl.cls == CLS_MAGE ? pl.mag * 3 : pl.mag * 2, 70);
        if (pl.mp_acc >= period) { pl.mp_acc = 0; pl.mp++; }
    }
}

/* ------------------------------------------------------------------ drawing */
static int hero_tile(int *flip) {
    int f = 0;
    *flip = 0;
    int d = pl.dir;
    int base_walk = d == DIR_DOWN ? HF_WALK_DOWN : d == DIR_UP ? HF_WALK_UP : HF_WALK_SIDE;
    int base_atk = d == DIR_DOWN ? HF_ATK_DOWN : d == DIR_UP ? HF_ATK_UP : HF_ATK_SIDE;
    int base_hurt = d == DIR_DOWN ? HF_HURT_DOWN : d == DIR_UP ? HF_HURT_UP : HF_HURT_SIDE;
    *flip = d == DIR_RIGHT;
    switch (pl.state) {
    case PS_IDLE:
        if (pl.anim_t) f = base_walk + ((pl.anim_t / 5) % 6);
        else f = base_walk + ((pl.idle_t > 90 && (pl.idle_t & 63) < 8) ? 2 : 0);
        break;
    case PS_ATTACK: {
        const Atk *a = &atk_tab[pl.cls][pl.combo];
        int ph = pl.t < a->windup ? 0 : (pl.t < a->windup + a->active ? 1 : 2);
        f = base_atk + ph;
        break;
    }
    case PS_SKILL:
        if (pl.cls == CLS_KNIGHT) {
            static const u8 order[4] = { HF_ATK_DOWN + 1, HF_ATK_SIDE + 1, HF_ATK_UP + 1, HF_ATK_SIDE + 1 };
            int k = (pl.t / 3) & 3;
            f = order[k];
            *flip = (k == 3);
        } else if (pl.cls == CLS_ROGUE) f = HF_ROLL + ((pl.t / 2) & 3);
        else f = base_atk + (pl.t < 8 ? 0 : 1);
        break;
    case PS_ROLL:
        f = HF_ROLL + ((pl.anim_t / 3) & 3);
        *flip = (pl.dash_x < 0) ? 1 : (pl.dash_x > 0 ? 0 : (pl.anim_t & 4) != 0);
        break;
    case PS_HURT: f = base_hurt; break;
    case PS_DEAD: f = HF_DEAD + imin(pl.t / 14, 2); *flip = 0; break;
    case PS_ITEM: f = HF_WALK_DOWN; *flip = 0; break;
    }
    return f * 4;
}

static void draw_weapon(int sx, int sy) {
    int key = (int)(pl.y >> 8);
    int wt = OT_WEAPON + pl.cls * 16;
    int cx = sx, cy = sy - 7;
    int center = dir_angle(pl.dir);
    if (pl.state == PS_ATTACK) {
        const Atk *a = &atk_tab[pl.cls][pl.combo];
        int t = pl.t;
        int fwd = 3;
        if (pl.cls == CLS_KNIGHT || pl.cls == CLS_ROGUE) {
            int ang;
            if (t < a->windup) ang = swing_a0 + (swing_a0 > swing_a1 ? 10 : -10) * t / (a->windup ? a->windup : 1);
            else ang = sweep_angle(t - a->windup, a->active + 2, swing_a0 + (swing_a0 > swing_a1 ? 10 : -10), swing_a1);
            if (t >= a->windup + a->active + 5 && pl.cls == CLS_ROGUE) return;
            if (t >= a->windup + a->active + 8) return;
            int ox = dxv[pl.dir] * fwd, oy = dyv[pl.dir] * fwd;
            spr_add_aff(cx + ox - 16, cy + oy - 16, SH_SQUARE, 2, wt, PB_WEAPON, 2, 0, pl.dir == DIR_UP ? key - 1 : key + 1, ang & 255, 256);
            if (t >= a->windup - 1 && t < a->windup + a->active + 3) {
                int trail = (ang + (swing_a0 > swing_a1 ? 6 : -6)) & 255;
                spr_add_aff(cx + ox - 16, cy + oy - 16, SH_SQUARE, 2, OT_ARC, PB_WEAPON, 2, (swing_a0 > swing_a1 ? SF_HFLIP : 0) | SF_ALPHA, pl.dir == DIR_UP ? key - 2 : key + 2, trail, 256);
            }
        } else if (pl.cls == CLS_MAGE) {
            int pull = t < a->windup ? t : (t < a->windup + a->active ? a->windup : a->windup - (t - a->windup - a->active) / 2);
            int ang = center + (t < a->windup ? -(24 - pull * 3) : 0);
            if (t >= a->windup + a->active + 6) ang = center + 20;
            int ox = dxv[pl.dir] * (fwd + (t >= a->windup ? 2 : 0)), oy = dyv[pl.dir] * (fwd + (t >= a->windup ? 2 : 0));
            spr_add_aff(cx + ox - 16, cy + oy - 16, SH_SQUARE, 2, wt, PB_WEAPON, 2, 0, pl.dir == DIR_UP ? key - 1 : key + 1, ang & 255, 256);
        } else {
            int ox = dxv[pl.dir] * (fwd + (t < a->windup ? -2 : 3)), oy = dyv[pl.dir] * (fwd + (t < a->windup ? -2 : 3));
            spr_add_aff(cx + ox - 16, cy + oy - 16, SH_SQUARE, 2, wt, PB_WEAPON, 2, 0, pl.dir == DIR_UP ? key - 1 : key + 1, center, 256);
        }
    } else if (pl.state == PS_SKILL) {
        int t = pl.t;
        if (pl.cls == CLS_KNIGHT) {
            int ang = (t * 24) & 255;
            spr_add_aff(cx - 16, cy - 16, SH_SQUARE, 2, wt, PB_WEAPON, 2, 0, key + 2, ang, 256);
            spr_add_aff(cx - 16, cy - 16, SH_SQUARE, 2, OT_ARC, PB_WEAPON, 2, SF_ALPHA, key + 3, (ang + 4) & 255, 256);
            spr_add_aff(cx - 16, cy - 16, SH_SQUARE, 2, OT_ARC, PB_WEAPON, 2, SF_ALPHA, key + 4, (ang + 128 + 4) & 255, 256);
        } else if (pl.cls == CLS_MAGE) {
            int ang = center + (t < 10 ? -20 + t * 2 : 0);
            spr_add_aff(cx + dxv[pl.dir] * 4 - 16, cy + dyv[pl.dir] * 4 - 16, SH_SQUARE, 2, wt, PB_WEAPON, 2, 0, key + 1, ang & 255, 256);
            if (t < 10) {
                int r = 2 + t / 2;
                spr_add(cx + dxv[pl.dir] * 12 - 4 + (t & 1), cy + dyv[pl.dir] * 12 - 4, SH_SQUARE, 0, OT_PART + PT_SPARKLE + ((t / 3) & 3), PB_FX, 2, 0, key + 5);
                (void)r;
            }
        } else if (pl.cls == CLS_RANGER) {
            spr_add_aff(cx + dxv[pl.dir] * 4 - 16, cy + dyv[pl.dir] * 4 - 16, SH_SQUARE, 2, wt, PB_WEAPON, 2, 0, key + 1, center, 256);
        }
    } else if (pl.state == PS_IDLE && pl.idle_t == 0 && pl.cls != CLS_KNIGHT && 0) {
        (void)wt;
    }
}

static void draw_prompt(int sx, int sy) {
    if (pl.state != PS_IDLE) return;
    int px = (int)(pl.x >> 8), py = (int)(pl.y >> 8);
    int fx = px + dxv[pl.dir] * 12, fy = py - 5 + dyv[pl.dir] * 12;
    int tx = -1, ty = 0;
    Npc *n = world_npc_near(fx, fy - 4, 17);
    if (n) { tx = n->x; ty = n->y - 18; }
    else {
        int f = pt_flags(fx, fy);
        if (f & (TF_SIGN | TF_CHEST | TF_DOOR)) { tx = (fx >> 4) * 16 + 8; ty = (fy >> 4) * 16 - 4; }
    }
    if (tx < 0) return;
    (void)sx; (void)sy;
    spr_add(tx - cam_x - 8, ty - cam_y - 14, SH_SQUARE, 1, OT_PROMPT + ((frame_count >> 4) & 1) * 4, PB_UI, 2, 0, 21000);
}

void player_draw(void) {
    int px = (int)(pl.x >> 8), py = (int)(pl.y >> 8);
    int sx = px - cam_x, sy = py - cam_y;
    int blink = pl.invuln > 0 && pl.state != PS_DEAD && pl.state != PS_ROLL && ((pl.invuln >> 1) & 1) && pl.flash == 0;
    int flip;
    int tile = hero_tile(&flip);
    int pal = pl.flash ? PB_FLASH : PB_HERO;
    int yoff = 0;
    if (pl.state == PS_ROLL) yoff = 1;
    if (!blink) spr16(sx - 8, sy - 15 + yoff, tile, pal, flip ? SF_HFLIP : 0, py);
    if (fade_level == 0 && pl.state != PS_DEAD)
        spr_add(sx - 8, sy - 4, SH_WIDE, 0, OT_SHADOW, PB_FX, 2, SF_ALPHA, -2000 + py);
    if (pl.state == PS_ITEM && pl.t < 60) {
        spr_add(sx - 4, sy - 28 - (pl.t < 10 ? pl.t / 2 : 5), SH_SQUARE, 0, OT_PART + PT_SPARKLE + ((pl.t / 4) & 3), PB_FX, 2, 0, 20000);
    }
    if (!blink || pl.state == PS_ATTACK || pl.state == PS_SKILL) draw_weapon(sx, sy);
    draw_prompt(sx, sy);
}
