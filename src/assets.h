#ifndef ASSETS_H
#define ASSETS_H
#include "hw.h"

#define HERO_FRAMES 46
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
#define HF_HIGH_DOWN 37
#define HF_HIGH_UP 38
#define HF_HIGH_SIDE 39
#define HF_SLAM_DOWN 40
#define HF_SLAM_UP 41
#define HF_SLAM_SIDE 42
#define HF_THRUST_DOWN 43
#define HF_THRUST_UP 44
#define HF_THRUST_SIDE 45
#define OT_WEAPON 184
#define OT_ARC 248
#define OT_STREAK 264
#define OT_SHOCK_H 280
#define OT_SHOCK_V 288
#define OT_BURST 296
#define OT_ARCANE 308
#define OT_SHADOW 316
#define OT_SHADOW_S 318
#define OT_SLIME 319
#define OT_BAT 335
#define OT_WOLF 347
#define OT_SKEL 363
#define OT_BOSS 423
#define OT_WARDEN 567
#define OT_RING 711
#define OT_LANE 727
#define OT_NPC 729
#define OT_ARROW 753
#define OT_BOLT 757
#define OT_FIRE 759
#define OT_ORB 767
#define OT_EXPL 769
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
#define OT_PART 785
#define OT_NUM 807
#define OT_ITEM 817
#define IT_HEART 4
#define IT_MANA 5
#define IT_POTION 6
#define IT_ETHER 7
#define IT_KEY 8
#define OT_CURSOR 826
#define OT_PROMPT 834
#define OT_ALERT 842
#define OT_STAR 850
#define OT_TOTAL 854
#define OT_COMMON 184
#define OT_COMMON_TILES 670
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
#define META_D_STAIRS_L 33
#define META_D_STAIRS_R 34
#define META_D_FLOOR 0
#define META_D_RUNE 9

extern const u32 *const hero_tiles[4];
extern const u16 hero_pal[4][16];
extern const u32 obj_common_tiles[];
extern const u16 obj_pal_init[256];
extern const u16 weapon_tier_pal[5][16];
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
