#include "game.h"
#include "assets.h"

/* lets emulators and flash carts detect battery RAM */
const char sram_marker[] __attribute__((used)) = "SRAM_V113";

#define SAVE_MAGIC0 'E'
#define SAVE_MAGIC1 'M'
#define SAVE_MAGIC2 'B'
#define SAVE_MAGIC3 'V'
/* Format history:
     2  the original layout (26 items, 3 gear slots)
     3  stat points and the helm / boots slots
     4  extension block before the checksum: auto-equip option, passive points + ranks, the trophy / cinder items added
        after format 3, and quest state. Every older save still loads (missing parts get sensible defaults). */
#define SAVE_VER 4
#define SAVE_VER_V3 3
#define SAVE_VER_LEGACY 2
#define SAVE_FIXED (6 + 1 + 1 + 4 + 2 + 2 + 2 + 2)                 /* header .. reserved word */
#define SAVE_TAIL (4 + 4 + 1 + 1 + 1 + 4 + 2)                      /* flags, chests, area, tile x/y, playtime, checksum */
#define SAVE_EXT (1 + 1 + PASS_COUNT + (ITEM_COUNT - ITEM_COUNT_V3) + QUEST_COUNT * 2)
#define SAVE_LEN_V3 (SAVE_FIXED + ITEM_COUNT_V3 + SLOT_COUNT + STAT_COUNT + 1 + SAVE_TAIL)
#define SAVE_LEN (SAVE_LEN_V3 + SAVE_EXT)
#define SAVE_LEN_V2 (SAVE_FIXED + ITEM_COUNT_V2 + 3 + SAVE_TAIL)

int save_area, save_tx, save_ty;

static void put8(u8 *b, int *i, u32 v) { b[(*i)++] = (u8)v; }
static void put16(u8 *b, int *i, u32 v) { put8(b, i, v); put8(b, i, v >> 8); }
static void put32(u8 *b, int *i, u32 v) { put16(b, i, v); put16(b, i, v >> 16); }
static u32 get8(const u8 *b, int *i) { return b[(*i)++]; }
static u32 get16(const u8 *b, int *i) { u32 a = get8(b, i); return a | (get8(b, i) << 8); }
static u32 get32(const u8 *b, int *i) { u32 a = get16(b, i); return a | (get16(b, i) << 16); }

static int save_version(void) {
    if (SRAM8[0] == SAVE_MAGIC0 && SRAM8[1] == SAVE_MAGIC1 && SRAM8[2] == SAVE_MAGIC2 && SRAM8[3] == SAVE_MAGIC3) {
        if (SRAM8[4] == SAVE_VER) return SAVE_VER;
        if (SRAM8[4] == SAVE_VER_V3) return SAVE_VER_V3;
        if (SRAM8[4] == SAVE_VER_LEGACY) return SAVE_VER_LEGACY;
    }
    return 0;
}

int save_exists(void) { return save_version() != 0; }

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
    for (int k = 0; k < ITEM_COUNT_V3; k++) put8(b, &i, pl.inv[k]);
    for (int k = 0; k < SLOT_COUNT; k++) put8(b, &i, pl.equip[k]);
    for (int k = 0; k < STAT_COUNT; k++) put8(b, &i, pl.pts[k]);
    put8(b, &i, pl.stat_pts);
    put32(b, &i, gflags);
    put32(b, &i, gchests);
    put8(b, &i, (u32)cur_area_id);
    put8(b, &i, (u32)((pl.x >> 8) >> 4));
    put8(b, &i, (u32)(((pl.y >> 8) - 1) >> 4));
    put32(b, &i, pl.playtime);
    /* ---- format 4 extension */
    put8(b, &i, pl.auto_equip);
    put8(b, &i, pl.pass_pts);
    for (int k = 0; k < PASS_COUNT; k++) put8(b, &i, pl.pass[k]);
    for (int k = ITEM_COUNT_V3; k < ITEM_COUNT; k++) put8(b, &i, pl.inv[k]);
    for (int k = 0; k < QUEST_COUNT; k++) { put8(b, &i, quest_state[k]); put8(b, &i, quest_prog[k]); }
    int sum = 0;
    for (int k = 0; k < i; k++) sum += b[k];
    put16(b, &i, (u32)sum);
    for (int k = 0; k < i; k++) SRAM8[k] = b[k];
}

int save_read(void) {
    int ver = save_version();
    if (!ver) return 0;
    int len = ver == SAVE_VER ? SAVE_LEN : (ver == SAVE_VER_V3 ? SAVE_LEN_V3 : SAVE_LEN_V2);
    int n_items = ver >= SAVE_VER_V3 ? ITEM_COUNT_V3 : ITEM_COUNT_V2;
    int n_equip = ver >= SAVE_VER_V3 ? SLOT_COUNT : 3;
    u8 b[SAVE_LEN + 8];
    for (int k = 0; k < len; k++) b[k] = SRAM8[k];
    int i = len - 2;
    int sum = 0;
    for (int k = 0; k < len - 2; k++) sum += b[k];
    int stored = (int)get16(b, &i);
    if ((sum & 0xFFFF) != stored) return 0;
    i = 6;
    u32 cls = get8(b, &i);
    u32 lvl = get8(b, &i);
    if (cls > 3 || lvl < 1 || lvl > LEVEL_CAP) return 0;
    player_new((int)cls);
    pl.level = (u8)lvl;
    pl.xp = get32(b, &i);
    pl.gold = (u16)get16(b, &i);
    s16 hp = (s16)get16(b, &i);
    s16 mp = (s16)get16(b, &i);
    get16(b, &i);
    for (int k = 0; k < ITEM_COUNT; k++) pl.inv[k] = 0;
    for (int k = 0; k < n_items; k++) pl.inv[k] = (u8)get8(b, &i);
    for (int k = 0; k < SLOT_COUNT; k++) pl.equip[k] = 0;
    for (int k = 0; k < n_equip; k++) pl.equip[k] = (u8)get8(b, &i);
    if (ver >= SAVE_VER_V3) {
        for (int k = 0; k < STAT_COUNT; k++) pl.pts[k] = (u8)get8(b, &i);
        pl.stat_pts = (u8)get8(b, &i);
    } else {
        /* older save: every level already earned grants its points, all unspent */
        pl.stat_pts = (u8)((pl.level - 1) * STAT_PTS_PER_LEVEL);
    }
    gflags = get32(b, &i);
    gchests = get32(b, &i);
    save_area = (int)get8(b, &i);
    save_tx = (int)get8(b, &i);
    save_ty = (int)get8(b, &i);
    pl.playtime = get32(b, &i);
    if (ver == SAVE_VER) {
        pl.auto_equip = (u8)(get8(b, &i) ? 1 : 0);
        pl.pass_pts = (u8)get8(b, &i);
        for (int k = 0; k < PASS_COUNT; k++) pl.pass[k] = (u8)imin((int)get8(b, &i), pass_max(k));
        for (int k = ITEM_COUNT_V3; k < ITEM_COUNT; k++) pl.inv[k] = (u8)get8(b, &i);
        for (int k = 0; k < QUEST_COUNT; k++) {
            quest_state[k] = (u8)imin((int)get8(b, &i), 3);
            quest_prog[k] = (u8)get8(b, &i);
        }
    } else {
        pl.pass_pts = (u8)pass_points_for(1, pl.level);        /* older saves earn their passive points retroactively */
    }
    /* never trust equipment ids from SRAM: each must fit its slot (and the weapon its class) */
    for (int k = 0; k < SLOT_COUNT; k++) {
        int it = pl.equip[k];
        int bad = it >= ITEM_COUNT || (it && item_defs[it].slot != k) || (k == SLOT_WEAPON && (!it || item_defs[it].cls != pl.cls));
        if (bad) pl.equip[k] = (u8)(k == SLOT_WEAPON ? item_weapon(pl.cls, 0) : 0);
    }
    if (pl.inv[pl.equip[SLOT_WEAPON]] == 0) pl.inv[pl.equip[SLOT_WEAPON]] = 1;
    player_recalc();
    pl.hp = hp > 0 ? (hp > pl.maxhp ? pl.maxhp : hp) : pl.maxhp;
    pl.mp = mp < 0 ? 0 : (mp > pl.maxmp ? pl.maxmp : mp);
    pl.hp_trail = pl.hp;
    if (save_area < 0 || save_area > 4) save_area = 0;
    return 1;
}

void save_erase(void) {
    SRAM8[0] = 0;
}
