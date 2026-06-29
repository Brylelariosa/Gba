#ifndef SIMULATION_H
#define SIMULATION_H

#include <gba.h>

/* ── Grid dimensions ─────────────────────────────── */
#define GRID_W   120
#define GRID_H    70
#define GRID_SIZE (GRID_W * GRID_H)

/* ── Screen layout ───────────────────────────────── */
#define SCREEN_W  240
#define SCREEN_H  160
#define SIM_H     140   /* 70 rows × 2 px */
#define STATUS_Y  140
#define STATUS_H   20
#define CELL_PX     2   /* pixels per cell */

/* ── Automata types ──────────────────────────────── */
typedef enum {
    AUTO_CONWAY = 0,
    AUTO_HIGHLIFE,
    AUTO_SEEDS,
    AUTO_DAYNIGHT,
    AUTO_MAZE,
    AUTO_ANNEAL,
    AUTO_BRIANS_BRAIN,
    AUTO_WIREWORLD,
    AUTO_LANGTONS_ANT,
    AUTO_CYCLIC,
    AUTO_ELEMENTARY,
    AUTO_CUSTOM,
    AUTO_COUNT
} AutomataType;

/* Wireworld states */
#define WW_EMPTY  0
#define WW_WIRE   1
#define WW_HEAD   2
#define WW_TAIL   3

/* Brian's Brain states */
#define BB_OFF    0
#define BB_DYING  1
#define BB_ON     2

/* ── Simulation state ────────────────────────────── */
typedef struct {
    u8  cur[GRID_H][GRID_W];
    u8  nxt[GRID_H][GRID_W];
    u32 generation;
    u32 population;
    u32 births;
    u32 deaths;
    int paused;
    int speed;          /* steps per frame 1–8 */
    AutomataType type;
    int wrap;
    /* Cursor */
    int cur_x, cur_y;
    int draw_state;     /* state placed when drawing */
    /* Langton's Ant */
    int ant_x, ant_y, ant_dir;
    /* Elementary CA */
    u8  elem_rule;
    int elem_row;
    /* Life-like custom rule */
    u16 birth_mask;
    u16 surv_mask;
    /* Cyclic */
    u8  cyc_states;
    u8  cyc_thresh;
} SimState;

extern SimState g_sim;

void sim_init(void);
void sim_step(void);
void sim_clear(void);
void sim_randomize(int density);
void sim_set_cell(int x, int y, u8 val);
u8   sim_get_cell(int x, int y);
void sim_set_automata(AutomataType t);
void sim_place_pattern(const u8 *data, int pw, int ph, int ox, int oy);

#endif