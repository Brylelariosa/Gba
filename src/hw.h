#ifndef HW_H
#define HW_H
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;

#ifdef HOST_BUILD
extern u8 g_io[0x400];
extern u8 g_pal[0x400];
extern u8 g_vram[0x18000];
extern u8 g_oam[0x400];
extern u8 g_sram[0x10000];
#define IO_BASE g_io
#define PAL_BASE g_pal
#define VRAM_BASE g_vram
#define OAM_BASE g_oam
#define SRAM_BASE g_sram
#define EWRAM_BSS
#else
#define IO_BASE ((u8 *)0x04000000)
#define PAL_BASE ((u8 *)0x05000000)
#define VRAM_BASE ((u8 *)0x06000000)
#define OAM_BASE ((u8 *)0x07000000)
#define SRAM_BASE ((u8 *)0x0E000000)
#define EWRAM_BSS __attribute__((section(".sbss")))
#endif

#define REG16(o) (*(volatile u16 *)(IO_BASE + (o)))
#define REG32(o) (*(volatile u32 *)(IO_BASE + (o)))

#define REG_DISPCNT REG16(0x000)
#define REG_DISPSTAT REG16(0x004)
#define REG_VCOUNT REG16(0x006)
#define REG_BG0CNT REG16(0x008)
#define REG_BG1CNT REG16(0x00A)
#define REG_BG2CNT REG16(0x00C)
#define REG_BG3CNT REG16(0x00E)
#define REG_BG0HOFS REG16(0x010)
#define REG_BG0VOFS REG16(0x012)
#define REG_BG1HOFS REG16(0x014)
#define REG_BG1VOFS REG16(0x016)
#define REG_BG2HOFS REG16(0x018)
#define REG_BG2VOFS REG16(0x01A)
#define REG_BG3HOFS REG16(0x01C)
#define REG_BG3VOFS REG16(0x01E)
#define REG_MOSAIC REG16(0x04C)
#define REG_BLDCNT REG16(0x050)
#define REG_BLDALPHA REG16(0x052)
#define REG_BLDY REG16(0x054)

#define REG_SND1CNT_L REG16(0x060)
#define REG_SND1CNT_H REG16(0x062)
#define REG_SND1CNT_X REG16(0x064)
#define REG_SND2CNT_L REG16(0x068)
#define REG_SND2CNT_H REG16(0x06C)
#define REG_SND3CNT_L REG16(0x070)
#define REG_SND3CNT_H REG16(0x072)
#define REG_SND3CNT_X REG16(0x074)
#define REG_SND4CNT_L REG16(0x078)
#define REG_SND4CNT_H REG16(0x07C)
#define REG_SNDCNT_L REG16(0x080)
#define REG_SNDCNT_H REG16(0x082)
#define REG_SNDCNT_X REG16(0x084)
#define REG_WAVERAM(i) REG16(0x090 + (i) * 2)

#define REG_DMA1SAD REG32(0x0BC)
#define REG_DMA1DAD REG32(0x0C0)
#define REG_DMA1CNT_H REG16(0x0C6)
#define REG_DMA2SAD REG32(0x0C8)
#define REG_DMA2DAD REG32(0x0CC)
#define REG_DMA2CNT_H REG16(0x0D2)
#define REG_TM0CNT_L REG16(0x100)
#define REG_TM0CNT_H REG16(0x102)
#define REG_KEYINPUT REG16(0x130)
#define REG_IE REG16(0x200)
#define REG_IF REG16(0x202)
#define REG_WAITCNT REG16(0x204)
#define REG_IME REG16(0x208)

#define KEY_A 0x001
#define KEY_B 0x002
#define KEY_SELECT 0x004
#define KEY_START 0x008
#define KEY_RIGHT 0x010
#define KEY_LEFT 0x020
#define KEY_UP 0x040
#define KEY_DOWN 0x080
#define KEY_R 0x100
#define KEY_L 0x200

#define PAL_BG ((volatile u16 *)(PAL_BASE))
#define PAL_OBJ ((volatile u16 *)(PAL_BASE + 0x200))
#define VRAM16 ((volatile u16 *)(VRAM_BASE))
#define VRAM32 ((volatile u32 *)(VRAM_BASE))
#define OBJ_VRAM32 ((volatile u32 *)(VRAM_BASE + 0x10000))
#define OAM32 ((volatile u32 *)(OAM_BASE))
#define SRAM8 ((volatile u8 *)(SRAM_BASE))

/* copies must use volatile destinations so the compiler never turns them into memcpy/memset
   (VRAM, palette and OAM do not tolerate 8 bit writes) */
static inline void copy32(volatile u32 *d, const u32 *s, int n) {
    for (int i = 0; i < n; i++) d[i] = s[i];
}
static inline void copy16(volatile u16 *d, const u16 *s, int n) {
    for (int i = 0; i < n; i++) d[i] = s[i];
}
static inline void fill32(volatile u32 *d, u32 v, int n) {
    for (int i = 0; i < n; i++) d[i] = v;
}
static inline void fill16(volatile u16 *d, u16 v, int n) {
    for (int i = 0; i < n; i++) d[i] = v;
}

#endif
