#ifndef MOVE_H
#define MOVE_H

#include "common.h"

/* Direction table: 0 = up (-y), 1 = right (+x), 2 = down (+y), 3 = left (-x). */
extern const s8 g_dir_dx[4];
extern const s8 g_dir_dy[4];

/* Try to walk from (x,y,z) one block in (dx,dy). Walks onto air, steps up one
 * block, or drops down up to max_fall blocks. Water is never entered.
 * Doors are walked through when opens_doors is set (the player), and are a
 * solid wall otherwise (mobs). A wall two blocks high can not be climbed.
 * Returns true and the new z in *out_z on success. */
bool move_try(int x, int y, int z, int dx, int dy, int max_fall, bool opens_doors, int *out_z);

#endif
