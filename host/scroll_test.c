/* Walks continuously and checks that the picture scrolls smoothly: a fixed
 * world point must never jump more than a few pixels between frames. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game.h"
#include "../src/hw.h"
#include "../src/player.h"
#include "../src/world.h"
#include "../src/render.h"
#include "../src/mobs.h"
#include "../src/inventory.h"

u8 host_io[0x400] __attribute__((aligned(4)));
u8 host_pal[0x400] __attribute__((aligned(4)));
u8 host_vram[0x18000] __attribute__((aligned(4)));
u8 host_oam[0x400] __attribute__((aligned(4)));
u8 host_sram[0x8000] __attribute__((aligned(4)));
extern Camera g_host_cam;

static void set_keys(u16 k) { u16 v = (u16)(~k) & 0x3FF; memcpy(host_io + 0x130, &v, 2); }

int main(void)
{
    set_keys(0);
    game_init();
    set_keys(KEY_A); game_frame(); set_keys(0); game_frame(); game_frame();
    /* flat walk: find a long straight walkable line */
    int fixed_x = g_pl.x + 6, fixed_y = g_pl.y, fixed_z = g_pl.z - 1;
    int prev_sx = 0, prev_sy = 0, have = 0, maxjump = 0, frames_moving = 0, steps = 0;
    int lastx = g_pl.x, lasty = g_pl.y;
    unsigned dirs[4] = { KEY_RIGHT, KEY_DOWN, KEY_LEFT, KEY_UP };
    for (int f = 0; f < 1500; f++) {
        unsigned key = dirs[(f / 90) % 4];
        set_keys((u16)key);
        game_frame();
        mobs_clear();
        g_pl.hp = 20;
        if (g_pl.x != lastx || g_pl.y != lasty) { steps++; lastx = g_pl.x; lasty = g_pl.y; }
        const Camera *c = &g_host_cam;
        /* screen position of the fixed point (top-left of its cube) */
        int sx, sy;
        cam_cube_origin(c, fixed_x, fixed_y, fixed_z, &sx, &sy);
        if (have) {
            int jx = abs(sx - prev_sx), jy = abs(sy - prev_sy);
            int j = jx > jy ? jx : jy;
            if (j > maxjump) maxjump = j;
            if (j > 6) printf("jump %d at frame %d: cam %d,%d,%d off %d,%d  pl %d,%d,%d moving %d t %d\n", j, f, c->cx, c->cy, c->cz, c->offx, c->offy, g_pl.x, g_pl.y, g_pl.z, g_pl.moving, g_pl.step_t);
        }
        prev_sx = sx; prev_sy = sy; have = 1;
        if (g_pl.moving) frames_moving++;
    }
    printf("steps=%d moving frames=%d max frame-to-frame jump=%d px\n", steps, frames_moving, maxjump);
    /* a drop of two or more blocks may jump by up to 12 px (margin limit) */
    return maxjump > 12;
}
