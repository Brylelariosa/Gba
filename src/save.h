#ifndef SAVE_H
#define SAVE_H

#include <gba.h>
#include "simulation.h"

#define SAVE_SLOTS   4
#define SAVE_MAGIC   0xCAFEBABE

typedef struct {
    u32 magic;
    u8  grid[GRID_H][GRID_W];
    u32 generation;
    u32 population;
    int type;
    u16 birth_mask;
    u16 surv_mask;
    int theme;
    int speed;
} SaveSlot;

void save_init(void);
void save_write(int slot);
int  save_read(int slot);
int  save_slot_used(int slot);
void save_draw_menu(int selected);

#endif