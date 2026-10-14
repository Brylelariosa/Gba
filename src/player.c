#include "player.h"
#include "config.h"
#include "game.h"
#include "hw.h"
#include "inventory.h"
#include "items.h"
#include "mobs.h"
#include "move.h"
#include "rng.h"
#include "sfx.h"
#include "world.h"
#include <string.h>

Player g_pl;

void player_reset(int x, int y, int z)
{
    memset(&g_pl, 0, sizeof g_pl);
    g_pl.x = g_pl.fx = (s16)x;
    g_pl.y = g_pl.fy = (s16)y;
    g_pl.z = g_pl.fz = (s16)z;
    g_pl.dir = 2;
    g_pl.hp = MAX_HP;
    g_pl.food = MAX_FOOD;
}

void player_hurt(int dmg)
{
    if (g_pl.inv_t || dmg <= 0) return;
    g_pl.hp = (s16)MAX(0, g_pl.hp - dmg);
    g_pl.hurt_t = 20;
    g_pl.inv_t = 36;
    sfx_play(SFX_HURT);
}

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static void give(u8 id, int n)
{
    if (inv_add(id, n) > 0) game_toast("INVENTORY FULL");
}

static void consume_selected(Player *p)
{
    Slot *s = &g_inv[p->sel];
    if (s->count > 0) s->count--;
    if (s->count == 0) memset(s, 0, sizeof *s);
}

static bool minable(u8 b)
{
    return block_solid(b) && b != B_BEDROCK;
}

static int tool_speed10(u8 tool, u8 block)
{
    u8 kind = item_kind(tool);
    u8 want = g_block_info[block].tool;
    if ((want == TC_PICK && kind == IK_PICK) || (want == TC_AXE && kind == IK_AXE))
        return item_tier(tool) * 20;
    return 10;
}

static bool harvestable(u8 tool, u8 block)
{
    u8 tier = g_block_info[block].tier;
    if (tier == 0) return true;
    return item_kind(tool) == IK_PICK && item_tier(tool) >= tier;
}

static int sword_damage(u8 tool)
{
    switch (item_kind(tool)) {
    case IK_SWORD: return 2 + item_tier(tool);
    case IK_AXE:
    case IK_PICK:  return 2;
    default:       return 1;
    }
}

static void wear_tool(Player *p)
{
    Slot *s = &g_inv[p->sel];
    u8 kind = item_kind(s->id);
    if (kind != IK_PICK && kind != IK_AXE && kind != IK_SWORD) return;
    if (s->dur <= 1) game_toast("YOUR TOOL BROKE");
    inv_wear(p->sel);
}

/* ------------------------------------------------------------------ */
/* Aiming                                                              */
/* ------------------------------------------------------------------ */

static void find_target(Player *p)
{
    p->has_target = false;
    int fx = p->x + g_dir_dx[p->dir], fy = p->y + g_dir_dy[p->dir];
    if (!world_in(fx, fy, 0)) return;

    u8 b0 = world_get(fx, fy, p->z);
    if (minable(b0)) {
        p->has_target = true;
        p->tx = (s16)fx; p->ty = (s16)fy; p->tz = p->z;
    } else if (b0 == B_AIR && minable(world_get(fx, fy, p->z - 1))) {
        p->has_target = true;                  /* dig the ground in front */
        p->tx = (s16)fx; p->ty = (s16)fy; p->tz = (s16)(p->z - 1);
    }
}

/* Where would a block land if placed now? */
static bool find_place_cell(const Player *p, int *ox, int *oy, int *oz)
{
    int fx = p->x + g_dir_dx[p->dir], fy = p->y + g_dir_dy[p->dir];
    if (!world_in(fx, fy, 0)) return false;
    int z = p->z;
    u8 b = world_get(fx, fy, z);

    if (b == B_AIR) {
        while (z > 0 && p->z - z < 2 && world_get(fx, fy, z - 1) == B_AIR) z--;
        if (z > 0 && world_get(fx, fy, z - 1) == B_WATER) z--; /* replace the water surface */
    } else if (b != B_WATER) {
        z++;                                   /* build up on top of what is there */
        while (z < WZ && z <= p->z + 3 && block_solid(world_get(fx, fy, z))) z++;
        if (z >= WZ || z > p->z + 3) return false;
    }
    if (mob_at(fx, fy, z)) return false;
    *ox = fx; *oy = fy; *oz = z;
    return true;
}

/* ------------------------------------------------------------------ */
/* Actions                                                             */
/* ------------------------------------------------------------------ */

static bool log_near(int x, int y, int z)
{
    for (int dz = -4; dz <= 1; dz++)
        for (int dy = -2; dy <= 2; dy++)
            for (int dx = -2; dx <= 2; dx++)
                if (world_get(x + dx, y + dy, z + dz) == B_LOG) return true;
    return false;
}

static void fell_tree(int x, int y, int z)
{
    int zz = z + 1;
    while (world_get(x, y, zz) == B_LOG) {
        world_set(x, y, zz, B_AIR);
        give(B_LOG, 1);
        zz++;
    }
    for (int lz = z; lz <= zz + 2; lz++)
        for (int ly = y - 3; ly <= y + 3; ly++)
            for (int lx = x - 3; lx <= x + 3; lx++) {
                if (world_get(lx, ly, lz) != B_LEAVES || log_near(lx, ly, lz)) continue;
                world_set(lx, ly, lz, B_AIR);
                if (rng_range(&g_rng, 14) == 0) give(I_APPLE, 1);
            }
}

static void break_block(Player *p)
{
    int x = p->tx, y = p->ty, z = p->tz;
    u8 b = world_get(x, y, z);
    u8 tool = g_inv[p->sel].id;
    bool ok = harvestable(tool, b);

    world_set(x, y, z, B_AIR);
    sfx_play(SFX_BREAK);

    if (ok) {
        u8 drop = block_drop(b);
        if (b == B_LEAVES) {
            int r = rng_range(&g_rng, 12);
            if (r == 0) drop = I_APPLE;
            else if (r == 1) drop = I_STICK;
        }
        if (drop) give(drop, 1);
    }
    if (b != B_LEAVES) wear_tool(p);
    if (b == B_LOG) fell_tree(x, y, z);
}

static void mine(Player *p)
{
    if (!p->has_target) { p->mine_t = 0; return; }
    u8 b = world_get(p->tx, p->ty, p->tz);
    u8 tool = g_inv[p->sel].id;
    if (p->mine_t == 0) {
        int need = g_block_info[b].hard * 10 / tool_speed10(tool, b);
        if (!harvestable(tool, b)) need *= 3;
        p->mine_need = (u16)MAX(need, 6);
    }
    if ((p->mine_t & 15) == 0) sfx_play(SFX_MINE);
    p->mine_t++;
    if (p->mine_t >= p->mine_need) {
        break_block(p);
        p->mine_t = 0;
    }
}

static void use_item(Player *p, bool pressed)
{
    Slot *s = &g_inv[p->sel];
    if (!s->id) return;
    u8 kind = item_kind(s->id);

    if (kind == IK_FOOD) {
        if (!pressed || p->food >= MAX_FOOD) return;
        p->food = (s16)MIN(MAX_FOOD, p->food + item_food(s->id));
        sfx_play(SFX_EAT);
        consume_selected(p);
        return;
    }
    if (kind != IK_BLOCK || !block_placeable(s->id) || p->place_cd) return;

    int x, y, z;
    if (!find_place_cell(p, &x, &y, &z)) return;
    u8 id = s->id;
    world_set(x, y, z, id);
    consume_selected(p);
    p->place_cd = 9;
    sfx_play(SFX_PLACE);
    g_game.placed++;
    if (g_game.placed >= 40) game_unlock(ADV_HOUSE);
    if (id == B_BEACON) {
        g_game.won = 1;
        game_unlock(ADV_BEACON);
    }
}

static void attack(Player *p, Mob *m)
{
    if (p->atk_cd) return;
    mob_hurt(m, sword_damage(g_inv[p->sel].id), p->dir);
    p->atk_cd = 14;
    wear_tool(p);
}

/* ------------------------------------------------------------------ */
/* Survival and movement                                               */
/* ------------------------------------------------------------------ */

static void survival(Player *p)
{
    if (++p->food_t >= 1500) {
        p->food_t = 0;
        if (p->food > 0) p->food--;
    }
    if (p->food >= 14) {
        if (++p->regen_t >= 200) {
            p->regen_t = 0;
            if (p->hp < MAX_HP) p->hp++;
        }
    } else {
        p->regen_t = 0;
        if (p->food == 0 && ++p->starve_t >= 240) {
            p->starve_t = 0;
            if (p->hp > 1) player_hurt(1);
        }
    }
}

static void try_step(Player *p, int dir)
{
    int nz;
    int nx = p->x + g_dir_dx[dir], ny = p->y + g_dir_dy[dir];
    if (!move_try(p->x, p->y, p->z, g_dir_dx[dir], g_dir_dy[dir], 99, &nz)) return;
    if (mob_at(nx, ny, nz)) return;

    int drop = p->z - nz;
    p->fx = p->x; p->fy = p->y; p->fz = p->z;
    p->x = (s16)nx; p->y = (s16)ny; p->z = (s16)nz;
    p->moving = 1;
    p->step_t = 0;
    if (drop > 3) player_hurt(drop - 3);
}

void player_update(u16 held, u16 pressed)
{
    Player *p = &g_pl;

    p->arrived = 0;
    if (p->hurt_t) p->hurt_t--;
    if (p->inv_t) p->inv_t--;
    if (p->atk_cd) p->atk_cd--;
    if (p->place_cd) p->place_cd--;

    if (pressed & KEY_R) { p->sel = (u8)((p->sel + 1) % HOTBAR_SLOTS); g_game.sel_t = 90; }
    if (pressed & KEY_L) { p->sel = (u8)((p->sel + HOTBAR_SLOTS - 1) % HOTBAR_SLOTS); g_game.sel_t = 90; }

    survival(p);

    if (p->moving) {
        if (++p->step_t < STEP_FRAMES) {
            p->has_target = false;
            p->mine_t = 0;
            return;
        }
        p->moving = 0;
        p->step_t = 0;
        p->arrived = 1;
    }

    /* walking: tapping a new direction only turns, holding it walks */
    int dir = -1;
    if (held & KEY_UP) dir = 0;
    else if (held & KEY_RIGHT) dir = 1;
    else if (held & KEY_DOWN) dir = 2;
    else if (held & KEY_LEFT) dir = 3;

    if (dir >= 0) {
        if (dir != p->dir) { p->dir = (u8)dir; p->turn_t = 5; }
        else if (p->turn_t) p->turn_t--;
        else try_step(p, dir);
    } else {
        p->turn_t = 0;
    }
    if (p->moving) { p->has_target = false; p->mine_t = 0; return; }

    /* actions on the block / mob in front */
    s16 ptx = p->tx, pty = p->ty, ptz = p->tz;
    bool had = p->has_target;
    find_target(p);
    if (!p->has_target || (had && (ptx != p->tx || pty != p->ty || ptz != p->tz))) p->mine_t = 0;

    if (held & KEY_A) {
        int fx = p->x + g_dir_dx[p->dir], fy = p->y + g_dir_dy[p->dir];
        Mob *m = mob_at(fx, fy, p->z);
        if (m) {
            attack(p, m);
            p->mine_t = 0;
        } else if (mob_at(fx + g_dir_dx[p->dir], fy + g_dir_dy[p->dir], p->z)) {
            p->mine_t = 0; /* a mob is about to step in front of us: do not dig */
        } else {
            mine(p);
        }
    } else {
        p->mine_t = 0;
    }

    if ((pressed & KEY_B) || (held & KEY_B)) use_item(p, (pressed & KEY_B) != 0);
}
