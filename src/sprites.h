#ifndef SPRITES_H
#define SPRITES_H

#include "common.h"

/* 16x16 hardware sprite frames (generated in code, no asset files). */
enum {
    SP_PLAYER_F0, SP_PLAYER_F1, SP_PLAYER_B0, SP_PLAYER_B1,
    SP_ZOMBIE_F0, SP_ZOMBIE_F1, SP_ZOMBIE_B0, SP_ZOMBIE_B1,
    SP_PIG0, SP_PIG1,
    SP_CURSOR, SP_CRACK0, SP_CRACK1, SP_CRACK2, SP_CRACK3,
    SP_COUNT
};

enum { BANK_PLAYER = 0, BANK_ZOMBIE = 1, BANK_PIG = 2, BANK_FX = 3 };

void spr_init(void);
void spr_begin(void);
void spr_put(int frame, int x, int y, bool hflip, int bank);
void spr_end(void); /* hides unused entries and uploads OAM (call in VBlank) */

#endif
