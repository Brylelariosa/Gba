#ifndef ITEMS_H
#define ITEMS_H

#include "common.h"
#include "blocks.h"

/* Item ids 1..B_COUNT-1 are the placeable blocks (same numbering). */
enum {
    I_NONE = 0,
    I_STICK = 32, I_COAL, I_IRON_INGOT, I_GOLD_INGOT, I_DIAMOND,
    I_APPLE, I_PORK_RAW, I_PORK_COOKED,
    I_PICK_WOOD, I_PICK_STONE, I_PICK_IRON, I_PICK_DIAMOND,
    I_AXE_WOOD, I_AXE_STONE, I_AXE_IRON, I_AXE_DIAMOND,
    I_SWORD_WOOD, I_SWORD_STONE, I_SWORD_IRON, I_SWORD_DIAMOND,
    I_COUNT
};

enum { IK_NONE = 0, IK_BLOCK, IK_MAT, IK_PICK, IK_AXE, IK_SWORD, IK_FOOD };

const char *item_name(u8 id);
u8 item_kind(u8 id);
u8 item_tier(u8 id);     /* tool tier 1..4 (wood, stone, iron, diamond) */
u8 item_max_dur(u8 id);  /* >0 for tools: also means "does not stack"   */
u8 item_food(u8 id);     /* hunger points restored                      */

#endif
