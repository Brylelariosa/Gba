#include "palette.h"
#include "hw.h"

/* Colour of each material at 100% brightness (shade index 4). */
static const u8 MAT_RGB[M_COUNT][3] = {
    [M_GRASS]   = {  96, 168,  56 },
    [M_DIRT]    = { 134,  96,  60 },
    [M_STONE]   = { 128, 128, 134 },
    [M_SAND]    = { 228, 208, 142 },
    [M_WATER]   = {  52, 104, 208 },
    [M_BARK]    = { 100,  74,  44 },
    [M_PLANK]   = { 184, 144,  84 },
    [M_LEAVES]  = {  56, 128,  44 },
    [M_COAL]    = {  52,  52,  60 },
    [M_IRON]    = { 222, 176, 146 },
    [M_GOLD]    = { 244, 214,  64 },
    [M_DIAMOND] = {  88, 232, 232 },
    [M_BEDROCK] = {  70,  70,  76 },
    [M_BRICK]   = { 158,  88,  76 },
    [M_GLASS]   = { 196, 232, 244 },
    [M_TABLE]   = { 140,  92,  48 },
    [M_FIRE]    = { 255, 130,  40 },
};

static const u8 SHADE_PCT[SHADES] = { 42, 58, 72, 86, 100, 114, 130 };

static const u8 UI_RGB[16][3] = {
    [C_TRANSPARENT] = {   0,   0,   0 },
    [C_SKY]         = { 110, 185, 255 },
    [C_WHITE]       = { 255, 255, 255 },
    [C_LGRAY]       = { 200, 200, 205 },
    [C_GRAY]        = { 140, 140, 148 },
    [C_DGRAY]       = {  60,  60,  68 },
    [C_BLACK]       = {  16,  16,  20 },
    [C_RED]         = { 220,  40,  40 },
    [C_DRED]        = { 120,  20,  24 },
    [C_GREEN]       = {  60, 200,  80 },
    [C_YELLOW]      = { 250, 225,  70 },
    [C_ORANGE]      = { 240, 140,  40 },
    [C_BROWN]       = { 120,  80,  44 },
    [C_BLUE]        = {  60, 100, 220 },
    [C_CYAN]        = {  90, 220, 230 },
    [C_PINK]        = { 240, 150, 170 },
};

static int clamp255(int v) { return v > 255 ? 255 : v; }

static u16 shade_color(int m, int s, int tr, int tg, int tb)
{
    int r = clamp255(MAT_RGB[m][0] * SHADE_PCT[s] / 100);
    int g = clamp255(MAT_RGB[m][1] * SHADE_PCT[s] / 100);
    int b = clamp255(MAT_RGB[m][2] * SHADE_PCT[s] / 100);
    return RGB15((r * tr) >> 8, (g * tg) >> 8, (b * tb) >> 8);
}

void palette_init(void)
{
    for (int i = 0; i < 16; i++)
        PAL_BG[i] = RGB15(UI_RGB[i][0], UI_RGB[i][1], UI_RGB[i][2]);
    for (int m = 0; m < M_COUNT; m++)
        for (int s = 0; s < SHADES; s++)
            PAL_BG[PAL_UI(m, s)] = shade_color(m, s, 256, 256, 256);
    palette_set_daylight(0);
}

void palette_set_daylight(int level)
{
    if (level < 0) level = 0;
    if (level > 8) level = 8;
    /* night multiplies the world towards a dark blue (256 = no change) */
    int tr = 256 - (256 - 100) * level / 8;
    int tg = 256 - (256 - 116) * level / 8;
    int tb = 256 - (256 - 184) * level / 8;
    for (int m = 0; m < M_COUNT; m++)
        for (int s = 0; s < SHADES; s++)
            PAL_BG[PAL_WORLD(m, s)] = shade_color(m, s, tr, tg, tb);

    int sr = UI_RGB[C_SKY][0] + (8 - UI_RGB[C_SKY][0]) * level / 8;
    int sg = UI_RGB[C_SKY][1] + (12 - UI_RGB[C_SKY][1]) * level / 8;
    int sb = UI_RGB[C_SKY][2] + (38 - UI_RGB[C_SKY][2]) * level / 8;
    PAL_BG[C_SKY] = RGB15(sr, sg, sb);
}
