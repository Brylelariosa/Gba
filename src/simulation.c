#include "simulation.h"
#include <string.h>

SimState g_sim __attribute__((section(".ewram")));

/* ── Fast LCG RNG ────────────────────────────────── */
static u32 s_seed = 0xDEADBEEF;
static inline u32 lcg(void) {
    s_seed = s_seed * 1664525u + 1013904223u;
    return s_seed;
}

/* ── Init ────────────────────────────────────────── */
void sim_init(void) {
    memset(&g_sim, 0, sizeof(g_sim));
    g_sim.speed      = 1;
    g_sim.wrap       = 1;
    g_sim.type       = AUTO_CONWAY;
    g_sim.birth_mask = (1 << 3);
    g_sim.surv_mask  = (1 << 2) | (1 << 3);
    g_sim.cyc_states = 16;
    g_sim.cyc_thresh = 1;
    g_sim.ant_x      = GRID_W / 2;
    g_sim.ant_y      = GRID_H / 2;
    g_sim.elem_rule  = 110;
    g_sim.cur_x      = GRID_W / 2;
    g_sim.cur_y      = GRID_H / 2;
    g_sim.draw_state = 1;
}

void sim_clear(void) {
    memset(g_sim.cur, 0, GRID_SIZE);
    memset(g_sim.nxt, 0, GRID_SIZE);
    g_sim.generation = 0;
    g_sim.population = 0;
    g_sim.births     = 0;
    g_sim.deaths     = 0;
    g_sim.elem_row   = 0;
    g_sim.ant_x      = GRID_W / 2;
    g_sim.ant_y      = GRID_H / 2;
    g_sim.ant_dir    = 0;
    if (g_sim.type == AUTO_ELEMENTARY)
        g_sim.cur[0][GRID_W / 2] = 1;
}

void sim_randomize(int density) {
    sim_clear();
    for (int y = 0; y < GRID_H; y++) {
        for (int x = 0; x < GRID_W; x++) {
            u32 r = lcg() & 0xFF;
            if ((int)r < density * 256 / 100) {
                if (g_sim.type == AUTO_WIREWORLD)
                    g_sim.cur[y][x] = WW_WIRE;
                else if (g_sim.type == AUTO_CYCLIC)
                    g_sim.cur[y][x] = (u8)(lcg() % g_sim.cyc_states);
                else
                    g_sim.cur[y][x] = 1;
            }
        }
    }
}

void sim_set_cell(int x, int y, u8 v) {
    if (x >= 0 && x < GRID_W && y >= 0 && y < GRID_H)
        g_sim.cur[y][x] = v;
}

u8 sim_get_cell(int x, int y) {
    if (x < 0 || x >= GRID_W || y < 0 || y >= GRID_H) return 0;
    return g_sim.cur[y][x];
}

void sim_place_pattern(const u8 *data, int pw, int ph, int ox, int oy) {
    for (int py = 0; py < ph; py++)
        for (int px = 0; px < pw; px++) {
            int gx = ox + px, gy = oy + py;
            if (gx >= 0 && gx < GRID_W && gy >= 0 && gy < GRID_H)
                g_sim.cur[gy][gx] = data[py * pw + px];
        }
}

void sim_set_automata(AutomataType t) {
    g_sim.type = t;
    switch (t) {
        case AUTO_CONWAY:
            g_sim.birth_mask = (1<<3);
            g_sim.surv_mask  = (1<<2)|(1<<3);
            break;
        case AUTO_HIGHLIFE:
            g_sim.birth_mask = (1<<3)|(1<<6);
            g_sim.surv_mask  = (1<<2)|(1<<3);
            break;
        case AUTO_SEEDS:
            g_sim.birth_mask = (1<<2);
            g_sim.surv_mask  = 0;
            break;
        case AUTO_DAYNIGHT:
            g_sim.birth_mask = (1<<3)|(1<<6)|(1<<7)|(1<<8);
            g_sim.surv_mask  = (1<<3)|(1<<4)|(1<<6)|(1<<7)|(1<<8);
            break;
        case AUTO_MAZE:
            g_sim.birth_mask = (1<<3);
            g_sim.surv_mask  = (1<<1)|(1<<2)|(1<<3)|(1<<4)|(1<<5);
            break;
        case AUTO_ANNEAL:
            g_sim.birth_mask = (1<<4)|(1<<6)|(1<<7)|(1<<8);
            g_sim.surv_mask  = (1<<3)|(1<<5)|(1<<6)|(1<<7)|(1<<8);
            break;
        default: break;
    }
    sim_clear();
}

/* ── Neighbor helpers ────────────────────────────── */
static int count_neigh(int x, int y) {
    int c = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (!dx && !dy) continue;
            int nx, ny;
            if (g_sim.wrap) {
                nx = (x + dx + GRID_W) % GRID_W;
                ny = (y + dy + GRID_H) % GRID_H;
            } else {
                nx = x + dx; ny = y + dy;
                if (nx < 0 || nx >= GRID_W || ny < 0 || ny >= GRID_H) continue;
            }
            if (g_sim.cur[ny][nx]) c++;
        }
    }
    return c;
}

static int count_state(int x, int y, u8 state) {
    int c = 0;
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            if (!dx && !dy) continue;
            int nx, ny;
            if (g_sim.wrap) {
                nx = (x + dx + GRID_W) % GRID_W;
                ny = (y + dy + GRID_H) % GRID_H;
            } else {
                nx = x + dx; ny = y + dy;
                if (nx < 0 || nx >= GRID_W || ny < 0 || ny >= GRID_H) continue;
            }
            if (g_sim.cur[ny][nx] == state) c++;
        }
    }
    return c;
}

/* ── Step ────────────────────────────────────────── */
void sim_step(void) {
    u32 births = 0, deaths = 0, pop = 0;

    switch (g_sim.type) {

        /* Life-like rules (Conway, HighLife, Seeds, Day&Night, Maze, Anneal, Custom) */
        case AUTO_CONWAY: case AUTO_HIGHLIFE: case AUTO_SEEDS:
        case AUTO_DAYNIGHT: case AUTO_MAZE: case AUTO_ANNEAL: case AUTO_CUSTOM:
            for (int y = 0; y < GRID_H; y++) {
                for (int x = 0; x < GRID_W; x++) {
                    int n = count_neigh(x, y);
                    u8 c = g_sim.cur[y][x];
                    u8 v;
                    if (c) {
                        v = (g_sim.surv_mask >> n) & 1;
                        if (!v) deaths++;
                    } else {
                        v = (g_sim.birth_mask >> n) & 1;
                        if (v) births++;
                    }
                    g_sim.nxt[y][x] = v;
                    if (v) pop++;
                }
            }
            break;

        case AUTO_BRIANS_BRAIN:
            for (int y = 0; y < GRID_H; y++) {
                for (int x = 0; x < GRID_W; x++) {
                    u8 c = g_sim.cur[y][x], v;
                    if (c == BB_ON)         { v = BB_DYING; deaths++; }
                    else if (c == BB_DYING) { v = BB_OFF; }
                    else {
                        int on = count_state(x, y, BB_ON);
                        v = (on == 2) ? BB_ON : BB_OFF;
                        if (v) births++;
                    }
                    g_sim.nxt[y][x] = v;
                    if (v == BB_ON) pop++;
                }
            }
            break;

        case AUTO_WIREWORLD:
            for (int y = 0; y < GRID_H; y++) {
                for (int x = 0; x < GRID_W; x++) {
                    u8 c = g_sim.cur[y][x], v;
                    if (c == WW_EMPTY)    v = WW_EMPTY;
                    else if (c == WW_HEAD) v = WW_TAIL;
                    else if (c == WW_TAIL) v = WW_WIRE;
                    else {
                        int h = count_state(x, y, WW_HEAD);
                        v = (h == 1 || h == 2) ? WW_HEAD : WW_WIRE;
                    }
                    g_sim.nxt[y][x] = v;
                    if (v) pop++;
                }
            }
            break;

        case AUTO_LANGTONS_ANT: {
            memcpy(g_sim.nxt, g_sim.cur, GRID_SIZE);
            int ax = g_sim.ant_x, ay = g_sim.ant_y;
            u8 cell = g_sim.cur[ay][ax];
            g_sim.nxt[ay][ax] = cell ? 0 : 1;
            /* Turn: white→right, black→left */
            g_sim.ant_dir = cell
                ? (g_sim.ant_dir + 3) % 4
                : (g_sim.ant_dir + 1) % 4;
            const int DX[] = {0,1,0,-1};
            const int DY[] = {-1,0,1,0};
            ax += DX[g_sim.ant_dir];
            ay += DY[g_sim.ant_dir];
            if (g_sim.wrap) {
                ax = (ax + GRID_W) % GRID_W;
                ay = (ay + GRID_H) % GRID_H;
            } else {
                if (ax < 0) ax = 0; if (ax >= GRID_W) ax = GRID_W - 1;
                if (ay < 0) ay = 0; if (ay >= GRID_H) ay = GRID_H - 1;
            }
            g_sim.ant_x = ax; g_sim.ant_y = ay;
            for (int y = 0; y < GRID_H; y++)
                for (int x = 0; x < GRID_W; x++)
                    if (g_sim.nxt[y][x]) pop++;
            break;
        }

        case AUTO_CYCLIC:
            for (int y = 0; y < GRID_H; y++) {
                for (int x = 0; x < GRID_W; x++) {
                    u8 c  = g_sim.cur[y][x];
                    u8 ns = (u8)((c + 1) % g_sim.cyc_states);
                    int cnt = count_state(x, y, ns);
                    g_sim.nxt[y][x] = (cnt >= g_sim.cyc_thresh) ? ns : c;
                    if (g_sim.nxt[y][x]) pop++;
                }
            }
            break;

        case AUTO_ELEMENTARY: {
            /* Copy current rows to nxt */
            memcpy(g_sim.nxt, g_sim.cur, GRID_SIZE);
            int row = (g_sim.elem_row < GRID_H - 1)
                      ? g_sim.elem_row
                      : GRID_H - 2;
            int nrow = row + 1;
            if (g_sim.elem_row >= GRID_H - 1) {
                /* Scroll up */
                memmove(g_sim.nxt[0], g_sim.nxt[1], (GRID_H - 1) * GRID_W);
                row  = GRID_H - 2;
                nrow = GRID_H - 1;
            }
            memset(g_sim.nxt[nrow], 0, GRID_W);
            for (int x = 0; x < GRID_W; x++) {
                int xl = (x - 1 + GRID_W) % GRID_W;
                int xr = (x + 1) % GRID_W;
                u8 pat = (u8)((g_sim.cur[row][xl] << 2) |
                              (g_sim.cur[row][x]  << 1) |
                               g_sim.cur[row][xr]);
                g_sim.nxt[nrow][x] = (g_sim.elem_rule >> pat) & 1;
            }
            if (g_sim.elem_row < GRID_H - 1) g_sim.elem_row++;
            for (int y = 0; y < GRID_H; y++)
                for (int x = 0; x < GRID_W; x++)
                    if (g_sim.nxt[y][x]) pop++;
            break;
        }

        default: break;
    }

    memcpy(g_sim.cur, g_sim.nxt, GRID_SIZE);
    g_sim.generation++;
    g_sim.population = pop;
    g_sim.births     = births;
    g_sim.deaths     = deaths;
}