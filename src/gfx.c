#include "gfx.h"
#include "hw.h"
#include "icons.h"
#include "items.h"
#include "palette.h"
#include "render.h"
#include "texgen.h"

static u16 *s_dst;

void gfx_begin(u16 *page) { s_dst = page; }
u16 *gfx_target(void) { return s_dst; }

void gfx_clear(u8 color)
{
    hw_fill16(s_dst, (u16)(color * 0x0101), SCREEN_W * SCREEN_H / 2);
}

void gfx_pixel(int x, int y, u8 color)
{
    if ((unsigned)x >= SCREEN_W || (unsigned)y >= SCREEN_H) return;
    u16 *p = &s_dst[(y * SCREEN_W + x) >> 1];
    if (x & 1) *p = (u16)((*p & 0x00FF) | (color << 8));
    else       *p = (u16)((*p & 0xFF00) | color);
}

void gfx_rect(int x, int y, int w, int h, u8 color)
{
    int x1 = x + w, y1 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x1 > SCREEN_W) x1 = SCREEN_W;
    if (y1 > SCREEN_H) y1 = SCREEN_H;
    if (x >= x1 || y >= y1) return;
    u16 v = (u16)(color * 0x0101);
    for (int row = y; row < y1; row++) {
        int cx = x;
        u16 *line = &s_dst[(row * SCREEN_W) >> 1];
        if (cx & 1) { gfx_pixel(cx, row, color); cx++; }
        while (cx + 1 < x1) { line[cx >> 1] = v; cx += 2; }
        if (cx < x1) gfx_pixel(cx, row, color);
    }
}

void gfx_frame(int x, int y, int w, int h, u8 color)
{
    gfx_rect(x, y, w, 1, color);
    gfx_rect(x, y + h - 1, w, 1, color);
    gfx_rect(x, y, 1, h, color);
    gfx_rect(x + w - 1, y, 1, h, color);
}

/* ------------------------------------------------------------------ */
/* Font: ASCII 32..95, each glyph is 5 rows of 3 bits (row 0 on top).  */
/* ------------------------------------------------------------------ */

#define G(a, b, c, d, e) ((u16)(((a) << 12) | ((b) << 9) | ((c) << 6) | ((d) << 3) | (e)))

static const u16 FONT[64] = {
    G(0,0,0,0,0), G(2,2,2,0,2), G(5,5,0,0,0), G(5,7,5,7,5),  /*  ! " # */
    G(3,6,2,3,6), G(5,1,2,4,5), G(2,5,2,5,3), G(2,2,0,0,0),  /*  $ % & ' */
    G(1,2,2,2,1), G(4,2,2,2,4), G(0,5,2,5,0), G(0,2,7,2,0),  /*  ( ) * + */
    G(0,0,0,2,4), G(0,0,7,0,0), G(0,0,0,0,2), G(1,1,2,4,4),  /*  , - . / */
    G(7,5,5,5,7), G(2,6,2,2,7), G(6,1,2,4,7), G(6,1,2,1,6),  /*  0 1 2 3 */
    G(5,5,7,1,1), G(7,4,6,1,6), G(3,4,7,5,7), G(7,1,2,2,2),  /*  4 5 6 7 */
    G(7,5,7,5,7), G(7,5,7,1,6), G(0,2,0,2,0), G(0,2,0,2,4),  /*  8 9 : ; */
    G(1,2,4,2,1), G(0,7,0,7,0), G(4,2,1,2,4), G(6,1,2,0,2),  /*  < = > ? */
    G(0,0,0,0,0), G(2,5,7,5,5), G(6,5,6,5,6), G(3,4,4,4,3),  /*  @ A B C */
    G(6,5,5,5,6), G(7,4,6,4,7), G(7,4,6,4,4), G(3,4,5,5,3),  /*  D E F G */
    G(5,5,7,5,5), G(7,2,2,2,7), G(1,1,1,5,2), G(5,5,6,5,5),  /*  H I J K */
    G(4,4,4,4,7), G(5,7,7,5,5), G(6,5,5,5,5), G(2,5,5,5,2),  /*  L M N O */
    G(6,5,6,4,4), G(2,5,5,6,3), G(6,5,6,5,5), G(3,4,2,1,6),  /*  P Q R S */
    G(7,2,2,2,2), G(5,5,5,5,7), G(5,5,5,5,2), G(5,5,7,7,5),  /*  T U V W */
    G(5,5,2,5,5), G(5,5,2,2,2), G(7,1,2,4,7), G(3,2,2,2,3),  /*  X Y Z [ */
    G(4,4,2,1,1), G(6,2,2,2,6), G(2,5,0,0,0), G(0,0,0,0,7),  /*  \ ] ^ _ */
};

static void glyph(int x, int y, int ch, u8 color, int scale)
{
    if (ch >= 'a' && ch <= 'z') ch -= 32;
    if (ch < 32 || ch > 95) ch = '?';
    u16 g = FONT[ch - 32];
    for (int row = 0; row < 5; row++) {
        int bits = (g >> (12 - row * 3)) & 7;
        for (int col = 0; col < 3; col++) {
            if (!(bits & (4 >> col))) continue;
            if (scale == 1) gfx_pixel(x + col, y + row, color);
            else gfx_rect(x + col * scale, y + row * scale, scale, scale, color);
        }
    }
}

int gfx_text_w(const char *s, int scale)
{
    int n = 0;
    while (s[n]) n++;
    return n > 0 ? n * 4 * scale - scale : 0;
}

void gfx_text(int x, int y, const char *s, u8 color, int scale)
{
    for (; *s; s++, x += 4 * scale) glyph(x, y, *s, color, scale);
}

void gfx_text_shadow(int x, int y, const char *s, u8 color, int scale)
{
    gfx_text(x + 1, y + 1, s, C_BLACK, scale);
    gfx_text(x, y, s, color, scale);
}

void gfx_text_center(int cx, int y, const char *s, u8 color, int scale)
{
    gfx_text_shadow(cx - gfx_text_w(s, scale) / 2, y, s, color, scale);
}

void gfx_number(int x, int y, int value, u8 color, int scale)
{
    char buf[12];
    int n = 0;
    if (value < 0) value = 0;
    do { buf[n++] = (char)('0' + value % 10); value /= 10; } while (value && n < 11);
    char out[12];
    for (int i = 0; i < n; i++) out[i] = buf[n - 1 - i];
    out[n] = 0;
    gfx_text_shadow(x, y, out, color, scale);
}

/* ------------------------------------------------------------------ */
/* Icons                                                               */
/* ------------------------------------------------------------------ */

void gfx_item(int x, int y, u8 id)
{
    if (id == 0 || x < 0 || y < 0 || x > SCREEN_W - 16 || y > SCREEN_H - 16) return;

    if (id < B_COUNT) {
        blit_cube((u8 *)s_dst + y * SCREEN_W + x, SCREEN_W, g_cube_ui[id]);
        return;
    }
    const IconDef *ic = icon_for(id);
    if (!ic) return;
    x &= ~1;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            u8 col = icon_color(ic, ic->rows[r][c]);
            if (!col) continue;
            u16 v = (u16)(col * 0x0101);
            int px = x + c * 2, py = y + r * 2;
            s_dst[(py * SCREEN_W + px) >> 1] = v;
            s_dst[((py + 1) * SCREEN_W + px) >> 1] = v;
        }
    }
}

void gfx_mask7(int x, int y, const u8 *rows6, int cols, u8 color)
{
    for (int r = 0; r < 6; r++)
        for (int c = 0; c < 7 && c < cols; c++)
            if (rows6[r] & (0x80 >> c)) gfx_pixel(x + c, y + r, color);
}
