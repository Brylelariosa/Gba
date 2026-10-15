#ifndef PLAYER_H
#define PLAYER_H

#include "common.h"

typedef struct {
    s16 x, y, z;          /* block the feet are in (logical position)   */
    s16 fx, fy, fz;       /* block we are walking away from             */
    u8  dir;              /* 0 up(-y) 1 right(+x) 2 down(+y) 3 left(-x)  */
    u8  moving, step_t, turn_t;
    u8  arrived;          /* a step finished during the last update      */
    s16 hp, food;
    u16 food_t, regen_t, starve_t;
    u8  hurt_t, inv_t, atk_cd, place_cd;
    u8  sel;              /* selected hotbar slot                       */
    bool has_target;      /* a block is being aimed at                  */
    s16 tx, ty, tz;       /* ... this one                               */
    u16 mine_t, mine_need;
    u8  b_t;              /* frames B has been held                      */
    u8  streak;           /* blocks placed since B went down / last move */
    u8  climb_t;          /* frames pushed into a step up                */
} Player;

extern Player g_pl;

void player_reset(int x, int y, int z);
void player_update(u16 held, u16 pressed);
void player_hurt(int dmg);

#endif
