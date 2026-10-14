#include "craft.h"
#include "inventory.h"
#include "items.h"
#include "blocks.h"

/* out, count, station, {ingredients}, {amounts} */
const Recipe g_recipes[] = {
    { B_PLANKS,       4, ST_HAND,    { B_LOG, 0, 0 },                 { 1, 0, 0 } },
    { I_STICK,        4, ST_HAND,    { B_PLANKS, 0, 0 },              { 2, 0, 0 } },
    { B_TABLE,        1, ST_HAND,    { B_PLANKS, 0, 0 },              { 4, 0, 0 } },
    { I_PICK_WOOD,    1, ST_TABLE,   { B_PLANKS, I_STICK, 0 },        { 3, 2, 0 } },
    { I_AXE_WOOD,     1, ST_TABLE,   { B_PLANKS, I_STICK, 0 },        { 3, 2, 0 } },
    { I_SWORD_WOOD,   1, ST_TABLE,   { B_PLANKS, I_STICK, 0 },        { 2, 1, 0 } },
    { I_PICK_STONE,   1, ST_TABLE,   { B_COBBLE, I_STICK, 0 },        { 3, 2, 0 } },
    { I_AXE_STONE,    1, ST_TABLE,   { B_COBBLE, I_STICK, 0 },        { 3, 2, 0 } },
    { I_SWORD_STONE,  1, ST_TABLE,   { B_COBBLE, I_STICK, 0 },        { 2, 1, 0 } },
    { B_FURNACE,      1, ST_TABLE,   { B_COBBLE, 0, 0 },              { 8, 0, 0 } },
    { I_IRON_INGOT,   1, ST_FURNACE, { B_IRON_ORE, I_COAL, 0 },       { 1, 1, 0 } },
    { I_GOLD_INGOT,   1, ST_FURNACE, { B_GOLD_ORE, I_COAL, 0 },       { 1, 1, 0 } },
    { B_STONE,        1, ST_FURNACE, { B_COBBLE, I_COAL, 0 },         { 1, 1, 0 } },
    { B_GLASS,        2, ST_FURNACE, { B_SAND, I_COAL, 0 },           { 2, 1, 0 } },
    { I_PORK_COOKED,  1, ST_FURNACE, { I_PORK_RAW, I_COAL, 0 },       { 1, 1, 0 } },
    { B_BRICK,        4, ST_TABLE,   { B_STONE, 0, 0 },               { 4, 0, 0 } },
    { I_PICK_IRON,    1, ST_TABLE,   { I_IRON_INGOT, I_STICK, 0 },    { 3, 2, 0 } },
    { I_AXE_IRON,     1, ST_TABLE,   { I_IRON_INGOT, I_STICK, 0 },    { 3, 2, 0 } },
    { I_SWORD_IRON,   1, ST_TABLE,   { I_IRON_INGOT, I_STICK, 0 },    { 2, 1, 0 } },
    { I_PICK_DIAMOND, 1, ST_TABLE,   { I_DIAMOND, I_STICK, 0 },       { 3, 2, 0 } },
    { I_AXE_DIAMOND,  1, ST_TABLE,   { I_DIAMOND, I_STICK, 0 },       { 3, 2, 0 } },
    { I_SWORD_DIAMOND,1, ST_TABLE,   { I_DIAMOND, I_STICK, 0 },       { 2, 1, 0 } },
    { B_BEACON,       1, ST_TABLE,   { I_DIAMOND, I_GOLD_INGOT, B_GLASS }, { 3, 2, 1 } },
};
const int g_recipe_count = (int)(sizeof g_recipes / sizeof g_recipes[0]);

bool craft_station_ok(const Recipe *r, int near_mask)
{
    if (r->station == ST_HAND) return true;
    if (r->station == ST_TABLE) return (near_mask & NEAR_TABLE) != 0;
    return (near_mask & NEAR_FURNACE) != 0;
}

bool craft_has_items(const Recipe *r)
{
    for (int i = 0; i < 3; i++)
        if (r->in_id[i] && inv_count(r->in_id[i]) < r->in_n[i]) return false;
    return true;
}

bool craft_make(const Recipe *r, int near_mask)
{
    if (!craft_station_ok(r, near_mask) || !craft_has_items(r)) return false;
    if (inv_space(r->out) < r->out_n) return false;
    for (int i = 0; i < 3; i++)
        if (r->in_id[i]) inv_take(r->in_id[i], r->in_n[i]);
    inv_add(r->out, r->out_n);
    return true;
}
