#include "hw.h"
#include <string.h>

u16 g_keys_held, g_keys_pressed;
u32 g_frame;

static u16 s_prev_keys;
static int s_page; /* page currently displayed (0 or 1) */

#ifndef HOST_TEST
static volatile u16 s_fill_value;
#define REG_DMA3SAD REG32(0x0D4)
#define REG_DMA3DAD REG32(0x0D8)
#define REG_DMA3CNT REG32(0x0DC)
#endif

void hw_init(void)
{
    REG_DISPCNT = DCNT_BLANK; /* forced blank while setting up */
#ifndef HOST_TEST
    REG_WAITCNT = 0x4317;     /* faster ROM access + prefetch */
#endif
    REG_TM0CNT_L = 0;
    REG_TM0CNT_H = 0x0080;    /* free running timer = entropy */
    s_page = 0;
    s_prev_keys = 0;
    g_frame = 0;
}

void hw_show(void)
{
    REG_DISPCNT = (u16)(DCNT_MODE4 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D | (s_page ? DCNT_PAGE : 0));
}

void hw_vsync(void)
{
#ifndef HOST_TEST
    while (REG_VCOUNT >= 160) { }
    while (REG_VCOUNT < 160) { }
#endif
    g_frame++;
}

u16 *hw_back_page(void)
{
    return (u16 *)(HW_VRAM + (s_page ? 0 : 0xA000));
}

void hw_flip(void)
{
    s_page ^= 1;
    REG_DISPCNT = (u16)((REG_DISPCNT & ~DCNT_PAGE) | (s_page ? DCNT_PAGE : 0));
}

void hw_copy16(void *dst, const void *src, u32 halfwords)
{
#ifdef HOST_TEST
    memcpy(dst, src, halfwords * 2);
#else
    REG_DMA3SAD = (u32)src;
    REG_DMA3DAD = (u32)dst;
    REG_DMA3CNT = halfwords | 0x80000000u;
#endif
}

void hw_fill16(void *dst, u16 value, u32 halfwords)
{
#ifdef HOST_TEST
    u16 *d = (u16 *)dst;
    for (u32 i = 0; i < halfwords; i++) d[i] = value;
#else
    s_fill_value = value;
    REG_DMA3SAD = (u32)&s_fill_value;
    REG_DMA3DAD = (u32)dst;
    REG_DMA3CNT = halfwords | ((u32)(0x8000 | (2 << 7)) << 16);
#endif
}

void hw_oam_flush(const ObjAttr *shadow)
{
    hw_copy16(HW_OAM, shadow, 128 * 4);
}

void input_poll(void)
{
    u16 k = (u16)(~REG_KEYINPUT) & 0x03FF;
    g_keys_pressed = k & ~s_prev_keys;
    g_keys_held = k;
    s_prev_keys = k;
}

u32 hw_seed(void)
{
    u32 t = REG_TM0CNT_L;
    return (t * 2654435761u) ^ (g_frame * 40503u) ^ 0xA5A5u;
}
