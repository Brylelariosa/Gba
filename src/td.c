#include "td.h"
#include <gba_input.h>
#include <string.h>

/* ═══════════════════════════════════════════════════════════════════════════
   DATA
   ═══════════════════════════════════════════════════════════════════════════ */

const TDef TDEFS[NTYPE] = {
    {  50,   5,   3,    35,  RGB15(10,10,10),  RGB15(20,20,20) },  /* Arrow  */
    { 100,  18,   4,    80,  RGB15(18, 4, 4),  RGB15(28, 8, 8) },  /* Cannon */
    {  75,   7,   5,    20,  RGB15( 4, 8,20),  RGB15( 8,16,28) },  /* Magic  */
};

const u8 WPX[NWP] = { 0,  3,  3,  8,  8, 14 };
const u8 WPY[NWP] = { 4,  4,  1,  1,  7,  7 };

/* ═══════════════════════════════════════════════════════════════════════════
   GLOBALS
   ═══════════════════════════════════════════════════════════════════════════ */

u8     gmap[GROWS][GCOLS];
Enemy  gens[MAXE];
Tower  gtwrs[MAXT];
Bullet gblts[MAXB];
int    gntwrs;
int    glives, ggold, gscore, gwave;
int    gcol, grow, gsel, gstate;

static int sspawn;
static int snspawned;
static int snthiswave;
static int smove;

/* ═══════════════════════════════════════════════════════════════════════════
   DRAW HELPERS  –  write to BUF (EWRAM), never directly to VRAM
   ═══════════════════════════════════════════════════════════════════════════ */

static inline void px(int x, int y, u16 c) {
    if ((unsigned)x < 240u && (unsigned)y < 160u)
        BUF[y * 240 + x] = c;
}

static void rect(int x, int y, int w, int h, u16 c) {
    /* Clamp to screen bounds first */
    int x1 = x < 0 ? 0 : x;
    int y1 = y < 0 ? 0 : y;
    int x2 = (x + w) > 240 ? 240 : (x + w);
    int y2 = (y + h) > 160 ? 160 : (y + h);
    for (int r = y1; r < y2; r++) {
        u16 *row = BUF + r * 240;
        for (int i = x1; i < x2; i++)
            row[i] = c;
    }
}

static void border(int x, int y, int w, int h, u16 c) {
    for (int i = 0; i < w; i++) { px(x+i, y,     c); px(x+i, y+h-1, c); }
    for (int j = 0; j < h; j++) { px(x,   y+j,   c); px(x+w-1, y+j, c); }
}

/* 4×6 pixel digit font */
static const u8 DFONT[10][6] = {
    {0x6,0x9,0x9,0x9,0x9,0x6},
    {0x2,0x6,0x2,0x2,0x2,0x7},
    {0x6,0x9,0x1,0x2,0x4,0xF},
    {0xE,0x1,0x6,0x1,0x1,0xE},
    {0x9,0x9,0xF,0x1,0x1,0x1},
    {0xF,0x8,0xE,0x1,0x1,0xE},
    {0x6,0x8,0xE,0x9,0x9,0x6},
    {0xF,0x1,0x2,0x4,0x4,0x4},
    {0x6,0x9,0x6,0x9,0x9,0x6},
    {0x6,0x9,0x7,0x1,0x1,0x6},
};

static void drawDigit(int x, int y, int d, u16 fg, u16 bg) {
    for (int r = 0; r < 6; r++) {
        u8 bits = DFONT[d][r];
        for (int c = 0; c < 4; c++)
            px(x+c, y+r, (bits >> (3-c)) & 1 ? fg : bg);
    }
}

static void drawNum(int x, int y, int n, u16 fg, u16 bg) {
    u8 buf[6]; int len = 0;
    if (n < 0) n = 0;
    if (n == 0) { drawDigit(x, y, 0, fg, bg); return; }
    while (n > 0 && len < 6) { buf[len++] = (u8)(n % 10); n /= 10; }
    for (int i = len-1; i >= 0; i--, x += 5)
        drawDigit(x, y, buf[i], fg, bg);
}

/* ═══════════════════════════════════════════════════════════════════════════
   GAME LOGIC  (unchanged from original)
   ═══════════════════════════════════════════════════════════════════════════ */

static void buildMap(void) {
    memset(gmap, T_GRASS, sizeof(gmap));
    for (int i = 0; i < NWP-1; i++) {
        int c0 = WPX[i], r0 = WPY[i], c1 = WPX[i+1], r1 = WPY[i+1];
        if (r0 == r1) {
            int dc = (c1 > c0) ? 1 : -1;
            for (int c = c0; c != c1 + dc; c += dc)
                gmap[r0][c] = T_PATH;
        } else {
            int dr = (r1 > r0) ? 1 : -1;
            for (int r = r0; r != r1 + dr; r += dr)
                gmap[r][c0] = T_PATH;
        }
    }
}

static int waveEnemyCount(int w) { return 5 + w * 3; }
static int waveEnemyHP(int w)    { return 15 + w * 12; }
static int waveEnemySpeed(int w) { return w >= 3 ? 2 : 1; }

void gameInit(void) {
    memset(gens,  0, sizeof(gens));
    memset(gtwrs, 0, sizeof(gtwrs));
    memset(gblts, 0, sizeof(gblts));
    buildMap();
    glives = 20; ggold = 150;
    gscore = 0;  gwave = 1;
    gcol   = 7;  grow  = 4;
    gsel   = 0;  gntwrs = 0;
    gstate = GS_PLAY;
    snthiswave = waveEnemyCount(gwave);
    snspawned  = 0;
    sspawn     = 90;
    smove      = 0;
}

static void spawnEnemy(void) {
    for (int i = 0; i < MAXE; i++) {
        if (gens[i].alive) continue;
        Enemy *e = &gens[i];
        e->alive = 1; e->wp = 1;
        e->x = (s16)(WPX[0]*CELL + CELL/2);
        e->y = (s16)(WPY[0]*CELL + CELL/2);
        e->mhp = (s16)waveEnemyHP(gwave);
        e->hp  = e->mhp;
        e->spd = (u8)waveEnemySpeed(gwave);
        return;
    }
}

static int iabs(int v)        { return v < 0 ? -v : v; }
static int isign(int v)       { return v > 0 ? 1 : (v < 0 ? -1 : 0); }
static int imax(int a, int b) { return a > b ? a : b; }

static void updateEnemies(void) {
    for (int i = 0; i < MAXE; i++) {
        Enemy *e = &gens[i];
        if (!e->alive) continue;
        int tx = WPX[e->wp]*CELL + CELL/2, ty = WPY[e->wp]*CELL + CELL/2;
        int dx = tx - e->x, dy = ty - e->y;
        int dist = iabs(dx) + iabs(dy);
        if (dist <= (int)e->spd) {
            e->x = (s16)tx; e->y = (s16)ty;
            e->wp++;
            if (e->wp >= NWP) {
                e->alive = 0;
                glives--;
                if (glives <= 0) { glives = 0; gstate = GS_OVER; }
            }
        } else {
            if (iabs(dx) >= iabs(dy)) e->x += (s16)(isign(dx)*e->spd);
            else                       e->y += (s16)(isign(dy)*e->spd);
        }
    }
}

static void spawnBullet(int x, int y, u8 target, u8 dmg, u8 spd) {
    for (int i = 0; i < MAXB; i++) {
        if (gblts[i].on) continue;
        gblts[i].on = 1; gblts[i].x = (s16)x; gblts[i].y = (s16)y;
        gblts[i].target = target; gblts[i].dmg = dmg; gblts[i].spd = spd;
        return;
    }
}

static void updateTowers(void) {
    for (int i = 0; i < gntwrs; i++) {
        Tower *t = &gtwrs[i];
        if (!t->on) continue;
        if (t->cd > 0) { t->cd--; continue; }
        const TDef *td = &TDEFS[t->type];
        int tx = t->col*CELL + CELL/2, ty = t->row*CELL + CELL/2;
        int rng2 = (td->range*CELL) * (td->range*CELL);
        int best = -1, bestD2 = rng2 + 1;
        for (int j = 0; j < MAXE; j++) {
            if (!gens[j].alive) continue;
            int ddx = gens[j].x - tx, ddy = gens[j].y - ty;
            int d2 = ddx*ddx + ddy*ddy;
            if (d2 < bestD2) { bestD2 = d2; best = j; }
        }
        if (best >= 0) {
            spawnBullet(tx, ty, (u8)best, td->dmg, (u8)imax(3,(int)td->range));
            t->cd = td->cd;
        }
    }
}

static void updateBullets(void) {
    for (int i = 0; i < MAXB; i++) {
        Bullet *b = &gblts[i];
        if (!b->on) continue;
        Enemy *e = &gens[b->target];
        if (!e->alive) { b->on = 0; continue; }
        int dx = e->x - b->x, dy = e->y - b->y;
        if (iabs(dx) + iabs(dy) <= (int)b->spd + 2) {
            e->hp -= b->dmg;
            if (e->hp <= 0) { e->alive = 0; gscore += 10; ggold += 10; }
            b->on = 0;
        } else {
            if (iabs(dx) >= iabs(dy)) b->x += (s16)(isign(dx)*b->spd);
            else                       b->y += (s16)(isign(dy)*b->spd);
        }
    }
}

static void handleInput(void) {
    u16 held = keysHeld(), down = keysDown();
    if (smove > 0) smove--;
    if (smove == 0) {
        int moved = 0;
        if ((held & KEY_LEFT)  && gcol > 0)       { gcol--; moved=1; }
        if ((held & KEY_RIGHT) && gcol < GCOLS-1)  { gcol++; moved=1; }
        if ((held & KEY_UP)    && grow > 0)        { grow--; moved=1; }
        if ((held & KEY_DOWN)  && grow < GROWS-1)  { grow++; moved=1; }
        if (moved) smove = 7;
    }
    if (down & KEY_L) gsel = (gsel + NTYPE - 1) % NTYPE;
    if (down & KEY_R) gsel = (gsel + 1) % NTYPE;
    if ((down & KEY_A) && gmap[grow][gcol] == T_GRASS && gntwrs < MAXT
            && ggold >= TDEFS[gsel].cost) {
        ggold -= TDEFS[gsel].cost;
        gmap[grow][gcol] = T_TOWER;
        gtwrs[gntwrs].col  = (u8)gcol; gtwrs[gntwrs].row  = (u8)grow;
        gtwrs[gntwrs].type = (u8)gsel; gtwrs[gntwrs].cd   = 0;
        gtwrs[gntwrs].on   = 1;
        gntwrs++;
    }
    if ((down & KEY_B) && gmap[grow][gcol] == T_TOWER) {
        for (int i = 0; i < gntwrs; i++) {
            Tower *t = &gtwrs[i];
            if (t->on && t->col == gcol && t->row == grow) {
                ggold += TDEFS[t->type].cost / 2;
                t->on = 0; gmap[grow][gcol] = T_GRASS;
                break;
            }
        }
    }
}

static int allDead(void) {
    for (int i = 0; i < MAXE; i++) if (gens[i].alive) return 0;
    return 1;
}

void gameUpdate(void) {
    if (gstate != GS_PLAY) {
        if (keysDown() & KEY_START) gameInit();
        return;
    }
    handleInput();
    if (sspawn > 0) sspawn--;
    if (sspawn == 0 && snspawned < snthiswave) {
        spawnEnemy(); snspawned++; sspawn = 80;
    }
    if (snspawned >= snthiswave && allDead()) {
        gwave++;
        if (gwave > NWAVE) { gstate = GS_WIN; return; }
        ggold += 40;
        snthiswave = waveEnemyCount(gwave);
        snspawned = 0; sspawn = 150;
    }
    updateEnemies();
    updateTowers();
    updateBullets();
}

/* ═══════════════════════════════════════════════════════════════════════════
   DRAW  –  all writes target BUF (EWRAM), flushed to VRAM in main.c
   ═══════════════════════════════════════════════════════════════════════════ */

static void drawMap(void) {
    for (int r = 0; r < GROWS; r++) {
        for (int c = 0; c < GCOLS; c++) {
            int x = c*CELL, y = r*CELL;
            u16 col;
            if (c == WPX[0]       && r == WPY[0])       col = C_SPAWN;
            else if (c == WPX[NWP-1] && r == WPY[NWP-1]) col = C_BASE;
            else switch (gmap[r][c]) {
                case T_PATH: col = C_PATH;  break;
                default:     col = C_GRASS; break;
            }
            rect(x, y, CELL, CELL, col);
        }
    }
    /* Subtle 1-px grid lines on grass cells */
    for (int r = 0; r < GROWS; r++)
        for (int c = 0; c < GCOLS; c++)
            if (gmap[r][c] == T_GRASS)
                px(c*CELL, r*CELL, RGB15(3,12,3));
}

static void drawTowers(void) {
    for (int i = 0; i < gntwrs; i++) {
        Tower *t = &gtwrs[i];
        if (!t->on) continue;
        const TDef *td = &TDEFS[t->type];
        int x = t->col*CELL, y = t->row*CELL;
        int cx = x + CELL/2, cy = y + CELL/2;
        rect(x+2, y+2, CELL-4, CELL-4, td->body);
        rect(cx-1, cy-1, 3, 3, td->tip);
    }
}

static void drawEnemies(void) {
    for (int i = 0; i < MAXE; i++) {
        Enemy *e = &gens[i];
        if (!e->alive) continue;
        int x = e->x - 4, y = e->y - 4;
        rect(x, y, 8, 8, C_RED);
        px(x+2, y+2, C_WHITE); px(x+5, y+2, C_WHITE);
        /* HP bar */
        int bw = (int)(8 * e->hp / e->mhp);
        rect(x, y-3, 8, 2, C_HPBG);
        if (bw > 0) rect(x, y-3, bw, 2, C_HPBAR);
    }
}

static void drawBullets(void) {
    for (int i = 0; i < MAXB; i++) {
        Bullet *b = &gblts[i];
        if (!b->on) continue;
        rect(b->x-1, b->y-1, 3, 3, C_BULLET);
    }
}

static void drawCursor(void) {
    int x = gcol*CELL, y = grow*CELL;
    u16 col;
    if (gmap[grow][gcol] == T_GRASS)
        col = (ggold >= TDEFS[gsel].cost) ? C_WHITE : C_RED;
    else
        col = RGB15(12,12,12);
    border(x, y, CELL, CELL, col);
}

static void drawPanel(void) {
    rect(PANX, 0, PANW, 160, C_PANEL);
    rect(PANX+3,  2, 5, 5, C_RED);
    drawNum(PANX+2,  9, glives, C_RED,   C_PANEL);
    rect(PANX+3, 22, 5, 5, C_GOLD);
    drawNum(PANX+2, 29, ggold,  C_GOLD,  C_PANEL);
    rect(PANX+3, 42, 5, 5, C_WHITE);
    drawNum(PANX+2, 49, gscore, C_WHITE, C_PANEL);
    rect(PANX+3, 62, 5, 5, C_CYAN);
    drawNum(PANX+2, 69, gwave,  C_CYAN,  C_PANEL);
    for (int i = 0; i < NTYPE; i++) {
        int ty = 84 + i*25;
        u16 bord = (i == gsel) ? C_WHITE : RGB15(6,6,10);
        rect(PANX+1, ty, 28, 22, C_BLACK);
        border(PANX+1, ty, 28, 22, bord);
        rect(PANX+9,  ty+4, 12, 12, TDEFS[i].body);
        rect(PANX+13, ty+7,  4,  4, TDEFS[i].tip);
        drawNum(PANX+2, ty+16, (int)TDEFS[i].cost, C_GOLD, C_BLACK);
    }
}

static void drawOverlay(u16 bg, int score) {
    rect(55, 58, 100, 44, C_BLACK);
    rect(57, 60,  96, 40, bg);
    rect(65, 68,  30,  8, C_BLACK);
    drawNum(66, 69, score, C_WHITE, C_BLACK);
    border(65, 84, 50, 6, C_WHITE);
}

void gameDraw(void) {
    drawMap();
    drawTowers();
    drawEnemies();
    drawBullets();
    drawCursor();
    drawPanel();
    if (gstate == GS_OVER)
        drawOverlay(C_RED,          gscore);
    else if (gstate == GS_WIN)
        drawOverlay(RGB15(0,20,0),  gscore);
}
