#include "items.h"

typedef struct {
    const char *name;
    u8 kind, tier, dur, food;
} ItemInfo;

static const ItemInfo ITEMS[I_COUNT] = {
    [I_STICK]         = { "STICK",           IK_MAT,   0, 0,   0 },
    [I_COAL]          = { "COAL",            IK_MAT,   0, 0,   0 },
    [I_IRON_INGOT]    = { "IRON INGOT",      IK_MAT,   0, 0,   0 },
    [I_GOLD_INGOT]    = { "GOLD INGOT",      IK_MAT,   0, 0,   0 },
    [I_DIAMOND]       = { "DIAMOND",         IK_MAT,   0, 0,   0 },
    [I_APPLE]         = { "APPLE",           IK_FOOD,  0, 0,   4 },
    [I_PORK_RAW]      = { "RAW PORK",        IK_FOOD,  0, 0,   3 },
    [I_PORK_COOKED]   = { "COOKED PORK",     IK_FOOD,  0, 0,   8 },
    [I_PICK_WOOD]     = { "WOOD PICKAXE",    IK_PICK,  1, 60,  0 },
    [I_PICK_STONE]    = { "STONE PICKAXE",   IK_PICK,  2, 132, 0 },
    [I_PICK_IRON]     = { "IRON PICKAXE",    IK_PICK,  3, 250, 0 },
    [I_PICK_DIAMOND]  = { "DIAMOND PICKAXE", IK_PICK,  4, 255, 0 },
    [I_AXE_WOOD]      = { "WOOD AXE",        IK_AXE,   1, 60,  0 },
    [I_AXE_STONE]     = { "STONE AXE",       IK_AXE,   2, 132, 0 },
    [I_AXE_IRON]      = { "IRON AXE",        IK_AXE,   3, 250, 0 },
    [I_AXE_DIAMOND]   = { "DIAMOND AXE",     IK_AXE,   4, 255, 0 },
    [I_SWORD_WOOD]    = { "WOOD SWORD",      IK_SWORD, 1, 60,  0 },
    [I_SWORD_STONE]   = { "STONE SWORD",     IK_SWORD, 2, 132, 0 },
    [I_SWORD_IRON]    = { "IRON SWORD",      IK_SWORD, 3, 250, 0 },
    [I_SWORD_DIAMOND] = { "DIAMOND SWORD",   IK_SWORD, 4, 255, 0 },
};

const char *item_name(u8 id)
{
    if (id == 0) return "";
    if (id < B_COUNT) return g_block_info[id].name;
    if (id < I_COUNT && ITEMS[id].name) return ITEMS[id].name;
    return "?";
}

u8 item_kind(u8 id)
{
    if (id == 0) return IK_NONE;
    if (id < B_COUNT) return IK_BLOCK;
    if (id < I_COUNT) return ITEMS[id].kind;
    return IK_NONE;
}

u8 item_tier(u8 id)    { return (id >= B_COUNT && id < I_COUNT) ? ITEMS[id].tier : 0; }
u8 item_max_dur(u8 id) { return (id >= B_COUNT && id < I_COUNT) ? ITEMS[id].dur  : 0; }
u8 item_food(u8 id)    { return (id >= B_COUNT && id < I_COUNT) ? ITEMS[id].food : 0; }
