#pragma once
#include <gba_types.h>
#include <gba_video.h>

/* ── Screen layout ─────────────────────────────────────────────────────────
   15 cols × 14 px = 210   right panel: 30 px  → total 240
   10 rows × 14 px = 140   bottom bar:  20 px  → total 160           */
#define CELL   14
#define GCOLS  15
#define GROWS  10
#define MAPW   (GCOLS * CELL)
#define MAPH   (GROWS * CELL)
#define PANX   MAPW
#define PANW   30

/* ── Tile types ─────────────────────────────────────────────────────────── */
#define T_GRASS  0
#define T_PATH   1
#define T_TOWER  2

/* ── Game states ────────────────────────────────────────────────────────── */
#define GS_PLAY  0
#define GS_OVER  1
#define GS_WIN   2

/* ── Limits ─────────────────────────────────────────────────────────────── */
#define MAXE  14
#define MAXT  40
#define MAXB  20
#define NWP    6
#define NTYPE  3
#define NWAVE  5

/* ── Back-buffer in EWRAM ───────────────────────────────────────────────────
   Draw everything here, then DMA the whole thing to VRAM in one shot
   during VBlank – zero tearing, zero flicker.                              */
#define BACKBUF_ADDR  0x02000000u
#define BUF  ((u16*)BACKBUF_ADDR)   /* 240×160 × 2 bytes = 76 800 B in EWRAM */
#define SCREEN_PIXELS (240 * 160)

/* ── Palette (15-bit BGR) ───────────────────────────────────────────────── */
#define C_GRASS  RGB15( 4,14, 4)
#define C_PATH   RGB15(17,13, 7)
#define C_SPAWN  RGB15( 0,20, 8)
#define C_BASE   RGB15(20, 8, 0)
#define C_PANEL  RGB15( 2, 2, 7)
#define C_BLACK  RGB15( 0, 0, 0)
#define C_WHITE  RGB15(31,31,31)
#define C_RED    RGB15(28, 3, 3)
#define C_GOLD   RGB15(28,22, 0)
#define C_CYAN   RGB15( 0,24,24)
#define C_BULLET RGB15(31,28, 0)
#define C_HPBAR  RGB15( 3,25, 3)
#define C_HPBG   RGB15(12, 2, 2)

/* ── Tower definition ───────────────────────────────────────────────────── */
typedef struct {
    u16 cost;
    u8  dmg;
    u8  range;
    u8  cd;
    u16 body;
    u16 tip;
} TDef;

/* ── Entity structs ─────────────────────────────────────────────────────── */
typedef struct {
    s16 x, y;
    u8  wp;
    s16 hp, mhp;
    u8  spd;
    u8  alive;
} Enemy;

typedef struct {
    u8  col, row;
    u8  type;
    u8  cd;
    u8  on;
} Tower;

typedef struct {
    s16 x, y;
    u8  target;
    u8  dmg;
    u8  spd;
    u8  on;
} Bullet;

/* ── Globals ────────────────────────────────────────────────────────────── */
extern const TDef  TDEFS[NTYPE];
extern const u8    WPX[NWP];
extern const u8    WPY[NWP];

extern u8     gmap[GROWS][GCOLS];
extern Enemy  gens[MAXE];
extern Tower  gtwrs[MAXT];
extern Bullet gblts[MAXB];
extern int    gntwrs;
extern int    glives, ggold, gscore, gwave;
extern int    gcol, grow;
extern int    gsel;
extern int    gstate;

/* ── API ────────────────────────────────────────────────────────────────── */
void gameInit(void);
void gameUpdate(void);
void gameDraw(void);
