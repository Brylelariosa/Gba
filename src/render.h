#ifndef RENDER_H
#define RENDER_H

#include "common.h"

/* Camera: (cx,cy,cz) is the block the player stands ON (cz = feet z - 1).
 * clipz hides everything above that layer (used when the player is indoors).
 * offx/offy slide the visible window for smooth sub-block scrolling. */
typedef struct {
    s16 cx, cy, cz;
    s16 clipz;
    s16 offx, offy;
} Camera;

/* Off-screen world image: the 240x160 screen plus a margin for scrolling.
 * The margin is the largest slide (MAX_OFF*) plus one lattice step, because
 * cubes are only ever painted whole, never clipped. */
#define MAX_OFFX 8
#define MAX_OFFY 8
#define WIN_X    16
#define WIN_Y    20
#define CACHE_W  (240 + 2 * WIN_X)
#define CACHE_H  (160 + 2 * WIN_Y)

/* Screen position of the top-left corner of the 16x16 cube at (x,y,z). */
static inline void cam_cube_origin(const Camera *c, int x, int y, int z, int *sx, int *sy)
{
    int dx = x - c->cx, dy = y - c->cy, dz = z - c->cz;
    *sx = 112 + (dx - dy) * 8 - c->offx;
    *sy = 76 + (dx + dy) * 4 - dz * 8 - c->offy;
}

/* The world image is double buffered: one image is shown while the next one
 * is painted a slice per frame, then render_swap() flips them. */
void render_begin(const Camera *cam);   /* start painting into the hidden image  */
bool render_step(bool unlimited);       /* paint for a while; true when finished */
bool render_busy(void);                 /* a job is painting                     */
bool render_ready(void);                /* a finished job waits for render_swap  */
void render_swap(void);
void render_now(const Camera *cam);     /* blocking begin + step + swap          */
bool render_has_front(void);
const Camera *render_front(void);       /* camera of the image being shown       */
void render_invalidate(void);           /* forget the shown image                */

/* Copy the visible window of the shown image to a VRAM page. */
void render_present(u16 *page, int offx, int offy);

/* Copy a 16x16 cube image to dst (4-byte aligned) with the given stride. */
IWRAM_CODE void blit_cube(u8 *dst, int stride, const u8 *src);

#endif
