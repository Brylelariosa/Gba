#ifndef MOBS_H
#define MOBS_H

#include "common.h"

enum { MT_NONE = 0, MT_ZOMBIE, MT_PIG };

typedef struct {
    u8  type;
    u8  dir, moving, step_t, cd, flash;
    s16 x, y, z;      /* logical block (destination while walking) */
    s16 fx, fy, fz;   /* block being walked away from              */
    s8  hp;
} Mob;

#define MAX_MOBS 10
extern Mob g_mobs[MAX_MOBS];

void mobs_clear(void);
void mobs_populate(void);                 /* initial pigs around the world */
void mobs_update(void);                   /* AI, spawning and despawning   */
Mob *mob_at(int x, int y, int z);         /* mob standing here (z +-1)     */
void mob_hurt(Mob *m, int dmg, int kdir); /* kdir: knock-back direction    */

#endif
