#include "game.h"
#include "assets.h"
#include "audio_data.h"

Enemy en[MAX_EN];
int boss_dead_flag;

enum { S_IDLE, S_ALERT, S_HOP, S_WINDUP, S_ATTACK, S_RECOVER, S_CHARGE, S_TIRED, S_CHASE, S_SWOOP, S_RETREAT,
       S_DORMANT, S_INTRO, S_FAN, S_SUMMON, S_SLAM_UP, S_SLAM_DOWN, S_RING, S_DEAD };

typedef struct { s16 hp; u8 atk, def; u16 xp; u8 glo, ghi, hw, hh; } EnInfo;
static const EnInfo einfo[EN_COUNT] = {
    { 0, 0, 0, 0, 0, 0, 0, 0 },
    { 14, 5, 0, 6, 1, 4, 5, 7 },     /* slime */
    { 8, 6, 0, 7, 1, 4, 5, 5 },      /* bat */
    { 30, 9, 1, 18, 4, 9, 6, 6 },    /* wolf */
    { 38, 12, 2, 26, 6, 12, 5, 7 },  /* skeleton */
    { 420, 18, 3, 320, 150, 200, 9, 12 }, /* boss */
};

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
        e->active = 1;
        e->x = (s32)(tx * 16 + 8) << 8;
        e->y = (s32)(ty * 16 + 15) << 8;
        e->hp = e->maxhp = einfo[type].hp;
        e->state = (type == EN_BOSS) ? S_DORMANT : S_IDLE;
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

/* ------------------------------------------------------------------ helpers */
static int en_move(Enemy *e, int vx, int vy) {
    const EnInfo *in = &einfo[e->type];
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
    const EnInfo *in = &einfo[e->type];
    int dx = iabs(ppx() - ex(e)), dy = iabs((ppy() - 6) - (ey(e) - in->hh / 2));
    return dx < in->hw + 5 + extra && dy < (in->hh >> 1) + 6 + extra;
}

static void touch_damage(Enemy *e, int mult16) {
    if (pl.state == PS_DEAD) return;
    if (touches_player(e, 0)) {
        int atk = einfo[e->type].atk * mult16 >> 4;
        int d = atk + rnd_n(atk / 4 + 1);
        player_hurt(d, ex(e), ey(e) - 6);
    }
}

static void smoke_at(int x, int y, int n) {
    for (int i = 0; i < n; i++) part_spawn(PK_SMOKE, x - 6 + rnd_n(12), y - 4 - rnd_n(8), rnd_n(40) - 20, -20 - rnd_n(30), 20 + rnd_n(10), 0);
}

static void drop_loot(Enemy *e) {
    const EnInfo *in = &einfo[e->type];
    int x = ex(e), y = ey(e);
    int g = in->glo + rnd_n(in->ghi - in->glo + 1);
    int n5 = g / 5, n1 = g % 5;
    if (n1 > 2) n1 = 2;
    for (int i = 0; i < n5 && i < 5; i++) pickup_spawn(PK_COIN5, x, y);
    for (int i = 0; i < n1; i++) pickup_spawn(PK_COIN, x, y);
    int r = rnd_n(100), luck = pl.lck;
    if (e->type == EN_BOSS) { for (int i = 0; i < 4; i++) pickup_spawn(PK_HEART, x - 12 + i * 8, y); for (int i = 0; i < 3; i++) pickup_spawn(PK_MANA, x - 8 + i * 8, y); return; }
    if (r < 20 + luck / 2) pickup_spawn(PK_HEART, x, y);
    else if (r < 32 + luck / 2) pickup_spawn(PK_MANA, x, y);
    else if (r < 36 + luck / 3) pickup_spawn(PK_POTION, x, y);
}

static void enemy_kill(Enemy *e) {
    e->dying = 1;
    e->t = 0;
    e->alert = 0;
    if (e->type == EN_BOSS) { e->state = S_DEAD; snd_sfx(SFX_BOSS_DIE); }
    else snd_sfx(SFX_ENEMY_DIE);
}

int enemy_damage(Enemy *e, int dmg, int kx, int ky, int crit) {
    if (e->dying || !e->active) return 0;
    const EnInfo *in = &einfo[e->type];
    dmg -= in->def;
    if (e->type == EN_WOLF && e->state == S_TIRED) dmg = dmg * 3 / 2;
    if (e->type == EN_BOSS && e->state == S_RECOVER) dmg = dmg * 5 / 4;
    if (dmg < 1) dmg = 1;
    if (e->type == EN_BOSS && e->state == S_DORMANT) dmg = 0;
    e->hp -= (s16)dmg;
    e->flash = e->type == EN_BOSS ? 3 : 5;
    int x = ex(e), y = ey(e) - 8 - (e->z >> 8);
    dmgnum(x, y - (e->type == EN_BOSS ? 14 : 0), dmg, crit ? 3 : 0, crit);
    part_burst(PK_HIT, x, y, 1, 0, 8);
    snd_sfx(crit ? SFX_CRIT : SFX_HIT);
    shake_add(crit ? 3 : 1);
    hitstop_t = imax(hitstop_t, crit ? 6 : 3);
    if (e->type != EN_BOSS) {
        int a = vec_angle(kx, ky);
        int k = (e->type == EN_SKELETON) ? 440 : 600;
        e->kx = (s16)((isin(a) * k) >> 8);
        e->ky = (s16)((-icos(a) * k) >> 8);
        e->stun = (u8)(e->type == EN_SKELETON ? 6 : 10);
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
    for (int i = 0; i < MAX_EN; i++) {
        Enemy *e = &en[i];
        if (!e->active || e->dying || e->hitid == id) continue;
        int dx = ex(e) - cx, dy = ey(e) - 6 - cy;
        if (idist(dx, dy) <= r + (e->type == EN_BOSS ? 14 : 6)) {
            e->hitid = (u8)id;
            int crit = rnd_n(100) < 4 + pl.lck;
            int d = dmg + rnd_n(dmg / 4 + 1);
            if (crit) d = (d * 7) >> 2;
            enemy_damage(e, d, dx, dy, crit);
        }
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
            e->vx = (s16)((isin(a) * 290) >> 8);
            e->vy = (s16)((-icos(a) * 290) >> 8);
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
        if (pdist(e) < 90 && pl.state != PS_DEAD) { e->state = S_CHASE; e->t = 200; snd_sfx(SFX_BAT); }
        break;
    }
    case S_CHASE: {
        e->z = (s16)hover;
        int vx, vy;
        toward(e, 220, &vx, &vy);
        int wob = isin(e->t2 * 7) >> 2;
        en_move_free(e, vx + wob, vy + (icos(e->t2 * 5) >> 3));
        if (pdist(e) < 46) { e->state = S_ALERT; e->t = 14; e->alert = 14; snd_sfx(SFX_ALERT); }
        if (e->t && --e->t == 0) e->state = S_IDLE;
        break;
    }
    case S_ALERT:
        e->z = (s16)hover;
        e->vx = 0; e->vy = 0;
        if (--e->t == 0) {
            int vx, vy;
            toward(e, 620, &vx, &vy);
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
        if (pdist(e) < 105 && pl.state != PS_DEAD) { e->state = S_CHASE; e->t = 0; }
        touch_damage(e, 12);
        break;
    case S_CHASE: {
        int vx, vy;
        toward(e, 240, &vx, &vy);
        en_move(e, vx, vy);
        e->anim = (u8)(1 + ((frame_count >> 2) & 1));
        if (vx) e->dir = (u8)(vx < 0 ? DIR_LEFT : DIR_RIGHT);
        touch_damage(e, 14);
        if (pdist(e) < 60) { e->state = S_ALERT; e->t = 26; e->alert = 26; snd_sfx(SFX_WOLF); }
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
            toward(e, 880, &vx, &vy);
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
        if (pdist(e) < 100 && pl.state != PS_DEAD) { e->state = S_CHASE; }
        break;
    case S_CHASE: {
        int dx = ppx() - ex(e), dy = ppy() - ey(e);
        int vx = 0, vy = 0;
        if (iabs(dx) > 3) vx = dx > 0 ? 150 : -150;
        if (iabs(dy) > 3) vy = dy > 0 ? 150 : -150;
        if (vx && vy) { vx = vx * 181 >> 8; vy = vy * 181 >> 8; }
        en_move(e, vx, vy);
        face_toward(e, dx, dy);
        e->anim++;
        touch_damage(e, 8);
        if (idist(dx, dy) < 27) { e->state = S_WINDUP; e->t = 22; e->alert = 22; snd_sfx(SFX_ALERT); }
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
            int atk = einfo[EN_SKELETON].atk;
            player_hurt(atk + rnd_n(atk / 4 + 1), ex(e), ey(e) - 6);
        }
        if (--e->t == 0) { e->state = S_RECOVER; e->t = 24; }
        break;
    }
    case S_RECOVER:
        if (--e->t == 0) e->state = S_CHASE;
        break;
    default: e->state = S_IDLE;
    }
}

/* ------------------------------------------------------------------ boss */
static int boss_phase2(const Enemy *e) { return e->hp * 2 < e->maxhp; }

static void boss_fan(Enemy *e, int n, int spread) {
    int base = vec_angle(ppx() - ex(e), ppy() - 8 - (ey(e) - 16));
    for (int k = 0; k < n; k++) {
        int a = base + (n > 1 ? ((k * 2 - (n - 1)) * spread) / (n - 1) : 0);
        proj_spawn(PJ_ORB, 1, ex(e), ey(e) - 14, a & 255, 400, 12 + rnd_n(4));
    }
    snd_sfx(SFX_MAGIC);
}

static void boss_next(Enemy *e) {
    static const u8 p1[5] = { S_FAN, S_SLAM_UP, S_SUMMON, S_FAN, S_SLAM_UP };
    static const u8 p2[7] = { S_FAN, S_RING, S_SLAM_UP, S_FAN, S_SUMMON, S_RING, S_SLAM_UP };
    int p2f = boss_phase2(e);
    e->state = p2f ? p2[e->aux % 7] : p1[e->aux % 5];
    e->aux++;
    e->t = 0;
    e->anim = 2;
    if (e->state == S_SLAM_UP) { e->hx = (s16)ppx(); e->hy = (s16)ppy(); }
}

static void ai_boss(Enemy *e) {
    int p2 = boss_phase2(e);
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
        if (e->t >= 96) { e->z = 60 * 38; e->state = S_RECOVER; e->t = 20; e->aux = 0; }
        break;
    case S_RECOVER:
        /* idle hover */
        e->anim = (u8)((frame_count >> 5) & 1);
        e->z = (s16)(60 * 38 + isin(frame_count * 4) * 5);
        {
            int dx = ppx() - ex(e);
            int vx = dx > 8 ? 130 : (dx < -8 ? -130 : 0);
            if (vx) {
                int nx = (int)(e->x + (p2 ? vx * 3 / 2 : vx));
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
        if (e->t == 26 || e->t == 46 || (p2 && e->t == 66)) boss_fan(e, p2 ? 9 : 7, 46);
        if (e->t >= (p2 ? 84 : 66)) { e->state = S_RECOVER; e->t = p2 ? 30 : 55; }
        break;
    case S_SUMMON:
        e->anim = 2;
        e->t++;
        if (e->t == 28) {
            int skel = 0;
            for (int i = 0; i < MAX_EN; i++) if (en[i].active && !en[i].dying && en[i].type == EN_SKELETON) skel++;
            snd_sfx(SFX_BOSS_ROAR);
            if (skel < 4) {
                for (int s = -1; s <= 1; s += 2) {
                    int tx = (ex(e) + s * 36) >> 4, ty = (ey(e) + 10) >> 4;
                    if (!(tile_flags(tx, ty) & TF_SOLID)) {
                        enemy_spawn(EN_SKELETON, tx, ty);
                        smoke_at(tx * 16 + 8, ty * 16 + 12, 6);
                    }
                }
            }
            shake_add(2);
        }
        if (e->t >= 58) { e->state = S_RECOVER; e->t = p2 ? 25 : 50; }
        break;
    case S_RING:
        e->anim = 2;
        e->t++;
        if (e->t > 20 && e->t < 56 && (e->t & 3) == 0) {
            int a = (e->t * 16) & 255;
            proj_spawn(PJ_ORB, 1, ex(e), ey(e) - 14, a, 330, 11);
            proj_spawn(PJ_ORB, 1, ex(e), ey(e) - 14, (a + 128) & 255, 330, 11);
            snd_sfx(SFX_MAGIC);
        }
        if (e->t >= 70) { e->state = S_RECOVER; e->t = 28; }
        break;
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
            int n = p2 ? 12 : 8;
            for (int k = 0; k < n; k++) proj_spawn(PJ_ORB, 1, cx, cy - 6, (k * 256 / n) & 255, 340, 11);
            if (idist(ppx() - cx, ppy() - cy) < 26) player_hurt(20, cx, cy);
            e->state = S_RECOVER;
            e->t = 45;
            e->z = 0;
            e->aux++;
        }
        break;
    default: break;
    }
    /* keep the boss in the room */
    int bx = ex(e), by = ey(e);
    if (e->state != S_SLAM_UP && e->state != S_SLAM_DOWN) {
        if (by < 4 * 16) e->y = (s32)(4 * 16) << 8;
        if (by > 9 * 16 + 8) e->y = (s32)(9 * 16 + 8) << 8;
    }
    if (bx < 9 * 16) e->x = (s32)(9 * 16) << 8;
    if (bx > 23 * 16) e->x = (s32)(23 * 16) << 8;
}

/* ------------------------------------------------------------------ update */
static void dying_update(Enemy *e) {
    e->t++;
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
            player_add_xp(einfo[EN_BOSS].xp);
            dmgnum(ex(e), ey(e) - 24, einfo[EN_BOSS].xp, 3, 1);
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
        player_add_xp(einfo[e->type].xp);
        dmgnum(ex(e), ey(e) - 22, einfo[e->type].xp, 3, 0);
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
        if (e->dying && e->type != EN_BOSS && ((e->t >> 1) & 1)) flash = 1;
        switch (e->type) {
        case EN_SLIME: {
            pal = PB_SLIME + ((cur_area_id == 2) ? 1 : 0);
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
        case EN_BOSS:
            if (e->dying && (e->t >> 2) & 1) flash = 1;
            tile = OT_BOSS + e->anim * 16;
            spr_add(sx - 16, sy - 30 - z, SH_SQUARE, 2, tile, flash ? PB_FLASH : PB_BOSS, 2, 0, y);
            break;
        }
        if (fade_level == 0) {
            if (e->type == EN_BOSS) {
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
            int top = e->type == EN_BOSS ? 44 : 24;
            spr_add(sx - 8, sy - top - z - (e->alert & 2 ? 1 : 0), SH_SQUARE, 1, OT_ALERT + ((frame_count >> 3) & 1) * 4, PB_UI, 2, 0, 20000);
        }
    }
}
