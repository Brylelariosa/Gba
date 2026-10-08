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

int main(int argc, char **argv) {
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
            else if (!strcmp(e->cmd, "bossdie")) { Enemy *b = enemy_boss(); if (b) enemy_damage(b, 9999, 0, 1, 0); }
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
