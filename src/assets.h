#ifndef ASSETS_H
#define ASSETS_H
#include "hw.h"

#define HERO_FRAMES 37
#define HF_WALK_DOWN 0
#define HF_WALK_UP 6
#define HF_WALK_SIDE 12
#define HF_ATK_DOWN 18
#define HF_ATK_UP 21
#define HF_ATK_SIDE 24
#define HF_ROLL 27
#define HF_HURT_DOWN 31
#define HF_HURT_UP 32
#define HF_HURT_SIDE 33
#define HF_DEAD 34
#define OT_WEAPON 148
#define OT_ARC 212
#define OT_SHADOW 228
#define OT_SHADOW_S 230
#define OT_SLIME 231
#define OT_BAT 247
#define OT_WOLF 259
#define OT_SKEL 275
#define OT_BOSS 335
#define OT_NPC 415
#define OT_ARROW 439
#define OT_BOLT 443
#define OT_FIRE 445
#define OT_ORB 453
#define OT_EXPL 455
#define PT_SPARKLE 0
#define PT_SPARKLE_N 4
#define PT_SMOKE 4
#define PT_SMOKE_N 4
#define PT_LEAF 8
#define PT_LEAF_N 2
#define PT_HIT 10
#define PT_HIT_N 3
#define PT_DUST 13
#define PT_DUST_N 3
#define PT_EMBER 16
#define PT_EMBER_N 3
#define PT_HEAL 19
#define PT_HEAL_N 3
#define OT_PART 471
#define OT_NUM 493
#define OT_ITEM 503
#define IT_HEART 4
#define IT_MANA 5
#define IT_POTION 6
#define IT_ETHER 7
#define IT_KEY 8
#define OT_CURSOR 512
#define OT_PROMPT 520
#define OT_ALERT 528
#define OT_STAR 536
#define OT_TOTAL 540
#define OT_COMMON 148
#define OT_COMMON_TILES 392
#define SKF_WALK_D 0
#define SKF_WALK_U 4
#define SKF_WALK_S 8
#define SKF_ATK_D 12
#define SKF_ATK_U 13
#define SKF_ATK_S 14
#define PB_HERO 0
#define PB_WEAPON 1
#define PB_SLIME 2
#define PB_SLIME2 3
#define PB_BAT 4
#define PB_SKEL 5
#define PB_WOLF 6
#define PB_BOSS 7
#define PB_NPC0 8
#define PB_FX 11
#define PB_ITEM 12
#define PB_UI 13
#define PB_NUMR 14
#define PB_FLASH 15
#define UT_FILL 1
#define UT_FRAME_TL 2
#define UT_FRAME_T 3
#define UT_FRAME_TR 4
#define UT_FRAME_L 5
#define UT_FRAME_R 6
#define UT_FRAME_BL 7
#define UT_FRAME_B 8
#define UT_FRAME_BR 9
#define UT_COIN 10
#define UT_KEY 11
#define UT_POTION 12
#define UT_ARROW_UP 13
#define UT_ARROW_DN 14
#define UT_HEART 15
#define UT_POOL 32
#define UI_TILES_N 32
#define TITLE_NTILES 300

extern const u32 *const hero_tiles[4];
extern const u16 hero_pal[4][16];
extern const u32 obj_common_tiles[];
extern const u16 obj_pal_init[256];
extern const u16 ui_pal[64];
extern const u32 ui_tiles[];
extern const u8 font_w[95];
extern const u8 font_rows[95 * 8];
extern const s16 sin256[256];
extern const u32 title_tiles[];
extern const u16 title_map[600];
extern const u16 scene_map[600];
extern const u16 title_pal[16];

#endif
