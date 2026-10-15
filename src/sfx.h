#ifndef SFX_H
#define SFX_H

#include "common.h"

enum {
    SFX_MINE, SFX_BREAK, SFX_PLACE, SFX_PICKUP, SFX_CRAFT,
    SFX_HURT, SFX_EAT, SFX_ADV, SFX_MENU, SFX_DEATH
};

void sfx_init(void);
void sfx_play(int id);

#endif
