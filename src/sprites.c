#include "sprites.h"
#include "hw.h"
#include "texgen.h"
#include <string.h>

static ObjAttr s_oam[128] ALIGN4;
static int s_count;

typedef u8 Px[16][16];

static void rect(Px px, int x, int y, int w, int h, u8 c)
{
    for (int yy = y; yy < y + h; yy++)
        for (int xx = x; xx < x + w; xx++)
            if (xx >= 0 && xx < 16 && yy >= 0 && yy < 16) px[yy][xx] = c;
}

/* Grow a dark outline (colour 7) around every opaque pixel. */
static void outline(Px px)
{
    u8 copy[16][16];
    memcpy(copy, px, 256);
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            if (copy[y][x]) continue;
            bool near = (y > 0 && copy[y - 1][x]) || (y < 15 && copy[y + 1][x]) ||
                        (x > 0 && copy[y][x - 1]) || (x < 15 && copy[y][x + 1]);
            if (near) px[y][x] = 7;
        }
    }
}

static void pack(int frame, Px px)
{
    volatile u16 *t = OBJ_TILES + frame * 64;
    for (int ty = 0; ty < 2; ty++)
        for (int tx = 0; tx < 2; tx++)
            for (int r = 0; r < 8; r++)
                for (int q = 0; q < 2; q++) {
                    u16 v = 0;
                    for (int k = 0; k < 4; k++)
                        v |= (u16)((px[ty * 8 + r][tx * 8 + q * 4 + k] & 15) << (4 * k));
                    t[(ty * 2 + tx) * 16 + r * 2 + q] = v;
                }
}

static void setpal(int bank, int idx, int r, int g, int b)
{
    PAL_OBJ[bank * 16 + idx] = RGB15(r, g, b);
}

/* Blocky humanoid (player / zombie). Colour indices: 1 skin, 2 hair, 3 shirt,
 * 4 pants, 5 shoes, 6 eyes. The figure looks to the right; hflip mirrors it. */
static void humanoid(Px px, bool front, int step, bool arms_out)
{
    memset(px, 0, 256);
    int lh = step ? 3 : 4, rh = step ? 4 : 3;
    rect(px, 5, 12, 3, lh, 4);
    rect(px, 8, 12, 3, rh, 4);
    rect(px, 5, 12 + lh - 1, 3, 1, 5);
    rect(px, 8, 12 + rh - 1, 3, 1, 5);
    rect(px, 5, 7, 6, 5, 3);                       /* body */
    if (arms_out) {
        rect(px, 1, 8, 4, 2, 1);
        rect(px, 11, 8, 4, 2, 1);
    } else {
        rect(px, 3, 7, 2, 5, 1);
        rect(px, 11, 7, 2, 5, 1);
        rect(px, 3, 7, 2, 2, 3);
        rect(px, 11, 7, 2, 2, 3);
    }
    rect(px, 5, 1, 6, 6, 1);                       /* head */
    if (front) {
        rect(px, 5, 1, 6, 2, 2);                   /* hair */
        px[4][7] = 6;
        px[4][9] = 6;
    } else {
        rect(px, 5, 1, 6, 5, 2);
    }
    outline(px);
}

static void pig(Px px, int step)
{
    memset(px, 0, 256);
    int a = step ? 3 : 4, b = step ? 4 : 3;
    rect(px, 3, 12, 2, a, 2);
    rect(px, 5, 12, 2, b, 2);
    rect(px, 8, 12, 2, a, 2);
    rect(px, 10, 12, 2, b, 2);
    rect(px, 3, 12 + a - 1, 2, 1, 5);
    rect(px, 5, 12 + b - 1, 2, 1, 5);
    rect(px, 8, 12 + a - 1, 2, 1, 5);
    rect(px, 10, 12 + b - 1, 2, 1, 5);
    rect(px, 2, 7, 10, 5, 1);                      /* body */
    rect(px, 11, 5, 4, 5, 1);                      /* head */
    rect(px, 14, 7, 2, 3, 3);                      /* snout */
    rect(px, 11, 4, 2, 1, 2);                      /* ear */
    px[6][12] = 4;                                 /* eye */
    px[7][1] = 2;                                  /* tail */
    outline(px);
}

static bool in_cube(int r, int c)
{
    if (r < 0 || r > 15 || c < 0 || c > 15) return false;
    return c >= g_cube_row_x0[r] && c < g_cube_row_x0[r] + g_cube_row_w[r];
}

/* White outline of a cube: marks the block the player is aiming at. */
static void cursor(Px px)
{
    memset(px, 0, 256);
    for (int r = 0; r < 16; r++) {
        for (int c = g_cube_row_x0[r]; c < g_cube_row_x0[r] + g_cube_row_w[r]; c++) {
            if (!in_cube(r - 1, c) || !in_cube(r + 1, c) || !in_cube(r, c - 1) || !in_cube(r, c + 1))
                px[r][c] = 1;
        }
    }
}

static void crack(Px px, int stage)
{
    memset(px, 0, 256);
    u32 seed = 0x9E3779B1u;
    for (int r = 0; r < 16; r++) {
        for (int c = g_cube_row_x0[r]; c < g_cube_row_x0[r] + g_cube_row_w[r]; c++) {
            seed = seed * 1664525u + 1013904223u;
            if (((seed >> 24) & 255) < (u32)(28 + stage * 34)) px[r][c] = 2;
        }
    }
}

void spr_init(void)
{
    for (int bank = 0; bank < 4; bank++) setpal(bank, 7, 20, 16, 12);

    setpal(BANK_PLAYER, 1, 240, 190, 150);
    setpal(BANK_PLAYER, 2, 80, 50, 30);
    setpal(BANK_PLAYER, 3, 40, 180, 190);
    setpal(BANK_PLAYER, 4, 60, 70, 170);
    setpal(BANK_PLAYER, 5, 50, 50, 55);
    setpal(BANK_PLAYER, 6, 30, 30, 40);

    setpal(BANK_ZOMBIE, 1, 100, 170, 80);
    setpal(BANK_ZOMBIE, 2, 50, 100, 50);
    setpal(BANK_ZOMBIE, 3, 50, 110, 170);
    setpal(BANK_ZOMBIE, 4, 60, 50, 120);
    setpal(BANK_ZOMBIE, 5, 40, 40, 50);
    setpal(BANK_ZOMBIE, 6, 210, 30, 30);

    setpal(BANK_PIG, 1, 245, 160, 170);
    setpal(BANK_PIG, 2, 210, 115, 130);
    setpal(BANK_PIG, 3, 255, 195, 205);
    setpal(BANK_PIG, 4, 20, 20, 20);
    setpal(BANK_PIG, 5, 120, 70, 70);

    setpal(BANK_FX, 1, 255, 255, 255);
    setpal(BANK_FX, 2, 20, 20, 24);

    Px px;
    humanoid(px, true, 0, false);  pack(SP_PLAYER_F0, px);
    humanoid(px, true, 1, false);  pack(SP_PLAYER_F1, px);
    humanoid(px, false, 0, false); pack(SP_PLAYER_B0, px);
    humanoid(px, false, 1, false); pack(SP_PLAYER_B1, px);
    humanoid(px, true, 0, true);   pack(SP_ZOMBIE_F0, px);
    humanoid(px, true, 1, true);   pack(SP_ZOMBIE_F1, px);
    humanoid(px, false, 0, true);  pack(SP_ZOMBIE_B0, px);
    humanoid(px, false, 1, true);  pack(SP_ZOMBIE_B1, px);
    pig(px, 0); pack(SP_PIG0, px);
    pig(px, 1); pack(SP_PIG1, px);
    cursor(px); pack(SP_CURSOR, px);
    for (int i = 0; i < 4; i++) { crack(px, i); pack(SP_CRACK0 + i, px); }

    spr_begin();
    spr_end();
}

void spr_begin(void)
{
    s_count = 0;
}

void spr_put(int frame, int x, int y, bool hflip, int bank)
{
    if (s_count >= 128) return;
    if (x <= -16 || x >= SCREEN_W || y <= -16 || y >= SCREEN_H) return;
    ObjAttr *o = &s_oam[s_count++];
    o->a0 = (u16)(y & 0xFF);
    o->a1 = (u16)((x & 0x1FF) | (hflip ? 0x1000 : 0) | 0x4000);
    o->a2 = (u16)((512 + frame * 4) | (bank << 12));
    o->pad = 0;
}

void spr_end(void)
{
    for (int i = s_count; i < 128; i++) {
        s_oam[i].a0 = 0x0200; /* disabled */
        s_oam[i].a1 = 0;
        s_oam[i].a2 = 0;
        s_oam[i].pad = 0;
    }
    hw_oam_flush(s_oam);
}
