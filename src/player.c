#include "game.h"
#include "assets.h"
#include "audio_data.h"

/* player.c - movement, skills, rolling, interaction and drawing.
   Stats live in stats.c, items/gear in items.c and the attack/combo state machine in combat.c. */

Player pl;
int level_up_pending;

static const s8 dxv[4] = { 0, 0, -1, 1 };
static const s8 dyv[4] = { 1, -1, 0, 0 };

/* B skills: each is a short multi-stage move. cost/cooldown/length are in MP / frames. */
typedef struct { const char *name; u8 cost, cd, total; const char *d1, *d2; } SkillDef;
static const SkillDef skill_defs[CLS_COUNT] = {
    { "Blade Storm", 6, 24, 36, "Four spins, then a ground slam", "that hits hard all around." },
    { "Inferno", 7, 26, 30, "A fire nova, then two waves", "of bolts that burn through." },
    { "Volley Step", 6, 22, 28, "Hop back, fire a fan of arrows,", "then a piercing triple shot." },
    { "Shadow Strike", 6, 24, 26, "Dash through foes, then two", "crit-boosted slashes." },
};
const char *class_skill_name(int cls) { return skill_defs[cls].name; }
int class_skill_cost(int cls) { return skill_defs[cls].cost; }
const char *class_skill_desc(int cls, int line) { return line ? skill_defs[cls].d2 : skill_defs[cls].d1; }

static const HitFx skill_fx = { HK_SLASH, 0, 0, 16, 1, 3 };
static const HitFx skill_slam = { HK_SMASH, 1, 1, 30, 5, 6 };
static const HitFx skill_fire = { HK_FIRE, 0, 0, 24, 2, 3 };
static const HitFx skill_cut = { HK_STAB, 1, 0, 16, 2, 4 };

int player_weapon_item(void) { return pl.equip[SLOT_WEAPON]; }

/* ------------------------------------------------------------------ lifecycle */
void player_new(int cls) {
    u8 *p = (u8 *)&pl;
    for (unsigned i = 0; i < sizeof(pl); i++) p[i] = 0;
    pl.cls = (u8)cls;
    pl.level = 1;
    pl.gold = 20;
    pl.equip[SLOT_WEAPON] = (u8)item_weapon(cls, 0);
    pl.equip[SLOT_ARMOR] = ITEM_ARMOR1;
    pl.inv[pl.equip[SLOT_WEAPON]] = 1;
    pl.inv[ITEM_ARMOR1] = 1;
    pl.inv[ITEM_POTION] = 3;
    pl.inv[ITEM_ETHER] = 1;
    pl.auto_equip = 1;
    quests_reset();
    player_recalc();
    pl.hp = pl.maxhp;
    pl.mp = pl.maxmp;
    pl.hp_trail = pl.hp;
    gflags = 0;
    gchests = 0;
    gchests2 = 0;
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
    combat_reset();
    pl.invuln = 0;
}

void player_heal(int hp, int mp) {
    if (hp > 0) {
        int before = pl.hp;
        pl.hp = (s16)imin(pl.maxhp, pl.hp + hp);
        if (pl.hp > before) dmgnum((int)(pl.x >> 8), (int)(pl.y >> 8) - 18, pl.hp - before, 1, 0);
    }
    if (mp > 0) pl.mp = (s16)imin(pl.maxmp, pl.mp + mp);
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
        int usable = !(it->slot == SLOT_WEAPON && it->cls != pl.cls);
        if (usable && pl.auto_equip && item_is_better(item, pl.equip[it->slot])) player_equip(item);
    }
    quest_refresh();
}

/* equips the highest-scoring piece the hero owns in every slot; returns how many slots changed */
int player_auto_equip_best(void) {
    int changed = 0;
    for (int sl = 0; sl < SLOT_COUNT; sl++) {
        int best = pl.equip[sl];
        for (int i = 1; i < ITEM_COUNT; i++) {
            const ItemDef *d = &item_defs[i];
            if (d->type != IT_EQUIP || d->slot != sl || !pl.inv[i]) continue;
            if (sl == SLOT_WEAPON && d->cls != pl.cls) continue;
            if (best == ITEM_NONE || item_score(i) > item_score(best)) best = i;
        }
        if (best != pl.equip[sl]) { pl.equip[sl] = (u8)best; changed++; }
    }
    if (changed) { player_recalc(); gear_apply_look(); }
    return changed;
}

void player_equip(int item) {
    const ItemDef *it = &item_defs[item];
    if (it->type != IT_EQUIP || pl.inv[item] == 0) return;
    if (it->slot == SLOT_WEAPON && it->cls != pl.cls) return;
    pl.equip[it->slot] = (u8)item;
    player_recalc();
    gear_apply_look();
}

/* potions share a short cooldown so they cannot simply be chugged through a boss fight */
int player_use_item(int item) {
    const ItemDef *it = &item_defs[item];
    if (it->type != IT_USE || pl.inv[item] == 0) return 0;
    if (it->hp && pl.hp >= pl.maxhp && !it->mp) return 0;
    if (it->mp && !it->hp && pl.mp >= pl.maxmp) return 0;
    if (pl.pot_cd) { toast_show("Not yet! Potion cooldown."); return 0; }
    if (it->hp >= 900) { pl.hp = pl.maxhp; pl.mp = pl.maxmp; player_heal(0, 0); }
    else player_heal(it->hp, it->mp);
    pl.inv[item]--;
    pl.pot_cd = (u8)(it->hp >= 900 ? 240 : 150);
    snd_sfx(SFX_POTION);
    return 1;
}

void player_add_gold(int g) {
    int v = pl.gold + g;
    pl.gold = (u16)iclamp(v, 0, 9999);
}

void player_add_xp(int xp) {
    if (pl.level >= LEVEL_CAP) return;
    pl.xp += (u32)xp;
    int lv = 0;
    while (pl.level < LEVEL_CAP && pl.xp >= (u32)xp_for_level(pl.level)) {
        pl.xp -= (u32)xp_for_level(pl.level);
        pl.level++;
        lv++;
    }
    if (lv) {
        pl.stat_pts = (u8)(pl.stat_pts + lv * STAT_PTS_PER_LEVEL);
        pl.pass_pts = (u8)(pl.pass_pts + pass_points_for(pl.level - lv, pl.level));
        player_recalc();
        pl.hp = pl.maxhp;
        pl.mp = pl.maxmp;
        level_up_pending = 1;
    }
}

/* base damage of a hit (no crit) and the full version used by melee/skills */
int player_base_dmg(int mult16) {
    int base = (pl.atk * mult16) >> 4;
    int d = base + rnd_n(base / 4 + 1);
    return d < 1 ? 1 : d;
}

int player_calc_dmg(int mult16, int *crit) {
    int d = player_base_dmg(mult16);
    int c = player_roll_crit();
    if (crit) *crit = c;
    if (c) d = (d * 7) >> 2;
    return d;
}

/* ------------------------------------------------------------------ hurt / death */
/* defense is a percentage: 0 DEF takes full damage, 40 DEF takes ~44%. Fixed-point, no float. */
int player_mitigate(int dmg) {
    dmg = (dmg * 64) / (64 + pl.def * 2);
    return dmg < 1 ? 1 : dmg;
}

void player_hurt(int dmg, int fx, int fy) {
    if (pl.invuln || pl.state == PS_DEAD || pl.state == PS_ROLL || (pl.state == PS_SKILL && pl.cls == CLS_ROGUE && pl.t < 14)) return;
    if (pass_dodge()) return;
    dmg = pass_reduce(player_mitigate(dmg));
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
    combat_reset();
    if (pl.hp <= 0) {
        pl.hp = 0;
        pl.state = PS_DEAD;
        pl.t = 0;
        snd_stop_music();
        return;
    }
    pl.state = PS_HURT;
    pl.t = 0;
}

/* ------------------------------------------------------------------ movement helpers */
void player_move_axis(int vx, int vy) {
    int nx = (int)(pl.x + vx), ny = (int)(pl.y + vy);
    if (vx && box_free(nx >> 8, (int)(pl.y >> 8), 5, 6)) pl.x = nx;
    if (vy && box_free((int)(pl.x >> 8), ny >> 8, 5, 6)) pl.y = ny;
}

static void apply_knock(void) {
    if (pl.kx || pl.ky) {
        player_move_axis(pl.kx, pl.ky);
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

/* pace: ~0.8 px/frame at start, ~1.6 at high DEX (the old 0.9..2.0 felt too fast) */
static int move_speed(void) { return 200 + imin(pl.spd, 24) * 8; }

int player_body_x(void) { return (int)(pl.x >> 8); }
int player_body_y(void) { return (int)(pl.y >> 8) - 7; }

/* skills ---------------------------------------------------------------- */
static void new_hit_id(void) {
    pl.atk_id++;
    if (!pl.atk_id) pl.atk_id = 1;
}

static void start_skill(void) {
    const SkillDef *sk = &skill_defs[pl.cls];
    if (pl.skill_cd || pl.mp < sk->cost) {
        if (pl.mp < sk->cost) snd_sfx(SFX_ERROR);
        return;
    }
    pl.mp -= sk->cost;
    pl.state = PS_SKILL;
    pl.t = 0;
    new_hit_id();
    pl.skill_cd = sk->cd;
    if (pl.cls == CLS_KNIGHT) { snd_sfx(SFX_SPIN); pl.invuln = imax(pl.invuln, 10); }
    if (pl.cls == CLS_MAGE) snd_sfx(SFX_MAGIC);
    if (pl.cls == CLS_RANGER) {                            /* hop backwards to open some distance */
        snd_sfx(SFX_DASH);
        int dx, dy;
        if (read_dir(&dx, &dy)) face(dx, dy);
        pl.dash_x = (s16)(-dxv[pl.dir] * 3 * 256);
        pl.dash_y = (s16)(-dyv[pl.dir] * 3 * 256);
        pl.invuln = imax(pl.invuln, 8);
    }
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
    int bx = player_body_x(), by = player_body_y();
    switch (pl.cls) {
    case CLS_KNIGHT:                                       /* Blade Storm: four spins, then the slam */
        if (t == 4 || t == 10 || t == 16 || t == 22) {
            new_hit_id();
            combat_melee(22, 14, 20, 1, pl.atk_id, &skill_fx);          /* blade length: ~30px with the enemy's body */
            part_burst(PK_DUST, bx, (int)(pl.y >> 8) - 2, 3, 70, 14);
            if (t == 10 || t == 22) snd_sfx(SFX_SLASH2);
        }
        if (t == 28) {
            new_hit_id();
            combat_melee(28, 0, 52, 1, pl.atk_id, &skill_slam);          /* slam: ~36px, was 58px (3.5 tiles) */
            snd_sfx(SFX_EXPLODE);
            shake_add(5);
            part_spawn(PK_SHOCK, bx, (int)(pl.y >> 8) - 4, 0, 0, 14, 0);
            part_burst(PK_DUST, bx, (int)(pl.y >> 8) - 2, 8, 100, 16);
        }
        break;
    case CLS_MAGE:                                         /* Inferno: nova, then two fans of fire */
        if (t == 6) {
            new_hit_id();
            combat_melee(30, 0, 22, 1, pl.atk_id, &skill_fire);
            part_spawn(PK_SHOCK, bx, (int)(pl.y >> 8) - 4, 0, 0, 14, 0);
            part_burst(PK_EMBER, bx, by, 6, 90, 16);
            snd_sfx(SFX_FIRE);
        }
        if (t == 11 || t == 19) {
            int base = dir_angle(pl.dir);
            int big = t == 11;
            snd_sfx(SFX_FIRE);
            proj_spawn_ex(PJ_FIRE, 0, bx + dxv[pl.dir] * 10, by + dyv[pl.dir] * 10, base, 2 * 256 + 140, player_base_dmg(big ? 44 : 28), big ? 2 : 1, HK_FIRE, 0);
            for (int k = -1; k <= 1; k += 2)
                proj_spawn_ex(PJ_FIRE, 0, bx + dxv[pl.dir] * 8, by + dyv[pl.dir] * 8, (base + k * 13) & 255, 2 * 256 + 60, player_base_dmg(big ? 26 : 18), 1, HK_FIRE, 0);
        }
        break;
    case CLS_RANGER:                                       /* Volley Step */
        if (t < 7) {
            player_move_axis(pl.dash_x, pl.dash_y);
            if (t & 1) ghost_spawn(bx, (int)(pl.y >> 8), pl.dir, pl.dir);
        }
        if (t == 8) {
            int base = dir_angle(pl.dir);
            for (int k = -2; k <= 2; k++) combat_fire_arrow((base + k * 7) & 255, 12, 4 * 256);
            shake_add(1);
        }
        if (t == 16) {
            int base = dir_angle(pl.dir);
            snd_sfx(SFX_ARROW);
            for (int k = -1; k <= 1; k++)
                proj_spawn_ex(PJ_ARROW, 0, bx + dxv[pl.dir] * 8, by + dyv[pl.dir] * 8, (base + k * 3) & 255, 5 * 256, player_base_dmg(22), 2, HK_ARROW, 1);
            shake_add(2);
        }
        break;
    case CLS_ROGUE:                                        /* Shadow Strike: dash through, then cut both ways */
        if (t < 11) {
            player_move_axis(pl.dash_x, pl.dash_y);
            if (t & 1) ghost_spawn(bx, (int)(pl.y >> 8), pl.dir, pl.dir);
            combat_melee(11, 12, 22, 1, pl.atk_id, &skill_fx);
        }
        if (t == 12 || t == 18) {
            new_hit_id();
            snd_sfx(SFX_SLASH2);
            combat_melee(22, 0, t == 12 ? 34 : 40, 1, pl.atk_id, &skill_cut);   /* the two crescents reach ~30px */
            shake_add(2);
            part_burst(PK_HIT, bx, by, 2, 40, 8);
            ghost_spawn(bx, (int)(pl.y >> 8), pl.dir, pl.dir);
        }
        break;
    }
    if (t >= skill_defs[pl.cls].total) {
        pl.state = PS_IDLE;
        pl.idle_t = 0;
    }
}

/* roll ------------------------------------------------------------------ */
static void start_roll(void) {
    combat_reset();
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
    part_burst(PK_DUST, player_body_x(), (int)(pl.y >> 8) - 1, 3, 40, 16);
}

static void roll_update(void) {
    int t = pl.t++;
    int sp = 2 * 256 + 190 - t * 19;
    if (sp < 110) sp = 110;
    if (pl.dash_x == 0 && pl.dash_y == 0) { pl.dash_x = (s16)(dxv[pl.dir] * 256); pl.dash_y = (s16)(dyv[pl.dir] * 256); }
    player_move_axis((pl.dash_x * sp) >> 8, (pl.dash_y * sp) >> 8);
    pl.anim_t++;
    if ((t & 3) == 0) part_spawn(PK_DUST, player_body_x() - 3 + rnd_n(6), (int)(pl.y >> 8) - 1, 0, -10, 12, 0);
    if (pl.t >= 14) {
        pl.state = PS_IDLE;
        pl.idle_t = 0;
    }
}

int player_try_roll(void) {
    if (pl.roll_cd) return 0;
    start_roll();
    return 1;
}

/* ------------------------------------------------------------------ interaction */
static void unlock_door(int flag, int key, const char *msg) {
    pl.inv[key]--;
    gflags |= flag;
    if (flag == GF_DOOR_OPEN) world_unlock_door(TF_DOOR);
    else world_unlock_door(TF_DOOR2);
    snd_sfx(SFX_DOOR);
    shake_add(2);
    toast_show(msg);
}

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
            int item = item_chest_resolve(c->item, pl.cls);
            player_give(item, c->qty);
            snd_sfx(SFX_CHEST);
            item_get_show(item, c->qty);
            pl.state = PS_ITEM;
            pl.t = 0;
            return 1;
        }
    }
    if (f & TF_DOOR) {
        if (pl.inv[ITEM_CRYPT_KEY]) unlock_door(GF_DOOR_OPEN, ITEM_CRYPT_KEY, "The iron door creaks open!");
        else {
            snd_sfx(SFX_ERROR);
            play_dialogue("", "The iron door is locked. Ranger Bram, camped in the south-west of the woods, keeps the key.");
        }
        return 1;
    }
    if (f & TF_DOOR2) {
        if (pl.inv[ITEM_SKULL_KEY]) unlock_door(GF_SKULL_OPEN, ITEM_SKULL_KEY, "The skull seal shatters!");
        else {
            snd_sfx(SFX_ERROR);
            play_dialogue("", "A skull-shaped lock. Whatever guards its key lies in the depths below the crypt.");
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
    if (!pl.pot_cd) snd_sfx(SFX_ERROR);
}

static void idle_update(void) {
    int dx, dy;
    int moving = read_dir(&dx, &dy);
    if (moving) {
        face(dx, dy);
        int sp = move_speed();
        int vx = dx * sp, vy = dy * sp;
        if (dx && dy) { vx = (vx * 181) >> 8; vy = (vy * 181) >> 8; }
        player_move_axis(vx, vy);
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
        combat_start();
    } else if (pl.a_held >= HOLD_ATTACK_FRAMES) {
        combat_start();
    }
}

void player_update(void) {
    pl.playtime++;
    if (pl.invuln) pl.invuln--;
    if (pl.flash) pl.flash--;
    if (pl.roll_cd) pl.roll_cd--;
    if (pl.skill_cd) pl.skill_cd--;
    if (pl.pot_cd) pl.pot_cd--;
    combat_tick();
    apply_knock();
    switch (pl.state) {
    case PS_IDLE: idle_update(); break;
    case PS_ATTACK: combat_update(); break;
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
    case PS_ATTACK:
        f = combat_hero_frame(flip);
        break;
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

/* weapon overlay while a skill is active (normal attacks are drawn by combat_draw) */
static void draw_skill_weapon(int sx, int sy) {
    int key = (int)(pl.y >> 8);
    int wt = OT_WEAPON + pl.cls * 16;
    int cx = sx, cy = sy - 7;
    int center = dir_angle(pl.dir);
    int t = pl.t;
    if (pl.cls == CLS_KNIGHT) {
        int ang = t < 26 ? (t * 24) & 255 : 128;           /* the blade points down for the slam */
        spr_add_aff(cx - 16, cy - 16, SH_SQUARE, 2, wt, PB_WEAPON, 2, 0, key + 2, ang, t >= 26 ? 232 : 256);
        if (t < 26) {
            spr_add_aff(cx - 16, cy - 16, SH_SQUARE, 2, OT_ARC, PB_WEAPON, 2, SF_ALPHA, key + 3, (ang + 4) & 255, 256);
            spr_add_aff(cx - 16, cy - 16, SH_SQUARE, 2, OT_ARC, PB_WEAPON, 2, SF_ALPHA, key + 4, (ang + 128 + 4) & 255, 256);
        }
    } else if (pl.cls == CLS_MAGE) {
        int ang = center + (t < 10 ? -20 + t * 2 : 0);
        spr_add_aff(cx + dxv[pl.dir] * 4 - 16, cy + dyv[pl.dir] * 4 - 16, SH_SQUARE, 2, wt, PB_WEAPON, 2, 0, key + 1, ang & 255, 256);
        if (t < 22)
            spr_add(cx + dxv[pl.dir] * 12 - 4 + (t & 1), cy + dyv[pl.dir] * 12 - 4, SH_SQUARE, 0, OT_PART + PT_SPARKLE + ((t / 3) & 3), PB_FX, 2, 0, key + 5);
    } else if (pl.cls == CLS_RANGER) {
        spr_add_aff(cx + dxv[pl.dir] * 4 - 16, cy + dyv[pl.dir] * 4 - 16, SH_SQUARE, 2, wt, PB_WEAPON, 2, 0, key + 1, center, 256);
    } else if (pl.cls == CLS_ROGUE && t >= 11) {
        /* two crescent cuts: one ahead, one behind */
        int flick = (t == 12 || t == 13 || t == 18 || t == 19);
        if (flick) {
            spr_add_aff(cx - 16, cy - 16, SH_SQUARE, 2, OT_ARC, PB_WEAPON, 2, SF_ALPHA, key + 3, center, 256);
            spr_add_aff(cx - 16, cy - 16, SH_SQUARE, 2, OT_ARC, PB_WEAPON, 2, SF_ALPHA, key + 4, (center + 128) & 255, 256);
        }
        spr_add_aff(cx + dxv[pl.dir] * 5 - 16, cy + dyv[pl.dir] * 5 - 16, SH_SQUARE, 2, wt, PB_WEAPON, 2, 0, key + 2, (center + (t & 4 ? 40 : -40)) & 255, 256);
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
        if (f & (TF_SIGN | TF_CHEST | TF_DOOR | TF_DOOR2)) { tx = (fx >> 4) * 16 + 8; ty = (fy >> 4) * 16 - 4; }
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
    if (!blink || pl.state == PS_ATTACK || pl.state == PS_SKILL) {
        if (pl.state == PS_ATTACK) combat_draw(sx, sy);
        else if (pl.state == PS_SKILL) draw_skill_weapon(sx, sy);
    }
    draw_prompt(sx, sy);
}
