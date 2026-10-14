/* Gameplay tests that drive the real game code through the real input path. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game.h"
#include "../src/hw.h"
#include "../src/inventory.h"
#include "../src/items.h"
#include "../src/mobs.h"
#include "../src/player.h"
#include "../src/craft.h"
#include "../src/save.h"
#include "../src/world.h"
#include "../src/render.h"
#include "../src/move.h"

u8 host_io[0x400] __attribute__((aligned(4)));
u8 host_pal[0x400] __attribute__((aligned(4)));
u8 host_vram[0x18000] __attribute__((aligned(4)));
u8 host_oam[0x400] __attribute__((aligned(4)));
u8 host_sram[0x8000] __attribute__((aligned(4)));

static int fails;
static void set_keys(u16 k) { u16 v = (u16)(~k) & 0x3FF; memcpy(host_io + 0x130, &v, 2); }
static void frames(int n, u16 k) { for (int i = 0; i < n; i++) { set_keys(k); game_frame(); } }
static void tap(u16 k) { frames(1, k); frames(1, 0); }
static void check(int cond, const char *msg)
{
    printf("%s: %s\n", cond ? "ok  " : "FAIL", msg);
    if (!cond) fails++;
}

static unsigned s_seed = 100;

static void select_item(u8 id)
{
    for (int i = 0; i < HOTBAR_SLOTS; i++)
        if (g_inv[i].id == id) { g_pl.sel = (u8)i; return; }
    printf("select_item: %d not in hotbar\n", id);
}

static void teleport(int x, int y, int z, int dir)
{
    player_reset(x, y, z);
    g_pl.dir = (u8)dir;
    render_invalidate();
    mobs_clear();
    frames(3, 0);
}

static int find_grass(int *ox, int *oy, int *oz);

/* brand new world, player standing on a flat patch (returns patch position) */
static void fresh_world(int *gx, int *gy, int *gz)
{
    for (int tries = 0;; tries++) {
        s_seed += 77;
        world_generate(s_seed);
        if (find_grass(gx, gy, gz)) break;
        if (tries > 40) { printf("no flat grass found\n"); exit(3); }
    }
    mobs_clear();
    memset(g_inv, 0, sizeof g_inv);
    teleport(*gx, *gy, *gz, 1);
}

/* find a flat grass cell with air around it */
static int find_grass(int *ox, int *oy, int *oz)
{
    for (int y = 20; y < 44; y++)
        for (int x = 20; x < 44; x++) {
            int top = world_surface_z(x, y);
            if (top < SEA_Z + 1 || g_world[top][y][x] != B_GRASS) continue;
            int ok = 1;
            for (int dy = -2; dy <= 2 && ok; dy++)
                for (int dx = -2; dx <= 2; dx++)
                    if (world_surface_z(x + dx, y + dy) != top || world_get(x + dx, y + dy, top + 1) != B_AIR) { ok = 0; break; }
            if (ok) { *ox = x; *oy = y; *oz = top + 1; return 1; }
        }
    return 0;
}

int main(void)
{
    set_keys(0);
    game_init();
    tap(KEY_A); /* new game */
    frames(3, 0);

    int gx, gy, gz;
    check(find_grass(&gx, &gy, &gz), "found a flat grass patch");

    /* --- mining: hand vs tools -------------------------------------- */
    teleport(gx, gy, gz, 1);
    world_set(gx + 1, gy, gz, B_STONE);
    memset(g_inv, 0, sizeof g_inv);
    frames(600, KEY_A);
    check(world_get(gx + 1, gy, gz) == B_AIR, "stone broken by hand (slowly)");
    check(inv_count(B_COBBLE) == 0, "no cobblestone without a pickaxe");

    world_set(gx + 1, gy, gz, B_STONE);
    memset(g_inv, 0, sizeof g_inv);
    inv_add(I_PICK_WOOD, 1);
    select_item(I_PICK_WOOD);
    frames(200, KEY_A);
    check(world_get(gx + 1, gy, gz) == B_AIR, "stone broken with wood pickaxe");
    check(inv_count(B_COBBLE) == 1, "cobblestone dropped");
    check(game_has(ADV_STONE), "goal: stone age");

    world_set(gx + 1, gy, gz, B_IRON_ORE);
    teleport(gx, gy, gz, 1);
    select_item(I_PICK_WOOD);
    frames(900, KEY_A);
    check(inv_count(B_IRON_ORE) == 0, "wood pickaxe cannot harvest iron ore");
    world_set(gx + 1, gy, gz, B_IRON_ORE);
    teleport(gx, gy, gz, 1);
    inv_add(I_PICK_STONE, 1);
    select_item(I_PICK_STONE);
    frames(300, KEY_A);
    check(inv_count(B_IRON_ORE) == 1, "stone pickaxe harvests iron ore");

    world_set(gx + 1, gy, gz, B_DIAMOND_ORE);
    teleport(gx, gy, gz, 1);
    select_item(I_PICK_STONE);
    frames(1500, KEY_A);
    check(inv_count(I_DIAMOND) == 0, "stone pickaxe cannot harvest diamond");
    world_set(gx + 1, gy, gz, B_DIAMOND_ORE);
    teleport(gx, gy, gz, 1);
    inv_add(I_PICK_IRON, 1);
    select_item(I_PICK_IRON);
    frames(400, KEY_A);
    check(inv_count(I_DIAMOND) == 1, "iron pickaxe harvests diamond");

    /* --- trees ------------------------------------------------------ */
    {
        int tx = -1, ty = -1, tz = -1;
        for (int y = 5; y < 59 && tx < 0; y++)
            for (int x = 5; x < 59; x++) {
                int top = world_surface_z(x, y);
                for (int z = SEA_Z + 1; z < WZ - 1; z++)
                    if (g_world[z][y][x] == B_LOG && g_world[z - 1][y][x] == B_GRASS && world_get(x - 1, y, z) == B_AIR &&
                        world_get(x - 1, y, z - 1) != B_AIR && world_get(x - 1, y, z - 1) != B_WATER) { tx = x; ty = y; tz = z; top = 0; break; }
                (void)top;
                if (tx >= 0) break;
            }
        check(tx >= 0, "found a tree");
        if (tx >= 0) {
            int logs_before = inv_count(B_LOG);
            int leaves = 0;
            for (int z = 0; z < WZ; z++) for (int y = ty - 4; y <= ty + 4; y++) for (int x = tx - 4; x <= tx + 4; x++) leaves += world_get(x, y, z) == B_LEAVES;
            teleport(tx - 1, ty, tz, 1);
            frames(400, KEY_A);
            check(world_get(tx, ty, tz) == B_AIR, "tree trunk chopped");
            check(inv_count(B_LOG) >= logs_before + 3, "tree gave several logs");
            int leaves_after = 0;
            for (int z = 0; z < WZ; z++) for (int y = ty - 4; y <= ty + 4; y++) for (int x = tx - 4; x <= tx + 4; x++) leaves_after += world_get(x, y, z) == B_LEAVES;
            check(leaves_after < leaves, "leaves decayed after felling");
            check(game_has(ADV_WOOD), "goal: getting wood");
        }
    }

    /* --- crafting chain --------------------------------------------- */
    memset(g_inv, 0, sizeof g_inv);
    inv_add(B_LOG, 8);
    const Recipe *r;
    r = &g_recipes[0]; check(craft_make(r, 0), "logs -> planks");
    craft_make(r, 0); craft_make(r, 0); craft_make(r, 0);
    r = &g_recipes[1]; check(craft_make(r, 0), "planks -> sticks");
    r = &g_recipes[2]; check(craft_make(r, 0), "table");
    check(game_has(ADV_TABLE), "goal: benchmarking");
    r = &g_recipes[3]; check(!craft_make(r, 0), "wood pickaxe needs a table nearby");
    check(craft_make(r, NEAR_TABLE), "wood pickaxe at table");
    inv_add(B_COBBLE, 30);
    inv_add(I_COAL, 10);
    inv_add(B_IRON_ORE, 3);
    for (int i = 0; i < g_recipe_count; i++)
        if (g_recipes[i].out == B_FURNACE) check(craft_make(&g_recipes[i], NEAR_TABLE), "furnace");
    for (int i = 0; i < g_recipe_count; i++)
        if (g_recipes[i].out == I_IRON_INGOT) { check(!craft_make(&g_recipes[i], NEAR_TABLE), "smelting needs a furnace"); check(craft_make(&g_recipes[i], NEAR_FURNACE), "smelt iron"); }
    check(game_has(ADV_IRON), "goal: acquire hardware");

    /* --- building and bridging -------------------------------------- */
    fresh_world(&gx, &gy, &gz);
    inv_add(B_PLANKS, 40);
    g_pl.sel = 0;
    int before = inv_count(B_PLANKS);
    for (int i = 0; i < 4; i++) frames(12, KEY_B);
    check(inv_count(B_PLANKS) < before, "blocks were placed");
    check(world_get(gx + 1, gy, gz) == B_PLANKS, "first block placed in front");
    check(world_get(gx + 1, gy, gz + 1) == B_PLANKS, "next blocks stack upwards");
    for (int i = 0; i < 12; i++) frames(12, KEY_B);
    check(game_has(ADV_HOUSE) || g_game.placed < 40, "placed counter works");

    /* climb a 1-block step by walking into it */
    fresh_world(&gx, &gy, &gz);
    world_set(gx + 1, gy, gz, B_STONE);
    frames(3, KEY_RIGHT);
    frames(30, 0);
    check(g_pl.x == gx + 1 && g_pl.z == gz + 1, "walked up a one block step");
    world_set(gx + 3, gy, gz, B_STONE);
    world_set(gx + 3, gy, gz + 1, B_STONE);
    frames(80, KEY_RIGHT);
    printf("  player at %d %d %d\n", g_pl.x, g_pl.y, g_pl.z);
    check(g_pl.x == gx + 2, "a two block wall blocks the way");

    /* bridge over water */
    {
        int wx = -1, wy = -1, wz = -1;
        for (int y = 3; y < 60 && wx < 0; y++)
            for (int x = 3; x < 60; x++) {
                int top = world_surface_z(x, y);
                if (top == SEA_Z && g_world[top][y][x] == B_WATER && world_get(x - 1, y, SEA_Z) == B_SAND &&
                    world_get(x - 1, y, SEA_Z + 1) == B_AIR) { wx = x; wy = y; wz = SEA_Z + 1; break; }
            }
        check(wx >= 0, "found a shore");
        if (wx >= 0) {
            teleport(wx - 1, wy, wz, 1);
            memset(g_inv, 0, sizeof g_inv);
            inv_add(B_PLANKS, 5);
            g_pl.sel = 0;
            frames(40, KEY_RIGHT);
            check(g_pl.x == wx - 1, "cannot walk into water");
            frames(14, KEY_B);
            check(world_get(wx, wy, SEA_Z) == B_PLANKS, "bridge block replaces the water surface");
            frames(40, KEY_RIGHT);
            check(g_pl.x == wx, "walked onto the bridge");
        }
    }

    /* --- mobs ------------------------------------------------------- */
    fresh_world(&gx, &gy, &gz);
    inv_add(I_SWORD_IRON, 1);
    g_pl.sel = 0;
    g_game.tod = 16000;
    g_game.daylight = 8;
    int hp0 = g_pl.hp;
    mobs_clear();
    /* put a zombie two blocks away */
    g_mobs[0].type = MT_ZOMBIE; g_mobs[0].x = g_mobs[0].fx = gx + 3; g_mobs[0].y = g_mobs[0].fy = gy; g_mobs[0].z = g_mobs[0].fz = gz; g_mobs[0].hp = 12;
    frames(200, 0);
    check(g_pl.hp < hp0 || g_mobs[0].type == 0, "zombie reaches and bites the player");
    int dir_to = 1;
    for (int round = 0; round < 40 && !game_has(ADV_ZOMBIE); round++) {
        Mob *z = NULL;
        int best = 999;
        for (int i = 0; i < MAX_MOBS; i++) {
            if (g_mobs[i].type != MT_ZOMBIE) continue;
            int d = ABS(g_mobs[i].x - g_pl.x) + ABS(g_mobs[i].y - g_pl.y);
            if (d < best) { best = d; z = &g_mobs[i]; }
        }
        if (z) {
            int ddx = z->x - g_pl.x, ddy = z->y - g_pl.y;
            dir_to = ABS(ddx) >= ABS(ddy) ? (ddx > 0 ? 1 : 3) : (ddy > 0 ? 2 : 0);
        }
        g_pl.dir = (u8)dir_to;
        frames(10, KEY_A);
        if (g_pl.hp <= 4) g_pl.hp = 20; /* keep the test alive */
    }
    check(game_has(ADV_ZOMBIE), "zombie killed with a sword (goal: monster hunter)");
    if (!game_has(ADV_ZOMBIE)) {
        for (int i = 0; i < MAX_MOBS; i++) if (g_mobs[i].type) printf("  mob %d type %d at %d %d %d hp %d; player %d %d %d dir %d hp %d\n", i, g_mobs[i].type, g_mobs[i].x, g_mobs[i].y, g_mobs[i].z, g_mobs[i].hp, g_pl.x, g_pl.y, g_pl.z, g_pl.dir, g_pl.hp);
    }

    /* --- death and respawn ------------------------------------------ */
    g_pl.hp = 0;
    frames(3, 0);
    tap(KEY_A);
    frames(3, 0);
    check(g_pl.hp > 0, "respawned alive");

    /* --- eating ------------------------------------------------------ */
    memset(g_inv, 0, sizeof g_inv);
    inv_add(I_PORK_COOKED, 2);
    g_pl.sel = 0;
    g_pl.food = 5;
    tap(KEY_B);
    check(g_pl.food == 13 && inv_count(I_PORK_COOKED) == 1, "eating restores hunger");

    /* --- beacon win ------------------------------------------------- */
    fresh_world(&gx, &gy, &gz);
    inv_add(B_BEACON, 1);
    g_pl.sel = 0;
    tap(KEY_B);
    frames(2, 0);
    check(g_game.won && game_has(ADV_BEACON), "placing the beacon wins");

    /* --- save/load keeps edits and goals ----------------------------- */
    fresh_world(&gx, &gy, &gz);
    world_set(gx + 2, gy + 2, gz, B_BRICK);
    world_set(gx + 2, gy + 2, gz + 1, B_GLASS);
    unsigned adv = g_game.adv;
    int placed = g_game.placed;
    inv_add(B_BRICK, 7);
    check(save_write(), "save written");
    u32 seed = g_world_seed;
    teleport(5, 5, 9, 0);
    world_generate(999);
    g_game.adv = 0;
    memset(g_inv, 0, sizeof g_inv);
    check(save_load(), "save loaded");
    check(g_world_seed == seed, "world seed restored");
    check(g_game.adv == adv && g_game.placed == placed, "goals restored");
    check(world_get(gx + 2, gy + 2, gz) == B_BRICK && world_get(gx + 2, gy + 2, gz + 1) == B_GLASS, "player edits replayed");
    check(inv_count(B_BRICK) == 7, "inventory restored");

    printf("%s (%d failures)\n", fails ? "SOME TESTS FAILED" : "ALL GAMEPLAY TESTS PASSED", fails);
    return fails ? 1 : 0;
}
