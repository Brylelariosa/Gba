#include "render.h"
#include "world.h"
#include "texgen.h"
#include "palette.h"
#include "hw.h"

/* Two off-screen images of the visible world (8 bit palette indices). */
static u8 s_cache[2][CACHE_H][CACHE_W] EWRAM_BSS ALIGN4;

/* Where the camera block's cube sits inside an off-screen image. */
#define ORG_X (WIN_X + 112)
#define ORG_Y (WIN_Y + 76)

/* A render slice may run until this many scanlines after VBlank started; the
 * rest of the frame is needed to copy the image to the screen. */
#define RENDER_DEADLINE 96

typedef enum { JOB_IDLE, JOB_RUNNING, JOB_READY } JobState;

static struct {
    Camera cam;
    int clear_row;
    int d;
    JobState state;
} s_job;

static int s_front;          /* index of the image being shown */
static Camera s_front_cam;
static bool s_has_front;

/* ------------------------------------------------------------------ */
/* Time slicing                                                        */
/* ------------------------------------------------------------------ */

#ifdef HOST_TEST
unsigned long host_blit_count;
static int s_budget;
static void time_begin(bool unlimited) { s_budget = unlimited ? 100000000 : 220; }
static void spend(int cubes) { s_budget -= cubes; }
static bool out_of_time(void) { return s_budget <= 0; }
#else
static bool s_unlimited;
static void time_begin(bool unlimited) { s_unlimited = unlimited; }
static void spend(int cubes) { (void)cubes; }
static bool out_of_time(void)
{
    if (s_unlimited) return false;
    int v = REG_VCOUNT;
    int t = v >= 160 ? v - 160 : v + 68;
    return t >= RENDER_DEADLINE;
}
#endif

/* ------------------------------------------------------------------ */
/* Cube blitter                                                        */
/* ------------------------------------------------------------------ */

/* Copies the hexagon of a cube. Rows are unrolled by width: 4, 8, 12 or 16
 * pixels, so every copy is 16 or 32 bit aligned. Leaf function on purpose:
 * it lives in IWRAM and must not call back into ROM. */
IWRAM_CODE void blit_cube(u8 *dst, int stride, const u8 *src)
{
#ifdef HOST_TEST
    host_blit_count++;
#endif
#define COPY16(dr, sr) do { u32 *d_ = (u32 *)(dr); const u32 *s_ = (const u32 *)(sr); \
        d_[0] = s_[0]; d_[1] = s_[1]; d_[2] = s_[2]; d_[3] = s_[3]; } while (0)
#define COPY8(dr, sr) do { u32 *d_ = (u32 *)((dr) + 4); const u32 *s_ = (const u32 *)((sr) + 4); \
        d_[0] = s_[0]; d_[1] = s_[1]; } while (0)
#define COPY12(dr, sr) do { ((u16 *)((dr) + 2))[0] = ((const u16 *)((sr) + 2))[0]; COPY8(dr, sr); \
        ((u16 *)((dr) + 12))[0] = ((const u16 *)((sr) + 12))[0]; } while (0)
#define COPY4(dr, sr) do { ((u16 *)((dr) + 6))[0] = ((const u16 *)((sr) + 6))[0]; \
        ((u16 *)((dr) + 8))[0] = ((const u16 *)((sr) + 8))[0]; } while (0)

    COPY4(dst, src);   dst += stride; src += 16;
    COPY8(dst, src);   dst += stride; src += 16;
    COPY12(dst, src);  dst += stride; src += 16;
    for (int i = 0; i < 10; i++) {
        COPY16(dst, src);
        dst += stride;
        src += 16;
    }
    COPY12(dst, src);  dst += stride; src += 16;
    COPY8(dst, src);   dst += stride; src += 16;
    COPY4(dst, src);

#undef COPY16
#undef COPY8
#undef COPY12
#undef COPY4
}

/* ------------------------------------------------------------------ */
/* Painter's algorithm                                                 */
/* ------------------------------------------------------------------ */

/* One row of the isometric lattice (d = x + y). Rows are painted from back to
 * front and inside a row each column of cubes from bottom to top. Cubes that
 * are completely hidden behind their neighbours are skipped. */
static void draw_diagonal(u8 (*buf)[CACHE_W], const Camera *cam, int d)
{
    const int cx = cam->cx, cy = cam->cy, cz = cam->cz;
    const int clipz = cam->clipz;
    const int cd = cx + cy, ce = cx - cy;

    int base_y = ORG_Y + (d - cd) * 4 + cz * 8; /* y of a cube at z = 0 */
    if (base_y < 0 || base_y - (WZ - 1) * 8 > CACHE_H - 16) return;

    int emin = MAX(MAX(-d, d - 2 * (WY - 1)), ce - ORG_X / 8);
    int emax = MIN(MIN(d, 2 * (WX - 1) - d), ce + (CACHE_W - 16 - ORG_X) / 8);
    if ((emin ^ d) & 1) emin++;

    for (int e = emin; e <= emax; e += 2) {
        int x = (d + e) >> 1, y = (d - e) >> 1;
        int lo = g_col_lo[y][x], hi = g_col_hi[y][x];
        if (hi > clipz + 1) hi = clipz + 1;
        if (lo > clipz) lo = clipz;
        if (lo >= hi) continue;

        int ox = ORG_X + (e - ce) * 8;
        const u8 *col = &g_world[lo][y][x];
        for (int z = lo; z < hi; z++, col += WY * WX) {
            u8 b = *col;
            if (b == B_AIR) continue;
            int oy = base_y - z * 8;
            if (oy < 0 || oy > CACHE_H - 16) continue;
            bool r_open = (x + 1 >= WX) || col[1] == B_AIR;
            bool l_open = (y + 1 >= WY) || col[WX] == B_AIR;
            bool t_open = (z >= clipz) || (z + 1 >= WZ) || col[WY * WX] == B_AIR;
            if (!(r_open || l_open || t_open)) continue;
            blit_cube(&buf[oy][ox], CACHE_W, g_cube_tex[b]);
            spend(1);
        }
    }
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

void render_begin(const Camera *cam)
{
    s_job.cam = *cam;
    s_job.clear_row = 0;
    s_job.d = 0;
    s_job.state = JOB_RUNNING;
}

bool render_step(bool unlimited)
{
    if (s_job.state != JOB_RUNNING) return s_job.state == JOB_READY;
    u8 (*buf)[CACHE_W] = s_cache[s_front ^ 1];
    time_begin(unlimited);

    while (s_job.clear_row < CACHE_H) {
        int n = MIN(8, CACHE_H - s_job.clear_row);
        hw_fill16(&buf[s_job.clear_row][0], (u16)(C_SKY * 0x0101), (u32)(n * CACHE_W / 2));
        s_job.clear_row += n;
        spend(8);
        if (out_of_time()) return false;
    }
    while (s_job.d <= WX + WY - 2) {
        draw_diagonal(buf, &s_job.cam, s_job.d);
        s_job.d++;
        if (out_of_time()) return false;
    }
    s_job.state = JOB_READY;
    return true;
}

bool render_busy(void)  { return s_job.state == JOB_RUNNING; }
bool render_ready(void) { return s_job.state == JOB_READY; }

void render_swap(void)
{
    if (s_job.state != JOB_READY) return;
    s_front ^= 1;
    s_front_cam = s_job.cam;
    s_has_front = true;
    s_job.state = JOB_IDLE;
}

void render_now(const Camera *cam)
{
    render_begin(cam);
    while (!render_step(true)) { }
    render_swap();
}

bool render_has_front(void) { return s_has_front; }
const Camera *render_front(void) { return &s_front_cam; }

void render_invalidate(void)
{
    s_has_front = false;
    s_job.state = JOB_IDLE;
}

void render_present(u16 *page, int offx, int offy)
{
    offx = CLAMP(offx, -MAX_OFFX, MAX_OFFX) & ~1;
    offy = CLAMP(offy, -MAX_OFFY, MAX_OFFY);
    const u8 *src = &s_cache[s_front][WIN_Y + offy][WIN_X + offx];
    for (int y = 0; y < SCREEN_H; y++)
        hw_copy16(page + y * (SCREEN_W / 2), src + y * CACHE_W, SCREEN_W / 2);
}
