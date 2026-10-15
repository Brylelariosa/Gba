/* Counts how many cubes the renderer draws for typical camera positions. */
#include <stdio.h>
#include <string.h>
#include "../src/hw.h"
#include "../src/world.h"
#include "../src/render.h"
#include "../src/texgen.h"
#include "../src/palette.h"

u8 host_io[0x400] __attribute__((aligned(4)));
u8 host_pal[0x400] __attribute__((aligned(4)));
u8 host_vram[0x18000] __attribute__((aligned(4)));
u8 host_oam[0x400] __attribute__((aligned(4)));
u8 host_sram[0x8000] __attribute__((aligned(4)));
extern unsigned long host_blit_count;

int main(void)
{
    palette_init();
    texgen_init();
    unsigned long total = 0, worst = 0, n = 0;
    for (unsigned seed = 1; seed <= 6; seed++) {
        world_generate(seed * 7919u);
        for (int y = 14; y < 50; y += 6) {
            for (int x = 14; x < 50; x += 6) {
                int top = world_surface_z(x, y);
                if (top < 0) continue;
                Camera c = { x, y, top, WZ - 1, 0, 0 };
                host_blit_count = 0;
                render_now(&c);
                total += host_blit_count; n++;
                if (host_blit_count > worst) worst = host_blit_count;
            }
        }
    }
    printf("renders=%lu avg cubes=%lu worst=%lu\n", n, total / n, worst);
    return 0;
}
