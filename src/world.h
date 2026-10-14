#ifndef WORLD_H
#define WORLD_H

#include "common.h"
#include "blocks.h"

#define WX 64
#define WY 64
#define WZ 16
#define SEA_Z 6

extern u8 g_world[WZ][WY][WX];
extern u8 g_col_lo[WY][WX];   /* lowest possibly-visible z in the column    */
extern u8 g_col_hi[WY][WX];   /* one above the highest non-air block        */
extern u8 g_world_mod[(WX * WY * WZ) / 8]; /* bitmap of player edits (saved) */
extern u32 g_world_seed;
extern bool g_world_dirty;    /* set whenever a block changes               */

static inline bool world_in(int x, int y, int z)
{
    return (unsigned)x < WX && (unsigned)y < WY && (unsigned)z < WZ;
}

/* Out of range reads: below the world is bedrock, everywhere else is air. */
static inline u8 world_get(int x, int y, int z)
{
    if (world_in(x, y, z)) return g_world[z][y][x];
    return z < 0 ? B_BEDROCK : B_AIR;
}

void world_generate(u32 seed);
void world_set(int x, int y, int z, u8 b);      /* gameplay edit (tracked) */
void world_edit_raw(int x, int y, int z, u8 b); /* tracked, columns NOT updated (bulk load) */
void world_rebuild_columns(void);
bool world_find_spawn(int *sx, int *sy, int *sz);
int  world_surface_z(int x, int y);             /* top solid z or -1       */

#endif
