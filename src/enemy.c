#include "game.h"
#include "assets.h"
#include "audio_data.h"

Enemy en[MAX_EN];
int boss_dead_flag;

enum { S_IDLE, S_ALERT, S_HOP, S_WINDUP, S_ATTACK, S_RECOVER, S_CHARGE, S_TIRED, S_CHASE, S_SWOOP, S_RETREAT,
       S_DORMANT, S_INTRO, S_FAN, S_SUMMON, S_SLAM_UP, S_SLAM_DOWN, S_RING, S_DEAD,
       S_PHASE, S_SWEEP, S_RAIN, S_SWING, S_POUND, S_SPIN,
       S_CURTAIN,                                            /* the Hollow King: walls of orbs with a gap */
       S_BREATH, S_GEYSER, S_METEOR, S_RUSH, S_STOMP };       /* the Cinder Tyrant */

typedef struct { s16 hp; u8 atk, def; u16 xp; u8 glo, ghi, hw, hh; } EnInfo;

/* Stats by difficulty tier (0 = woods, 1 = crypt, 2 = depths). The tier of an enemy is fixed when it spawns,
   so the same slime/bat/wolf/skeleton hits harder and lasts longer the deeper the hero goes. */
#define BOSS_INFO    { 9000, 34, 20, 1500, 150, 200, 9, 12 }
#define WARDEN_INFO  { 3200, 30, 10, 520, 120, 160, 9, 10 }
#define TYRANT_INFO  { 7200, 40, 14, 2000, 200, 250, 10, 12 }
#define ENEMY_TIERS 6
static const EnInfo einfo[ENEMY_TIERS][EN_COUNT] = {
    { { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 18, 6, 0, 7, 1, 4, 5, 7 },       /* slime */
      { 10, 7, 0, 8, 1, 4, 5, 5 },       /* bat */
      { 44, 11, 1, 20, 4, 9, 6, 6 },     /* wolf */
      { 60, 13, 2, 28, 6, 12, 5, 7 },    /* skeleton */
      BOSS_INFO, WARDEN_INFO, TYRANT_INFO },
    { { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 90, 15, 3, 30, 4, 10, 5, 7 },
      { 64, 15, 2, 26, 2, 7, 5, 5 },
      { 160, 22, 4, 60, 8, 16, 6, 6 },
      { 170, 22, 5, 46, 8, 18, 5, 7 },
      BOSS_INFO, WARDEN_INFO, TYRANT_INFO },
    { { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 220, 26, 6, 60, 8, 16, 5, 7 },
      { 150, 26, 4, 50, 4, 10, 5, 5 },
      { 320, 33, 8, 110, 12, 24, 6, 6 },
      { 340, 33, 8, 84, 12, 26, 5, 7 },
      BOSS_INFO, WARDEN_INFO, TYRANT_INFO },
    { { 0, 0, 0, 0, 0, 0, 0, 0 },            /* the Cinder Hollows */
      { 340, 36, 9, 96, 10, 22, 5, 7 },      /* magma slime */
      { 230, 36, 6, 80, 6, 14, 5, 5 },       /* ember bat */
      { 470, 44, 11, 170, 16, 32, 6, 6 },    /* cinder hound */
      { 500, 44, 11, 130, 16, 34, 5, 7 },    /* charred skeleton */
      BOSS_INFO, WARDEN_INFO, TYRANT_INFO },
    { { 0, 0, 0, 0, 0, 0, 0, 0 },            /* Sunmeadow Road: opens after the Cinder Tyrant, levels 22-30 */
      { 520, 46, 13, 150, 16, 34, 5, 7 },
      { 340, 46, 9, 125, 10, 22, 5, 5 },
      { 700, 56, 15, 250, 24, 48, 6, 6 },
      { 740, 56, 15, 190, 24, 52, 5, 7 },
      BOSS_INFO, WARDEN_INFO, TYRANT_INFO },
    { { 0, 0, 0, 0, 0, 0, 0, 0 },            /* Amberwood: the last climb to the level cap */
      { 720, 56, 17, 205, 22, 46, 5, 7 },
      { 460, 56, 12, 170, 14, 30, 5, 5 },
      { 960, 68, 19, 330, 32, 64, 6, 6 },
      { 1000, 68, 19, 255, 32, 70, 5, 7 },
      BOSS_INFO, WARDEN_INFO, TYRANT_INFO },
};
static const u8 area_tier[AREA_COUNT] = { 0, 0, 1, 2, 3, 4, 5, 0 };   /* village woods crypt depths ember road amberwood amberfall (interiors: 0) */
/* behaviour (speed, wind-up times) stops scaling after the Cinder Hollows; only stats grow in the Amber lands */
#define TT(e) ((e)->tier > 3 ? 3 : (e)->tier)
#define ESPD(v) ((v) * 7 / 8)        /* global enemy pace: 12% slower than the raw numbers */

static const EnInfo *info(const Enemy *e) { return &einfo[e->tier < ENEMY_TIERS ? e->tier : ENEMY_TIERS - 1][e->type]; }
static int is_big(const Enemy *e) { return e->type >= EN_BOSS; }

static const s8 dxv[4] = { 0, 0, -1, 1 };
static const s8 dyv[4] = { 1, -1, 0, 0 };

static int ppx(void) { return (int)(pl.x >> 8); }
static int ppy(void) { return (int)(pl.y >> 8); }
static int ex(const Enemy *e) { return (int)(e->x >> 8); }
static int ey(const Enemy *e) { return (int)(e->y >> 8); }

static void hz_clear(void);

void enemies_clear(void) {
    for (int i = 0; i < MAX_EN; i++) { en[i].active = 0; en[i].type = EN_NONE; }
    boss_dead_flag = 0;
    hz_clear();
}

Enemy *enemy_spawn(int type, int tx, int ty) {
    for (int i = 0; i < MAX_EN; i++) {
        if (en[i].active) continue;
        Enemy *e = &en[i];
        u8 *p = (u8 *)e;
        for (unsigned k = 0; k < sizeof(*e); k++) p[k] = 0;
        e->type = (u8)type;
        e->tier = area_tier[cur_area_id < AREA_COUNT ? cur_area_id : 4];
        e->active = 1;
        e->x = (s32)(tx * 16 + 8) << 8;
        e->y = (s32)(ty * 16 + 15) << 8;
        e->hp = e->maxhp = info(e)->hp;
        e->state = is_big(e) ? S_DORMANT : S_IDLE;
        e->t = (u16)(20 + rnd_n(40));
        e->t2 = (u16)rnd_n(256);
        e->dir = DIR_DOWN;
        e->hitid = 0;
        return e;
    }
    return 0;
}

int enemies_alive(void) {
    int n = 0;
    for (int i = 0; i < MAX_EN; i++) if (en[i].active && !en[i].dying) n++;
    return n;
}

Enemy *enemy_boss(void) {
    for (int i = 0; i < MAX_EN; i++) if (en[i].active && en[i].type == EN_BOSS) return &en[i];
    return 0;
}

/* the boss-class enemy that is currently fighting (drives the boss health bar) */
Enemy *enemy_big(void) {
    for (int i = 0; i < MAX_EN; i++)
        if (en[i].active && is_big(&en[i]) && en[i].state != S_DORMANT) return &en[i];
    return 0;
}

static int count_type(int type) {
    int n = 0;
    for (int i = 0; i < MAX_EN; i++) if (en[i].active && !en[i].dying && en[i].type == type) n++;
    return n;
}

/* ------------------------------------------------------------------ helpers */
/* e->pad0 counts how long an enemy has been unable to move (an obstacle in its way); e->pad1 is the detour timer. */
static int en_move(Enemy *e, int vx, int vy) {
    const EnInfo *in = info(e);
    int moved = 0;
    int nx = (int)(e->x + vx), ny = (int)(e->y + vy);
    if (vx && box_free(nx >> 8, ey(e), in->hw, in->hh)) { e->x = nx; moved = 1; }
    if (vy && box_free(ex(e), ny >> 8, in->hw, in->hh)) { e->y = ny; moved = 1; }
    if (moved) { if (e->pad0) e->pad0--; }
    else if ((vx || vy) && e->pad0 < 250) e->pad0++;
    return moved;
}

/* A chasing enemy that has been blocked for a few frames (a tree trunk, a rock, a corner) slides sideways around the
   obstacle for a third of a second instead of pushing against it for ever. pad1: low 5 bits = frames left, bit 5 = side. */
static void chase_step(Enemy *e, int vx, int vy) {
    if (e->pad1 & 31) {
        int side = (e->pad1 & 32) ? 1 : -1;
        e->pad1--;
        if (!en_move(e, -vy * side, vx * side)) { e->pad1 = 0; en_move(e, vx, vy); }
        return;
    }
    e->pad1 = 0;
    en_move(e, vx, vy);
    if (e->pad0 > 10) { e->pad0 = 0; e->pad1 = (u8)(20 | (rnd_n(2) ? 32 : 0)); }
}

static void en_move_free(Enemy *e, int vx, int vy) {
    /* flying movement ignores obstacles except map bounds */
    int nx = (int)(e->x + vx), ny = (int)(e->y + vy);
    if ((nx >> 8) > 20 && (nx >> 8) < 492) e->x = nx;
    if ((ny >> 8) > 20 && (ny >> 8) < 492) e->y = ny;
}

static void face_toward(Enemy *e, int dx, int dy) {
    if (iabs(dx) > iabs(dy)) e->dir = (u8)(dx < 0 ? DIR_LEFT : DIR_RIGHT);
    else e->dir = (u8)(dy < 0 ? DIR_UP : DIR_DOWN);
}

static void toward(Enemy *e, int spd88, int *vx, int *vy) {
    int a = vec_angle(ppx() - ex(e), ppy() - 6 - (ey(e) - 6));
    *vx = (isin(a) * spd88) >> 8;
    *vy = (-icos(a) * spd88) >> 8;
}

static int pdist(const Enemy *e) { return idist(ppx() - ex(e), ppy() - ey(e)); }

static int touches_player(const Enemy *e, int extra) {
    const EnInfo *in = info(e);
    int dx = iabs(ppx() - ex(e)), dy = iabs((ppy() - 6) - (ey(e) - in->hh / 2));
    return dx < in->hw + 5 + extra && dy < (in->hh >> 1) + 6 + extra;
}

static void touch_damage(Enemy *e, int mult16) {
    if (pl.state == PS_DEAD) return;
    if (touches_player(e, 0)) {
        int atk = info(e)->atk * mult16 >> 4;
        int d = atk + rnd_n(atk / 4 + 1);
        player_hurt(d, ex(e), ey(e) - 6);
    }
}

static void smoke_at(int x, int y, int n) {
    for (int i = 0; i < n; i++) part_spawn(PK_SMOKE, x - 6 + rnd_n(12), y - 4 - rnd_n(8), rnd_n(40) - 20, -20 - rnd_n(30), 20 + rnd_n(10), 0);
}

static void drop_loot(Enemy *e) {
    const EnInfo *in = info(e);
    int x = ex(e), y = ey(e);
    int g = in->glo + rnd_n(in->ghi - in->glo + 1);
    int n25 = g / 25, n5 = (g % 25) / 5, n1 = g % 5;
    if (n1 > 2) n1 = 2;
    for (int i = 0; i < n25 && i < 7; i++) pickup_spawn(PK_COIN25, x, y);
    for (int i = 0; i < n5 && i < 4; i++) pickup_spawn(PK_COIN5, x, y);
    for (int i = 0; i < n1; i++) pickup_spawn(PK_COIN, x, y);
    int r = rnd_n(100), luck = pl.lck;
    if (is_big(e)) {
        for (int i = 0; i < 4; i++) pickup_spawn(PK_HEART, x - 12 + i * 8, y);
        for (int i = 0; i < 3; i++) pickup_spawn(PK_MANA, x - 8 + i * 8, y);
        return;
    }
    if (r < 20 + luck / 2) pickup_spawn(PK_HEART, x, y);
    else if (r < 32 + luck / 2) pickup_spawn(PK_MANA, x, y);
    else if (r < 36 + luck / 3) pickup_spawn(PK_POTION, x, y);
}

static void enemy_kill(Enemy *e) {
    e->dying = 1;
    e->t = 0;
    e->alert = 0;
    if (is_big(e)) { e->state = S_DEAD; snd_sfx(SFX_BOSS_DIE); }
    else snd_sfx(SFX_ENEMY_DIE);
}

/* every hit the hero lands goes through here: defense, phase rules, feedback, knockback */
int enemy_damage(Enemy *e, int dmg, int kx, int ky, int crit, const HitFx *fx) {
    static const HitFx plain = { HK_SLASH, 0, 0, 16, 1, 3 };
    if (e->dying || !e->active) return 0;
    if (!fx) fx = &plain;
    const EnInfo *in = info(e);
    int big = is_big(e);
    int x = ex(e), y = ey(e) - 8 - (e->z >> 8);

    /* bosses ignore damage while waking up or shielded */
    if (big && (e->state == S_DORMANT || e->state == S_INTRO || e->state == S_PHASE || (e->type >= EN_WARDEN && e->state == S_SUMMON))) {
        snd_sfx(SFX_HIT_ARMOR);
        part_burst(PK_SPARKLE, x, y, 2, 60, 10);
        return 0;
    }
    if (dmg > 20000) dmg = 20000;                              /* hp is 16-bit: never let a huge hit wrap it */
    dmg = (dmg * 64) / (64 + in->def * 2);                     /* defense is a percentage */
    if (e->type == EN_WOLF && e->state == S_TIRED) dmg = dmg * 3 / 2;
    if (big && e->state == S_RECOVER) dmg = dmg * 5 / 4;
    if (big && e->state == S_TIRED) dmg = dmg * 8 / 5;         /* staggered: a window to hit hard */
    if (dmg < 1) dmg = 1;
    if (e->type == EN_BOSS && e->phase < 2) {                  /* the king cannot skip a phase change */
        int floor = e->phase == 0 ? e->maxhp * 2 / 3 : e->maxhp / 3;
        if (e->hp - dmg < floor) dmg = e->hp - floor;
        if (dmg <= 0) { snd_sfx(SFX_HIT_ARMOR); return 0; }
    }
    e->hp -= (s16)dmg;
    e->flash = big ? 3 : 5;
    dmgnum(x, y - (big ? 14 : 0), dmg, crit ? 3 : 0, crit);
    if (fx->heavy || crit) part_spawn(PK_BURST, x, y, 0, 0, 10, 0);
    part_burst(PK_HIT, x, y, fx->heavy ? 3 : 1, 0, 8);
    snd_impact(fx->kind, e->type, crit, fx->heavy, fx->finisher);
    shake_add(crit ? imax(3, fx->shake) : fx->shake);
    hitstop_t = imax(hitstop_t, crit ? imax(6, fx->stop) : fx->stop);
    combat_on_hit(fx);
    pass_on_hit();
    if (!big) {
        int a = vec_angle(kx, ky);
        int k = (e->type == EN_SKELETON) ? 440 : 600;
        k = (k * fx->kb) >> 4;
        k = (k * (40 + imin(pl.str, 48))) / 48;                 /* STR adds knockback */
        if (k > 1400) k = 1400;
        e->kx = (s16)((isin(a) * k) >> 8);
        e->ky = (s16)((-icos(a) * k) >> 8);
        e->stun = (u8)((e->type == EN_SKELETON ? 6 : 10) + (fx->heavy ? 8 : 0));
        if (e->state == S_WINDUP || e->state == S_ALERT) { e->state = S_RECOVER; e->t = 8; }
    }
    if (e->hp <= 0) {
        e->hp = 0;
        enemy_kill(e);
        return 1;
    }
    return 0;
}

void enemy_aoe(int cx, int cy, int r, int dmg, int id) {
    static const HitFx fire = { HK_FIRE, 0, 0, 16, 2, 4 };
    for (int i = 0; i < MAX_EN; i++) {
        Enemy *e = &en[i];
        if (!e->active || e->dying || e->hitid == id) continue;
        int dx = ex(e) - cx, dy = ey(e) - 6 - cy;
        if (idist(dx, dy) <= r + (is_big(e) ? 14 : 6)) {
            e->hitid = (u8)id;
            int crit = player_roll_crit();
            int d = dmg + rnd_n(dmg / 4 + 1);
            if (crit) d = (d * 7) >> 2;
            enemy_damage(e, d, dx, dy, crit, &fire);
        }
    }
}

/* nearby enemies of the same kind join the fight together */
static void alert_pack(const Enemy *src) {
    for (int i = 0; i < MAX_EN; i++) {
        Enemy *o = &en[i];
        if (!o->active || o->dying || o == src || o->type != src->type || o->state != S_IDLE) continue;
        if (idist(ex(o) - ex(src), ey(o) - ey(src)) < 110) { o->state = S_CHASE; o->t = 0; }
    }
}

/* ------------------------------------------------------------------ AI */
static void ai_slime(Enemy *e) {
    switch (e->state) {
    case S_IDLE:
        e->anim = (u8)(((frame_count >> 4) & 3) == 3 ? 1 : 0);
        if (e->t) e->t--;
        if (e->t == 0) {
            if (pdist(e) < 100 && pl.state != PS_DEAD) { e->state = S_WINDUP; e->t = 10; }
            else { e->t = (u16)(40 + rnd_n(50)); }
        }
        break;
    case S_WINDUP:
        e->anim = 3;
        if (--e->t == 0) {
            int a = vec_angle(ppx() - ex(e), ppy() - ey(e)) + rnd_n(31) - 15;
            if (e->pad0 > 8) { a += rnd_n(2) ? 64 : -64; e->pad0 = 0; }       /* stuck against something: hop around it */
            int sp = ESPD(290 + TT(e) * 40);
            e->vx = (s16)((isin(a) * sp) >> 8);
            e->vy = (s16)((-icos(a) * sp) >> 8);
            face_toward(e, e->vx, e->vy);
            e->state = S_HOP;
            e->t = 24;
            snd_sfx(SFX_SLIME);
        }
        break;
    case S_HOP: {
        int el = 24 - e->t;
        e->z = (s16)(((4 * 11 * el * (24 - el)) << 8) / (24 * 24));
        e->anim = 2;
        en_move(e, e->vx, e->vy);
        touch_damage(e, 16);
        if (--e->t == 0) {
            e->z = 0;
            e->state = S_RECOVER;
            e->t = 12;
            part_burst(PK_DUST, ex(e), ey(e) - 1, 2, 30, 10);
        }
        break;
    }
    case S_RECOVER:
        e->anim = 1;
        touch_damage(e, 12);
        if (--e->t == 0) { e->state = S_IDLE; e->t = (u16)(20 + rnd_n(40)); }
        break;
    default: e->state = S_IDLE;
    }
    if (e->state == S_IDLE) touch_damage(e, 12);
}

static void ai_bat(Enemy *e) {
    e->t2++;
    e->anim = (u8)(((frame_count >> 2) % 4) == 3 ? 1 : ((frame_count >> 2) % 4));
    if (e->anim > 2) e->anim = 1;
    int hover = 15 * 256 + (isin(e->t2 * 6) * 3);
    switch (e->state) {
    case S_IDLE: {
        e->z = (s16)hover;
        en_move_free(e, isin(e->t2 * 2) / 4, icos(e->t2 * 3) / 6);
        if (pdist(e) < 90 && pl.state != PS_DEAD) { e->state = S_CHASE; e->t = 200; snd_sfx(SFX_BAT); alert_pack(e); }
        break;
    }
    case S_CHASE: {
        e->z = (s16)hover;
        int vx, vy;
        toward(e, ESPD(220 + TT(e) * 25), &vx, &vy);
        int wob = isin(e->t2 * 7) >> 2;
        en_move_free(e, vx + wob, vy + (icos(e->t2 * 5) >> 3));
        if (pdist(e) < 46) { e->state = S_ALERT; e->t = (u16)(14 - TT(e) * 2); e->alert = (u8)e->t; snd_sfx(SFX_ALERT); }
        if (e->t && --e->t == 0) e->state = S_IDLE;
        break;
    }
    case S_ALERT:
        e->z = (s16)hover;
        e->vx = 0; e->vy = 0;
        if (--e->t == 0) {
            int vx, vy;
            toward(e, ESPD(620 + TT(e) * 50), &vx, &vy);
            e->vx = (s16)vx; e->vy = (s16)vy;
            e->state = S_SWOOP;
            e->t = 22;
        }
        break;
    case S_SWOOP:
        if (e->z > 5 * 256) e->z -= 256;
        en_move_free(e, e->vx, e->vy);
        touch_damage(e, 16);
        if (--e->t == 0) { e->state = S_RETREAT; e->t = 36; }
        break;
    case S_RETREAT: {
        if (e->z < 16 * 256) e->z += 128;
        int vx, vy;
        toward(e, 200, &vx, &vy);
        en_move_free(e, -vx, -vy);
        if (--e->t == 0) { e->state = S_CHASE; e->t = 200; }
        break;
    }
    default: e->state = S_IDLE;
    }
}

static void ai_wolf(Enemy *e) {
    switch (e->state) {
    case S_IDLE:
        e->anim = 0;
        if (e->t) e->t--;
        if (e->t == 0) {
            int a = rnd_n(256);
            e->vx = (s16)((isin(a) * 90) >> 8);
            e->vy = (s16)((-icos(a) * 90) >> 8);
            e->t = (u16)(30 + rnd_n(60));
            if (e->vx) e->dir = (u8)(e->vx < 0 ? DIR_LEFT : DIR_RIGHT);
        }
        if (e->t < 30) { en_move(e, e->vx, e->vy); e->anim = (u8)(1 + ((frame_count >> 3) & 1)); }
        if (pdist(e) < 105 && pl.state != PS_DEAD) { e->state = S_CHASE; e->t = 0; alert_pack(e); }
        touch_damage(e, 12);
        break;
    case S_CHASE: {
        int vx, vy;
        toward(e, ESPD(240 + TT(e) * 20), &vx, &vy);
        chase_step(e, vx, vy);
        e->anim = (u8)(1 + ((frame_count >> 2) & 1));
        if (vx) e->dir = (u8)(vx < 0 ? DIR_LEFT : DIR_RIGHT);
        touch_damage(e, 14);
        if (pdist(e) < 60) { e->state = S_ALERT; e->t = (u16)(26 - TT(e) * 3); e->alert = (u8)e->t; snd_sfx(SFX_WOLF); }
        if (pdist(e) > 170) e->state = S_IDLE;
        break;
    }
    case S_ALERT:
        e->anim = 3;
        {
            int dx = ppx() - ex(e);
            if (dx) e->dir = (u8)(dx < 0 ? DIR_LEFT : DIR_RIGHT);
        }
        if (--e->t == 0) {
            int vx, vy;
            toward(e, ESPD(880 + TT(e) * 40), &vx, &vy);
            e->vx = (s16)vx; e->vy = (s16)vy;
            e->state = S_CHARGE;
            e->t = 22;
        }
        break;
    case S_CHARGE:
        e->anim = (u8)(1 + ((frame_count >> 1) & 1));
        if (!en_move(e, e->vx, e->vy)) e->t = e->t > 4 ? 4 : e->t;
        if ((frame_count & 3) == 0) part_spawn(PK_DUST, ex(e), ey(e) - 1, 0, -10, 10, 0);
        touch_damage(e, 22);
        if (--e->t == 0) { e->state = S_TIRED; e->t = 40; }
        break;
    case S_TIRED:
        e->anim = 0;
        touch_damage(e, 10);
        if (--e->t == 0) { e->state = S_CHASE; }
        break;
    case S_RECOVER:
        e->anim = 0;
        if (--e->t == 0) e->state = S_CHASE;
        break;
    default: e->state = S_IDLE;
    }
}

static void ai_skeleton(Enemy *e) {
    switch (e->state) {
    case S_IDLE:
        e->anim = 0;
        if (pdist(e) < 100 && pl.state != PS_DEAD) { e->state = S_CHASE; alert_pack(e); }
        break;
    case S_CHASE: {
        int dx = ppx() - ex(e), dy = ppy() - ey(e);
        int vx = 0, vy = 0;
        int sp = ESPD(150 + TT(e) * 20);
        if (iabs(dx) > 3) vx = dx > 0 ? sp : -sp;
        if (iabs(dy) > 3) vy = dy > 0 ? sp : -sp;
        if (vx && vy) { vx = vx * 181 >> 8; vy = vy * 181 >> 8; }
        chase_step(e, vx, vy);
        face_toward(e, dx, dy);
        e->anim++;
        touch_damage(e, 8);
        if (idist(dx, dy) < 27) { e->state = S_WINDUP; e->t = (u16)(22 - TT(e) * 4); e->alert = (u8)e->t; snd_sfx(SFX_ALERT); }
        if (idist(dx, dy) > 190) e->state = S_IDLE;
        break;
    }
    case S_WINDUP:
        if (--e->t == 0) { e->state = S_ATTACK; e->t = 8; snd_sfx(SFX_SLASH); }
        break;
    case S_ATTACK: {
        /* slash box in front */
        int fx = ex(e) + dxv[e->dir] * 14, fy = ey(e) - 6 + dyv[e->dir] * 14;
        if (iabs(ppx() - fx) < 15 && iabs(ppy() - 6 - fy) < 14 && pl.state != PS_DEAD) {
            int atk = info(e)->atk;
            player_hurt(atk + rnd_n(atk / 4 + 1), ex(e), ey(e) - 6);
        }
        if (--e->t == 0) {
            /* deeper skeletons follow a slash with a second, faster one */
            if (TT(e) >= 1 && e->aux == 0 && pdist(e) < 36 && pl.state != PS_DEAD) {
                e->aux = 1; e->state = S_WINDUP; e->t = 9; e->alert = 9; face_toward(e, ppx() - ex(e), ppy() - ey(e));
            } else { e->aux = 0; e->state = S_RECOVER; e->t = (u16)(24 - TT(e) * 3); }
        }
        break;
    }
    case S_RECOVER:
        if (--e->t == 0) e->state = S_CHASE;
        break;
    default: e->state = S_IDLE;
    }
}

/* ------------------------------------------------------------------ boss arenas and ground hazards */
static int arena_x0, arena_x1, arena_y0, arena_y1;
#define ARENA_X0 arena_x0
#define ARENA_X1 arena_x1
#define ARENA_Y0 arena_y0
#define ARENA_Y1 arena_y1

/* pixel bounds of the boss room of the current area (the king's chamber in the crypt, the warden's hall below) */
static void arena_load(void) {
    if (cur_area_id == 4) { arena_x0 = 4 * 16 + 8; arena_x1 = 27 * 16; arena_y0 = 2 * 16 + 6; arena_y1 = 8 * 16 - 2; }
    else if (cur_area_id == 3) { arena_x0 = 2 * 16 + 10; arena_x1 = 16 * 16 + 6; arena_y0 = 2 * 16 + 4; arena_y1 = 10 * 16 - 6; }
    else { arena_x0 = 4 * 16; arena_x1 = 27 * 16 + 8; arena_y0 = 2 * 16 + 6; arena_y1 = 9 * 16 + 8; }
}

/* Ground hazards are the bosses' telegraphs: a ring is drawn where something is about to land and goes off when its
   timer runs out. dmg == 0 makes a harmless marker (the slam draws its own landing ring). */
#define MAX_HZ 28
typedef struct { s16 x, y; u8 t, t0, r, dmg; } Hazard;
static Hazard hz[MAX_HZ];

static void hz_clear(void) { for (int i = 0; i < MAX_HZ; i++) hz[i].t = 0; }

static void hz_add(int x, int y, int r, int delay, int dmg) {
    for (int i = 0; i < MAX_HZ; i++) {
        if (hz[i].t) continue;
        hz[i].x = (s16)x; hz[i].y = (s16)y;
        hz[i].r = (u8)r;
        hz[i].t = hz[i].t0 = (u8)delay;
        hz[i].dmg = (u8)(dmg > 250 ? 250 : dmg);
        return;
    }
}

static void hz_update(void) {
    for (int i = 0; i < MAX_HZ; i++) {
        Hazard *h = &hz[i];
        if (!h->t || --h->t) continue;
        if (!h->dmg) continue;
        part_burst(PK_DUST, h->x, h->y - 2, 5, 70, 14);
        part_spawn(PK_EXPL, h->x, h->y - 8, 0, 0, 16, 0);
        snd_sfx(SFX_EXPLODE);
        shake_add(2);
        if (pl.state != PS_DEAD && idist(ppx() - h->x, ppy() - h->y) < h->r + 4) player_hurt(h->dmg, h->x, h->y);
    }
}

static void hz_draw(void) {
    for (int i = 0; i < MAX_HZ; i++) {
        const Hazard *h = &hz[i];
        if (!h->t) continue;
        int sx = h->x - cam_x, sy = h->y - cam_y;
        if (sx < -40 || sx > 280 || sy < -40 || sy > 200) continue;
        int blink = h->t < 22 ? ((h->t >> 1) & 1) : ((h->t >> 3) & 1);
        int r = h->r < 6 ? 6 : h->r;
        int dbl = r > 15;
        spr_add_aff(sx - (dbl ? 32 : 16), sy - (dbl ? 32 : 16), SH_SQUARE, 2, OT_RING, h->dmg && blink ? PB_FLASH : PB_FX, 2,
                    SF_ALPHA | (dbl ? SF_DOUBLE : 0), -2100, 0, 256 * 15 / r);
    }
}

/* ------------------------------------------------------------------ boss: The Hollow King */
enum { KF_IDLE, KF_IDLE2, KF_WIND, KF_RAISED, KF_SLAM, KF_CHARGE, KF_DAZED, KF_SHIELD, KF_CAST };

static int boss_floor(const Enemy *e) { return e->phase == 0 ? e->maxhp * 2 / 3 : e->maxhp / 3; }

static int orb_dmg(const Enemy *e, int num, int den) {
    int d = info(e)->atk * num / den;
    return d + rnd_n(d / 5 + 1);
}

static void boss_fan(Enemy *e, int n, int spread) {
    int base = vec_angle(ppx() - ex(e), ppy() - 8 - (ey(e) - 16));
    for (int k = 0; k < n; k++) {
        int a = base + (n > 1 ? ((k * 2 - (n - 1)) * spread) / (n - 1) : 0);
        proj_spawn(PJ_ORB, 1, ex(e), ey(e) - 14, a & 255, 340 + e->phase * 30, orb_dmg(e, 2, 3));
    }
    snd_sfx(SFX_MAGIC);
}

/* the king floats to a hover height with a lazy bob */
static void boss_hover(Enemy *e, int level) {
    int target = level * 38 + isin(frame_count * 4) * 5;
    e->z = (s16)(e->z + ((target - e->z) >> 3));
}

/* reinforcements appear at fixed spots in the arena so they never spawn inside walls */
static void arena_adds(int n, int tier) {
    static const u8 spots[6][2] = { { 6, 9 }, { 25, 9 }, { 10, 6 }, { 21, 6 }, { 9, 4 }, { 22, 4 } };
    int start = rnd_n(6), made = 0;
    for (int i = 0; i < 6 && made < n; i++) {
        int tx = spots[(start + i) % 6][0], ty = spots[(start + i) % 6][1];
        if (tile_flags(tx, ty) & TF_SOLID) continue;
        Enemy *s = enemy_spawn(EN_SKELETON, tx, ty);
        if (!s) break;
        s->tier = (u8)tier;
        s->hp = s->maxhp = einfo[tier][EN_SKELETON].hp;
        s->state = S_CHASE;
        smoke_at(tx * 16 + 8, ty * 16 + 12, 6);
        made++;
    }
}

static void boss_next(Enemy *e) {
    static const u8 pat[3][8] = {
        { S_FAN, S_SLAM_UP, S_SUMMON, S_FAN, S_SLAM_UP, S_FAN, S_SUMMON, S_SLAM_UP },
        { S_FAN, S_RING, S_SWEEP, S_SLAM_UP, S_RAIN, S_SUMMON, S_CURTAIN, S_SWEEP },
        { S_RAIN, S_FAN, S_CURTAIN, S_RING, S_SLAM_UP, S_RAIN, S_CURTAIN, S_SUMMON },
    };
    e->state = pat[e->phase > 2 ? 2 : e->phase][e->aux & 7];
    e->aux++;
    e->t = 0;
    e->anim = KF_WIND;
    e->vy = e->phase;                                           /* extra slams in a row */
    if (e->state == S_SLAM_UP) { e->hx = (s16)ppx(); e->hy = (s16)ppy(); }
    if (e->state == S_SWEEP) e->hx = (s16)(ppx() < (ARENA_X0 + ARENA_X1) / 2 ? ARENA_X1 : ARENA_X0);      /* charge starts on the far wall */
}

static void ai_boss(Enemy *e) {
    int ph = e->phase;
    arena_load();
    /* at 2/3 and 1/3 health the king shields himself and calls reinforcements; he is only hurt again once they fall */
    if (ph < 2 && e->hp <= boss_floor(e) && e->state != S_DORMANT && e->state != S_INTRO && e->state != S_PHASE &&
        e->state != S_SLAM_UP && e->state != S_SLAM_DOWN && e->state != S_SWEEP) {
        e->phase = (u8)(ph + 1);
        ph = e->phase;
        e->state = S_PHASE;
        e->t = 0;
        e->aux = 0;
        hz_clear();
    }
    switch (e->state) {
    case S_DORMANT:
        e->anim = KF_IDLE;
        if (pl.state != PS_DEAD && (ppy() < 12 * 16 && idist(ppx() - ex(e), ppy() - ey(e)) < 170)) {
            e->state = S_INTRO;
            e->t = 0;
            gflags |= GF_BOSS_SEEN;
            snd_stop_music();
            snd_sfx(SFX_BOSS_ROAR);
            shake_add(5);
        }
        break;
    case S_INTRO:
        e->t++;
        e->anim = (u8)(e->t < 50 ? KF_WIND : (e->t < 70 ? KF_CAST : KF_IDLE));
        e->z = (s16)(e->t < 60 ? e->t * 38 : 60 * 38);
        if (e->t == 20 || e->t == 40) shake_add(3);
        if (e->t == 70) { snd_play_song(SONG_BOSS); banner_show("THE HOLLOW KING"); shake_add(4); }
        if (e->t >= 96) { e->z = 60 * 38; e->state = S_RECOVER; e->t = 30; e->aux = 0; }
        break;
    case S_PHASE:
        e->anim = KF_SHIELD;
        e->t++;
        e->z = (s16)(60 * 38 + isin(frame_count * 4) * 5);
        if (e->t == 1) { snd_sfx(SFX_BOSS_ROAR); shake_add(6); }
        if (e->t == 36) { snd_sfx(SFX_BOSS_ROAR); shake_add(3); arena_adds(ph == 1 ? 3 : 4, 2); }
        if (e->t > 70 && (count_type(EN_SKELETON) == 0 || e->t > 1800)) { e->state = S_TIRED; e->t = 150; snd_sfx(SFX_EXPLODE); shake_add(4); }
        break;
    case S_TIRED:                                              /* shield broken: the king crashes down, dazed */
        e->anim = KF_DAZED;
        if (e->z > 0) e->z = (s16)imax(0, e->z - 220);
        touch_damage(e, 8);
        if (--e->t == 0) { e->state = S_RECOVER; e->t = 30; }
        break;
    case S_RECOVER:
        e->anim = (u8)((frame_count >> 5) & 1);
        {
            boss_hover(e, 60);
            int dx = ppx() - ex(e);
            int sp = 130 + ph * 22;
            int vx = dx > 8 ? sp : (dx < -8 ? -sp : 0);
            if (vx) {
                int nx = (int)(e->x + vx);
                if ((nx >> 8) > ARENA_X0 + 16 && (nx >> 8) < ARENA_X1 - 16) e->x = nx;
            }
        }
        if (e->t) e->t--;
        if (e->t == 0) boss_next(e);
        touch_damage(e, 16);
        break;
    case S_FAN: {                                              /* aimed volleys: glowing hands, then a spread of orbs */
        int first = 24, gap = 17 - ph * 2, shots = 2 + ph;
        int k = (int)e->t - first;
        e->t++;
        if (e->t == 2) { e->alert = 20; snd_sfx(SFX_ALERT); }
        e->anim = (u8)((k >= 0 && (k % gap) < 6) ? KF_CAST : KF_WIND);
        boss_hover(e, 60);
        if (k >= 0 && (k % gap) == 0 && (k / gap) < shots) boss_fan(e, 5 + ph * 2 + ((k / gap) & 1) * 2, 44 + ph * 6);
        if ((int)e->t >= first + gap * shots + 14) { e->state = S_RECOVER; e->t = (u16)(54 - ph * 14); }
        break;
    }
    case S_SUMMON:
        e->anim = (u8)(e->t < 28 ? KF_RAISED : KF_CAST);
        e->t++;
        if (e->t == 28) {
            snd_sfx(SFX_BOSS_ROAR);
            if (count_type(EN_SKELETON) < 3) {
                for (int s = -1; s <= 1; s += 2) {
                    int tx = (ex(e) + s * 36) >> 4, ty = (ey(e) + 10) >> 4;
                    if (!(tile_flags(tx, ty) & TF_SOLID)) {
                        Enemy *m = enemy_spawn(EN_SKELETON, tx, ty);
                        if (m) { m->state = S_CHASE; smoke_at(tx * 16 + 8, ty * 16 + 12, 6); }
                    }
                }
            }
            shake_add(2);
        }
        if (e->t >= 58) { e->state = S_RECOVER; e->t = (u16)(50 - ph * 12); }
        break;
    case S_RING: {                                             /* rotating spiral; the direction flips every time */
        int arms = 2 + ph;
        e->t++;
        e->anim = (u8)(e->t < 24 ? KF_WIND : KF_CAST);
        if (e->t == 2) { e->alert = 20; snd_sfx(SFX_ALERT); }
        boss_hover(e, 60);
        if (e->t > 24 && e->t < 72 && (e->t & 3) == 0) {
            int a = (e->t * ((e->aux & 1) ? 14 : -14)) & 255;
            for (int k = 0; k < arms; k++) proj_spawn(PJ_ORB, 1, ex(e), ey(e) - 14, (a + k * 256 / arms) & 255, 300, orb_dmg(e, 3, 5));
            snd_sfx(SFX_MAGIC);
        }
        if (e->t >= 86) { e->state = S_RECOVER; e->t = (u16)(34 - ph * 6); }
        break;
    }
    case S_SLAM_UP:                                            /* leap, lock the landing spot (ring), then drop */
        e->anim = KF_RAISED;
        e->t++;
        if (e->z < 100 * 38) e->z += 170;
        if (e->t < 14) { e->hx = (s16)ppx(); e->hy = (s16)ppy(); }
        if (e->t == 14) { hz_add(e->hx, e->hy, 30, 38, 0); snd_sfx(SFX_ALERT); }
        {
            int tx = e->hx * 256, ty = e->hy * 256;
            e->x += (tx - e->x) >> 4;
            e->y += (ty - e->y) >> 4;
        }
        if (e->t >= 52) { e->state = S_SLAM_DOWN; e->t = 0; }
        break;
    case S_SLAM_DOWN:
        e->anim = KF_SLAM;
        e->t++;
        e->z -= 900;
        if (e->z <= 0 || e->t > 9) {
            e->z = 0;
            int cx = ex(e), cy = ey(e);
            snd_sfx(SFX_EXPLODE);
            shake_add(7);
            hitstop_t = 4;
            part_burst(PK_DUST, cx, cy - 2, 8, 90, 16);
            part_spawn(PK_EXPL, cx, cy - 10, 0, 0, 16, 0);
            int n = 8 + ph * 2;
            for (int k = 0; k < n; k++) proj_spawn(PJ_ORB, 1, cx, cy - 6, (k * 256 / n + (e->aux & 1) * 128 / n) & 255, 300, orb_dmg(e, 3, 5));
            if (idist(ppx() - cx, ppy() - cy) < 28) player_hurt(info(e)->atk * 11 / 10, cx, cy);
            if (e->vy > 0) {                                    /* later phases chain a second slam onto the first */
                e->vy--;
                e->state = S_SLAM_UP;
                e->t = 0;
                e->hx = (s16)ppx(); e->hy = (s16)ppy();
            } else {
                e->state = S_RECOVER;
                e->t = (u16)(48 - ph * 8);
            }
        }
        break;
    case S_SWEEP:                                              /* wall-to-wall charge: a lane warning, then he is gone */
        e->t++;
        if (e->t < 34) {
            e->anim = KF_WIND;
            e->x += ((int)e->hx * 256 - e->x) >> 3;
            e->y += ((iclamp(ppy(), ARENA_Y0, ARENA_Y1) * 256) - e->y) >> 4;
            if (e->z > 0) e->z = (s16)imax(0, e->z - 110);
            e->alert = 3;
            if (e->t == 6) snd_sfx(SFX_ALERT);
        } else if (e->t < 34 + (ARENA_X1 - ARENA_X0) / 5 + 6) {
            e->anim = KF_CHARGE;
            e->x += (e->hx == ARENA_X0 ? 5 : -5) * 256;
            if ((e->t & 3) == 0) part_burst(PK_DUST, ex(e), ey(e) - 1, 2, 60, 10);
            if ((e->t & 7) == 0) shake_add(1);
            touch_damage(e, 30);
        } else { e->state = S_TIRED; e->t = 60; }
        break;
    case S_CURTAIN: {                                          /* walls of spectral orbs sweep in from alternating sides, each with a gap */
        int waves = ph >= 2 ? 3 : 2;
        e->t++;
        e->anim = (u8)(e->t < 22 ? KF_RAISED : KF_CAST);
        boss_hover(e, 60);
        if (e->t == 2) { e->alert = 20; snd_sfx(SFX_ALERT); }
        for (int w = 0; w < waves; w++) {
            if (e->t != 26 + w * 46) continue;
            int from_left = (w + e->aux) & 1, slots = 6, gap = rnd_n(slots - 1);
            for (int k = 0; k < slots; k++) {
                if (k == gap || k == gap + 1) continue;
                int y = ARENA_Y0 + 8 + (ARENA_Y1 - ARENA_Y0 - 12) * k / (slots - 1);
                proj_spawn(PJ_ORB, 1, from_left ? ARENA_X0 + 6 : ARENA_X1 - 6, y, from_left ? 64 : 192, 360 + ph * 30, orb_dmg(e, 3, 5));
            }
            snd_sfx(SFX_MAGIC);
        }
        if (e->t >= 26 + waves * 46 + 40) { e->state = S_RECOVER; e->t = (u16)(40 - ph * 8); }
        break;
    }
    case S_RAIN: {                                             /* ring markers appear across the room and go off */
        int period = 8 - ph * 2;
        e->anim = KF_RAISED;
        e->t++;
        boss_hover(e, 70);
        if (e->t == 2) { e->alert = 20; snd_sfx(SFX_ALERT); }
        if (e->t >= 24 && e->t < 24 + 11 * period && (e->t % period) == 0) {
            int bx, by;
            if (rnd_n(3) == 0) { bx = ppx(); by = ppy(); }
            else { bx = ARENA_X0 + 8 + rnd_n(ARENA_X1 - ARENA_X0 - 16); by = ARENA_Y0 + 8 + rnd_n(ARENA_Y1 - ARENA_Y0 - 8); }
            hz_add(bx, by, 15, 36 - ph * 5, info(e)->atk * 9 / 10);
            snd_sfx(SFX_MAGIC);
        }
        if (e->t >= 24 + 11 * period + 44) { e->state = S_RECOVER; e->t = 26; }
        break;
    }
    default: break;
    }
    /* keep the boss in the room */
    int bx = ex(e), by = ey(e);
    if (e->state != S_SLAM_UP && e->state != S_SLAM_DOWN) {
        if (by < ARENA_Y0) e->y = (s32)ARENA_Y0 << 8;
        if (by > ARENA_Y1) e->y = (s32)ARENA_Y1 << 8;
    }
    if (bx < ARENA_X0) e->x = (s32)ARENA_X0 << 8;
    if (bx > ARENA_X1) e->x = (s32)ARENA_X1 << 8;
}

/* ------------------------------------------------------------------ mini-boss: the Bone Warden */
enum { WF_IDLE, WF_WALK1, WF_WALK2, WF_RAISED, WF_SMASH, WF_BASH, WF_TOSS, WF_DAZED, WF_SUMMON };

static void warden_minions(Enemy *e) {
    for (int s = -1; s <= 1; s += 2) {
        int tx = (ex(e) + s * 34) >> 4, ty = (ey(e) + 6) >> 4;
        if (tile_flags(tx, ty) & TF_SOLID) continue;
        Enemy *m = enemy_spawn(EN_SKELETON, tx, ty);
        if (!m) return;
        m->tier = e->type == EN_TYRANT ? 3 : 1;
        m->hp = m->maxhp = einfo[m->tier][EN_SKELETON].hp;
        m->state = S_CHASE;
        smoke_at(tx * 16 + 8, ty * 16 + 6, 6);
    }
}

static void warden_intro(Enemy *e, int ty, int dx, int dy) {
    arena_load();
    switch (e->state) {
    case S_DORMANT:
        e->anim = 0;
        if (pl.state != PS_DEAD && idist(dx, dy) < 130) {
            e->state = S_INTRO;
            e->t = 0;
            gflags |= ty ? GF_TYRANT_SEEN : GF_WARDEN_SEEN;
            snd_sfx(SFX_BOSS_ROAR);
            shake_add(4);
            if (ty) snd_stop_music();
            banner_show(ty ? "THE CINDER TYRANT" : "THE BONE WARDEN");
        }
        break;
    case S_INTRO:
        e->t++;
        if (e->t == 20) shake_add(3);
        if (ty && e->t == 40) snd_play_song(SONG_BOSS);
        if (e->t >= 70) { e->state = S_CHASE; e->t = 40; }
        break;
    default: break;
    }
}

static void boss_clamp(Enemy *e) {
    if (ex(e) < ARENA_X0) e->x = (s32)ARENA_X0 << 8;
    if (ex(e) > ARENA_X1) e->x = (s32)ARENA_X1 << 8;
    if (ey(e) < ARENA_Y0) e->y = (s32)ARENA_Y0 << 8;
    if (ey(e) > ARENA_Y1) e->y = (s32)ARENA_Y1 << 8;
}

/* ------------------------------------------------------------------ mini-boss: the Bone Warden
   A bone brute: shield bashes, a club smash that sends a shockwave along the floor, triple stomps on the spot you stood,
   volleys of thrown bone spears, a whirlwind below half health, and skeleton helpers. Pure melee pressure. */
static void ai_warden(Enemy *e) {
    int dx = ppx() - ex(e), dy = ppy() - ey(e);
    int enr = e->phase >= 1;                                  /* below half health */
    if (e->state == S_DORMANT || e->state == S_INTRO) { warden_intro(e, 0, dx, dy); return; }
    arena_load();
    switch (e->state) {
    case S_CHASE: {
        int vx, vy;
        toward(e, enr ? 185 : 140, &vx, &vy);
        chase_step(e, vx, vy);
        face_toward(e, dx, dy);
        e->anim++;
        touch_damage(e, 12);
        if (!enr && e->hp * 2 < e->maxhp) { e->phase = 1; e->state = S_SUMMON; e->t = 0; hz_clear(); break; }
        if (e->t) e->t--;
        if (e->t == 0) {
            /* 0 shield bash, 1 spear volley, 2 club smash + shockwave, 3 triple stomp, 4 whirlwind (below half health only) */
            static const u8 seq[2][8] = { { 0, 1, 2, 3, 1, 2, 0, 3 }, { 4, 1, 2, 3, 4, 0, 2, 3 } };
            int pick = seq[enr][e->aux & 7];
            e->aux++;
            e->hx = (s16)pick;
            e->alert = 24;
            snd_sfx(SFX_ALERT);
            if (pick == 1) { e->state = S_FAN; e->t = 0; }
            else if (pick == 3) { e->state = S_POUND; e->t = 0; }
            else if (pick == 4) { e->state = S_SPIN; e->t = 0; }
            else {
                e->state = S_WINDUP;
                e->t = (u16)(pick == 0 ? 32 : 30);
                if (pick == 2) hz_add(ex(e) + dxv[e->dir] * 28, ey(e) + dyv[e->dir] * 28, 22, 30, 0);   /* where the club lands */
            }
        }
        break;
    }
    case S_WINDUP:
        if (e->t > 12) face_toward(e, dx, dy);                /* the aim locks shortly before the blow */
        e->anim++;
        if (--e->t == 0) {
            if (e->hx == 0) {
                int vx, vy;
                toward(e, 560, &vx, &vy);
                e->vx = (s16)vx; e->vy = (s16)vy;
                e->state = S_CHARGE;
                e->t = 24;
                snd_sfx(SFX_WOLF);
            } else {
                e->state = S_SWING;
                e->t = 14;
                snd_sfx(SFX_SLASH2);
            }
        }
        break;
    case S_CHARGE:
        e->anim++;
        if (!en_move(e, e->vx, e->vy)) e->t = e->t > 3 ? 3 : e->t;
        if ((frame_count & 3) == 0) part_spawn(PK_DUST, ex(e), ey(e) - 1, 0, -10, 10, 0);
        touch_damage(e, 26);
        if (--e->t == 0) { e->state = S_TIRED; e->t = (u16)(enr ? 50 : 70); }
        break;
    case S_SWING: {
        int fx = ex(e) + dxv[e->dir] * 20, fy = ey(e) - 8 + dyv[e->dir] * 20;
        if (e->t == 9) {                                      /* the blow lands and a shockwave runs along the floor */
            shake_add(4);
            part_burst(PK_DUST, fx, fy + 6, 4, 80, 14);
            snd_sfx(SFX_EXPLODE);
            for (int k = 1; k <= (enr ? 4 : 3); k++)
                hz_add(ex(e) + dxv[e->dir] * (22 + k * 20), ey(e) + dyv[e->dir] * (22 + k * 20), 13, 3 + k * 5, info(e)->atk * 7 / 10);
        }
        if (e->t <= 9 && e->t >= 5 && iabs(ppx() - fx) < 26 && iabs(ppy() - 6 - fy) < 22 && pl.state != PS_DEAD)
            player_hurt(info(e)->atk * 13 / 10 + rnd_n(6), ex(e), ey(e) - 6);
        if (--e->t == 0) { e->state = S_TIRED; e->t = (u16)(enr ? 34 : 50); }
        break;
    }
    case S_FAN: {                                             /* bone spears: fast, straight, aimed volleys */
        e->t++;
        face_toward(e, dx, dy);
        if (e->t == 20 || e->t == 38 || (enr && e->t == 56)) {
            int base = vec_angle(dx, dy);
            int n = enr ? 4 : 3;
            for (int k = 0; k < n; k++)
                proj_spawn(PJ_ARROW, 1, ex(e), ey(e) - 12, (base + (k * 2 - (n - 1)) * 7) & 255, 560, orb_dmg(e, 3, 5));
            snd_sfx(SFX_SLASH2);
        }
        if (e->t >= (enr ? 74 : 56)) { e->state = S_CHASE; e->t = (u16)(enr ? 26 : 46); }
        break;
    }
    case S_POUND:                                             /* stomps: each lands where the hero stood half a second ago */
        e->t++;
        if (e->t == 6 || e->t == 24 || (enr && e->t == 42)) hz_add(ppx(), ppy(), 26, 32, info(e)->atk * 11 / 10);
        if (e->t >= (enr ? 90 : 70)) { e->state = S_TIRED; e->t = (u16)(enr ? 30 : 46); }
        break;
    case S_SPIN:                                              /* whirlwind: sweeps toward the hero and sprays bones */
        e->t++;
        if (e->t > 26 && e->t < 90) {
            int vx, vy;
            toward(e, 150, &vx, &vy);
            en_move(e, vx, vy);
            touch_damage(e, 22);
            if ((e->t & 7) == 0) {
                snd_sfx(SFX_SLASH2);
                for (int k = 0; k < 3; k++) proj_spawn(PJ_ORB, 1, ex(e), ey(e) - 10, (e->t * 7 + k * 85) & 255, 250, orb_dmg(e, 3, 5));
            }
        }
        if (e->t >= 100) { e->state = S_TIRED; e->t = 46; }
        break;
    case S_SUMMON:
        e->t++;
        if (e->t == 20) { snd_sfx(SFX_BOSS_ROAR); shake_add(3); warden_minions(e); }
        if (e->t >= 50) { e->state = S_CHASE; e->t = 30; }
        break;
    case S_TIRED:
        e->anim = 0;
        touch_damage(e, 8);
        if (--e->t == 0) { e->state = S_CHASE; e->t = (u16)(enr ? 20 : 36); }
        break;
    default: e->state = S_CHASE; break;
    }
    boss_clamp(e);
}

/* ------------------------------------------------------------------ boss: the Cinder Tyrant
   Nothing like the Warden: it stalks the hero and attacks with fire at range and in the ground. Flame breath (a stream
   of fireballs sweeping an arc), cinder rush (a charge that leaves a trail of eruptions), geyser columns (walls of
   eruptions with a gap to slip through), encircling meteors (a ring of impacts that closes in), a fire stomp if you hug
   it, and ember bats. It enrages below 65% and again below 30% of its health (e->phase 1 and 2). */
enum { TA_BREATH, TA_GEYSER, TA_RUSH, TA_METEOR, TA_BATS, TA_STOMP };

static void tyrant_bats(Enemy *e) {
    if (count_type(EN_BAT) >= 4) return;
    for (int s = -1; s <= 1; s += 2) {
        int tx = (ex(e) + s * 42) >> 4, ty = (ey(e) - 8) >> 4;
        if (tile_flags(tx, ty) & TF_SOLID) continue;
        Enemy *m = enemy_spawn(EN_BAT, tx, ty);
        if (!m) return;
        m->tier = 3;
        m->hp = m->maxhp = einfo[3][EN_BAT].hp;
        m->state = S_CHASE;
        m->t = 200;
        smoke_at(tx * 16 + 8, ty * 16 + 6, 6);
    }
}

/* a hazard ring clamped to the hall */
static void hz_in_arena(int x, int y, int r, int delay, int dmg) {
    hz_add(iclamp(x, ARENA_X0 + 6, ARENA_X1 - 6), iclamp(y, ARENA_Y0 + 4, ARENA_Y1), r, delay, dmg);
}

static void ai_tyrant(Enemy *e) {
    int dx = ppx() - ex(e), dy = ppy() - ey(e);
    int enr = e->phase;                                       /* 0 calm, 1 below 65% health, 2 below 30% */
    int atk = info(e)->atk;
    if (e->state == S_DORMANT || e->state == S_INTRO) { warden_intro(e, 1, dx, dy); return; }
    arena_load();
    if (enr < 2 && e->state != S_SUMMON && e->hp * 100 < e->maxhp * (enr == 0 ? 65 : 30)) {
        e->phase = (u8)(enr + 1);
        enr = e->phase;
        e->state = S_SUMMON;                                  /* each enrage: a roar and a swarm of ember bats */
        e->t = 0;
        hz_clear();
    }
    switch (e->state) {
    case S_CHASE: {
        int vx, vy;
        toward(e, 150 + enr * 25, &vx, &vy);
        if (idist(dx, dy) > 36) chase_step(e, vx, vy);
        face_toward(e, dx, dy);
        e->anim++;
        touch_damage(e, 12);
        if (e->t) e->t--;
        if (e->t == 0) {
            static const u8 seq[3][8] = {
                { TA_BREATH, TA_GEYSER, TA_RUSH, TA_METEOR, TA_BREATH, TA_RUSH, TA_GEYSER, TA_BATS },
                { TA_RUSH, TA_BREATH, TA_GEYSER, TA_METEOR, TA_RUSH, TA_GEYSER, TA_BREATH, TA_METEOR },
                { TA_GEYSER, TA_RUSH, TA_BREATH, TA_METEOR, TA_GEYSER, TA_BREATH, TA_RUSH, TA_METEOR } };
            int pick = seq[enr][e->aux & 7];
            e->aux++;
            if (idist(dx, dy) < 40 && rnd_n(3) == 0) pick = TA_STOMP;       /* hugging the Tyrant is punished */
            e->hx = (s16)pick;
            e->alert = 24;
            snd_sfx(SFX_ALERT);
            e->t = 0;
            switch (pick) {
            case TA_BREATH: e->state = S_BREATH; break;
            case TA_GEYSER: e->state = S_GEYSER; break;
            case TA_RUSH: e->state = S_WINDUP; e->t = 34; hz_add(ppx(), ppy(), 18, 34, 0); break;
            case TA_METEOR: e->state = S_METEOR; break;
            case TA_BATS: e->state = S_SUMMON; break;
            default: e->state = S_STOMP; break;
            }
        }
        break;
    }
    case S_WINDUP:                                            /* crouch and glare; the landing marker shows where the rush heads */
        face_toward(e, dx, dy);
        e->anim++;
        if (--e->t == 0) {
            int vx, vy;
            toward(e, 780, &vx, &vy);
            e->vx = (s16)vx; e->vy = (s16)vy;
            e->state = S_RUSH;
            e->t = 34;
            snd_sfx(SFX_WOLF);
        }
        break;
    case S_RUSH:                                              /* the charge leaves a trail of eruptions that go off behind it */
        e->anim++;
        if (!en_move(e, e->vx, e->vy)) e->t = e->t > 3 ? 3 : e->t;
        if ((e->t % 3) == 0) hz_add(ex(e), ey(e), 12, 26, atk * 7 / 10);
        if ((frame_count & 3) == 0) part_spawn(PK_EMBER, ex(e), ey(e) - 6, 0, -14, 12, 0);
        touch_damage(e, 28);
        if (--e->t == 0) { e->state = S_TIRED; e->t = (u16)(52 - enr * 10); }
        break;
    case S_BREATH: {                                          /* flame breath: a stream of fireballs sweeping across an arc */
        int span = 40 + enr * 14;
        e->t++;
        face_toward(e, dx, dy);
        if (e->t == 1) { e->vx = (s16)vec_angle(dx, dy); e->vy = (s16)(rnd_n(2) ? 1 : -1); }       /* aim and sweep direction */
        if (e->t == 2) { snd_sfx(SFX_ALERT); }
        if (e->t >= 24 && e->t < 24 + span && (e->t & 3) == 0) {
            int k = e->t - 24;
            int a = e->vx + e->vy * (-26 + 52 * k / span);
            proj_spawn(PJ_FIRE, 1, ex(e), ey(e) - 14, a & 255, 300, orb_dmg(e, 3, 5));
            if (enr) proj_spawn(PJ_FIRE, 1, ex(e), ey(e) - 14, (e->vx - e->vy * (-26 + 52 * k / span)) & 255, 300, orb_dmg(e, 3, 5));
            snd_sfx(SFX_MAGIC);
        }
        if (e->t >= 24 + span + 20) { e->state = S_TIRED; e->t = (u16)(46 - enr * 8); }
        break;
    }
    case S_GEYSER: {                                          /* columns of eruptions across the hall, each with one gap */
        int cols = 2 + (enr ? 1 : 0) + (enr > 1 ? 1 : 0);
        e->t++;
        if (e->t == 2) snd_sfx(SFX_ALERT);
        for (int c = 0; c < cols; c++) {
            if (e->t != 16 + c * 22) continue;
            int cx = ppx() + (c & 1 ? -1 : 1) * (28 + rnd_n(26)), gap = rnd_n(3);
            for (int k = 0; k < 3; k++) {
                if (k == gap) continue;
                hz_in_arena(cx, ARENA_Y0 + 14 + k * ((ARENA_Y1 - ARENA_Y0 - 14) / 3), 10, 34, atk * 8 / 10);
            }
            snd_sfx(SFX_MAGIC);
        }
        if (e->t >= 16 + cols * 22 + 44) { e->state = S_TIRED; e->t = (u16)(40 - enr * 8); }
        break;
    }
    case S_METEOR: {                                          /* a ring of meteors closes in; the middle goes last */
        e->t++;
        if (e->t == 2) snd_sfx(SFX_ALERT);
        if (e->t == 16 || (enr && e->t == 62) || (enr > 1 && e->t == 108)) {
            int cx = ppx(), cy = ppy(), n = 5 + enr, base = rnd_n(256);
            for (int k = 0; k < n; k++) {
                int a = base + k * 256 / n;
                hz_in_arena(cx + ((isin(a) * 50) >> 8), cy - ((icos(a) * 40) >> 8), 14, 40, atk * 9 / 10);
            }
            hz_in_arena(cx, cy, 20, 58, atk * 11 / 10);
            snd_sfx(SFX_MAGIC);
        }
        if (e->t >= (enr > 1 ? 108 : (enr ? 62 : 16)) + 80) { e->state = S_TIRED; e->t = (u16)(44 - enr * 8); }
        break;
    }
    case S_STOMP:                                             /* fire stomp: a ring of eruptions around its feet */
        e->t++;
        if (e->t == 14) {
            shake_add(5);
            snd_sfx(SFX_EXPLODE);
            part_burst(PK_EMBER, ex(e), ey(e) - 4, 8, 120, 20);
            for (int k = 0; k < 6; k++) {
                int a = k * 256 / 6 + 20;
                hz_in_arena(ex(e) + ((isin(a) * 36) >> 8), ey(e) - ((icos(a) * 28) >> 8), 12, 16, atk * 8 / 10);
            }
            if (idist(dx, dy) < 26) player_hurt(atk * 11 / 10, ex(e), ey(e));
        }
        if (e->t >= 34) { e->state = S_TIRED; e->t = 26; }
        break;
    case S_SUMMON:
        e->t++;
        if (e->t == 20) { snd_sfx(SFX_BOSS_ROAR); shake_add(3); tyrant_bats(e); }
        if (e->t >= 50) { e->state = S_CHASE; e->t = 30; }
        break;
    case S_TIRED:
        e->anim = 0;
        touch_damage(e, 8);
        if (--e->t == 0) { e->state = S_CHASE; e->t = (u16)(34 - enr * 8); }
        break;
    default: e->state = S_CHASE; break;
    }
    boss_clamp(e);
}

/* ------------------------------------------------------------------ update */
/* trophies go straight into the pack with a short toast */
static void loot_drop(const Enemy *e) {
    static char msg[32];
    for (int slot = 0; slot < 2; slot++) {
        int it = loot_roll(e->type, e->tier, slot);
        if (!it) continue;
        player_give(it, 1);
        int k = 0;
        for (const char *s = item_defs[it].name; *s && k < 22; s++) msg[k++] = *s;
        msg[k++] = ' '; msg[k++] = '+'; msg[k++] = '1'; msg[k] = 0;
        toast_show(msg);
    }
}

static void dying_update(Enemy *e) {
    e->t++;
    if (e->t <= 1 && is_big(e)) hz_clear();
    if (e->type == EN_TYRANT) {
        e->flash = (u8)((e->t >> 1) & 1 ? 2 : 0);
        if ((e->t % 8) == 0) {
            int x = ex(e) - 16 + rnd_n(32), y = ey(e) - 28 + rnd_n(28);
            part_spawn(PK_EXPL, x, y, 0, 0, 16, 0);
            snd_sfx(SFX_EXPLODE);
            smoke_at(x, y, 2);
        }
        shake_add(3);
        if (e->t >= 130) {
            smoke_at(ex(e), ey(e) - 10, 10);
            drop_loot(e);
            player_add_xp(info(e)->xp);
            dmgnum(ex(e), ey(e) - 26, info(e)->xp, 3, 1);
            gflags |= GF_TYRANT_DEAD;
            player_give(ITEM_EMBER, 2);
            quest_refresh();
            snd_sfx(SFX_VICTORY);
            banner_show("THE CURSE IS BROKEN");
            toast_show("South gate open! Ember Core x2");
            e->active = 0;
        }
        return;
    }
    if (e->type == EN_WARDEN) {
        e->flash = (u8)((e->t >> 1) & 1 ? 2 : 0);
        if ((e->t % 9) == 0) {
            int x = ex(e) - 12 + rnd_n(24), y = ey(e) - 24 + rnd_n(24);
            part_spawn(PK_EXPL, x, y, 0, 0, 16, 0);
            snd_sfx(SFX_EXPLODE);
            smoke_at(x, y, 2);
        }
        shake_add(2);
        if (e->t >= 100) {
            smoke_at(ex(e), ey(e) - 10, 8);
            drop_loot(e);
            player_add_xp(info(e)->xp);
            dmgnum(ex(e), ey(e) - 24, info(e)->xp, 3, 1);
            gflags |= GF_WARDEN_DEAD;
            quest_refresh();
            player_give(ITEM_SKULL_KEY, 1);
            snd_sfx(SFX_ITEM);
            toast_show("Obtained the Skull Key!");
            e->active = 0;
        }
        return;
    }
    if (e->type == EN_BOSS) {
        e->flash = (u8)((e->t >> 1) & 1 ? 2 : 0);
        if ((e->t % 7) == 0) {
            int x = ex(e) - 14 + rnd_n(28), y = ey(e) - 30 + rnd_n(30);
            part_spawn(PK_EXPL, x, y, 0, 0, 16, 0);
            snd_sfx(SFX_EXPLODE);
            smoke_at(x, y, 2);
        }
        shake_add(3);
        if (e->t >= 120) {
            smoke_at(ex(e), ey(e) - 10, 10);
            drop_loot(e);
            player_add_xp(info(e)->xp);
            dmgnum(ex(e), ey(e) - 24, info(e)->xp, 3, 1);
            gflags |= GF_BOSS_DEAD;
            quest_refresh();
            boss_dead_flag = 1;
            e->active = 0;
        }
        return;
    }
    e->flash = (u8)((e->t >> 1) & 1 ? 2 : 0);
    if (e->t == 2) smoke_at(ex(e), ey(e), 3);
    if (e->t >= 16) {
        smoke_at(ex(e), ey(e) - 2, 4);
        drop_loot(e);
        loot_drop(e);
        player_add_xp(info(e)->xp);
        dmgnum(ex(e), ey(e) - 22, info(e)->xp, 3, 0);
        pass_on_kill();
        quest_on_kill(e->type);
        e->active = 0;
    }
}

void enemies_update(void) {
    hz_update();
    for (int i = 0; i < MAX_EN; i++) {
        Enemy *e = &en[i];
        if (!e->active) continue;
        if (e->flash) e->flash--;
        if (e->dying) { dying_update(e); continue; }
        if (e->kx || e->ky) {
            en_move(e, e->kx, e->ky);
            e->kx = (s16)((e->kx * 13) >> 4);
            e->ky = (s16)((e->ky * 13) >> 4);
            if (iabs(e->kx) < 20) e->kx = 0;
            if (iabs(e->ky) < 20) e->ky = 0;
        }
        if (e->stun) { e->stun--; continue; }
        if (e->alert) e->alert--;
        switch (e->type) {
        case EN_SLIME: ai_slime(e); break;
        case EN_BAT: ai_bat(e); break;
        case EN_WOLF: ai_wolf(e); break;
        case EN_SKELETON: ai_skeleton(e); break;
        case EN_BOSS: ai_boss(e); break;
        case EN_WARDEN: ai_warden(e); break;
        case EN_TYRANT: ai_tyrant(e); break;
        }
    }
}

/* ------------------------------------------------------------------ draw */
/* warning stripe along the row the king is about to charge down */
static void boss_lane_draw(void) {
    for (int i = 0; i < MAX_EN; i++) {
        const Enemy *e = &en[i];
        if (!e->active || e->type != EN_BOSS || e->state != S_SWEEP || e->t < 4 || e->t >= 34 + 6) continue;
        int sy = ey(e) - cam_y - 4;
        int pal = ((e->t >> 2) & 1) ? PB_FLASH : PB_FX;
        for (int x = (arena_x0 / 16) * 16; x <= arena_x1 + 8; x += 16) {
            int sx = x - cam_x;
            if (sx < -16 || sx > 240) continue;
            spr_add(sx, sy, SH_WIDE, 0, OT_LANE, pal, 2, SF_ALPHA, -2100);
        }
    }
}

void enemies_draw(void) {
    for (int i = 0; i < MAX_EN; i++) {
        Enemy *e = &en[i];
        if (!e->active) continue;
        int x = ex(e), y = ey(e);
        int sx = x - cam_x, sy = y - cam_y;
        int z = e->z >> 8;
        int pal, tile = 0, flip = 0;
        int flash = e->flash > 0;
        if (e->dying && !is_big(e) && ((e->t >> 1) & 1)) flash = 1;
        if ((e->state == S_PHASE || (e->type >= EN_WARDEN && e->state == S_SUMMON)) && (frame_count & 8)) flash = 1;   /* shield shimmer */
        switch (e->type) {
        case EN_SLIME: {
            pal = PB_SLIME + ((cur_area_id >= 2 && cur_area_id <= 4) ? 1 : 0);   /* the crypt's blue slimes; the Amber lands keep the green ones */
            tile = OT_SLIME + e->anim * 4;
            spr16(sx - 8, sy - 15 - z, tile, flash ? PB_FLASH : pal, 0, y);
            break;
        }
        case EN_BAT:
            tile = OT_BAT + e->anim * 4;
            spr16(sx - 8, sy - 12 - z, tile, flash ? PB_FLASH : PB_BAT, 0, y + 20);
            break;
        case EN_WOLF:
            tile = OT_WOLF + e->anim * 4;
            flip = (e->dir == DIR_RIGHT);
            spr16(sx - 8, sy - 15, tile, flash ? PB_FLASH : PB_WOLF, flip ? SF_HFLIP : 0, y);
            break;
        case EN_SKELETON: {
            int d = e->dir;
            if (e->state == S_WINDUP || e->state == S_ATTACK)
                tile = OT_SKEL + (d == DIR_DOWN ? SKF_ATK_D : d == DIR_UP ? SKF_ATK_U : SKF_ATK_S) * 4;
            else
                tile = OT_SKEL + ((d == DIR_DOWN ? SKF_WALK_D : d == DIR_UP ? SKF_WALK_U : SKF_WALK_S) + ((e->anim >> 3) & 3)) * 4;
            spr16(sx - 8, sy - 15, tile, flash ? PB_FLASH : PB_SKEL, d == DIR_RIGHT ? SF_HFLIP : 0, y);
            break;
        }
        case EN_WARDEN:
        case EN_TYRANT: {
            int wf;
            switch (e->state) {
            case S_WINDUP: wf = (e->type == EN_TYRANT || e->hx == 0) ? WF_BASH : WF_RAISED; break;
            case S_CHARGE: case S_RUSH: wf = WF_BASH; break;
            case S_SWING: case S_SPIN: case S_STOMP: wf = WF_SMASH; break;
            case S_FAN: case S_BREATH: wf = WF_TOSS; break;
            case S_METEOR: wf = WF_RAISED; break;
            case S_POUND: case S_GEYSER: wf = ((e->t >> 3) & 1) ? WF_SMASH : WF_RAISED; break;
            case S_SUMMON: case S_INTRO: wf = WF_SUMMON; break;
            case S_TIRED: wf = WF_DAZED; break;
            case S_DORMANT: wf = WF_IDLE; break;
            default: wf = ((e->anim >> 3) & 1) ? WF_WALK1 : WF_WALK2; break;
            }
            flip = e->state == S_SPIN ? ((e->t >> 2) & 1) : (e->dir == DIR_LEFT);
            if (e->state == S_WINDUP && (e->t & 4)) flash = 1;                       /* telegraph */
            if (e->type == EN_TYRANT)                                              /* a head taller than the Warden */
                spr_add_aff(sx - 32, sy - 50, SH_SQUARE, 2, OT_WARDEN + wf * 16, flash ? PB_FLASH : PB_BOSS, 2, SF_DOUBLE | (flip ? SF_HFLIP : 0), y, 0, 205);
            else
                spr_add(sx - 16, sy - 30, SH_SQUARE, 2, OT_WARDEN + wf * 16, flash ? PB_FLASH : PB_BOSS, 2, flip ? SF_HFLIP : 0, y);
            break;
        }
        case EN_BOSS:
            if (e->dying && (e->t >> 2) & 1) flash = 1;
            tile = OT_BOSS + e->anim * 16;
            if (e->state == S_SWEEP && e->anim == KF_CHARGE) {                       /* afterimages trail the charge */
                int dirc = e->hx == arena_x0 ? 1 : -1;
                for (int k = 1; k <= 2; k++)
                    spr_add(sx - 16 - dirc * k * 14, sy - 30 - z, SH_SQUARE, 2, tile, PB_BOSS, 2, SF_ALPHA, y - 4 * k);
            }
            spr_add(sx - 16, sy - 30 - z, SH_SQUARE, 2, tile, flash ? PB_FLASH : PB_BOSS, 2, 0, y);
            break;
        }
        if (fade_level == 0) {
            if (is_big(e)) {
                int w = 8 + (e->z >> 11);
                (void)w;
                spr_add(sx - 16, sy - 4, SH_WIDE, 0, OT_SHADOW, PB_FX, 2, SF_ALPHA, -2000 + y);
                spr_add(sx, sy - 4, SH_WIDE, 0, OT_SHADOW, PB_FX, 2, SF_ALPHA, -2000 + y);
            } else if (e->type == EN_BAT) {
                spr_add(sx - 4, sy - 4, SH_SQUARE, 0, OT_SHADOW_S, PB_FX, 2, SF_ALPHA, -2000 + y);
            } else if (!(e->dying && e->t > 4)) {
                int shrink = e->type == EN_SLIME && z > 5;
                spr_add(sx - (shrink ? 4 : 8), sy - 4, shrink ? SH_SQUARE : SH_WIDE, 0, shrink ? OT_SHADOW_S : OT_SHADOW, PB_FX, 2, SF_ALPHA, -2000 + y);
            }
        }
        if (e->alert && !e->dying) {
            int top = e->type == EN_BOSS ? 44 : (e->type == EN_TYRANT ? 48 : (e->type == EN_WARDEN ? 40 : 24));
            spr_add(sx - 8, sy - top - z - (e->alert & 2 ? 1 : 0), SH_SQUARE, 1, OT_ALERT + ((frame_count >> 3) & 1) * 4, PB_UI, 2, 0, 20000);
        }
    }
    hz_draw();
    boss_lane_draw();
}
