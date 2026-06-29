/*
 * Cellular Automata Laboratory
 * Game Boy Advance
 * main.c - entry point, main loop
 */

#include <gba.h>
#include "simulation.h"
#include "renderer.h"
#include "input.h"
#include "ui.h"
#include "themes.h"
#include "statistics.h"
#include "audio.h"
#include "save.h"
#include "benchmark.h"

/* Required GBA ROM header fields */
__attribute__((section(".rodata")))
const char *GAME_TITLE = "CELLABGBA";

int main(void) {
    /* ── Hardware init ─────────────────────────── */
    irqInit();
    irqEnable(IRQ_VBLANK);

    /* ── Sub-system init ───────────────────────── */
    sim_init();
    renderer_init();
    themes_init(0);
    stats_init();
    bench_init();
    save_init();
    audio_init();
    ui_init();

    /* ── Main loop ─────────────────────────────── */
    for (;;) {
        /* VSync */
        renderer_vsync();

        /* Input */
        input_update();

        /* Logic */
        ui_update();

        /* Simulate if running */
        if (g_state == STATE_SIM && !g_sim.paused) {
            for (int s = 0; s < g_sim.speed; s++)
                sim_step();
            stats_update();
        }

        /* Draw */
        ui_draw();
        renderer_flip();
    }

    return 0;
}