#ifndef BLOCKS_H
#define BLOCKS_H

#include "common.h"

enum {
    B_AIR = 0, B_GRASS, B_DIRT, B_STONE, B_COBBLE, B_SAND, B_WATER, B_LOG,
    B_LEAVES, B_PLANKS, B_COAL_ORE, B_IRON_ORE, B_GOLD_ORE, B_DIAMOND_ORE,
    B_BEDROCK, B_TABLE, B_FURNACE, B_GLASS, B_BRICK, B_BEACON,
    B_DOOR,     /* lower half of a door; also the inventory item              */
    B_DOORTOP,  /* upper half, only ever exists above a B_DOOR in the world   */
    B_COUNT
};

enum { TC_HAND = 0, TC_PICK, TC_AXE };

typedef struct {
    const char *name;
    u8 hard; /* frames to break bare handed (255 = unbreakable)       */
    u8 tool; /* preferred tool class                                  */
    u8 tier; /* minimum tool tier needed to get a drop (0 = anything) */
} BlockInfo;

extern const BlockInfo g_block_info[B_COUNT];

static inline bool block_solid(u8 b)     { return b != B_AIR && b != B_WATER; }
static inline bool block_placeable(u8 b) { return b != B_AIR && b != B_WATER && b != B_BEDROCK && b != B_DOORTOP; }

/* Doors are two blocks tall; the player (and only the player) walks through them. */
static inline bool block_is_door(u8 b)   { return b == B_DOOR || b == B_DOORTOP; }

/* Item id dropped when the block is harvested (0 = nothing). */
u8 block_drop(u8 b);

#endif
