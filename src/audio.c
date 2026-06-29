#include "audio.h"

/*
 * GBA audio: we use Direct Sound channel A.
 * All samples are tiny synthesised square-wave / noise bursts
 * stored as 8-bit signed PCM at 8000 Hz.
 * Timer0 reload = 65536 - (16777216 / SAMPLE_RATE)
 */
#define SAMPLE_RATE  8000
#define TIMER_RELOAD ((u16)(65536 - (16777216 / SAMPLE_RATE)))

/* ── Synth sound table (small 8-bit PCM bursts) ──── */
typedef struct { const s8 *data; u32 len; } SfxWave;

/* 50-sample square blip ~160 Hz */
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
    100,100,100,100,100, 100, 100, 100, 100, 100,
   -100,-100,-100,-100,-100,-100,-100,-100,-100,-100,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
};

/* Falling chime for load */
static const s8 SFX_LOAD_DATA[90] = {
    100,100,100,100,100,-100,-100,-100,-100,-100,
     80, 80, 80, 80, 80, -80, -80, -80, -80, -80,
     60, 60, 60, 60, 60, -60, -60, -60, -60, -60,
     40, 40, 40, 40, 40, -40, -40, -40, -40, -40,
     20, 20, 20, 20, 20, -20, -20, -20, -20, -20,
      0,  0,  0,  0,  0,   0,   0,   0,   0,   0,
      0,  0,  0,  0,  0,   0,   0,   0,   0,   0,
      0,  0,  0,  0,  0,   0,   0,   0,   0,   0,
      0,  0,  0,  0,  0,   0,   0,   0,   0,   0,
};

static const SfxWave SFX_TABLE[SFX_COUNT] = {
    { SFX_SOFT_DATA,   20 }, /* SFX_CURSOR */
    { SFX_HIBLIP_DATA, 40 }, /* SFX_PLACE  */
    { SFX_ERASE_DATA,  60 }, /* SFX_ERASE  */
    { SFX_MENU_DATA,   80 }, /* SFX_MENU   */
    { SFX_SAVE_DATA,   88 }, /* SFX_SAVE   (88 = multiple of 4) */
    { SFX_LOAD_DATA,   88 }, /* SFX_LOAD   (88 = multiple of 4) */
    { SFX_BLIP_DATA,   50 }, /* SFX_PAUSE  (50 rounded to 48 below) */
};

/* ── Init ────────────────────────────────────────── */
void audio_init(void) {
    /* Set up mixer: Direct Sound A full volume, both speakers */
    REG_SOUNDCNT_H = SNDA_VOL_100 | SNDA_L_ENABLE | SNDA_R_ENABLE | SNDA_RESET_FIFO;

    /* Master sound enable — raw bit 7 of REG_SOUNDCNT_X */
    /* SOUND_ENABLE is not defined in this libgba; use raw value */
    REG_SOUNDCNT_X = (1 << 7);

    /* Timer 0 at 8000 Hz drives Direct Sound A */
    REG_TM0CNT_L = TIMER_RELOAD;
    /* TIMER_ENABLE renamed to TIMER_START in current libgba */
    REG_TM0CNT_H = TIMER_START;

    /* Timer 0 is the default for SNDA (bit 10 of SOUNDCNT_H = 0),
       no extra bit needed — removed SNDA_TIMER0 which is undefined */
}

/* ── Stop ────────────────────────────────────────── */
void audio_stop(void) {
    REG_SOUNDCNT_H |= SNDA_RESET_FIFO;
}

/* ── Play SFX ────────────────────────────────────── */
void audio_play_sfx(SfxId id) {
    /* id < 0 check removed: SfxId is an unsigned enum on ARM,
       the comparison always false warning from -Wextra fires */
    if (id >= SFX_COUNT) return;

    const SfxWave *w = &SFX_TABLE[id];

    /* DMA transfer length must be a multiple of 4 bytes (32-bit words) */
    u32 words = (w->len / 4);
    if (words == 0) words = 1;

    /* Reset FIFO before filling */
    REG_SOUNDCNT_H |= SNDA_RESET_FIFO;
    REG_SOUNDCNT_H &= ~SNDA_RESET_FIFO;

    /* DMA1 → Sound FIFO A
       DMA_32 renamed to DMA32 in current libgba (error said "did you mean DMA32?") */
    REG_DMA1SAD = (u32)w->data;
    REG_DMA1DAD = (u32)&REG_FIFO_A;
    REG_DMA1CNT = DMA_DST_FIXED | DMA_SRC_INC | DMA_REPEAT |
                  DMA32 | DMA_SPECIAL | DMA_ENABLE |
                  (words & 0xFFFF);
}