#include "game.h"
#include "assets.h"
#include "audio_data.h"

Enemy en[MAX_EN];
int boss_dead_flag;

enum { S_IDLE, S_ALERT, S_HOP, S_WINDUP, S_ATTACK, S_RECOVER, S_CHARGE, S_TIRED, S_CHASE, S_SWOOP, S_RETREAT,
       S_DORMANT, S_INTRO, S_FAN, S_SUMMON, S_SLAM_UP, S_SLAM_DOWN, S_RING, S_DEAD,
       S_PHASE, S_SWEEP, S_RAIN, S_SWING };

typedef struct { s16 hp; u8 atk, def; u16 xp; u8 glo, ghi, hw, hh; } EnInfo;

/* Stats by difficulty tier (0 = woods, 1 = crypt, 2 = depths). The tier of an enemy is fixed when it spawns,
   so the same slime/bat/wolf/skeleton hits harder and lasts longer the deeper the hero goes. */
#define BOSS_INFO    { 9000, 34, 20, 1500, 150, 200, 9, 12 }
#define WARDEN_INFO  { 3200, 30, 10, 520, 120, 160, 9, 10 }
static const EnInfo einfo[3][EN_COUNT] = {
    { { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 18, 6, 0, 7, 1, 4, 5, 7 },       /* slime */
      { 10, 7, 0, 8, 1, 4, 5, 5 },       /* bat */
      { 44, 11, 1, 20, 4, 9, 6, 6 },     /* wolf */
      { 60, 13, 2, 28, 6, 12, 5, 7 },    /* skeleton */
      BOSS_INFO, WARDEN_INFO },
    { { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 90, 15, 3, 30, 4, 10, 5, 7 },
      { 64, 15, 2, 26, 2, 7, 5, 5 },
      { 160, 22, 4, 60, 8, 16, 6, 6 },
      { 170, 22, 5, 46, 8, 18, 5, 7 },
      BOSS_INFO, WARDEN_INFO },
    { { 0, 0, 0, 0, 0, 0, 0, 0 },
      { 220, 26, 6, 60, 8, 16, 5, 7 },
      { 150, 26, 4, 50, 4, 10, 5, 5 },
      { 320, 33, 8, 110, 12, 24, 6, 6 },
      { 340, 33, 8, 84, 12, 26, 5, 7 },
      BOSS_INFO, WARDEN_INFO },
};
static const u8 area_tier[4] = { 0, 0, 1, 2 };

static const EnInfo *info(const Enemy *e) { return &einfo[e->tier < 3 ? e->tier : 2][e->type]; }
static int is_big(const Enemy *e) { return e->type == EN_BOSS || e->type == EN_WARDEN; }

static const s8 dxv[4] = { 0, 0, -1, 1 };
static const s8 dyv[4] = { 1, -1, 0, 0 };

static int ppx(void) { return (int)(pl.x >> 8); }
static int ppy(void) { return (int)(pl.y >> 8); }
static int ex(const Enemy *e) { return (int)(e->x >> 8); }
static int ey(const Enemy *e) { return (int)(e->y >> 8); }

void enemies_clear(void) {
    for (int i = 0; i < MAX_EN; i++) { en[i].active = 0; en[i].type = EN_NONE; }
    boss_dead_flag = 0;
}

Enemy *enemy_spawn(int type, int tx, int ty) {
    for (int i = 0; i < MAX_EN; i++) {
        if (en[i].active) continue;
        Enemy *e = &en[i];
        u8 *p = (u8 *)e;
        for (unsigned k = 0; k < sizeof(*e); k++) p[k] = 0;
        e->type = (u8)type;
        e->tier = area_tier[cur_area_id & 3];
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
static int en_move(Enemy *e, int vx, int vy) {
    const EnInfo *in = info(e);
    int moved = 0;
    int nx = (int)(e->x + vx), ny = (int)(e->y + vy);
    if (vx && box_free(nx >> 8, ey(e), in->hw, in->hh)) { e->x = nx; moved = 1; }
    if (vy && box_free(ex(e), ny >> 8, in->hw, in->hh)) { e->y = ny; moved = 1; }
    return moved;
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
    if (big && (e->state == S_DORMANT || e->state == S_INTRO || e->state == S_PHASE || (e->type == EN_WARDEN && e->state == S_SUMMON))) {
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
            int sp = 290 + e->tier * 40;
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
        toward(e, 220 + e->tier * 25, &vx, &vy);
        int wob = isin(e->t2 * 7) >> 2;
        en_move_free(e, vx + wob, vy + (icos(e->t2 * 5) >> 3));
        if (pdist(e) < 46) { e->state = S_ALERT; e->t = (u16)(14 - e->tier * 2); e->alert = (u8)e->t; snd_sfx(SFX_ALERT); }
        if (e->t && --e->t == 0) e->state = S_IDLE;
        break;
    }
    case S_ALERT:
        e->z = (s16)hover;
        e->vx = 0; e->vy = 0;
        if (--e->t == 0) {
            int vx, vy;
            toward(e, 620 + e->tier * 50, &vx, &vy);
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
        toward(e, 240 + e->tier * 20, &vx, &vy);
        en_move(e, vx, vy);
        e->anim = (u8)(1 + ((frame_count >> 2) & 1));
        if (vx) e->dir = (u8)(vx < 0 ? DIR_LEFT : DIR_RIGHT);
        touch_damage(e, 14);
        if (pdist(e) < 60) { e->state = S_ALERT; e->t = (u16)(26 - e->tier * 3); e->alert = (u8)e->t; snd_sfx(SFX_WOLF); }
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
            toward(e, 880 + e->tier * 40, &vx, &vy);
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
        int sp = 150 + e->tier * 20;
        if (iabs(dx) > 3) vx = dx > 0 ? sp : -sp;
        if (iabs(dy) > 3) vy = dy > 0 ? sp : -sp;
        if (vx && vy) { vx = vx * 181 >> 8; vy = vy * 181 >> 8; }
        en_move(e, vx, vy);
        face_toward(e, dx, dy);
        e->anim++;
        touch_damage(e, 8);
        if (idist(dx, dy) < 27) { e->state = S_WINDUP; e->t = (u16)(22 - e->tier * 4); e->alert = (u8)e->t; snd_sfx(SFX_ALERT); }
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
            if (e->tier >= 1 && e->aux == 0 && pdist(e) < 36 && pl.state != PS_DEAD) {
                e->aux = 1; e->state = S_WINDUP; e->t = 9; e->alert = 9; face_toward(e, ppx() - ex(e), ppy() - ey(e));
            } else { e->aux = 0; e->state = S_RECOVER; e->t = (u16)(24 - e->tier * 3); }
        }
        break;
    }
    case S_RECOVER:
        if (--e->t == 0) e->state = S_CHASE;
        break;
    default: e->state = S_IDLE;
    }
}

/* ------------------------------------------------------------------ boss: The Hollow King */
#define ARENA_X0 (9 * 16)
#define ARENA_X1 (23 * 16)
#define ARENA_Y0 (4 * 16)
#define ARENA_Y1 (9 * 16 + 8)

static int boss_floor(const Enemy *e) { return e->phase == 0 ? e->maxhp * 2 / 3 : e->maxhp / 3; }

static int orb_dmg(const Enemy *e, int num, int den) {
    int d = info(e)->atk * num / den;
    return d + rnd_n(d / 5 + 1);
}

static void boss_fan(Enemy *e, int n, int spread) {
    int base = vec_angle(ppx() - ex(e), ppy() - 8 - (ey(e) - 16));
    for (int k = 0; k < n; k++) {
        int a = base + (n > 1 ? ((k * 2 - (n - 1)) * spread) / (n - 1) : 0);
        proj_spawn(PJ_ORB, 1, ex(e), ey(e) - 14, a & 255, 380 + e->phase * 30, orb_dmg(e, 2, 3));
    }
    snd_sfx(SFX_MAGIC);
}

/* reinforcements appear at fixed spots in the arena so they never spawn inside walls */
static void arena_adds(int n, int tier) {
    static const u8 spots[6][2] = { { 10, 8 }, { 21, 8 }, { 12, 6 }, { 19, 6 }, { 11, 5 }, { 20, 5 } };
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
        { S_FAN, S_RING, S_SWEEP, S_SLAM_UP, S_FAN, S_SUMMON, S_RING, S_SWEEP },
        { S_RAIN, S_FAN, S_SWEEP, S_RING, S_SLAM_UP, S_RAIN, S_SLAM_UP, S_SUMMON },
    };
    e->state = pat[e->phase > 2 ? 2 : e->phase][e->aux & 7];
    e->aux++;
    e->t = 0;
    e->anim = 2;
    if (e->state == S_SLAM_UP) { e->hx = (s16)ppx(); e->hy = (s16)ppy(); }
    if (e->state == S_SWEEP) e->hx = (s16)(ppx() < 16 * 16 ? ARENA_X1 : ARENA_X0);      /* charge starts on the far wall */
}

static void ai_boss(Enemy *e) {
    int ph = e->phase;
    /* at 2/3 and 1/3 health the king shields himself and calls reinforcements; he is only hurt again once they fall */
    if (ph < 2 && e->hp <= boss_floor(e) && e->state != S_DORMANT && e->state != S_INTRO && e->state != S_PHASE &&
        e->state != S_SLAM_UP && e->state != S_SLAM_DOWN && e->state != S_SWEEP) {
        e->phase = (u8)(ph + 1);
        ph = e->phase;
        e->state = S_PHASE;
        e->t = 0;
        e->aux = 0;
    }
    switch (e->state) {
    case S_DORMANT:
        e->anim = 0;
        if (pl.state != PS_DEAD && (ppy() < 12 * 16 && idist(ppx() - ex(e), ppy() - ey(e)) < 150)) {
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
        e->anim = (u8)(e->t < 50 ? 2 : 0);
        e->z = (s16)(e->t < 60 ? e->t * 38 : 60 * 38);
        if (e->t == 20 || e->t == 40) shake_add(3);
        if (e->t == 70) { snd_play_song(SONG_BOSS); banner_show("THE HOLLOW KING"); }
        if (e->t >= 96) { e->z = 60 * 38; e->state = S_RECOVER; e->t = 30; e->aux = 0; }
        break;
    case S_PHASE:
        e->anim = 2;
        e->t++;
        e->z = (s16)(60 * 38 + isin(frame_count * 4) * 5);
        if (e->t == 1) { snd_sfx(SFX_BOSS_ROAR); shake_add(6); }
        if (e->t == 36) { snd_sfx(SFX_BOSS_ROAR); shake_add(3); arena_adds(ph == 1 ? 3 : 4, 2); }
        if (e->t > 70 && (count_type(EN_SKELETON) == 0 || e->t > 1800)) { e->state = S_TIRED; e->t = 150; snd_sfx(SFX_EXPLODE); shake_add(4); }
        break;
    case S_TIRED:                                              /* shield broken: the king crashes down, dazed */
        e->anim = 0;
        if (e->z > 0) e->z = (s16)imax(0, e->z - 220);
        touch_damage(e, 8);
        if (--e->t == 0) { e->state = S_RECOVER; e->t = 30; }
        break;
    case S_RECOVER:
        /* idle hover */
        e->anim = (u8)((frame_count >> 5) & 1);
        {
            int target = 60 * 38 + isin(frame_count * 4) * 5;
            e->z = (s16)(e->z + ((target - e->z) >> 3));
            int dx = ppx() - ex(e);
            int sp = 130 + ph * 22;
            int vx = dx > 8 ? sp : (dx < -8 ? -sp : 0);
            if (vx) {
                int nx = (int)(e->x + vx);
                if ((nx >> 8) > 10 * 16 && (nx >> 8) < 22 * 16) e->x = nx;
            }
        }
        if (e->t) e->t--;
        if (e->t == 0) boss_next(e);
        touch_damage(e, 16);
        break;
    case S_FAN:
        e->anim = 2;
        e->t++;
        if (e->t == 26 || e->t == 44 || (ph >= 1 && e->t == 62) || (ph >= 2 && e->t == 80)) boss_fan(e, 7 + ph * 2, 46);
        if (e->t >= 66 + ph * 16) { e->state = S_RECOVER; e->t = (u16)(54 - ph * 14); }
        break;
    case S_SUMMON:
        e->anim = 2;
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
    case S_RING: {
        int arms = 2 + ph;
        e->anim = 2;
        e->t++;
        if (e->t > 20 && e->t < 56 && (e->t & 3) == 0) {
            int a = (e->t * 16) & 255;
            for (int k = 0; k < arms; k++) proj_spawn(PJ_ORB, 1, ex(e), ey(e) - 14, (a + k * 256 / arms) & 255, 330, orb_dmg(e, 3, 5));
            snd_sfx(SFX_MAGIC);
        }
        if (e->t >= 70) { e->state = S_RECOVER; e->t = (u16)(34 - ph * 6); }
        break;
    }
    case S_SLAM_UP:
        e->anim = 3;
        e->t++;
        if (e->z < 100 * 38) e->z += 160;
        if (e->t < 24) { e->hx = (s16)ppx(); e->hy = (s16)ppy(); }
        {
            int tx = e->hx * 256, ty = e->hy * 256;
            e->x += (tx - e->x) >> 4;
            e->y += (ty - e->y) >> 4;
        }
        if (e->t >= 34) { e->state = S_SLAM_DOWN; e->t = 0; }
        break;
    case S_SLAM_DOWN:
        e->anim = 4;
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
            for (int k = 0; k < n; k++) proj_spawn(PJ_ORB, 1, cx, cy - 6, (k * 256 / n) & 255, 340, orb_dmg(e, 3, 5));
            if (idist(ppx() - cx, ppy() - cy) < 28) player_hurt(info(e)->atk * 11 / 10, cx, cy);
            e->state = S_RECOVER;
            e->t = (u16)(48 - ph * 8);
        }
        break;
    case S_SWEEP:                                              /* wall-to-wall charge along the hero's row */
        e->t++;
        if (e->t < 34) {
            e->anim = 2;
            e->x += ((int)e->hx * 256 - e->x) >> 3;
            e->y += ((iclamp(ppy(), ARENA_Y0, ARENA_Y1) * 256) - e->y) >> 4;
            if (e->z > 0) e->z = (s16)imax(0, e->z - 110);
            e->alert = 3;
        } else if (e->t < 34 + 44) {
            e->anim = 3;
            e->x += (e->hx == ARENA_X0 ? 6 : -6) * 256;
            if ((e->t & 3) == 0) part_burst(PK_DUST, ex(e), ey(e) - 1, 2, 60, 10);
            if ((e->t & 7) == 0) shake_add(1);
            touch_damage(e, 30);
        } else { e->state = S_TIRED; e->t = 60; }
        break;
    case S_RAIN:                                               /* orbs fall from the ceiling across the room */
        e->anim = 3;
        e->t++;
        e->z = (s16)(70 * 38 + isin(frame_count * 4) * 5);
        if (e->t >= 24 && e->t < 108 && (e->t % 7) == 0) {
            for (int k = 0; k < 2; k++)
                proj_spawn(PJ_ORB, 1, ARENA_X0 - 20 + rnd_n(ARENA_X1 - ARENA_X0 + 40), 3 * 16 + 4, 128, 300, orb_dmg(e, 3, 5));
            snd_sfx(SFX_MAGIC);
        }
        if (e->t >= 130) { e->state = S_RECOVER; e->t = 26; }
        break;
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
static void warden_minions(Enemy *e) {
    for (int s = -1; s <= 1; s += 2) {
        int tx = (ex(e) + s * 34) >> 4, ty = (ey(e) + 6) >> 4;
        if (tile_flags(tx, ty) & TF_SOLID) continue;
        Enemy *m = enemy_spawn(EN_SKELETON, tx, ty);
        if (!m) return;
        m->tier = 1;
        m->hp = m->maxhp = einfo[1][EN_SKELETON].hp;
        m->state = S_CHASE;
        smoke_at(tx * 16 + 8, ty * 16 + 12, 6);
    }
}

static void ai_warden(Enemy *e) {
    int dx = ppx() - ex(e), dy = ppy() - ey(e);
    int enr = e->phase >= 1;                                  /* below half health */
    switch (e->state) {
    case S_DORMANT:
        e->anim = 0;
        if (pl.state != PS_DEAD && idist(dx, dy) < 120) {
            e->state = S_INTRO;
            e->t = 0;
            gflags |= GF_WARDEN_SEEN;
            snd_sfx(SFX_BOSS_ROAR);
            shake_add(4);
            banner_show("THE BONE WARDEN");
        }
        break;
    case S_INTRO:
        e->t++;
        if (e->t == 20) shake_add(3);
        if (e->t >= 70) { e->state = S_CHASE; e->t = 40; }
        break;
    case S_CHASE: {
        int vx, vy;
        toward(e, enr ? 185 : 135, &vx, &vy);
        en_move(e, vx, vy);
        face_toward(e, dx, dy);
        e->anim++;
        touch_damage(e, 12);
        if (!enr && e->hp * 2 < e->maxhp) { e->phase = 1; e->state = S_SUMMON; e->t = 0; break; }
        if (e->t) e->t--;
        if (e->t == 0) {
            static const u8 seq[6] = { 0, 1, 2, 0, 2, 1 };   /* 0 shield bash, 1 bone toss, 2 club smash */
            int pick = seq[e->aux % 6];
            e->aux++;
            e->hx = (s16)pick;
            if (pick == 1) { e->state = S_FAN; e->t = 0; }
            else { e->state = S_WINDUP; e->t = (u16)(pick == 0 ? 30 : 26); e->alert = (u8)e->t; snd_sfx(SFX_ALERT); }
        }
        break;
    }
    case S_WINDUP:
        face_toward(e, dx, dy);
        e->anim++;
        if (--e->t == 0) {
            if (e->hx == 0) {
                int vx, vy;
                toward(e, 640, &vx, &vy);
                e->vx = (s16)vx; e->vy = (s16)vy;
                e->state = S_CHARGE;
                e->t = 20;
                snd_sfx(SFX_WOLF);
            } else {
                e->state = S_SWING;
                e->t = 12;
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
        if (e->t == 8) { shake_add(3); part_burst(PK_DUST, fx, fy + 6, 4, 80, 14); snd_sfx(SFX_EXPLODE); }
        if (e->t <= 8 && e->t >= 5 && iabs(ppx() - fx) < 26 && iabs(ppy() - 6 - fy) < 22 && pl.state != PS_DEAD)
            player_hurt(info(e)->atk * 13 / 10 + rnd_n(6), ex(e), ey(e) - 6);
        if (--e->t == 0) { e->state = S_TIRED; e->t = (u16)(enr ? 30 : 46); }
        break;
    }
    case S_FAN: {
        e->t++;
        face_toward(e, dx, dy);
        if (e->t == 18 || e->t == 36 || (enr && e->t == 54)) {
            int base = vec_angle(dx, dy);
            int n = enr ? 5 : 3;
            for (int k = 0; k < n; k++)
                proj_spawn(PJ_ORB, 1, ex(e), ey(e) - 12, (base + (k * 2 - (n - 1)) * 10) & 255, 330, orb_dmg(e, 3, 5));
            snd_sfx(SFX_MAGIC);
        }
        if (e->t >= (enr ? 70 : 52)) { e->state = S_CHASE; e->t = (u16)(enr ? 24 : 44); }
        break;
    }
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
}

/* ------------------------------------------------------------------ update */
static void dying_update(Enemy *e) {
    e->t++;
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
        player_add_xp(info(e)->xp);
        dmgnum(ex(e), ey(e) - 22, info(e)->xp, 3, 0);
        e->active = 0;
    }
}

void enemies_update(void) {
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
        }
    }
}

/* ------------------------------------------------------------------ draw */
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
        if ((e->state == S_PHASE || (e->type == EN_WARDEN && e->state == S_SUMMON)) && (frame_count & 8)) flash = 1;   /* shield shimmer */
        switch (e->type) {
        case EN_SLIME: {
            pal = PB_SLIME + ((cur_area_id >= 2) ? 1 : 0);
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
        case EN_WARDEN: {
            /* the warden is a skeleton drawn at double size (affine scaling of the 16x16 art) */
            int d = e->dir;
            if (e->state == S_WINDUP || e->state == S_SWING || e->state == S_CHARGE)
                tile = OT_SKEL + (d == DIR_DOWN ? SKF_ATK_D : d == DIR_UP ? SKF_ATK_U : SKF_ATK_S) * 4;
            else
                tile = OT_SKEL + ((d == DIR_DOWN ? SKF_WALK_D : d == DIR_UP ? SKF_WALK_U : SKF_WALK_S) + ((e->anim >> 3) & 3)) * 4;
            if (e->state == S_WINDUP && (e->t & 4)) flash = 1;                       /* telegraph */
            spr_add_aff(sx - 16, sy - 31, SH_SQUARE, 1, tile, flash ? PB_FLASH : PB_SKEL, 2, SF_DOUBLE | (d == DIR_RIGHT ? SF_HFLIP : 0), y, 0, 128);
            break;
        }
        case EN_BOSS:
            if (e->dying && (e->t >> 2) & 1) flash = 1;
            tile = OT_BOSS + e->anim * 16;
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
            int top = e->type == EN_BOSS ? 44 : (e->type == EN_WARDEN ? 40 : 24);
            spr_add(sx - 8, sy - top - z - (e->alert & 2 ? 1 : 0), SH_SQUARE, 1, OT_ALERT + ((frame_count >> 3) & 1) * 4, PB_UI, 2, 0, 20000);
        }
    }
}
