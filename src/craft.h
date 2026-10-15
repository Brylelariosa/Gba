#ifndef CRAFT_H
#define CRAFT_H

#include "common.h"

enum { ST_HAND = 0, ST_TABLE = 1, ST_FURNACE = 2 };
enum { NEAR_TABLE = 1, NEAR_FURNACE = 2 };

typedef struct {
    u8 out, out_n;
    u8 station;
    u8 in_id[3];
    u8 in_n[3];
} Recipe;

extern const Recipe g_recipes[];
extern const int g_recipe_count;

bool craft_station_ok(const Recipe *r, int near_mask);
bool craft_has_items(const Recipe *r);
bool craft_make(const Recipe *r, int near_mask);

#endif
