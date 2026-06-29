#ifndef AUDIO_H
#define AUDIO_H

#include <gba.h>

typedef enum {
    SFX_CURSOR = 0,
    SFX_PLACE,
    SFX_ERASE,
    SFX_MENU,
    SFX_SAVE,
    SFX_LOAD,
    SFX_PAUSE,
    SFX_COUNT
} SfxId;

void audio_init(void);
void audio_play_sfx(SfxId id);
void audio_stop(void);

#endif