#include "game.h"
#include "assets.h"
#include "audio_data.h"

static Particle parts[MAX_PART];
static Proj projs[MAX_PROJ];
static Pickup picks[MAX_PICK];
static DmgNum nums[MAX_NUM];
static u8 fire_id = 100;

void fx_clear(void) {
    for (int i = 0; i < MAX_PART; i++) parts[i].life = 0;
    for (int i = 0; i < MAX_PROJ; i++) projs[i].type = PJ_NONE;
    for (int i = 0; i < MAX_PICK; i++) picks[i].type = PK_NONE;
    for (int i = 0; i < MAX_NUM; i++) nums[i].life = 0;
}

/* ------------------------------------------------------------------ particles */
typedef struct { u8 base, n, per, loop; } PInfo;
static const PInfo pinfo[] = {
    { PT_SPARKLE, 4, 4, 0 },   /* PK_SPARKLE */
    { PT_SMOKE, 4, 6, 0 },     /* PK_SMOKE */
    { PT_LEAF, 2, 9, 1 },      /* PK_LEAF */
    { PT_HIT, 3, 3, 0 },       /* PK_HIT */
    { PT_DUST, 3, 4, 0 },      /* PK_DUST */
    { PT_EMBER, 3, 5, 1 },     /* PK_EMBER */
    { PT_HEAL, 3, 6, 1 },      /* PK_HEAL */
    { 0, 4, 3, 0 },            /* PK_EXPL (16x16) */
    { PT_EMBER, 3, 4, 0 },     /* PK_FIRE_TRAIL */
    { PT_SPARKLE, 4, 8, 1 },   /* PK_STAR */
    { 0, 1, 1, 0 },            /* PK_GHOST */
};

void part_spawn(int kind, int x, int y, int vx, int vy, int life, int gy) {
    for (int i = 0; i < MAX_PART; i++) {
        Particle *p = &parts[i];
        if (p->life) continue;
        p->kind = (u8)kind;
        p->frame = 0;
        p->ftick = 0;
        p->life = (u8)imin(life, 255);
        p->x = (s32)x * 256;
        p->y = (s32)y * 256;
        p->vx = (s16)vx;
        p->vy = (s16)vy;
        p->gy = (s8)gy;
        p->fspd = (s8)pinfo[kind].per;
        return;
    }
}

void part_burst(int kind, int x, int y, int n, int speed, int life) {
    for (int i = 0; i < n; i++) {
        int a = rnd_n(256);
        int s = speed ? speed / 2 + rnd_n(speed / 2 + 1) : 0;
        part_spawn(kind, x, y, (isin(a) * s) >> 8, (-icos(a) * s) >> 8, life - 3 + rnd_n(7), kind == PK_LEAF ? 6 : 0);
    }
}

void ghost_spawn(int x, int y, int frame, int dir) {
    (void)frame;
    int f = dir == DIR_DOWN ? HF_WALK_DOWN : dir == DIR_UP ? HF_WALK_UP : HF_WALK_SIDE;
    part_spawn(PK_GHOST, x, y, 0, 0, 14, 0);
    for (int i = 0; i < MAX_PART; i++)
        if (parts[i].life == 14 && parts[i].kind == PK_GHOST && parts[i].frame == 0 && parts[i].ftick == 0) {
            parts[i].frame = (u8)f;
            parts[i].ftick = (u8)(dir == DIR_RIGHT ? 1 : 0);
            parts[i].gy = (s8)(dir == DIR_RIGHT ? 1 : 0);
            break;
        }
}

static void parts_update(void) {
    for (int i = 0; i < MAX_PART; i++) {
        Particle *p = &parts[i];
        if (!p->life) continue;
        p->life--;
        p->x += p->vx;
        p->y += p->vy;
        if (p->kind != PK_GHOST) {
            p->vy = (s16)(p->vy + p->gy);
            if (p->kind == PK_LEAF) p->vx = (s16)(p->vx + (isin(frame_count * 3 + i * 40) >> 6));
            if (p->kind == PK_SMOKE) { p->vx = (s16)((p->vx * 15) >> 4); p->vy = (s16)((p->vy * 15) >> 4); }
            const PInfo *in = &pinfo[p->kind];
            if (++p->ftick >= p->fspd) {
                p->ftick = 0;
                if (p->frame + 1 < in->n) p->frame++;
                else if (in->loop) p->frame = 0;
            }
        }
    }
}

static void parts_draw(void) {
    for (int i = 0; i < MAX_PART; i++) {
        Particle *p = &parts[i];
        if (!p->life) continue;
        int sx = (int)(p->x >> 8) - cam_x, sy = (int)(p->y >> 8) - cam_y;
        if (p->kind == PK_EXPL) {
            spr_add(sx - 8, sy - 8, SH_SQUARE, 1, OT_EXPL + p->frame * 4, PB_FX, 2, 0, 25000);
        } else if (p->kind == PK_GHOST) {
            if (p->life > 2)
                spr_add(sx - 8, sy - 15, SH_SQUARE, 1, p->frame * 4, PB_HERO, 2, SF_ALPHA | (p->gy ? SF_HFLIP : 0), (int)(p->y >> 8) - 1);
        } else {
            if (p->kind == PK_SMOKE && p->life < 6 && (p->life & 1)) continue;
            if (p->kind == PK_SPARKLE || p->kind == PK_STAR) {
                if (p->life < 4 && (p->life & 1)) continue;
            }
            spr_add(sx - 4, sy - 4, SH_SQUARE, 0, OT_PART + pinfo[p->kind].base + p->frame, PB_FX, 2, 0, 24000);
        }
    }
}

/* ------------------------------------------------------------------ projectiles */
void proj_spawn(int type, int owner, int x, int y, int angle, int speed, int dmg) {
    for (int i = 0; i < MAX_PROJ; i++) {
        Proj *p = &projs[i];
        if (p->type != PJ_NONE) continue;
        p->type = (u8)type;
        p->owner = (u8)owner;
        p->x = (s32)x * 256;
        p->y = (s32)y * 256;
        p->angle = (u8)angle;
        p->vx = (s16)((isin(angle) * speed) >> 8);
        p->vy = (s16)((-icos(angle) * speed) >> 8);
        p->dmg = (s16)dmg;
        p->frame = 0;
        p->hit = 0;
        p->life = (u8)(type == PJ_ARROW ? 70 : type == PJ_BOLT ? 60 : type == PJ_FIRE ? 100 : 230);
        return;
    }
}

static void proj_die(Proj *p) {
    int x = (int)(p->x >> 8), y = (int)(p->y >> 8);
    if (p->type == PJ_FIRE) {
        fire_id++;
        if (fire_id < 100) fire_id = 100;
        enemy_aoe(x, y, 30, p->dmg, fire_id);
        part_spawn(PK_EXPL, x, y, 0, 0, 14, 0);
        part_burst(PK_EMBER, x, y, 6, 140, 24);
        part_burst(PK_SMOKE, x, y, 3, 40, 20);
        snd_sfx(SFX_EXPLODE);
        shake_add(4);
    } else if (p->type == PJ_ARROW) {
        part_burst(PK_HIT, x, y, 1, 0, 8);
    } else if (p->type == PJ_BOLT) {
        part_burst(PK_SPARKLE, x, y, 3, 60, 14);
    } else if (p->type == PJ_ORB) {
        part_burst(PK_SPARKLE, x, y, 2, 50, 12);
    }
    p->type = PJ_NONE;
}

void projs_update(void) {
    for (int i = 0; i < MAX_PROJ; i++) {
        Proj *p = &projs[i];
        if (p->type == PJ_NONE) continue;
        p->x += p->vx;
        p->y += p->vy;
        int x = (int)(p->x >> 8), y = (int)(p->y >> 8);
        if (--p->life == 0) { proj_die(p); continue; }
        if (x < 4 || y < 4 || x > 508 || y > 508 || proj_blocked(x, y + 6)) { proj_die(p); continue; }
        p->frame++;
        if ((p->type == PJ_FIRE) && (p->frame & 1)) part_spawn(PK_FIRE_TRAIL, x - 2 + rnd_n(5), y - 2 + rnd_n(5), 0, -12, 12, 0);
        if (p->type == PJ_BOLT && (p->frame % 3) == 0) part_spawn(PK_SPARKLE, x, y, 0, 0, 10, 0);
        if (p->owner == 0) {
            for (int k = 0; k < MAX_EN; k++) {
                Enemy *e = &en[k];
                if (!e->active || e->dying) continue;
                int r = e->type == EN_BOSS ? 16 : 9;
                int ez = e->z >> 8;
                if (e->type == EN_BAT) ez = 0;
                int dx = (int)(e->x >> 8) - x, dy = (int)(e->y >> 8) - 6 - y;
                if (e->type == EN_BOSS) dy -= 8;
                if (iabs(dx) < r && iabs(dy) < r + (ez > 6 ? 2 : 0)) {
                    if (p->type == PJ_FIRE) { proj_die(p); break; }
                    int crit = rnd_n(100) < 4 + pl.lck;
                    int d = p->dmg;
                    if (crit) d = (d * 7) >> 2;
                    enemy_damage(e, d, p->vx, p->vy, crit);
                    proj_die(p);
                    break;
                }
            }
        } else if (pl.state != PS_DEAD) {
            int dx = (int)(pl.x >> 8) - x, dy = (int)(pl.y >> 8) - 7 - y;
            if (iabs(dx) < 7 && iabs(dy) < 8) {
                player_hurt(p->dmg, x - p->vx / 64, y - p->vy / 64);
                proj_die(p);
            }
        }
    }
}

static void projs_draw(void) {
    for (int i = 0; i < MAX_PROJ; i++) {
        Proj *p = &projs[i];
        if (p->type == PJ_NONE) continue;
        int sx = (int)(p->x >> 8) - cam_x, sy = (int)(p->y >> 8) - cam_y;
        int key = (int)(p->y >> 8) + 12;
        switch (p->type) {
        case PJ_ARROW:
            spr_add_aff(sx - 8, sy - 8, SH_SQUARE, 1, OT_ARROW, PB_WEAPON, 2, 0, key, p->angle, 256);
            break;
        case PJ_BOLT:
            spr_add(sx - 4, sy - 4, SH_SQUARE, 0, OT_BOLT + ((p->frame >> 2) & 1), PB_FX, 2, 0, key);
            break;
        case PJ_FIRE:
            spr_add(sx - 8, sy - 8, SH_SQUARE, 1, OT_FIRE + ((p->frame >> 2) & 1) * 4, PB_FX, 2, 0, key);
            break;
        case PJ_ORB:
            spr_add(sx - 4, sy - 4, SH_SQUARE, 0, OT_ORB + ((p->frame >> 3) & 1), PB_FX, 2, 0, key);
            break;
        }
    }
}

/* ------------------------------------------------------------------ pickups */
void pickup_spawn(int type, int x, int y) {
    for (int i = 0; i < MAX_PICK; i++) {
        Pickup *p = &picks[i];
        if (p->type != PK_NONE) continue;
        p->type = (u8)type;
        p->x = (s32)(x + rnd_n(7) - 3) * 256;
        p->y = (s32)y * 256;
        p->vx = (s16)(rnd_n(160) - 80);
        p->vy = (s16)(rnd_n(120) - 60);
        p->z = 0;
        p->vz = (s16)(900 + rnd_n(300));
        p->life = 250;
        p->anim = (u8)rnd_n(32);
        return;
    }
}

static void pick_collect(Pickup *p) {
    int px = (int)(p->x >> 8), py = (int)(p->y >> 8);
    switch (p->type) {
    case PK_COIN: player_add_gold(1); snd_sfx(SFX_COIN); break;
    case PK_COIN5: player_add_gold(5); snd_sfx(SFX_COIN); break;
    case PK_HEART: player_heal(12 + pl.maxhp / 12, 0); snd_sfx(SFX_HEART); break;
    case PK_MANA: player_heal(0, 6 + pl.maxmp / 10); snd_sfx(SFX_MANA); dmgnum(px, py - 16, 6 + pl.maxmp / 10, 1, 0); break;
    case PK_POTION: player_give(ITEM_POTION, 1); snd_sfx(SFX_ITEM); toast_show("Found a Potion!"); break;
    }
    part_burst(PK_SPARKLE, px, py - 4, 3, 70, 12);
    p->type = PK_NONE;
}

void pickups_update(void) {
    for (int i = 0; i < MAX_PICK; i++) {
        Pickup *p = &picks[i];
        if (p->type == PK_NONE) continue;
        p->anim++;
        if (p->life) p->life--;
        if (!p->life) { p->type = PK_NONE; continue; }
        int airborne = p->z > 0 || p->vz > 0;
        if (airborne) {
            p->z = (s16)(p->z + p->vz);
            p->vz = (s16)(p->vz - 70);
            if (p->z <= 0) {
                p->z = 0;
                p->vz = (s16)(-p->vz / 2);
                if (p->vz < 120) p->vz = 0;
            }
            int nx = (int)(p->x + p->vx), ny = (int)(p->y + p->vy);
            if (!(pt_flags(nx >> 8, ny >> 8) & TF_SOLID)) { p->x = nx; p->y = ny; }
            p->vx = (s16)((p->vx * 15) >> 4);
            p->vy = (s16)((p->vy * 15) >> 4);
        }
        int dx = (int)(pl.x >> 8) - (int)(p->x >> 8), dy = (int)(pl.y >> 8) - 4 - (int)(p->y >> 8);
        int d = idist(dx, dy);
        if (pl.state != PS_DEAD && p->life < 235 && d < 34 && !airborne && p->type != PK_POTION) {
            int a = vec_angle(dx, dy);
            p->x += (isin(a) * 560) >> 8;
            p->y += (-icos(a) * 560) >> 8;
        }
        if (pl.state != PS_DEAD && d < 10 && p->z < 5 * 256 && p->life < 245) pick_collect(p);
    }
}

static void picks_draw(void) {
    for (int i = 0; i < MAX_PICK; i++) {
        Pickup *p = &picks[i];
        if (p->type == PK_NONE) continue;
        if (p->life < 70 && (p->life & 2)) continue;
        int sx = (int)(p->x >> 8) - cam_x, sy = (int)(p->y >> 8) - cam_y;
        int z = p->z >> 8;
        int tile;
        int pal = PB_ITEM;
        switch (p->type) {
        case PK_COIN: case PK_COIN5: tile = OT_ITEM + ((p->anim >> 3) & 3); break;
        case PK_HEART: tile = OT_ITEM + IT_HEART; break;
        case PK_MANA: tile = OT_ITEM + IT_MANA; break;
        default: tile = OT_ITEM + IT_POTION; break;
        }
        int bob = (p->z == 0) ? ((p->anim >> 4) & 1) : 0;
        spr_add(sx - 4, sy - 8 - z - bob, SH_SQUARE, 0, tile, pal, 2, 0, (int)(p->y >> 8));
        if (p->type == PK_COIN5)
            spr_add(sx - 2, sy - 11 - z - bob, SH_SQUARE, 0, OT_PART + PT_SPARKLE + ((p->anim >> 3) & 3), PB_FX, 2, 0, (int)(p->y >> 8) + 1);
        if (fade_level == 0 && z > 0) spr_add(sx - 4, sy - 4, SH_SQUARE, 0, OT_SHADOW_S, PB_FX, 2, SF_ALPHA, -2000);
    }
}

/* ------------------------------------------------------------------ damage numbers */
void dmgnum(int x, int y, int value, int color, int big) {
    int best = -1, low = 255;
    for (int i = 0; i < MAX_NUM; i++) {
        if (!nums[i].life) { best = i; break; }
        if (nums[i].life < low) { low = nums[i].life; best = i; }
    }
    DmgNum *n = &nums[best];
    n->x = (s16)x;
    n->y = (s16)(y * 16);
    n->vy = -24;
    n->life = 44;
    n->col = (u8)color;
    n->big = (u8)big;
    n->val = (u16)iclamp(value, 0, 9999);
}

static void nums_update(void) {
    for (int i = 0; i < MAX_NUM; i++) {
        DmgNum *n = &nums[i];
        if (!n->life) continue;
        n->life--;
        n->y = (s16)(n->y + n->vy);
        if (n->vy < 0 && (n->life & 3) == 0) n->vy++;
    }
}

static void nums_draw(void) {
    static const u8 pals[4] = { PB_UI, PB_FX, PB_NUMR, PB_ITEM };
    for (int i = 0; i < MAX_NUM; i++) {
        DmgNum *n = &nums[i];
        if (!n->life) continue;
        if (n->life < 10 && (n->life & 1)) continue;
        char buf[6];
        int len = 0, v = n->val;
        char tmp[6];
        int m = 0;
        do { tmp[m++] = (char)(v % 10); v /= 10; } while (v && m < 5);
        while (m) buf[len++] = tmp[--m];
        int w = len * 5;
        int sx = n->x - cam_x - w / 2 - 2, sy = (n->y >> 4) - cam_y - 4;
        int pop = n->life > 38 ? (n->life - 38) : 0;
        for (int k = 0; k < len; k++) {
            spr_add(sx + k * 5, sy - pop, SH_SQUARE, 0, OT_NUM + buf[k], pals[n->col & 3], 0, 0, 29000);
            if (n->big) spr_add(sx + k * 5 + 1, sy - pop, SH_SQUARE, 0, OT_NUM + buf[k], pals[n->col & 3], 0, 0, 28999);
        }
    }
}

/* ------------------------------------------------------------------ ambient */
void fx_ambient(void) {
    int amb = cur_area->ambient;
    if (amb == 1 && (frame_count % 28) == 0) {
        part_spawn(PK_LEAF, cam_x + 240 + 4, cam_y + rnd_n(100), -(60 + rnd_n(60)), 40 + rnd_n(40), 200, 0);
    } else if (amb == 2) {
        if ((frame_count % 34) == 0) {
            int x = cam_x + rnd_n(240), y = cam_y + 30 + rnd_n(120);
            part_spawn(PK_STAR, x, y, rnd_n(40) - 20, rnd_n(30) - 20, 140, 0);
        }
        if ((frame_count % 70) == 0) part_spawn(PK_LEAF, cam_x + rnd_n(240), cam_y - 4, rnd_n(50) - 10, 50 + rnd_n(30), 160, 0);
    } else if (amb == 3) {
        if ((frame_count % 44) == 0) part_spawn(PK_EMBER, cam_x + rnd_n(240), cam_y + 160, rnd_n(24) - 12, -(20 + rnd_n(20)), 120, 0);
    }
}

void fx_update(void) {
    parts_update();
    nums_update();
    if (hitstop_t == 0) {
        projs_update();
        pickups_update();
    }
}

void fx_draw(void) {
    picks_draw();
    projs_draw();
    parts_draw();
    nums_draw();
}
