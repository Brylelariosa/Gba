#ifndef STATISTICS_H
#define STATISTICS_H

#include <gba.h>

#define GRAPH_SAMPLES 120

typedef struct {
    u32 pop_history[GRAPH_SAMPLES];
    u32 births_history[GRAPH_SAMPLES];
    u32 deaths_history[GRAPH_SAMPLES];
    u32 largest_pop;
    u32 total_births;
    u32 total_deaths;
    u32 elapsed_frames;
    int fps;
    int head;
} Stats;

extern Stats g_stats;

void stats_init(void);
void stats_update(void);
void stats_draw(void);

#endif