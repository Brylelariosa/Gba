/* combat.c - the attack chain.
   Every class owns a table of COMBO_STEPS steps. Each step has its own timing, damage, reach, swing style,
   hit feedback and swing sound, so tapping (or holding) A produces a visibly different move every time:
     Knight: slash, reverse slash, thrust, overhead slam      Rogue: stab, stab, cross cut, flurry
     Mage:   bolt, bolt, triple fan, piercing arcane blast    Ranger: shot, shot, twin shot, piercing shot
   A press buffered during a step chains as soon as the strike is over (cancelling most of the recovery);
   holding A keeps the chain going on its own. DEX shortens every phase. */
#include "game.h"
#include "assets.h"
#include "audio_data.h"

static const s8 dxv[4] = { 0, 0, -1, 1 };
static const s8 dyv[4] = { 1, -1, 0, 0 };

enum { CS_SLASH, CS_SLASH_REV, CS_THRUST, CS_STAB, CS_CROSS, CS_OVERHEAD, CS_SPIN, CS_CAST, CS_SHOT };
enum { CF_FINISH = 1, CF_FAN = 2, CF_TWIN = 4, CF_BLAST = 8, CF_PIERCE = 16, CF_RADIAL = 32 };

typedef struct {
    u8 windup, active, recover;     /* frames at 1.0x attack speed */
    u8 chain;                       /* frames after the strike before a queued press may chain */
    u8 reach, arc;                  /* melee reach (px, 0 = projectile) and cone width in 1/16 of reach */
    u8 mult, lunge;                 /* damage multiplier /16 (per hit tick), forward lunge */
    u8 style, hk;                   /* CS_*, HK_* */
    u8 kb, shake, stop;             /* knockback /16, screen shake, hit-stop frames */
    u8 sfx;                         /* swing sound */
    u8 flags;                       /* CF_* */
} Step;

#define COMBO_WINDOW 24             /* frames after a step in which the next press still continues the chain */

static const Step combo[CLS_COUNT][COMBO_STEPS] = {
    [CLS_KNIGHT] = {
        { 3, 4, 7, 2, 24, 12, 16, 1, CS_SLASH,     HK_SLASH, 16, 1, 3, SFX_SWING1, 0 },
        { 3, 4, 7, 2, 24, 12, 18, 1, CS_SLASH_REV, HK_SLASH, 16, 1, 3, SFX_SWING1, 0 },
        { 5, 4, 9, 2, 30, 6, 22, 3,  CS_THRUST,    HK_STAB,  24, 2, 4, SFX_THRUST, 0 },
        { 8, 5, 16, 0, 28, 16, 34, 2, CS_OVERHEAD, HK_SMASH, 44, 4, 6, SFX_SMASH, CF_FINISH },
    },
    [CLS_MAGE] = {
        { 5, 2, 8, 2, 0, 0, 15, 0, CS_CAST, HK_MAGIC, 16, 0, 2, SFX_ZAP, 0 },
        { 4, 2, 8, 2, 0, 0, 15, 0, CS_CAST, HK_MAGIC, 16, 0, 2, SFX_ZAP, 0 },
        { 6, 2, 11, 2, 0, 0, 10, 0, CS_CAST, HK_MAGIC, 16, 1, 2, SFX_ZAP, CF_FAN },
        { 10, 2, 18, 0, 0, 0, 30, 0, CS_CAST, HK_MAGIC, 36, 3, 5, SFX_ZAP2, CF_FINISH | CF_BLAST },
    },
    [CLS_RANGER] = {
        { 5, 2, 6, 2, 0, 0, 15, 0, CS_SHOT, HK_ARROW, 16, 0, 2, SFX_BOW, 0 },
        { 4, 2, 6, 2, 0, 0, 15, 0, CS_SHOT, HK_ARROW, 16, 0, 2, SFX_BOW, 0 },
        { 5, 2, 8, 2, 0, 0, 13, 0, CS_SHOT, HK_ARROW, 16, 1, 2, SFX_BOW, CF_TWIN },
        { 9, 2, 16, 0, 0, 0, 30, 0, CS_SHOT, HK_ARROW, 36, 3, 5, SFX_BOW2, CF_FINISH | CF_PIERCE },
    },
    [CLS_ROGUE] = {
        { 2, 3, 5, 1, 19, 12, 12, 2, CS_STAB,  HK_STAB,  12, 1, 2, SFX_STAB, 0 },
        { 2, 3, 5, 1, 19, 12, 13, 2, CS_STAB,  HK_STAB,  12, 1, 2, SFX_STAB, 0 },
        { 3, 5, 6, 2, 24, 14, 9, 2,  CS_CROSS, HK_SLASH, 16, 1, 3, SFX_SWING1, 0 },
        { 4, 9, 12, 0, 26, 16, 11, 0, CS_SPIN, HK_SLASH, 20, 3, 4, SFX_FLURRY, CF_FINISH | CF_RADIAL },
    },
};

static const Step *cur_step(void) { return &combo[pl.cls][pl.combo]; }
int combat_step_is_finisher(void) { return (cur_step()->flags & CF_FINISH) != 0; }

/* number of separate hit ticks in a step (each re-arms the swing id) */
static int ticks(const Step *s) { return s->style == CS_SPIN ? 3 : s->style == CS_CROSS ? 2 : 1; }

static int scl(int frames) {
    int v = (frames * 256 * 9 / 8 + (pl.aspd >> 1)) / pl.aspd;      /* 9/8: every swing is ~12% slower than the raw table */
    return v < 1 ? 1 : v;
}

static void next_swing_id(void) {
    pl.atk_id++;
    if (pl.atk_id == 0) pl.atk_id = 1;
}

/* ------------------------------------------------------------------ state */
void combat_reset(void) {
    pl.combo = 0;
    pl.combo_t = 0;
    pl.chain_n = 0;
    pl.buffered = 0;
}

void combat_tick(void) {
    if (keys_held & KEY_A) { if (pl.a_held < 255) pl.a_held++; }
    else pl.a_held = 0;
    if (pl.chain_flash) pl.chain_flash--;
    if (pl.state != PS_ATTACK && pl.combo_t) {
        if (--pl.combo_t == 0) combat_reset();
    }
}

void combat_on_hit(const HitFx *fx) {
    (void)fx;
    if (pl.chain_n < 99) pl.chain_n++;
    pl.chain_flash = 28;
}

/* swing arc of the current step, set when the step begins */
static int swing_a0, swing_a1;

static void begin_step(int c) {
    if (c < 0 || c >= COMBO_STEPS) c = 0;
    const Step *s = &combo[pl.cls][c];
    pl.combo = (u8)c;
    pl.state = PS_ATTACK;
    pl.t = 0;
    pl.buffered = 0;
    pl.combo_t = 0;
    next_swing_id();
    pl.step_w = (u8)scl(s->windup);
    pl.step_a = (u8)scl(s->active);
    pl.step_r = (u8)scl(s->recover);
    pl.step_c = s->chain;
    int center = dir_angle(pl.dir);
    switch (s->style) {
    case CS_SLASH: swing_a0 = center - 58; swing_a1 = center + 58; break;
    case CS_SLASH_REV: swing_a0 = center + 58; swing_a1 = center - 58; break;
    case CS_CROSS: swing_a0 = center - 46; swing_a1 = center + 46; break;
    case CS_OVERHEAD: swing_a0 = center - 112; swing_a1 = center + 6; break;
    default: swing_a0 = center; swing_a1 = center; break;
    }
}

int combat_start(void) {
    int c = pl.combo_t ? pl.combo : 0;
    begin_step(c);
    return 1;
}

/* ------------------------------------------------------------------ hitting */
static int in_reach(const Enemy *e, int reach, int arc) {
    int bx = player_body_x(), by = player_body_y();
    int dx = (int)(e->x >> 8) - bx, dy = (int)(e->y >> 8) - 6 - by;
    int fx = dxv[pl.dir], fy = dyv[pl.dir];
    int along = dx * fx + dy * fy;
    int perp = iabs(dx * fy - dy * fx);
    int er = e->type >= EN_BOSS ? 14 : 8;
    return along > -5 && along <= reach + er && perp <= ((reach * arc) >> 4) + er;
}

int combat_melee(int reach, int arc, int mult, int radial, int id, const HitFx *fx) {
    int bx = player_body_x(), by = player_body_y();
    int n = 0;
    for (int i = 0; i < MAX_EN; i++) {
        Enemy *e = &en[i];
        if (!e->active || e->dying || e->hitid == id) continue;
        int ok;
        if (radial) {
            int dx = (int)(e->x >> 8) - bx, dy = (int)(e->y >> 8) - 6 - by;
            ok = idist(dx, dy) <= reach + (e->type >= EN_BOSS ? 14 : 8);
        } else ok = in_reach(e, reach, arc);
        if (!ok) continue;
        e->hitid = (u8)id;
        int crit;
        int d = player_calc_dmg(mult, &crit);
        enemy_damage(e, d, (int)(e->x >> 8) - bx, (int)(e->y >> 8) - 6 - by, crit, fx);
        n++;
    }
    return n;
}

static void bush_cut(int reach) {
    int px = player_body_x() + dxv[pl.dir] * (reach - 6), py = (int)(pl.y >> 8) - 4 + dyv[pl.dir] * (reach - 6);
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

/* ------------------------------------------------------------------ projectiles */
void combat_fire_arrow(int angle, int mult, int spd) {
    int d = player_base_dmg(mult);
    proj_spawn_ex(PJ_ARROW, 0, player_body_x() + dxv[pl.dir] * 8, player_body_y() + dyv[pl.dir] * 8, angle, spd, d, 0, HK_ARROW, 0);
}

static void fire_step_projectiles(const Step *s) {
    int bx = player_body_x() + dxv[pl.dir] * 9, by = player_body_y() + dyv[pl.dir] * 9;
    int center = dir_angle(pl.dir);
    int px = dyv[pl.dir], py = dxv[pl.dir];          /* perpendicular, for side-by-side shots */
    if (pl.cls == CLS_MAGE) {
        if (s->flags & CF_BLAST) {
            proj_spawn_ex(PJ_SHARD, 0, bx, by, center, 3 * 256, player_base_dmg(s->mult), 3, HK_MAGIC, 1);
            part_burst(PK_SPARKLE, bx, by, 6, 70, 18);
        } else if (s->flags & CF_FAN) {
            for (int k = -1; k <= 1; k++)
                proj_spawn_ex(PJ_BOLT, 0, bx, by, (center + k * 10) & 255, 3 * 256 + 64, player_base_dmg(s->mult), 0, HK_MAGIC, 0);
        } else {
            proj_spawn_ex(PJ_BOLT, 0, bx, by, center, 3 * 256 + 64, player_base_dmg(s->mult), 0, HK_MAGIC, 0);
        }
    } else if (pl.cls == CLS_RANGER) {
        if (s->flags & CF_PIERCE) {
            proj_spawn_ex(PJ_ARROW, 0, bx, by, center, 5 * 256, player_base_dmg(s->mult), 3, HK_ARROW, 1);
        } else if (s->flags & CF_TWIN) {
            for (int k = -1; k <= 1; k += 2)
                proj_spawn_ex(PJ_ARROW, 0, bx + px * 4 * k, by + py * 4 * k, center, 4 * 256 + 64, player_base_dmg(s->mult), 0, HK_ARROW, 0);
        } else {
            proj_spawn_ex(PJ_ARROW, 0, bx, by, center, 4 * 256, player_base_dmg(s->mult), 0, HK_ARROW, 0);
        }
    }
}

/* ------------------------------------------------------------------ update */
static void strike_fx(const Step *s) {
    int bx = player_body_x(), by = (int)(pl.y >> 8);
    int reach = s->reach;
    int tx = bx + dxv[pl.dir] * (reach - 6), ty = by - 4 + dyv[pl.dir] * (reach - 6);
    if (s->style == CS_OVERHEAD) {
        part_spawn_dir(PK_SHOCK, tx, ty, pl.dir, 14);
        part_burst(PK_DUST, tx, ty + 2, 6, 90, 18);
        shake_add(s->shake);
    }
}

void combat_update(void) {
    const Step *s = cur_step();
    int c = pl.combo;
    int fin = (s->flags & CF_FINISH) != 0;
    int w = pl.step_w, a = pl.step_a;
    int total = w + a + pl.step_r;
    int act_end = w + a;
    pl.t++;
    int t = pl.t;

    /* swing sound just before the strike so the whoosh leads the impact */
    if (t == imax(1, w - 1)) snd_sfx(s->sfx);

    /* projectile steps fire once, on the first strike frame */
    if (!s->reach && t == w) {
        fire_step_projectiles(s);
        if (s->flags & CF_FINISH) shake_add(s->shake);
    }

    if (s->reach && t >= w && t < act_end) {
        HitFx fx = { s->hk, (u8)((s->flags & CF_FINISH) ? 1 : 0), (u8)fin, s->kb, s->shake, s->stop };
        int n = ticks(s);
        int dmg_now = 1;
        if (s->style == CS_OVERHEAD) dmg_now = (t >= imax(w, act_end - 2));   /* the blade lands at the end of the arc */
        if (n > 1) {
            int span = imax(1, a / n);
            int k = (t - w) / span, ph = (t - w) % span;
            if (k >= n) dmg_now = 0;
            else {
                if (ph == 0) next_swing_id();                                /* every tick may hit the same enemy again */
                dmg_now = ph <= 1;
            }
        }
        if (dmg_now) combat_melee(s->reach, s->arc, s->mult, (s->flags & CF_RADIAL) != 0, pl.atk_id, &fx);
        bush_cut(s->reach);
        if (s->lunge) player_move_axis(dxv[pl.dir] * s->lunge * 128, dyv[pl.dir] * s->lunge * 128);
        if (s->style == CS_OVERHEAD && t == imax(w, act_end - 2)) strike_fx(s);
        if (s->style == CS_SPIN && ((t - w) % 3) == 0) part_burst(PK_DUST, player_body_x(), (int)(pl.y >> 8) - 2, 2, 70, 12);
    }

    /* input: a press during the step is remembered; holding A also counts (see HOLD_ATTACK_FRAMES) */
    if (keys_down & KEY_A) pl.buffered = 1;
    int want = pl.buffered || pl.a_held >= HOLD_ATTACK_FRAMES;

    if (t >= act_end && (keys_down & KEY_R) && pl.roll_cd == 0) {
        combat_reset();
        player_try_roll();
        return;
    }
    if (!fin && t >= act_end + pl.step_c && want) {
        begin_step(c + 1);
        return;
    }
    if (t >= total) {
        if (fin) {
            pl.combo = 0;
            pl.combo_t = COMBO_WINDOW;
            if (want) { begin_step(0); return; }
        } else {
            pl.combo = (u8)(c + 1);
            pl.combo_t = COMBO_WINDOW;
        }
        pl.state = PS_IDLE;
        pl.idle_t = 0;
    }
}

/* ------------------------------------------------------------------ hero pose */
int combat_hero_frame(int *flip) {
    const Step *s = cur_step();
    int d = pl.dir;
    int base_atk = d == DIR_DOWN ? HF_ATK_DOWN : d == DIR_UP ? HF_ATK_UP : HF_ATK_SIDE;
    int hi = d == DIR_DOWN ? HF_HIGH_DOWN : d == DIR_UP ? HF_HIGH_UP : HF_HIGH_SIDE;
    int slam = d == DIR_DOWN ? HF_SLAM_DOWN : d == DIR_UP ? HF_SLAM_UP : HF_SLAM_SIDE;
    int thr = d == DIR_DOWN ? HF_THRUST_DOWN : d == DIR_UP ? HF_THRUST_UP : HF_THRUST_SIDE;
    int t = pl.t, w = pl.step_w, a = pl.step_a;
    int ph = t < w ? 0 : (t < w + a ? 1 : 2);
    *flip = d == DIR_RIGHT;
    switch (s->style) {
    case CS_THRUST:
    case CS_STAB:
        return ph == 1 ? thr : base_atk + ph;
    case CS_OVERHEAD:
        if (ph == 0) return hi;
        if (ph == 1 || t < w + a + 6) return slam;
        return base_atk + 2;
    case CS_SPIN: {
        static const u8 order[4] = { HF_ATK_DOWN + 1, HF_ATK_SIDE + 1, HF_ATK_UP + 1, HF_ATK_SIDE + 1 };
        int k = (t / 2) & 3;
        *flip = (k == 3);
        return ph == 2 ? base_atk + 2 : order[k];
    }
    default:
        return base_atk + ph;
    }
}

/* ------------------------------------------------------------------ drawing */
static int sweep_angle(int num, int den, int a0, int a1) {
    /* ease-out interpolation between a0 and a1 */
    int p = (num * 256) / (den ? den : 1);
    p = iclamp(p, 0, 256);
    int e = 256 - (((256 - p) * (256 - p)) >> 8);
    return a0 + (((a1 - a0) * e) >> 8);
}

static void weapon_spr(int x, int y, int ang, int key, int inv_scale) {
    spr_add_aff(x - 16, y - 16, SH_SQUARE, 2, OT_WEAPON + pl.cls * 16, PB_WEAPON, 2, 0, key, ang & 255, inv_scale);
}

static void trail_spr(int x, int y, int ang, int key, int flip) {
    spr_add_aff(x - 16, y - 16, SH_SQUARE, 2, OT_ARC, PB_WEAPON, 2, (flip ? SF_HFLIP : 0) | SF_ALPHA, key, ang & 255, 256);
}

void combat_draw(int sx, int sy) {
    const Step *s = cur_step();
    int key = (int)(pl.y >> 8);
    int wkey = pl.dir == DIR_UP ? key - 1 : key + 1;
    int tkey = pl.dir == DIR_UP ? key - 2 : key + 2;
    int cx = sx, cy = sy - 7;
    int fx = dxv[pl.dir], fy = dyv[pl.dir];
    int center = dir_angle(pl.dir);
    int t = pl.t, w = pl.step_w, a = pl.step_a;
    int fwd = 3;
    int rev = swing_a0 > swing_a1;
    int pull = rev ? 10 : -10;

    switch (s->style) {
    case CS_SLASH:
    case CS_SLASH_REV: {
        int ang;
        if (t < w) ang = swing_a0 + pull * t / imax(w, 1);
        else ang = sweep_angle(t - w, a + 2, swing_a0 + pull, swing_a1);
        if (t >= w + a + 8) return;
        weapon_spr(cx + fx * fwd, cy + fy * fwd, ang, wkey, 256);
        if (t >= w - 1 && t < w + a + 3) trail_spr(cx + fx * fwd, cy + fy * fwd, ang + (rev ? 6 : -6), tkey, rev);
        break;
    }
    case CS_CROSS: {
        /* two quick cuts: first sweeps one way, the second comes back */
        int half = imax(1, a / 2);
        int ang, flipt;
        if (t < w) { ang = swing_a0 - 10 + 10 * t / imax(w, 1); flipt = 0; }
        else if (t < w + half) { ang = sweep_angle(t - w, half + 1, swing_a0, swing_a1); flipt = 0; }
        else { ang = sweep_angle(t - w - half, a - half + 2, swing_a1, swing_a0); flipt = 1; }
        if (t >= w + a + 5) return;
        weapon_spr(cx + fx * fwd, cy + fy * fwd, ang, wkey, 256);
        if (t >= w - 1 && t < w + a + 3) trail_spr(cx + fx * fwd, cy + fy * fwd, ang + (flipt ? 6 : -6), tkey, flipt);
        break;
    }
    case CS_THRUST:
    case CS_STAB: {
        int ext_max = s->style == CS_THRUST ? 6 : 3;
        int ext;
        if (t < w) ext = -3 + 3 * t / imax(w, 1);
        else if (t < w + a) ext = ext_max;
        else ext = ext_max - (t - w - a) * 2;
        if (ext < -3) return;
        if (t >= w + a + 6) return;
        int side = (s->style == CS_STAB) ? ((pl.combo & 1) ? -3 : 3) : 0;      /* alternate hands */
        int ox = fx * (fwd + ext) + fy * side, oy = fy * (fwd + ext) + fx * side;
        weapon_spr(cx + ox, cy + oy, center, wkey, 256);
        if (t >= w && t < w + a + 2)
            spr_add_aff(cx + ox - fx * 12 - 16, cy + oy - fy * 12 - 16, SH_SQUARE, 2, OT_STREAK, PB_WEAPON, 2, SF_ALPHA, tkey, center, 256);
        break;
    }
    case CS_OVERHEAD: {
        /* raised above the head, then brought down in front: the pivot travels with the swing */
        int ang, ox, oy;
        if (t >= w + a + 12) return;
        if (t < w) {
            ang = swing_a0 - 16 * t / imax(w, 1);
            ox = 0;
            oy = -4 - 4 * t / imax(w, 1);
        } else {
            int p = imin(256, ((t - w) * 256) / imax(a, 1));
            ang = sweep_angle(t - w, a, swing_a0 - 16, swing_a1);
            ox = (fx * 7 * p) >> 8;
            oy = -8 + (((fy * 7 + 8) * p) >> 8);
        }
        weapon_spr(cx + ox, cy + oy, ang, wkey, 232);     /* a touch larger: it is a heavy blow */
        if (t >= w && t < w + a + 4) trail_spr(cx + ox, cy + oy, ang - 6, tkey, 0);
        break;
    }
    case CS_SPIN: {
        if (t >= w + a + 4) return;
        int ang = (t * 36) & 255;
        weapon_spr(cx, cy, ang, key + 2, 256);
        if (t >= w) {
            trail_spr(cx, cy, ang + 4, key + 3, 0);
            trail_spr(cx, cy, ang + 128 + 4, key + 4, 0);
        }
        break;
    }
    case CS_CAST: {
        int bx = cx + fx * (fwd + (t >= w ? 2 : 0)), by = cy + fy * (fwd + (t >= w ? 2 : 0));
        int pullv = t < w ? t : (t < w + a ? w : w - (t - w - a) / 2);
        int ang = center + (t < w ? -(24 - pullv * 3) : 0);
        if (t >= w + a + 6) ang = center + 20;
        weapon_spr(bx, by, ang, wkey, 256);
        /* charge glow gathers at the staff tip; the finisher gathers more */
        if (t < w + a + 2 && (t & 1)) {
            int n = (s->flags & CF_BLAST) ? 3 : 1;
            for (int k = 0; k < n; k++)
                spr_add(cx + fx * 13 - 4 + ((t * 3 + k * 5) % 9) - 4, cy + fy * 13 - 4 + ((t * 5 + k * 7) % 9) - 4, SH_SQUARE, 0,
                        OT_PART + PT_SPARKLE + ((t / 2 + k) & 3), PB_FX, 2, 0, key + 5 + k);
        }
        break;
    }
    default: {                                                                 /* CS_SHOT */
        int back = t < w ? -2 : 3;
        if (t >= w + a + 6) { back = 1; }
        int ox = fx * (fwd + back), oy = fy * (fwd + back);
        weapon_spr(cx + ox, cy + oy, center, wkey, (s->flags & CF_PIERCE) && t < w ? 236 : 256);
        break;
    }
    }
}
