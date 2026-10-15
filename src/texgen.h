#ifndef TEXGEN_H
#define TEXGEN_H

#include "common.h"
#include "blocks.h"

/* 16x16 isometric cube images, one per block (palette indices, 0 = empty). */
extern u8 g_cube_tex[B_COUNT][256]; /* world colours (day/night tinted) */
extern u8 g_cube_ui[B_COUNT][256];  /* same image with the untinted UI colours */

/* Silhouette of a cube: per row start column and width. */
extern const u8 g_cube_row_x0[16];
extern const u8 g_cube_row_w[16];

void texgen_init(void);

#endif
