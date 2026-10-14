#ifndef INVENTORY_H
#define INVENTORY_H

#include "common.h"

#define INV_SLOTS     24
#define HOTBAR_SLOTS  8
#define STACK_MAX     64

typedef struct { u8 id, count, dur; } Slot;

extern Slot g_inv[INV_SLOTS];

/* Optional callback fired whenever items are added (used for goals). */
extern void (*g_inv_hook)(u8 id);

void inv_clear(void);
int  inv_add(u8 id, int n);       /* returns how many did NOT fit */
int  inv_space(u8 id);            /* how many more of this item fit */
int  inv_count(u8 id);
bool inv_take(u8 id, int n);      /* removes n if available */
void inv_move(int from, int to);  /* swap or merge two slots */
void inv_wear(int slot);          /* use up one point of tool durability */

#endif
