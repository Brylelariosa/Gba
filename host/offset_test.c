/* Checks that sliding the window over an image centred on block A by the
 * projected distance gives exactly the image centred on block B. */
#include <stdio.h>
#include <string.h>
#include "../src/hw.h"
#include "../src/world.h"
#include "../src/render.h"
#include "../src/texgen.h"
#include "../src/palette.h"
#include "../src/move.h"

u8 host_io[0x400] __attribute__((aligned(4)));
u8 host_pal[0x400] __attribute__((aligned(4)));
u8 host_vram[0x18000] __attribute__((aligned(4)));
u8 host_oam[0x400] __attribute__((aligned(4)));
u8 host_sram[0x8000] __attribute__((aligned(4)));

static u16 pa[240 * 160 / 2], pb[240 * 160 / 2];

int main(void)
{
    palette_init();
    texgen_init();
    world_generate(4242);
    int checked = 0, bad = 0, maxdiff = 0;
    for (int y = 12; y < 52; y += 5) {
        for (int x = 12; x < 52; x += 5) {
            int z = world_surface_z(x, y) + 1;
            for (int dir = 0; dir < 4; dir++) {
                int nz;
                if (!move_try(x, y, z, g_dir_dx[dir], g_dir_dy[dir], 99, &nz)) continue;
                int nx = x + g_dir_dx[dir], ny = y + g_dir_dy[dir];
                Camera a = { x, y, z - 1, WZ - 1, 0, 0 };
                Camera b = { nx, ny, nz - 1, WZ - 1, 0, 0 };
                int dx = nx - x, dy = ny - y, dz = nz - z;
                int offx = (dx - dy) * 8, offy = (dx + dy) * 4 - dz * 8;
                if (offy > MAX_OFFY || offy < -MAX_OFFY) continue;
                render_now(&a);
                render_present(pa, offx, offy);
                render_now(&b);
                render_present(pb, 0, 0);
                int diff = 0;
                const u8 *A = (const u8 *)pa, *B = (const u8 *)pb;
                for (int i = 0; i < 240 * 160; i++) if (A[i] != B[i]) diff++;
                checked++;
                if (diff) { bad++; if (diff > maxdiff) maxdiff = diff; }
            }
        }
    }
    printf("checked=%d mismatching=%d maxdiff=%d pixels\n", checked, bad, maxdiff);
    return 0;
}
