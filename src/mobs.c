#include "mobs.h"
#include "config.h"
#include "game.h"
#include "inventory.h"
#include "items.h"
#include "move.h"
#include "player.h"
#include "rng.h"
#include "sfx.h"
#include "world.h"
#include <string.h>

Mob g_mobs[MAX_MOBS];
static u32 s_tick;

void mobs_clear(void)
{
    memset(g_mobs, 0, sizeof g_mobs);
    s_tick = 0;
}

Mob *mob_at(int x, int y, int z)
{
    for (int i = 0; i < MAX_MOBS; i++) {
        Mob *m = &g_mobs[i];
        if (m->type && m->x == x && m->y == y && ABS(m->z - z) <= 1) return m;
    }
    return NULL;
}

static bool spawn_at(int type, int x, int y)
{
    if (!world_in(x, y, 0)) return false;
    int top = world_surface_z(x, y);
    if (top < SEA_Z || top + 2 >= WZ) return false;
    u8 b = world_get(x, y, top);
    if (b != B_GRASS && b != B_SAND && b != B_DIRT) return false;
    if (world_get(x, y, top + 1) != B_AIR) return false;
    if (ABS(g_pl.x - x) + ABS(g_pl.y - y) < 6) return false;
    if (mob_at(x, y, top + 1)) return false;

    Mob *slot = NULL;
    for (int i = 0; i < MAX_MOBS; i++)
        if (!g_mobs[i].type) { slot = &g_mobs[i]; break; }
    if (!slot) return false;

    memset(slot, 0, sizeof *slot);
    slot->type = (u8)type;
    slot->x = slot->fx = (s16)x;
    slot->y = slot->fy = (s16)y;
    slot->z = slot->fz = (s16)(top + 1);
    slot->hp = (s8)(type == MT_ZOMBIE ? 12 : 6);
    slot->cd = (u8)(20 + rng_range(&g_rng, 40));
    return true;
}

void mobs_populate(void)
{
    int pigs = 0;
    for (int tries = 0; tries < 300 && pigs < 6; tries++) {
        int x = 6 + rng_range(&g_rng, WX - 12), y = 6 + rng_range(&g_rng, WY - 12);
        if (spawn_at(MT_PIG, x, y)) pigs++;
    }
}

static bool mob_step(Mob *m, int dir)
{
    int nz;
    int nx = m->x + g_dir_dx[dir], ny = m->y + g_dir_dy[dir];
    if (!move_try(m->x, m->y, m->z, g_dir_dx[dir], g_dir_dy[dir], 1, false, &nz)) return false;
    if (mob_at(nx, ny, nz)) return false;
    if (g_pl.x == nx && g_pl.y == ny && ABS(g_pl.z - nz) <= 1) return false;
    m->fx = m->x; m->fy = m->y; m->fz = m->z;
    m->x = (s16)nx; m->y = (s16)ny; m->z = (s16)nz;
    m->dir = (u8)dir;
    m->moving = 1;
    m->step_t = 0;
    return true;
}

void mob_hurt(Mob *m, int dmg, int kdir)
{
    m->hp = (s8)(m->hp - dmg);
    m->flash = 10;
    if (m->hp <= 0) {
        if (m->type == MT_PIG) {
            inv_add(I_PORK_RAW, 1 + rng_range(&g_rng, 2));
        } else if (m->type == MT_ZOMBIE) {
            game_unlock(ADV_ZOMBIE);
            if (rng_range(&g_rng, 3) == 0) inv_add(I_COAL, 1);
            if (rng_range(&g_rng, 10) == 0) inv_add(I_IRON_INGOT, 1);
        }
        m->type = MT_NONE;
        sfx_play(SFX_BREAK);
        return;
    }
    sfx_play(SFX_MINE);
    if (!m->moving) {
        mob_step(m, kdir);
        m->cd = 14;
    }
}

static void think_zombie(Mob *m)
{
    int dx = g_pl.x - m->x, dy = g_pl.y - m->y;
    int adx = ABS(dx), ady = ABS(dy);

    if (adx + ady <= 1 && ABS(g_pl.z - m->z) <= 1) { /* next to the player: bite */
        if (adx) m->dir = (u8)(dx > 0 ? 1 : 3);
        else m->dir = (u8)(dy > 0 ? 2 : 0);
        player_hurt(2);
        m->cd = 40;
        return;
    }
    if (adx + ady <= 14) { /* chase */
        int d1, d2 = -1;
        if (adx >= ady) { d1 = dx > 0 ? 1 : 3; if (ady) d2 = dy > 0 ? 2 : 0; }
        else            { d1 = dy > 0 ? 2 : 0; if (adx) d2 = dx > 0 ? 1 : 3; }
        if (mob_step(m, d1) || (d2 >= 0 && mob_step(m, d2))) { m->cd = 3; return; }
    }
    if (rng_range(&g_rng, 3) == 0) mob_step(m, rng_range(&g_rng, 4));
    m->cd = 10;
}

static void think_pig(Mob *m)
{
    if (rng_range(&g_rng, 3) == 0) mob_step(m, rng_range(&g_rng, 4));
    m->cd = (u8)(24 + rng_range(&g_rng, 70));
}

static void try_spawn(int type)
{
    for (int tries = 0; tries < 6; tries++) {
        int dx = rng_range(&g_rng, 29) - 14, dy = rng_range(&g_rng, 29) - 14;
        if (ABS(dx) + ABS(dy) < 9) continue;
        if (spawn_at(type, g_pl.x + dx, g_pl.y + dy)) return;
    }
}

void mobs_update(void)
{
    s_tick++;
    int zombies = 0, pigs = 0;

    for (int i = 0; i < MAX_MOBS; i++) {
        Mob *m = &g_mobs[i];
        if (!m->type) continue;
        if (m->type == MT_ZOMBIE) zombies++; else pigs++;
        if (m->flash) m->flash--;

        int dist = ABS(m->x - g_pl.x) + ABS(m->y - g_pl.y);
        bool sunburn = m->type == MT_ZOMBIE && g_game.daylight <= 1 && rng_range(&g_rng, 240) == 0;
        if (dist > 30 || sunburn) { m->type = MT_NONE; continue; }

        if (m->moving) {
            if (++m->step_t >= MOB_STEP) m->moving = 0;
            continue;
        }
        if (m->cd) { m->cd--; continue; }
        if (m->type == MT_ZOMBIE) think_zombie(m);
        else think_pig(m);
    }

    if ((s_tick & 63) == 0 && g_game.daylight >= 6 && zombies < 4) try_spawn(MT_ZOMBIE);
    if ((s_tick & 511) == 0 && pigs < 5) try_spawn(MT_PIG);
}
