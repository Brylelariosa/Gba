#ifndef THEMES_H
#define THEMES_H

#include <gba.h>

#define THEME_COUNT  8

/* Palette indices */
#define PAL_DEAD     0   /* background / dead cell */
#define PAL_ALIVE    1   /* alive / state 1 */
#define PAL_STATE2   2   /* dying / tail */
#define PAL_STATE3   3   /* conductor */
#define PAL_CYC0     4   /* cyclic states 0-15 → indices 4-19 */
#define PAL_UI_BG   20
#define PAL_UI_TXT  21
#define PAL_UI_HI   22
#define PAL_STAT_BG 23
#define PAL_STAT_TX 24
#define PAL_BORDER  25
#define PAL_CURSOR  26
#define PAL_DARK    27
#define PAL_WHITE   28
#define PAL_RED     29
#define PAL_CYAN    30
#define PAL_YELLOW  31

extern int g_theme;

void themes_init(int idx);
void themes_apply(int idx);

/* GBA BGR555 colour helper */
#define RGB5(r,g,b) ((u16)((r)|((g)<<5)|((b)<<10)))

#endif