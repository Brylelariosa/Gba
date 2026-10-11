/* Host-side test harness (not part of the ROM build). Compile with -DHOST_BUILD. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/game.h"
#include "../src/assets.h"

u8 g_io[0x400] __attribute__((aligned(4)));
u8 g_pal[0x400] __attribute__((aligned(4)));
u8 g_vram[0x18000] __attribute__((aligned(4)));
u8 g_oam[0x400] __attribute__((aligned(4)));
u8 g_sram[0x10000] __attribute__((aligned(4)));

extern int boss_dead_flag;

static void dump(const char *prefix, int frame) {
    char name[256];
    snprintf(name, sizeof name, "%s_%05d.bin", prefix, frame);
    FILE *f = fopen(name, "wb");
    if (!f) return;
    fwrite(g_io, 1, 0x400, f);
    fwrite(g_pal, 1, 0x400, f);
    fwrite(g_vram, 1, 0x18000, f);
    fwrite(g_oam, 1, 0x400, f);
    fclose(f);
}

typedef struct { int frame; char cmd[16]; int a, b, c; } Ev;


/* ---- save format tests: v3 round trip, v2 migration, corruption detection */
extern int save_area, save_tx, save_ty;
static int sv_fail;
#define CHECK(c, msg) do { if (!(c)) { printf("SAVE TEST FAIL: %s\n", msg); sv_fail++; } } while (0)
static void run_save_tests(void) {
    /* v3 round trip */
    player_new(CLS_ROGUE);
    pl.level = 9; pl.xp = 77; pl.gold = 1234;
    pl.pts[STAT_DEX] = 7; pl.pts[STAT_LUK] = 3; pl.stat_pts = 5;
    pl.inv[ITEM_DAGGER2] = 1; pl.inv[ITEM_HELM2] = 1; pl.inv[ITEM_BOOTS3] = 1; pl.inv[ITEM_SKULL_KEY] = 1;
    pl.equip[SLOT_WEAPON] = ITEM_DAGGER2; pl.equip[SLOT_HELM] = ITEM_HELM2; pl.equip[SLOT_BOOTS] = ITEM_BOOTS3;
    gflags = 0x55; gchests = 0x1234;
    player_recalc(); pl.hp = 33; pl.mp = 4;
    int hp = pl.hp;
    save_write();
    player_new(CLS_KNIGHT);
    CHECK(save_read(), "v3 read");
    CHECK(pl.cls == CLS_ROGUE && pl.level == 9 && pl.xp == 77 && pl.gold == 1234, "v3 basics");
    CHECK(pl.pts[STAT_DEX] == 7 && pl.pts[STAT_LUK] == 3 && pl.stat_pts == 5, "v3 stat points");
    CHECK(pl.equip[SLOT_HELM] == ITEM_HELM2 && pl.equip[SLOT_BOOTS] == ITEM_BOOTS3 && pl.equip[SLOT_WEAPON] == ITEM_DAGGER2, "v3 equipment");
    CHECK(pl.inv[ITEM_SKULL_KEY] == 1 && gflags == 0x55 && gchests == 0x1234, "v3 inventory and flags");
    CHECK(pl.hp == hp, "v3 hp");
    /* corruption is rejected */
    SRAM8[20] ^= 0x40;
    CHECK(!save_read(), "corrupted save rejected");
    SRAM8[20] ^= 0x40;
    CHECK(save_read(), "restored save accepted");
    /* v2 (older ROM) migration: header, 26 items, 3 equip slots */
    {
        unsigned char b[200];
        int i = 0;
        b[i++] = 'E'; b[i++] = 'M'; b[i++] = 'B'; b[i++] = 'V'; b[i++] = 2; b[i++] = 0;
        b[i++] = CLS_KNIGHT; b[i++] = 7;
        b[i++] = 100; b[i++] = 0; b[i++] = 0; b[i++] = 0;                 /* xp */
        b[i++] = 0x2c; b[i++] = 0x01;                                      /* gold 300 */
        b[i++] = 60; b[i++] = 0; b[i++] = 5; b[i++] = 0; b[i++] = 0; b[i++] = 0;
        for (int k = 0; k < ITEM_COUNT_V2; k++) b[i++] = (unsigned char)(k == ITEM_POTION ? 4 : (k == ITEM_SWORD2 ? 1 : 0));
        b[i++] = ITEM_SWORD2; b[i++] = ITEM_ARMOR2; b[i++] = ITEM_CHARM_LUCK;
        b[i++] = 2; b[i++] = 0; b[i++] = 0; b[i++] = 0;                    /* gflags */
        b[i++] = 0; b[i++] = 0; b[i++] = 0; b[i++] = 0;                    /* chests */
        b[i++] = 1; b[i++] = 10; b[i++] = 12;                              /* area, tile */
        b[i++] = 1; b[i++] = 2; b[i++] = 0; b[i++] = 0;                    /* playtime */
        int sum = 0;
        for (int k = 0; k < i; k++) sum += b[k];
        b[i++] = (unsigned char)(sum & 255); b[i++] = (unsigned char)(sum >> 8);
        for (int k = 0; k < i; k++) SRAM8[k] = b[k];
        player_new(CLS_MAGE);
        CHECK(save_read(), "v2 read");
        CHECK(pl.cls == CLS_KNIGHT && pl.level == 7 && pl.gold == 300, "v2 basics");
        CHECK(pl.stat_pts == 18, "v2 grants stat points retroactively");
        CHECK(pl.equip[SLOT_WEAPON] == ITEM_SWORD2 && pl.equip[SLOT_ARMOR] == ITEM_ARMOR2 && pl.equip[SLOT_CHARM] == ITEM_CHARM_LUCK, "v2 equipment");
        CHECK(pl.equip[SLOT_HELM] == 0 && pl.equip[SLOT_BOOTS] == 0, "v2 new slots empty");
        CHECK(pl.inv[ITEM_POTION] == 4 && save_area == 1 && save_tx == 10 && save_ty == 12, "v2 inventory and position");
    }
    printf("save tests: %s\n", sv_fail ? "FAILED" : "all passed");
}

int main(int argc, char **argv) {
    for (int i = 1; i < ITEM_COUNT; i++) if (!item_defs[i].name || !item_defs[i].desc) { fprintf(stderr, "item %d has no definition\n", i); return 3; }

    if (argc < 3) { fprintf(stderr, "usage: sim script prefix [frames]\n"); return 1; }
    FILE *f = fopen(argv[1], "r");
    if (!f) { perror("script"); return 1; }
    static Ev evs[4096];
    int nev = 0;
    char line[256];
    int maxframe = 0;
    while (fgets(line, sizeof line, f)) {
        Ev e;
        memset(&e, 0, sizeof e);
        if (line[0] == '#' || line[0] == '\n') continue;
        int n = sscanf(line, "%d %15s %i %i %i", &e.frame, e.cmd, &e.a, &e.b, &e.c);
        if (n < 2) continue;
        evs[nev++] = e;
        if (e.frame > maxframe) maxframe = e.frame;
    }
    fclose(f);
    if (argc > 3) maxframe = atoi(argv[3]);
    game_init();
    int keys = 0;
    int god = 0, trace = 0;
    for (int fr = 0; fr <= maxframe; fr++) {
        int shot = 0;
        for (int i = 0; i < nev; i++) {
            if (evs[i].frame != fr) continue;
            Ev *e = &evs[i];
            if (!strcmp(e->cmd, "keys")) keys = e->a;
            else if (!strcmp(e->cmd, "shot")) shot = 1;
            else if (!strcmp(e->cmd, "newgame")) begin_new_game(e->a);
            else if (!strcmp(e->cmd, "trace")) trace = e->a;
            else if (!strcmp(e->cmd, "seed")) rnd_seed((u32)e->a);
            else if (!strcmp(e->cmd, "warp")) { if (game_state == ST_PLAY) start_transition(e->a, e->b, e->c, DIR_DOWN); }
            else if (!strcmp(e->cmd, "xp")) player_add_xp(e->a);
            else if (!strcmp(e->cmd, "hp")) pl.hp = (s16)e->a;
            else if (!strcmp(e->cmd, "gold")) pl.gold = (u16)e->a;
            else if (!strcmp(e->cmd, "god")) god = e->a;
            else if (!strcmp(e->cmd, "give")) player_give(e->a, e->b);
            else if (!strcmp(e->cmd, "flags")) gflags |= (u32)e->a;
            else if (!strcmp(e->cmd, "place")) player_place(e->a, e->b, e->c);
            else if (!strcmp(e->cmd, "state")) game_state = e->a;
            else if (!strcmp(e->cmd, "spawn")) enemy_spawn(e->a, e->b, e->c);
            else if (!strcmp(e->cmd, "killall")) { for (int k = 0; k < MAX_EN; k++) if (en[k].active) en[k].active = 0; }
            else if (!strcmp(e->cmd, "bossdie")) { Enemy *b = enemy_boss(); if (b) { b->phase = 2; enemy_damage(b, 99999, 0, 1, 0, 0); } }
            else if (!strcmp(e->cmd, "shop")) { shop_open(); game_state = ST_SHOP; }
            else if (!strcmp(e->cmd, "dump")) {
                Enemy *b = enemy_big();
                int skel = 0, tot = 0;
                for (int k = 0; k < MAX_EN; k++) if (en[k].active) { tot++; if (en[k].type == EN_SKELETON) skel++; }
                printf("f%d player(%d,%d) hp=%d state=%d | ", fr, (int)(pl.x >> 8) >> 4, (int)(pl.y >> 8) >> 4, pl.hp, pl.state);
                if (b) printf("big type=%d state=%d phase=%d hp=%d/%d tile=(%d,%d) z=%d | ", b->type, b->state, b->phase, b->hp, b->maxhp, (int)(b->x >> 8) >> 4, (int)(b->y >> 8) >> 4, b->z >> 8);
                printf("enemies=%d skeletons=%d skullkey=%d gflags=%x\n", tot, skel, pl.inv[ITEM_SKULL_KEY], (unsigned)gflags);
            }
            else if (!strcmp(e->cmd, "killadds")) { for (int k = 0; k < MAX_EN; k++) if (en[k].active && en[k].type == EN_SKELETON) { en[k].hp = 0; en[k].dying = 1; en[k].t = 100; } }
            else if (!strcmp(e->cmd, "warddie")) { for (int k = 0; k < MAX_EN; k++) if (en[k].active && en[k].type == EN_WARDEN) { en[k].state = 7; enemy_damage(&en[k], 99999, 0, 1, 0, 0); } }
            else if (!strcmp(e->cmd, "savetest")) { run_save_tests(); }
            else if (!strcmp(e->cmd, "tileflags")) { printf("tile(%d,%d) flags=%d area=%d dir=%d\n", e->a, e->b, tile_flags(e->a, e->b), cur_area_id, pl.dir); }
            else if (!strcmp(e->cmd, "pts")) { pl.stat_pts = (u8)e->a; }
            else if (!strcmp(e->cmd, "level")) { while (pl.level < e->a) player_add_xp(xp_for_level(pl.level)); }
            else if (!strcmp(e->cmd, "equip")) { player_give(e->a, 1); player_equip(e->a); }
            else if (!strcmp(e->cmd, "aspd")) { pl.aspd = (s16)e->a; }
            else if (!strcmp(e->cmd, "bosshp")) { Enemy *b = enemy_big(); if (b) b->hp = (s16)e->a; }
        }
        *(volatile u16 *)(g_io + 0x130) = (u16)(~keys & 0x3FF);
        if (god && game_state == ST_PLAY) { pl.hp = pl.maxhp; pl.invuln = 5; }
        game_frame();
        if (shot) dump(argv[2], fr);
        if (trace) printf("f%d sq1=%04x sq2=%04x wv=%04x nz=%04x cnt=%04x\n", fr, *(u16 *)(g_io + 0x64), *(u16 *)(g_io + 0x6C), *(u16 *)(g_io + 0x74), *(u16 *)(g_io + 0x7C), *(u16 *)(g_io + 0x80));
    }
    printf("done: frames=%d state=%d area=%d hp=%d lvl=%d gold=%d\n", maxframe, game_state, cur_area_id, pl.hp, pl.level, pl.gold);
    return 0;
}
