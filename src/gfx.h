#ifndef GFX_H
#define GFX_H

#include "common.h"

/* Immediate-mode drawing onto one 240x160 8 bit VRAM page. */
void gfx_begin(u16 *page);
u16 *gfx_target(void);
void gfx_clear(u8 color);
void gfx_pixel(int x, int y, u8 color);
void gfx_rect(int x, int y, int w, int h, u8 color);
void gfx_frame(int x, int y, int w, int h, u8 color);

/* 3x5 pixel font (upper case), scale 1 = 4 px per character. */
int  gfx_text_w(const char *s, int scale);
void gfx_text(int x, int y, const char *s, u8 color, int scale);
void gfx_text_shadow(int x, int y, const char *s, u8 color, int scale);
void gfx_text_center(int cx, int y, const char *s, u8 color, int scale);
void gfx_number(int x, int y, int value, u8 color, int scale);

/* 16x16 item or block icon. x must be a multiple of 4. */
void gfx_item(int x, int y, u8 item);

/* 7 pixel wide, 6 row bit mask (used for hearts / food). cols limits width. */
void gfx_mask7(int x, int y, const u8 *rows6, int cols, u8 color);

#endif
