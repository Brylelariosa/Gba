#include "statistics.h"
#include "simulation.h"
#include "renderer.h"
#include <string.h>

Stats g_stats;

void stats_init(void) {
    memset(&g_stats, 0, sizeof(g_stats));
}

void stats_update(void) {
    g_stats.elapsed_frames++;
    /* ring buffer */
    int h = g_stats.head;
    g_stats.pop_history[h]    = g_sim.population;
    g_stats.births_history[h] = g_sim.births;
    g_stats.deaths_history[h] = g_sim.deaths;
    g_stats.head = (h + 1) % GRAPH_SAMPLES;

    g_stats.total_births += g_sim.births;
    g_stats.total_deaths += g_sim.deaths;
    if (g_sim.population > g_stats.largest_pop)
        g_stats.largest_pop = g_sim.population;

    /* Simple FPS: count frames per 60 */
    static int frame_count = 0;
    frame_count++;
    if (frame_count >= 60) {
        g_stats.fps  = 60; /* GBA vsyncs at 60 */
        frame_count  = 0;
    }
}

/* Draw a scrolling mini-graph in the sim area (top-left corner) */
void stats_draw(void) {
    /* Graph area: x=0, y=0, w=120, h=30 */
    /* Background */
    renderer_fill_rect(0, 0, 120, 30, PAL_UI_BG);
    renderer_draw_string(2, 2, "Pop", PAL_UI_TXT);
    renderer_draw_uint(22, 2, g_sim.population, PAL_ALIVE);

    /* Plot population */
    u32 max_pop = 1;
    for (int i = 0; i < GRAPH_SAMPLES; i++)
        if (g_stats.pop_history[i] > max_pop)
            max_pop = g_stats.pop_history[i];

    for (int i = 0; i < GRAPH_SAMPLES; i++) {
        int idx = (g_stats.head + i) % GRAPH_SAMPLES;
        u32 v   = g_stats.pop_history[idx];
        int bar = (int)(v * 18 / max_pop);
        if (bar > 0)
            renderer_fill_rect(i, 30 - bar, 1, bar, PAL_ALIVE);
    }
}