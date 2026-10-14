#include "blocks.h"
#include "items.h"

const BlockInfo g_block_info[B_COUNT] = {
    [B_AIR]         = { "AIR",            0,   TC_HAND, 0 },
    [B_GRASS]       = { "GRASS",          40,  TC_HAND, 0 },
    [B_DIRT]        = { "DIRT",           36,  TC_HAND, 0 },
    [B_STONE]       = { "STONE",          150, TC_PICK, 1 },
    [B_COBBLE]      = { "COBBLESTONE",    150, TC_PICK, 1 },
    [B_SAND]        = { "SAND",           36,  TC_HAND, 0 },
    [B_WATER]       = { "WATER",          255, TC_HAND, 0 },
    [B_LOG]         = { "LOG",            110, TC_AXE,  0 },
    [B_LEAVES]      = { "LEAVES",         12,  TC_HAND, 0 },
    [B_PLANKS]      = { "PLANKS",         100, TC_AXE,  0 },
    [B_COAL_ORE]    = { "COAL ORE",       200, TC_PICK, 1 },
    [B_IRON_ORE]    = { "IRON ORE",       220, TC_PICK, 2 },
    [B_GOLD_ORE]    = { "GOLD ORE",       240, TC_PICK, 3 },
    [B_DIAMOND_ORE] = { "DIAMOND ORE",    250, TC_PICK, 3 },
    [B_BEDROCK]     = { "BEDROCK",        255, TC_PICK, 99 },
    [B_TABLE]       = { "CRAFTING TABLE", 100, TC_AXE,  0 },
    [B_FURNACE]     = { "FURNACE",        150, TC_PICK, 1 },
    [B_GLASS]       = { "GLASS",          20,  TC_HAND, 0 },
    [B_BRICK]       = { "STONE BRICK",    160, TC_PICK, 1 },
    [B_BEACON]      = { "EMBER BEACON",   100, TC_HAND, 0 },
};

u8 block_drop(u8 b)
{
    switch (b) {
    case B_GRASS:       return B_DIRT;
    case B_STONE:       return B_COBBLE;
    case B_COAL_ORE:    return I_COAL;
    case B_DIAMOND_ORE: return I_DIAMOND;
    case B_AIR:
    case B_WATER:
    case B_LEAVES:
    case B_GLASS:
    case B_BEDROCK:     return 0;
    default:            return b;
    }
}
