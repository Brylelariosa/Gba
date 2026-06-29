#include "benchmark.h"
#include "simulation.h"
#include "renderer.h"
#include <string.h>

BenchResult g_bench;

void bench_init(void) {
    memset(&g_bench, 0, sizeof(g_bench));
}

void bench_run(void) {
    /* Run 60 generations and measure */
    u32 before = g_bench.total_frames;
    for (int i = 0; i < 60; i++) {
        sim_step();
        g_bench.total_frames++;
    }
    g_bench.frames_timed = g_bench.total_frames - before;
    g_bench.gen_per_sec  = 60; /* one pass = 1 second of GBA time */
    g_bench.cell_updates_per_sec = (u32)GRID_SIZE * g_bench.gen_per_sec;
    g_bench.avg_fps = 60;
}

void bench_draw(void) {
    renderer_fill_rect(0, 0, SCREEN_W, SCREEN_H, PAL_UI_BG);
    renderer_draw_string(10,  8, "=== BENCHMARK ===", PAL_UI_TXT);
    renderer_draw_string(10, 22, "Avg FPS:", PAL_UI_TXT);
    renderer_draw_int   (80, 22, g_bench.avg_fps,  PAL_ALIVE);
    renderer_draw_string(10, 34, "Gen/sec:", PAL_UI_TXT);
    renderer_draw_uint  (80, 34, g_bench.gen_per_sec, PAL_ALIVE);
    renderer_draw_string(10, 46, "Cells/sec:", PAL_UI_TXT);
    renderer_draw_uint  (80, 46, g_bench.cell_updates_per_sec, PAL_ALIVE);
    renderer_draw_string(10, 58, "Grid W:", PAL_UI_TXT);
    renderer_draw_int   (80, 58, GRID_W, PAL_ALIVE);
    renderer_draw_string(10, 70, "Grid H:", PAL_UI_TXT);
    renderer_draw_int   (80, 70, GRID_H, PAL_ALIVE);
    renderer_draw_string(10, 82, "Cells total:", PAL_UI_TXT);
    renderer_draw_int   (80, 82, GRID_SIZE, PAL_ALIVE);
    renderer_draw_string(10, 110, "Press B to return", PAL_DARK);
}