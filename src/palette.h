#ifndef PALETTE_H
#define PALETTE_H

#include "common.h"

/* UI colours: palette indices 1..15 (never tinted by day/night). */
enum {
    C_TRANSPARENT = 0, C_SKY, C_WHITE, C_LGRAY, C_GRAY, C_DGRAY, C_BLACK,
    C_RED, C_DRED, C_GREEN, C_YELLOW, C_ORANGE, C_BROWN, C_BLUE, C_CYAN, C_PINK
};

/* Block materials. Every material has SHADES brightness levels. */
enum {
    M_GRASS, M_DIRT, M_STONE, M_SAND, M_WATER, M_BARK, M_PLANK, M_LEAVES,
    M_COAL, M_IRON, M_GOLD, M_DIAMOND, M_BEDROCK, M_BRICK, M_GLASS, M_TABLE,
    M_FIRE, M_COUNT
};

#define SHADES 7
/* World colours get tinted by the time of day, the UI copy never does. */
#define PAL_WORLD(m, s) (16 + (m) * SHADES + (s))
#define PAL_UI(m, s)    (135 + (m) * SHADES + (s))
#define PAL_UI_DELTA    (135 - 16)

void palette_init(void);
void palette_set_daylight(int level); /* 0 = full day .. 8 = deep night */

#endif
