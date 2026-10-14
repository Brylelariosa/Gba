#include "texgen.h"
#include "palette.h"
#include <string.h>

u8 g_cube_tex[B_COUNT][256] ALIGN4;
u8 g_cube_ui[B_COUNT][256] EWRAM_BSS ALIGN4;

/* A cube is a hexagon 16 px wide and 16 px tall: a 16x8 top diamond with two
 * 8 px high side faces below it. */
const u8 g_cube_row_x0[16] = { 6, 4, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 4, 6 };
const u8 g_cube_row_w[16]  = { 4, 8, 12, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 12, 8, 4 };
static const u8 TOP_W[8]   = { 4, 8, 12, 16, 16, 12, 8, 4 };

#define P(m, s) PAL_WORLD((m), CLAMP((s), 0, SHADES - 1))

static u32 hsh(u32 a, u32 b, u32 c, u32 d)
{
    u32 h = a * 374761393u + b * 668265263u + c * 2246822519u + d * 3266489917u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

static const u8 ORE_ROWS[8] = { 0x00, 0x62, 0x46, 0x10, 0x30, 0x86, 0xC4, 0x00 };

static bool ore_spot(int f, int u, int v)
{
    int uu = (u + f * 3) & 7, vv = (v + f * 2) & 7;
    return ((ORE_ROWS[vv] >> (7 - uu)) & 1) != 0;
}

static int stone_px(int base, u32 n)
{
    int j = (n < 3) ? -1 : (n > 12 ? 1 : 0);
    return P(M_STONE, base + j);
}

/* Colour of one texel. f: 0 = top, 1 = left face, 2 = right face.
 * u,v are 0..7 face coordinates (v counts down from the face's top edge). */
static int texel(u8 b, int f, int u, int v)
{
    int base = (f == 0) ? 5 : (f == 1 ? 4 : 3);
    u32 h = hsh(b, (u32)f, (u32)u, (u32)v);
    u32 n = h & 15;
    int j = (n < 2) ? -1 : (n > 13 ? 1 : 0);

    switch (b) {
    case B_GRASS:
        if (f == 0) return P(M_GRASS, base + j);
        if (v < 2 || (v == 2 && (n & 1))) return P(M_GRASS, base + j);
        return P(M_DIRT, base + j);
    case B_DIRT:
        return P(M_DIRT, base + j);
    case B_STONE:
        return stone_px(base, n);
    case B_COBBLE: {
        bool gap = ((v & 3) == 3) || (((u + ((v >> 2) * 2)) & 3) == 3);
        if (gap) return P(M_STONE, base - 2);
        return P(M_STONE, base + (n < 5 ? -1 : (n > 11 ? 1 : 0)));
    }
    case B_SAND:
        return P(M_SAND, base + j);
    case B_WATER: {
        int s = base;
        if (f == 0 && ((u + 2 * v) & 7) == 0) s += 1;
        return P(M_WATER, s + j);
    }
    case B_LOG:
        if (f == 0) {
            int m = MAX(ABS(2 * u - 7), ABS(2 * v - 7));
            if (m >= 6) return P(M_BARK, base - 1 + j);
            return P(M_PLANK, base + (((m >> 1) & 1) ? 0 : -1));
        }
        return P(M_BARK, base + ((u & 1) ? -1 : 0) + j);
    case B_LEAVES:
        return P(M_LEAVES, base + (n < 4 ? -2 : (n < 8 ? 1 : 0)));
    case B_PLANKS: {
        int s = base;
        if ((v & 3) == 3) s -= 2;
        else if (((u + ((v >> 2) * 4)) & 7) == 7) s -= 1;
        return P(M_PLANK, s + j);
    }
    case B_COAL_ORE:
        if (ore_spot(f, u, v)) return P(M_COAL, 2 + (f == 0 ? 1 : 0) + (int)(n & 1));
        return stone_px(base, n);
    case B_IRON_ORE:
        if (ore_spot(f, u, v)) return P(M_IRON, 4 + (int)(n & 1));
        return stone_px(base, n);
    case B_GOLD_ORE:
        if (ore_spot(f, u, v)) return P(M_GOLD, 5 + (int)(n & 1));
        return stone_px(base, n);
    case B_DIAMOND_ORE:
        if (ore_spot(f, u, v)) return P(M_DIAMOND, 5 + (int)(n & 1));
        return stone_px(base, n);
    case B_BEDROCK:
        return P(M_BEDROCK, base - 2 + (int)(n & 3));
    case B_TABLE:
        if (f == 0) {
            if (u == 0 || v == 0 || u == 7 || v == 7) return P(M_TABLE, base - 1);
            if (((u == 3 || u == 4) && v >= 1 && v <= 6) || ((v == 3 || v == 4) && u >= 1 && u <= 6))
                return P(M_BARK, base - 1);
            return P(M_PLANK, base + j);
        }
        if (v < 2) return P(M_PLANK, base + 1);
        if (u >= 2 && u <= 5 && v >= 3 && v <= 5) return P(M_COAL, base - 2);
        return P(M_TABLE, base + j);
    case B_FURNACE:
        if (f == 0) {
            if (u >= 2 && u <= 5 && v >= 2 && v <= 5) return P(M_COAL, 1);
            return P(M_STONE, base - 1 + (((v & 3) == 3) ? -1 : 0) + j);
        }
        if (u >= 2 && u <= 5 && v >= 3 && v <= 6) {
            if (v >= 5 && ((u + v) & 1)) return P(M_FIRE, 5);
            return P(M_COAL, 1);
        }
        return P(M_STONE, base - 1 + (((v & 3) == 3) ? -1 : 0) + j);
    case B_GLASS:
        if (u == 0 || v == 0 || u == 7 || v == 7) return P(M_GLASS, base - 1);
        if ((u + v) == 5 || (u + v) == 6) return P(M_GLASS, base + 2);
        return P(M_GLASS, base + (n < 3 ? -1 : 0));
    case B_BRICK: {
        bool mortar = (v % 3 == 2) || (((u + ((v / 3) & 1) * 2) & 3) == 3);
        if (mortar) return P(M_STONE, base + 1);
        return P(M_BRICK, base + j);
    }
    case B_BEACON: {
        int d = MAX(ABS(2 * u - 7), ABS(2 * v - 7));
        if (d <= 3) return P(M_FIRE, 6);
        if (d <= 5) return P(M_GOLD, base);
        return P(M_COAL, base - 1 + j);
    }
    default:
        return P(M_STONE, base);
    }
}

static bool cube_inside(int r, int c)
{
    if (r < 0 || r > 15 || c < 0 || c > 15) return false;
    return c >= g_cube_row_x0[r] && c < g_cube_row_x0[r] + g_cube_row_w[r];
}

static u8 darker(u8 idx, int amount)
{
    int rel = idx - 16;
    int m = rel / SHADES, s = rel % SHADES;
    s -= amount;
    if (s < 0) s = 0;
    return (u8)PAL_WORLD(m, s);
}

static void build_block(u8 b)
{
    u8 *t = g_cube_tex[b];
    memset(t, 0, 256);
    for (int r = 0; r < 16; r++) {
        for (int c = g_cube_row_x0[r]; c < g_cube_row_x0[r] + g_cube_row_w[r]; c++) {
            int f, u, v;
            if (r < 8 && c >= 8 - TOP_W[r] / 2 && c < 8 + TOP_W[r] / 2) {
                int a = 2 * c + 4 * r - 13;
                int bb = 4 * r - 2 * c + 17;
                f = 0;
                u = a < 0 ? 0 : MIN(7, a / 4);
                v = bb < 0 ? 0 : MIN(7, bb / 4);
            } else {
                int cc = (c < 8) ? c : 15 - c;
                f = (c < 8) ? 1 : 2;
                u = cc;
                v = CLAMP(r - (5 + (cc >> 1)), 0, 7);
            }
            t[r * 16 + c] = (u8)texel(b, f, u, v);
        }
    }
    /* darken the outline so neighbouring cubes read as separate blocks */
    u8 copy[256];
    memcpy(copy, t, 256);
    for (int r = 0; r < 16; r++) {
        for (int c = g_cube_row_x0[r]; c < g_cube_row_x0[r] + g_cube_row_w[r]; c++) {
            if (!cube_inside(r - 1, c) || !cube_inside(r + 1, c) ||
                !cube_inside(r, c - 1) || !cube_inside(r, c + 1))
                t[r * 16 + c] = darker(copy[r * 16 + c], 2);
        }
    }
}

void texgen_init(void)
{
    memset(g_cube_tex, 0, sizeof g_cube_tex);
    memset(g_cube_ui, 0, sizeof g_cube_ui);
    for (int b = 1; b < B_COUNT; b++) {
        build_block((u8)b);
        for (int i = 0; i < 256; i++)
            g_cube_ui[b][i] = g_cube_tex[b][i] ? (u8)(g_cube_tex[b][i] + PAL_UI_DELTA) : 0;
    }
}
