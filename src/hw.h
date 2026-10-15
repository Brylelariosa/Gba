#ifndef HW_H
#define HW_H

#include "common.h"

#define SCREEN_W 240
#define SCREEN_H 160

/* Memory map. The host test build redirects these to plain arrays. */
#ifdef HOST_TEST
extern u8 host_io[0x400];
extern u8 host_pal[0x400];
extern u8 host_vram[0x18000];
extern u8 host_oam[0x400];
extern u8 host_sram[0x8000];
#define HW_IO   (host_io)
#define HW_PAL  (host_pal)
#define HW_VRAM (host_vram)
#define HW_OAM  (host_oam)
#define HW_SRAM (host_sram)
#else
#define HW_IO   ((u8 *)0x04000000)
#define HW_PAL  ((u8 *)0x05000000)
#define HW_VRAM ((u8 *)0x06000000)
#define HW_OAM  ((u8 *)0x07000000)
#define HW_SRAM ((u8 *)0x0E000000)
#endif

#define REG16(off) (*(volatile u16 *)(HW_IO + (off)))
#define REG32(off) (*(volatile u32 *)(HW_IO + (off)))

#define REG_DISPCNT  REG16(0x000)
#define REG_VCOUNT   REG16(0x006)
#define REG_TM0CNT_L REG16(0x100)
#define REG_TM0CNT_H REG16(0x102)
#define REG_KEYINPUT REG16(0x130)
#define REG_WAITCNT  REG16(0x204)

#define DCNT_MODE4  0x0004
#define DCNT_PAGE   0x0010
#define DCNT_OBJ_1D 0x0040
#define DCNT_BLANK  0x0080
#define DCNT_BG2    0x0400
#define DCNT_OBJ    0x1000

#define PAL_BG    ((volatile u16 *)(HW_PAL))
#define PAL_OBJ   ((volatile u16 *)(HW_PAL + 0x200))
#define OBJ_TILES ((volatile u16 *)(HW_VRAM + 0x14000))

#define RGB15(r, g, b) ((u16)(((r) >> 3) | (((g) >> 3) << 5) | (((b) >> 3) << 10)))

#define KEY_A      0x0001
#define KEY_B      0x0002
#define KEY_SELECT 0x0004
#define KEY_START  0x0008
#define KEY_RIGHT  0x0010
#define KEY_LEFT   0x0020
#define KEY_UP     0x0040
#define KEY_DOWN   0x0080
#define KEY_R      0x0100
#define KEY_L      0x0200

typedef struct { u16 a0, a1, a2, pad; } ObjAttr;

extern u16 g_keys_held, g_keys_pressed;
extern u32 g_frame;

void hw_init(void);
void hw_show(void);
void hw_vsync(void);
u16 *hw_back_page(void);
void hw_flip(void);
void hw_copy16(void *dst, const void *src, u32 halfwords);
void hw_fill16(void *dst, u16 value, u32 halfwords);
void hw_oam_flush(const ObjAttr *shadow);
void input_poll(void);
u32  hw_seed(void);

#endif
