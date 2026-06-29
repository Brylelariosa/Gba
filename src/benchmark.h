#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <gba.h>

typedef struct {
    u32 frames_timed;
    u32 total_frames;
    int avg_fps;
    u32 cell_updates_per_sec;
    u32 gen_per_sec;
} BenchResult;

extern BenchResult g_bench;

void bench_init(void);
void bench_run(void);
void bench_draw(void);

#endif