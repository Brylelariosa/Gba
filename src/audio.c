#include "audio.h"

/*
 * GBA audio: we use Direct Sound channel A.
 * All samples are tiny synthesised square-wave / noise bursts
 * stored as 8-bit signed PCM at 8000 Hz.
 * 8000 Hz at 16.78 MHz / 512 Timer0 = 32786 counts, closest = 32786
 * Timer0 reload = 65536 - (16777216 / SAMPLE_RATE)
 */
#define SAMPLE_RATE 8000
#define TIMER_RELOAD ((u16)(65536 - (16777216 / SAMPLE_RATE)))

/* ── Synth sound table (small 8-bit PCM bursts) ──── */
/* Each SFX: array of s8 + length */
typedef struct { const s8 *data; u32 len; } SfxWave;

/* 50-sample square blip at ~160 Hz */
static const s8 SFX_BLIP_DATA[50] = {
     60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
     60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
     60, 60, 60, 60, 60,
    -60,-60,-60,-60,-60,-60,-60,-60,-60,-60,
    -60,-60,-60,-60,-60,-60,-60,-60,-60,-60,
    -60,-60,-60,-60,-60,
};

/* 40-sample higher blip */
static const s8 SFX_HIBLIP_DATA[40] = {
     80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
     80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
    -80,-80,-80,-80,-80,-80,-80,-80,-80,-80,
    -80,-80,-80,-80,-80,-80,-80,-80,-80,-80,
};

/* Soft short blip for cursor */
static const s8 SFX_SOFT_DATA[20] = {
     30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
    -30,-30,-30,-30,-30,-30,-30,-30,-30,-30,
};

/* Descending blip for erase */
static const s8 SFX_ERASE_DATA[60] = {
     40, 38, 36, 34, 32, 30, 28, 26, 24, 22,
     20, 18, 16, 14, 12, 10,  8,  6,  4,  2,
      0, -2, -4, -6, -8,-10,-12,-14,-16,-18,
    -20,-22,-24,-26,-28,-30,-32,-34,-36,-38,
    -40,-38,-36,-34,-32,-30,-28,-26,-24,-22,
    -20,-18,-16,-14,-12,-10, -8, -6, -4, -2,
};

/* Double-blip for menu select */
static const s8 SFX_MENU_DATA[80] = {
     70, 70, 70, 70, 70, 70, 70, 70, 70, 70,
    -70,-70,-70,-70,-70,-70,-70,-70,-70,-70,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
     90, 90, 90, 90, 90, 90, 90, 90, 90, 90,
    -90,-90,-90,-90,-90,-90,-90,-90,-90,-90,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

/* Rising chime for save */
static const s8 SFX_SAVE_DATA[90] = {
     20, 20, 20, 20, 20,-20,-20,-20,-20,-20,
     40, 40, 40, 40, 40,-40,-40,-40,-40,-40,
     60, 60, 60, 60, 60,-60,-60,-60,-60,-60,
     80, 80, 80, 80, 80,-80,-80,-80,-80,-80,
    100,100,100,100,100,-100,-100,-100,-100,-100,
    100,100,100,100,100,100,100,100,100,100,
    -100,-100,-100,-100,-100,-100,-100,-100,-100,-100,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

/* Falling chime for load */
static const s8 SFX_LOAD_DATA[90] = {
    100,100,100,100,100,-100,-100,-100,-100,-100,
     80, 80, 80, 80, 80,-80,-80,-80,-80,-80,
     60, 60, 60, 60, 60,-60,-60,-60,-60,-60,
     40, 40, 40, 40, 40,-40,-40,-40,-40,-40,
     20, 20, 20, 20, 20,-20,-20,-20,-20,-20,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

static const SfxWave SFX_TABLE[SFX_COUNT] = {
    { SFX_SOFT_DATA,  20 }, /* SFX_CURSOR */
    { SFX_HIBLIP_DATA,40 }, /* SFX_PLACE  */
    { SFX_ERASE_DATA, 60 }, /* SFX_ERASE  */
    { SFX_MENU_DATA,  80 }, /* SFX_MENU   */
    { SFX_SAVE_DATA,  90 }, /* SFX_SAVE   */
    { SFX_LOAD_DATA,  90 }, /* SFX_LOAD   */
    { SFX_BLIP_DATA,  50 }, /* SFX_PAUSE  */
};

/* ── Init ────────────────────────────────────────── */
void audio_init(void) {
    REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_R_ENABLE | SNDA_RESET_FIFO;
    REG_SOUNDCNT_X = SOUND_ENABLE;

    /* Timer 0 drives Direct Sound A */
    REG_TM0CNT_L = TIMER_RELOAD;
    REG_TM0CNT_H = TIMER_ENABLE;

    REG_SOUNDCNT_H |= SNDA_TIMER0;
}

void audio_stop(void) {
    REG_SOUNDCNT_H |= SNDA_RESET_FIFO;
}

void audio_play_sfx(SfxId id) {
    if (id < 0 || id >= SFX_COUNT) return;
    const SfxWave *w = &SFX_TABLE[id];

    /* Reset FIFO */
    REG_SOUNDCNT_H |= SNDA_RESET_FIFO;
    REG_SOUNDCNT_H &= ~SNDA_RESET_FIFO;

    /* Copy samples into FIFO via DMA1 */
    REG_DMA1SAD = (u32)w->data;
    REG_DMA1DAD = (u32)&REG_FIFO_A;
    REG_DMA1CNT = DMA_DST_FIXED | DMA_SRC_INC | DMA_REPEAT |
                  DMA_32 | DMA_SPECIAL | DMA_ENABLE |
                  ((w->len / 4) & 0xFFFF);
}