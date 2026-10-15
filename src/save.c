#include "save.h"
#include "game.h"
#include "hw.h"
#include "inventory.h"
#include "mobs.h"
#include "player.h"
#include "world.h"

/* The emulator / flash cart scans the ROM for this string to pick the save type. */
__attribute__((used)) static const char s_sram_id[] = "SRAM_V113";

#define SRAM_SIZE    32768
#define HEADER_SIZE  8
#define FIXED_SIZE   92          /* payload bytes before the edit list */
#define EDIT_BYTES   3
#define MAX_EDITS    ((SRAM_SIZE - HEADER_SIZE - FIXED_SIZE) / EDIT_BYTES)
#define SAVE_VERSION 1

typedef struct { u32 off; u16 sum; } Stream;

static volatile u8 *sram(void) { return (volatile u8 *)HW_SRAM; }

static void mix(Stream *s, u8 v)
{
    s->sum = (u16)(((s->sum << 1) | (s->sum >> 15)) ^ v);
}

static void put8(Stream *s, u8 v)
{
    sram()[s->off++] = v;
    mix(s, v);
}

static void put16(Stream *s, u16 v) { put8(s, (u8)v); put8(s, (u8)(v >> 8)); }
static void put32(Stream *s, u32 v) { put16(s, (u16)v); put16(s, (u16)(v >> 16)); }

static u8 get8(Stream *s)
{
    u8 v = sram()[s->off++];
    mix(s, v);
    return v;
}

static u16 get16(Stream *s) { u16 lo = get8(s); return (u16)(lo | (get8(s) << 8)); }
static u32 get32(Stream *s) { u32 lo = get16(s); return lo | ((u32)get16(s) << 16); }

/* Checks magic, version and checksum. */
static bool verify(void)
{
    volatile u8 *m = sram();
    if (m[0] != 'E' || m[1] != 'M' || m[2] != 'B' || m[3] != '2' || m[4] != SAVE_VERSION) return false;
    u16 stored = (u16)(m[6] | (m[7] << 8));

    Stream s = { HEADER_SIZE, 0 };
    for (int i = 0; i < FIXED_SIZE - 2; i++) get8(&s);
    u16 count = get16(&s);
    if (count > MAX_EDITS) return false;
    for (int i = 0; i < count * EDIT_BYTES; i++) get8(&s);
    return s.sum == stored;
}

bool save_exists(void)
{
    return verify();
}

static int count_edits(void)
{
    int c = 0;
    for (int i = 0; i < (int)sizeof g_world_mod; i++) {
        u8 v = g_world_mod[i];
        while (v) { v &= (u8)(v - 1); c++; }
    }
    return c;
}

bool save_write(void)
{
    int count = count_edits();
    bool complete = true;
    if (count > MAX_EDITS) { count = MAX_EDITS; complete = false; }

    Stream s = { HEADER_SIZE, 0 };
    put32(&s, g_world_seed);
    put16(&s, g_game.tod);
    put16(&s, g_game.adv);
    put16(&s, g_game.placed);
    put8(&s, g_game.won);
    put8(&s, g_pl.sel);
    put8(&s, (u8)g_pl.x);
    put8(&s, (u8)g_pl.y);
    put8(&s, (u8)g_pl.z);
    put8(&s, g_pl.dir);
    put8(&s, (u8)g_pl.hp);
    put8(&s, (u8)g_pl.food);
    for (int i = 0; i < INV_SLOTS; i++) {
        put8(&s, g_inv[i].id);
        put8(&s, g_inv[i].count);
        put8(&s, g_inv[i].dur);
    }
    put16(&s, (u16)count);

    const u8 *cells = &g_world[0][0][0];
    int written = 0;
    for (int idx = 0; idx < WX * WY * WZ && written < count; idx++) {
        if (!(g_world_mod[idx >> 3] & (1u << (idx & 7)))) continue;
        put16(&s, (u16)idx);
        put8(&s, cells[idx]);
        written++;
    }

    volatile u8 *m = sram();
    m[0] = 'E'; m[1] = 'M'; m[2] = 'B'; m[3] = '2';
    m[4] = SAVE_VERSION; m[5] = 0;
    m[6] = (u8)s.sum; m[7] = (u8)(s.sum >> 8);
    return complete;
}

bool save_load(void)
{
    if (!verify()) return false;

    Stream s = { HEADER_SIZE, 0 };
    u32 seed = get32(&s);
    u16 tod = get16(&s), adv = get16(&s), placed = get16(&s);
    u8 won = get8(&s), sel = get8(&s);
    int px = get8(&s), py = get8(&s), pz = get8(&s), dir = get8(&s);
    int hp = get8(&s), food = get8(&s);
    Slot inv[INV_SLOTS];
    for (int i = 0; i < INV_SLOTS; i++) {
        inv[i].id = get8(&s);
        inv[i].count = get8(&s);
        inv[i].dur = get8(&s);
    }
    int count = get16(&s);

    world_generate(seed);
    for (int i = 0; i < count; i++) {
        int idx = get16(&s);
        u8 b = get8(&s);
        world_edit_raw(idx % WX, (idx / WX) % WY, idx / (WX * WY), b);
    }
    world_rebuild_columns();
    g_world_dirty = true;

    for (int i = 0; i < INV_SLOTS; i++) g_inv[i] = inv[i];
    player_reset(px, py, pz);
    g_pl.dir = (u8)(dir & 3);
    g_pl.hp = (s16)(hp < 1 ? 1 : hp);
    g_pl.food = (s16)food;
    g_pl.sel = (u8)(sel % HOTBAR_SLOTS);
    g_game.tod = tod;
    g_game.adv = adv;
    g_game.placed = placed;
    g_game.won = won;
    mobs_clear();
    mobs_populate();
    return true;
}
