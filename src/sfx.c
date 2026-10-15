#include "sfx.h"
#include "hw.h"

/* Tiny sound effect player on the GBA PSG channels (no sample data). */

#define REG_SND1_SWEEP REG16(0x60)
#define REG_SND1_ENV   REG16(0x62)
#define REG_SND1_FREQ  REG16(0x64)
#define REG_SND4_ENV   REG16(0x78)
#define REG_SND4_CTRL  REG16(0x7C)
#define REG_SND_LR     REG16(0x80)
#define REG_SND_MIX    REG16(0x82)
#define REG_SND_MASTER REG16(0x84)

/* freq register = 2048 - 131072 / Hz */
#define F_220  1452
#define F_660  1849
#define F_880  1899
#define F_1320 1949
#define F_1760 1973

static void tone(int freq, int vol, int step, int len)
{
    REG_SND1_SWEEP = 0;
    REG_SND1_ENV = (u16)((vol << 12) | (step << 8) | (2 << 6) | (len & 63));
    REG_SND1_FREQ = (u16)(0x8000 | 0x4000 | freq);
}

static void noise(int vol, int step, int len, int shift, int div, int narrow)
{
    REG_SND4_ENV = (u16)((vol << 12) | (step << 8) | (len & 63));
    REG_SND4_CTRL = (u16)(0x8000 | 0x4000 | (shift << 4) | (narrow << 3) | div);
}

void sfx_init(void)
{
    REG_SND_MASTER = 0x0080;
    REG_SND_LR = 0xFF77;
    REG_SND_MIX = 0x0002;
}

void sfx_play(int id)
{
    switch (id) {
    case SFX_MINE:   noise(6, 1, 56, 6, 3, 1);  break;
    case SFX_BREAK:  noise(10, 2, 40, 5, 2, 0); break;
    case SFX_PLACE:  noise(7, 1, 52, 7, 4, 1);  break;
    case SFX_PICKUP: tone(F_880, 8, 1, 52);     break;
    case SFX_CRAFT:  tone(F_1320, 9, 2, 40);    break;
    case SFX_HURT:   noise(12, 2, 36, 4, 1, 0); break;
    case SFX_EAT:    noise(8, 1, 50, 6, 3, 0);  break;
    case SFX_ADV:    tone(F_1760, 11, 3, 24);   break;
    case SFX_MENU:   tone(F_660, 5, 1, 58);     break;
    case SFX_DEATH:  tone(F_220, 12, 4, 8);     break;
    default: break;
    }
}
