#include "game.h"
#include "assets.h"

/* lets emulators and flash carts detect battery RAM */
const char sram_marker[] __attribute__((used)) = "SRAM_V113";

#define SAVE_MAGIC0 'E'
#define SAVE_MAGIC1 'M'
#define SAVE_MAGIC2 'B'
#define SAVE_MAGIC3 'V'
#define SAVE_VER 2
#define SAVE_LEN (6 + 1 + 1 + 4 + 2 + 2 + 2 + 2 + ITEM_COUNT + 3 + 4 + 4 + 1 + 1 + 1 + 4 + 2)

int save_area, save_tx, save_ty;

static void put8(u8 *b, int *i, u32 v) { b[(*i)++] = (u8)v; }
static void put16(u8 *b, int *i, u32 v) { put8(b, i, v); put8(b, i, v >> 8); }
static void put32(u8 *b, int *i, u32 v) { put16(b, i, v); put16(b, i, v >> 16); }
static u32 get8(const u8 *b, int *i) { return b[(*i)++]; }
static u32 get16(const u8 *b, int *i) { u32 a = get8(b, i); return a | (get8(b, i) << 8); }
static u32 get32(const u8 *b, int *i) { u32 a = get16(b, i); return a | (get16(b, i) << 16); }

int save_exists(void) {
    return SRAM8[0] == SAVE_MAGIC0 && SRAM8[1] == SAVE_MAGIC1 && SRAM8[2] == SAVE_MAGIC2 && SRAM8[3] == SAVE_MAGIC3 && SRAM8[4] == SAVE_VER;
}

int has_save(void) { return save_exists(); }

void save_write(void) {
    u8 b[SAVE_LEN + 8];
    int i = 0;
    put8(b, &i, SAVE_MAGIC0);
    put8(b, &i, SAVE_MAGIC1);
    put8(b, &i, SAVE_MAGIC2);
    put8(b, &i, SAVE_MAGIC3);
    put8(b, &i, SAVE_VER);
    put8(b, &i, 0);
    put8(b, &i, pl.cls);
    put8(b, &i, pl.level);
    put32(b, &i, pl.xp);
    put16(b, &i, pl.gold);
    put16(b, &i, (u16)pl.hp);
    put16(b, &i, (u16)pl.mp);
    put16(b, &i, 0);
    for (int k = 0; k < ITEM_COUNT; k++) put8(b, &i, pl.inv[k]);
    for (int k = 0; k < 3; k++) put8(b, &i, pl.equip[k]);
    put32(b, &i, gflags);
    put32(b, &i, gchests);
    put8(b, &i, (u32)cur_area_id);
    put8(b, &i, (u32)((pl.x >> 8) >> 4));
    put8(b, &i, (u32)(((pl.y >> 8) - 1) >> 4));
    put32(b, &i, pl.playtime);
    int sum = 0;
    for (int k = 0; k < i; k++) sum += b[k];
    put16(b, &i, (u32)sum);
    for (int k = 0; k < i; k++) SRAM8[k] = b[k];
}

int save_read(void) {
    if (!save_exists()) return 0;
    u8 b[SAVE_LEN + 8];
    for (int k = 0; k < SAVE_LEN; k++) b[k] = SRAM8[k];
    int i = SAVE_LEN - 2;
    int sum = 0;
    for (int k = 0; k < SAVE_LEN - 2; k++) sum += b[k];
    int stored = (int)get16(b, &i);
    if ((sum & 0xFFFF) != stored) return 0;
    i = 6;
    u32 cls = get8(b, &i);
    u32 lvl = get8(b, &i);
    if (cls > 3 || lvl < 1 || lvl > 20) return 0;
    player_new((int)cls);
    pl.level = (u8)lvl;
    pl.xp = get32(b, &i);
    pl.gold = (u16)get16(b, &i);
    s16 hp = (s16)get16(b, &i);
    s16 mp = (s16)get16(b, &i);
    get16(b, &i);
    for (int k = 0; k < ITEM_COUNT; k++) pl.inv[k] = (u8)get8(b, &i);
    for (int k = 0; k < 3; k++) pl.equip[k] = (u8)get8(b, &i);
    gflags = get32(b, &i);
    gchests = get32(b, &i);
    save_area = (int)get8(b, &i);
    save_tx = (int)get8(b, &i);
    save_ty = (int)get8(b, &i);
    pl.playtime = get32(b, &i);
    player_recalc();
    pl.hp = hp > 0 ? (hp > pl.maxhp ? pl.maxhp : hp) : pl.maxhp;
    pl.mp = mp < 0 ? 0 : (mp > pl.maxmp ? pl.maxmp : mp);
    pl.hp_trail = pl.hp;
    if (save_area < 0 || save_area > 2) save_area = 0;
    return 1;
}

void save_erase(void) {
    SRAM8[0] = 0;
}
