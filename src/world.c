#include "world.h"
#include "rng.h"
#include "items.h"
#include <string.h>

u8 g_world[WZ][WY][WX] EWRAM_BSS ALIGN4;
u8 g_col_lo[WY][WX] EWRAM_BSS;
u8 g_col_hi[WY][WX] EWRAM_BSS;
u8 g_world_mod[(WX * WY * WZ) / 8] EWRAM_BSS;
u32 g_world_seed;
bool g_world_dirty;

static s8 s_hmap[WY][WX] EWRAM_BSS; /* terrain height per column (gen only) */
static u8 s_bio[WY][WX] EWRAM_BSS;  /* bit0 = desert, bit1 = forest         */

static const s8 DX4[4] = { 0, 1, 0, -1 };
static const s8 DY4[4] = { -1, 0, 1, 0 };

/* ------------------------------------------------------------------ */
/* Value noise                                                         */
/* ------------------------------------------------------------------ */

static u32 hash2(int x, int y, u32 seed)
{
    u32 h = (u32)x * 374761393u + (u32)y * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

/* Smooth value noise 0..255, lattice spacing = 1 << shift cells. */
static int vnoise(int x, int y, int shift, u32 seed)
{
    int ix = x >> shift, iy = y >> shift;
    int mask = (1 << shift) - 1;
    int tx = ((x & mask) << 8) >> shift;
    int ty = ((y & mask) << 8) >> shift;
    tx = (tx * tx * (768 - 2 * tx)) >> 16;
    ty = (ty * ty * (768 - 2 * ty)) >> 16;
    int a = (int)(hash2(ix, iy, seed) & 255);
    int b = (int)(hash2(ix + 1, iy, seed) & 255);
    int c = (int)(hash2(ix, iy + 1, seed) & 255);
    int d = (int)(hash2(ix + 1, iy + 1, seed) & 255);
    int top = a + (((b - a) * tx) >> 8);
    int bot = c + (((d - c) * tx) >> 8);
    return top + (((bot - top) * ty) >> 8);
}

/* ------------------------------------------------------------------ */
/* Column bookkeeping (drives the renderer's culling)                  */
/* ------------------------------------------------------------------ */

static void col_update(int x, int y)
{
    if ((unsigned)x >= WX || (unsigned)y >= WY) return;
    int hi = 0, lo = 255;
    for (int z = 0; z < WZ; z++) {
        if (g_world[z][y][x] == B_AIR) continue;
        hi = z + 1;
        if (lo == 255) {
            bool exposed = (x + 1 >= WX) || g_world[z][y][x + 1] == B_AIR ||
                           (y + 1 >= WY) || g_world[z][y + 1][x] == B_AIR ||
                           (z + 1 >= WZ) || g_world[z + 1][y][x] == B_AIR;
            if (exposed) lo = z;
        }
    }
    if (lo == 255) lo = hi;
    g_col_lo[y][x] = (u8)lo;
    g_col_hi[y][x] = (u8)hi;
}

void world_rebuild_columns(void)
{
    for (int y = 0; y < WY; y++)
        for (int x = 0; x < WX; x++) col_update(x, y);
}

void world_set(int x, int y, int z, u8 b)
{
    if (!world_in(x, y, z)) return;
    g_world[z][y][x] = b;
    int idx = (z * WY + y) * WX + x;
    g_world_mod[idx >> 3] |= (u8)(1u << (idx & 7));
    col_update(x, y);
    col_update(x - 1, y);
    col_update(x, y - 1);
    g_world_dirty = true;
}

void world_edit_raw(int x, int y, int z, u8 b)
{
    if (!world_in(x, y, z)) return;
    g_world[z][y][x] = b;
    int idx = (z * WY + y) * WX + x;
    g_world_mod[idx >> 3] |= (u8)(1u << (idx & 7));
}

int world_surface_z(int x, int y)
{
    if ((unsigned)x >= WX || (unsigned)y >= WY) return -1;
    return (int)g_col_hi[y][x] - 1;
}

/* ------------------------------------------------------------------ */
/* Generation                                                          */
/* ------------------------------------------------------------------ */

static void carve(int x, int y, int z)
{
    if (x < 1 || y < 1 || x >= WX - 1 || y >= WY - 1 || z < 1 || z >= WZ - 1) return;
    u8 b = g_world[z][y][x];
    if (b == B_STONE || b == B_DIRT || b == B_GRASS || b == B_SAND ||
        b == B_COAL_ORE || b == B_IRON_ORE)
        g_world[z][y][x] = B_AIR;
}

static void gen_caves(Rng *r)
{
    for (int w = 0; w < 16; w++) {
        bool entrance = (w & 1) == 0;
        int x = 8 + rng_range(r, WX - 16), y = 8 + rng_range(r, WY - 16);
        if (entrance && s_hmap[y][x] <= SEA_Z + 1) continue; /* keep entrances dry */
        int z = entrance ? s_hmap[y][x] : 3 + rng_range(r, 4);
        int dir = rng_range(r, 4);
        int len = 40 + rng_range(r, 60);
        for (int i = 0; i < len; i++) {
            carve(x, y, z);
            carve(x, y, z + 1);
            if (rng_range(r, 5) == 0) dir = rng_range(r, 4);
            x += DX4[dir];
            y += DY4[dir];
            int v = rng_range(r, 10);
            if (entrance && i < 10) z--;
            else if (v == 0) z++;
            else if (v == 1) z--;
            x = CLAMP(x, 3, WX - 4);
            y = CLAMP(y, 3, WY - 4);
            z = CLAMP(z, 2, WZ - 4);
        }
    }
}

static void gen_ore(Rng *r, u8 blk, int blobs, int size, int zmin, int zmax)
{
    for (int n = 0; n < blobs; n++) {
        int x = 2 + rng_range(r, WX - 4);
        int y = 2 + rng_range(r, WY - 4);
        int z = zmin + rng_range(r, zmax - zmin + 1);
        for (int k = 0; k < size; k++) {
            if (world_in(x, y, z) && g_world[z][y][x] == B_STONE) g_world[z][y][x] = blk;
            switch (rng_range(r, 6)) {
            case 0: x++; break;
            case 1: x--; break;
            case 2: y++; break;
            case 3: y--; break;
            case 4: z++; break;
            default: z--; break;
            }
        }
    }
}

static void place_tree(Rng *r, int x, int y, int h)
{
    int top = h + 4 + rng_range(r, 2);
    for (int z = h + 1; z <= top; z++) g_world[z][y][x] = B_LOG;
    for (int dz = -1; dz <= 1; dz++) {
        int z = top + dz;
        if (z >= WZ) continue;
        int rad = (dz == 1) ? 1 : 2;
        for (int dy = -rad; dy <= rad; dy++) {
            for (int dx = -rad; dx <= rad; dx++) {
                if (ABS(dx) == rad && ABS(dy) == rad) continue; /* round the corners */
                int xx = x + dx, yy = y + dy;
                if (!world_in(xx, yy, z)) continue;
                if (g_world[z][yy][xx] == B_AIR) g_world[z][yy][xx] = B_LEAVES;
            }
        }
    }
}

static void gen_trees(Rng *r)
{
    for (int y = 3; y < WY - 3; y++) {
        for (int x = 3; x < WX - 3; x++) {
            int h = s_hmap[y][x];
            if (h <= SEA_Z + 1 || h > 9 || g_world[h][y][x] != B_GRASS) continue;
            int chance = (s_bio[y][x] & 2) ? 24 : 110;
            if (rng_range(r, chance) != 0) continue;
            bool crowded = false;
            for (int dy = -3; dy <= 3 && !crowded; dy++)
                for (int dx = -3; dx <= 3; dx++) {
                    int hh = s_hmap[y + dy][x + dx];
                    if (g_world[hh + 1][y + dy][x + dx] == B_LOG) { crowded = true; break; }
                }
            if (!crowded) place_tree(r, x, y, h);
        }
    }
}

void world_generate(u32 seed)
{
    Rng rng;
    rng_seed(&rng, seed);
    g_world_seed = seed;
    memset(g_world, 0, sizeof g_world);
    memset(g_world_mod, 0, sizeof g_world_mod);

    /* 1. height map + biome noise */
    for (int y = 0; y < WY; y++) {
        for (int x = 0; x < WX; x++) {
            int n1 = vnoise(x, y, 4, seed);
            int n2 = vnoise(x, y, 3, seed + 101u);
            int n3 = vnoise(x, y, 2, seed + 202u);
            int hn = (n1 * 4 + n2 * 2 + n3) / 7;
            hn = CLAMP((hn - 128) * 3 / 2 + 128, 0, 255);   /* more contrast */
            int h = 4 + ((hn * 11) >> 8);
            int ed = MIN(MIN(x, y), MIN(WX - 1 - x, WY - 1 - y));
            if (ed < 12) h -= ((12 - ed) * (12 - ed)) / 9; /* island shore */
            h = CLAMP(h, 2, 13);
            s_hmap[y][x] = (s8)h;
            u8 bio = 0;
            if (vnoise(x + 97, y + 31, 5, seed + 303u) > 170) bio |= 1;
            if (vnoise(x + 13, y + 57, 5, seed + 404u) > 150) bio |= 2;
            s_bio[y][x] = bio;
        }
    }

    /* 2. fill columns */
    for (int y = 0; y < WY; y++) {
        for (int x = 0; x < WX; x++) {
            int h = s_hmap[y][x];
            bool mount = h >= 12;
            bool sandy = (h <= SEA_Z + 1) || ((s_bio[y][x] & 1) && h > SEA_Z + 1);
            u8 top = mount ? B_STONE : (sandy ? B_SAND : B_GRASS);
            u8 sub = mount ? B_STONE : (sandy ? B_SAND : B_DIRT);
            for (int z = 0; z <= h; z++) {
                u8 b;
                if (z == 0) b = B_BEDROCK;
                else if (z == h) b = top;
                else if (z >= h - 3) b = sub;
                else b = B_STONE;
                g_world[z][y][x] = b;
            }
            for (int z = h + 1; z <= SEA_Z; z++) g_world[z][y][x] = B_WATER;
        }
    }

    /* 3. ores, caves, trees */
    gen_ore(&rng, B_COAL_ORE,    44, 6, 2, 13);
    gen_ore(&rng, B_IRON_ORE,    28, 4, 2, 9);
    gen_ore(&rng, B_GOLD_ORE,    9,  3, 2, 5);
    gen_ore(&rng, B_DIAMOND_ORE, 7,  2, 1, 3);
    gen_caves(&rng);
    gen_trees(&rng);

    world_rebuild_columns();
    g_world_dirty = true;
}

bool world_find_spawn(int *sx, int *sy, int *sz)
{
    for (int r = 0; r < 31; r++) {
        for (int dy = -r; dy <= r; dy++) {
            for (int dx = -r; dx <= r; dx++) {
                if (MAX(ABS(dx), ABS(dy)) != r) continue;
                int x = WX / 2 + dx, y = WY / 2 + dy;
                if (!world_in(x, y, 0)) continue;
                int top = (int)g_col_hi[y][x] - 1;
                if (top < SEA_Z + 1 || top + 2 >= WZ) continue;
                if (g_world[top][y][x] != B_GRASS) continue;
                if (g_world[top + 1][y][x] != B_AIR) continue;
                *sx = x; *sy = y; *sz = top + 1;
                return true;
            }
        }
    }
    /* Fallback: build a little grass platform in the middle of the map. */
    int x = WX / 2, y = WY / 2, top = SEA_Z + 2;
    for (int z = 0; z < WZ; z++) g_world[z][y][x] = z == 0 ? B_BEDROCK : (z < top ? B_STONE : (z == top ? B_GRASS : B_AIR));
    world_rebuild_columns();
    *sx = x; *sy = y; *sz = top + 1;
    return false;
}
