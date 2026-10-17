#include "game.h"
#include "assets.h"

static u32 rng_state = 0x1234ABCDu;

void rnd_seed(u32 s) { rng_state = s * 2654435761u + 12345u; }

u32 rnd(void) {
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state >> 8;
}

int rnd_n(int n) {
    if (n <= 1) return 0;
    return (int)(((rnd() & 0xFFFF) * (u32)n) >> 16);
}

int isin(int a) { return sin256[a & 255]; }
int icos(int a) { return sin256[(a + 64) & 255]; }
int iabs(int v) { return v < 0 ? -v : v; }
int imin(int a, int b) { return a < b ? a : b; }
int imax(int a, int b) { return a > b ? a : b; }
int iclamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

/* angle units: 256 = full circle, 0 = up, 64 = right, 128 = down, 192 = left (clockwise) */
int dir_angle(int dir) {
    static const u8 t[4] = { 128, 0, 192, 64 };
    return t[dir & 3];
}

static int atan_oct(int mn, int mx) {
    if (mx <= 0) return 0;
    int t = (mn << 8) / mx;
    return (32 * t + ((11 * t * (256 - t)) >> 8)) >> 8;
}

int vec_angle(int dx, int dy) {
    if (dx == 0 && dy == 0) return 128;
    int ax = iabs(dx), ay = iabs(dy), base;
    if (ay >= ax) base = atan_oct(ax, ay);
    else base = 64 - atan_oct(ay, ax);
    if (dx >= 0) return (dy <= 0) ? base : 128 - base;
    return (dy >= 0) ? 128 + base : (256 - base) & 255;
}

int idist(int dx, int dy) {
    int ax = iabs(dx), ay = iabs(dy);
    return ax + ay - (imin(ax, ay) >> 1);
}

int scale_div(int v, int num, int shift) { return (v * num) >> shift; }
