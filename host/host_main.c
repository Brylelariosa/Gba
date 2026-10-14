/* Host-side test harness: runs the real game code against fake GBA memory and
 * dumps frames so they can be turned into PNGs. Not part of the ROM build. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game.h"
#include "../src/hw.h"
#include "../src/inventory.h"
#include "../src/items.h"
#include "../src/mobs.h"
#include "../src/player.h"
#include "../src/save.h"
#include "../src/world.h"
#include "../src/blocks.h"

u8 host_io[0x400] __attribute__((aligned(4)));
u8 host_pal[0x400] __attribute__((aligned(4)));
u8 host_vram[0x18000] __attribute__((aligned(4)));
u8 host_oam[0x400] __attribute__((aligned(4)));
u8 host_sram[0x8000] __attribute__((aligned(4)));

static const char *outdir = "out";

static void dump(const char *name)
{
    char path[256];
    snprintf(path, sizeof path, "%s/%s.bin", outdir, name);
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fwrite(host_pal, 1, 0x400, f);
    fwrite(host_oam, 1, 0x400, f);
    fwrite(host_vram, 1, 0x18000, f);
    fwrite(host_io, 1, 2, f); /* DISPCNT */
    fclose(f);
}

static void set_keys(u16 keys)
{
    u16 v = (u16)(~keys) & 0x03FF;
    memcpy(host_io + 0x130, &v, 2);
}

static void frames(int n, u16 keys)
{
    for (int i = 0; i < n; i++) {
        set_keys(keys);
        game_frame();
    }
}

static void tap(u16 key)
{
    frames(1, key);
    frames(1, 0);
}

static void check(bool cond, const char *msg)
{
    if (!cond) { printf("CHECK FAILED: %s\n", msg); exit(2); }
    printf("ok: %s\n", msg);
}

int main(int argc, char **argv)
{
    if (argc > 1) outdir = argv[1];
    set_keys(0);
    game_init();
    dump("title");

    tap(KEY_A);                 /* new game */
    frames(3, 0);
    dump("play0");
    printf("spawn at %d %d %d\n", g_pl.x, g_pl.y, g_pl.z);

    /* heightmap dump for tuning terrain */
    {
        char path[256];
        snprintf(path, sizeof path, "%s/world.txt", outdir);
        FILE *f = fopen(path, "w");
        for (int y = 0; y < WY; y++) {
            for (int x = 0; x < WX; x++) {
                int top = world_surface_z(x, y);
                u8 b = top >= 0 ? g_world[top][y][x] : 0;
                char c = '?';
                switch (b) {
                case B_GRASS: c = 'g'; break;
                case B_SAND: c = 's'; break;
                case B_WATER: c = '~'; break;
                case B_STONE: c = '#'; break;
                case B_LEAVES: c = 'T'; break;
                case B_LOG: c = 'l'; break;
                default: c = '?'; break;
                }
                fputc(c, f);
            }
            fputc('\n', f);
        }
        fclose(f);
    }

    /* give some items to exercise HUD and menus */
    inv_add(B_LOG, 5);
    inv_add(B_PLANKS, 12);
    inv_add(I_STICK, 6);
    inv_add(B_COBBLE, 20);
    inv_add(I_PICK_IRON, 1);
    inv_add(I_SWORD_STONE, 1);
    inv_add(I_APPLE, 3);
    inv_add(B_TABLE, 1);
    inv_add(B_DIAMOND_ORE, 2);
    inv_add(I_COAL, 7);
    frames(2, 0);
    dump("hud");

    /* walk around */
    frames(40, KEY_RIGHT);
    dump("walk_mid");
    frames(30, KEY_DOWN);
    frames(30, KEY_LEFT);
    frames(30, KEY_UP);
    dump("walk_end");
    printf("pos %d %d %d\n", g_pl.x, g_pl.y, g_pl.z);

    /* place blocks in front */
    g_pl.sel = 3;
    for (int i = 0; i < 6; i++) { frames(12, KEY_B); }
    dump("placed");
    /* mine them again */
    g_pl.sel = 4; /* iron pickaxe */
    frames(20, KEY_A);
    dump("mining");

    /* menus */
    tap(KEY_SELECT);
    frames(2, 0);
    dump("menu_inv");
    tap(KEY_R);
    frames(2, 0);
    dump("menu_craft");
    for (int i = 0; i < 4; i++) tap(KEY_DOWN);
    dump("menu_craft2");
    tap(KEY_R);
    frames(2, 0);
    dump("menu_goals");
    tap(KEY_SELECT);
    frames(2, 0);

    tap(KEY_START);
    frames(2, 0);
    dump("pause");
    tap(KEY_START);
    frames(2, 0);

    /* night */
    g_game.tod = 16000;
    frames(5, 0);
    dump("night");
    g_game.tod = 12500;

    /* zombie */
    for (int i = 0; i < 200; i++) frames(1, KEY_LEFT);
    dump("after_walk2");

    /* death */
    g_pl.hp = 0;
    frames(3, 0);
    dump("dead");
    tap(KEY_A);
    frames(3, 0);

    /* save / load round trip */
    inv_add(B_BRICK, 5);
    u8 id0 = g_inv[0].id;
    bool full = save_write();
    check(full, "save complete");
    check(save_exists(), "save exists");
    int px = g_pl.x, py = g_pl.y;
    int mods = 0;
    for (int i = 0; i < (int)sizeof g_world_mod; i++) mods += __builtin_popcount(g_world_mod[i]);
    printf("edits: %d\n", mods);
    memset(g_inv, 0, sizeof g_inv);
    g_pl.x = 1;
    check(save_load(), "load ok");
    check(g_pl.x == px && g_pl.y == py, "player restored");
    check(g_inv[0].id == id0, "inventory restored");
    frames(5, 0);
    dump("loaded");

    /* long random bot run to shake out crashes */
    unsigned s = 12345;
    for (int i = 0; i < 6000; i++) {
        s = s * 1103515245u + 12345u;
        u16 keys = 0;
        switch ((s >> 16) & 15) {
        case 0: keys = KEY_UP; break;
        case 1: keys = KEY_DOWN; break;
        case 2: keys = KEY_LEFT; break;
        case 3: keys = KEY_RIGHT; break;
        case 4: keys = KEY_A; break;
        case 5: keys = KEY_B; break;
        case 6: keys = KEY_R; break;
        case 7: keys = KEY_L; break;
        case 8: keys = KEY_UP | KEY_A; break;
        case 9: keys = KEY_RIGHT | KEY_B; break;
        default: keys = 0; break;
        }
        int n = 1 + ((s >> 8) & 7);
        frames(n, keys);
        if (g_pl.hp <= 0) tap(KEY_A);
    }
    dump("bot_end");
    printf("bot done pos %d %d %d hp %d\n", g_pl.x, g_pl.y, g_pl.z, g_pl.hp);
    printf("ALL DONE\n");
    return 0;
}
