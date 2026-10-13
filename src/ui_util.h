#ifndef UI_UTIL_H
#define UI_UTIL_H
/* small text/cursor helpers shared by the screen modules (screens.c, menu.c) */
#include "game.h"
#include "assets.h"

#define C_WHITE 1
#define C_YELLOW 3
#define C_RED 4
#define C_GREEN 5
#define C_CYAN 6
#define C_GRAY 7
#define C_ORANGE 12
#define C_PURPLE 13

static inline void cursor_at(int x, int y) {
    spr_ui16(x, y, OT_CURSOR + ((frame_count >> 4) & 1) * 4, PB_UI);
}

static inline void put_centered(TextArea *t, int y, const char *s, int col) {
    int w = ta_width(s);
    ta_print(t, (t->w * 8 - w) / 2, y, s, col);
}

static inline void put_right(TextArea *t, int y, int rx, const char *s, int col) {
    ta_print(t, rx - ta_width(s), y, s, col);
}

static inline void num_str(char *buf, int v) {
    char tmp[12];
    int m = 0, n = 0;
    if (v < 0) { buf[n++] = '-'; v = -v; }
    do { tmp[m++] = (char)('0' + v % 10); v /= 10; } while (v && m < 10);
    while (m) buf[n++] = tmp[--m];
    buf[n] = 0;
}

static inline int cat_str(char *d, int n, const char *s) {
    while (*s) d[n++] = *s++;
    d[n] = 0;
    return n;
}

static inline int cat_num(char *d, int n, int v) {
    char b[12];
    num_str(b, v);
    return cat_str(d, n, b);
}

#endif
