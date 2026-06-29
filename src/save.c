#include "save.h"
#include "simulation.h"
#include "themes.h"
#include "renderer.h"
#include <string.h>

/* SRAM is mapped at 0x0E000000 on GBA */
#define SRAM ((vu8 *)0x0E000000)

/* Write/read bytes through volatile 8-bit SRAM */
static void sram_write(u32 addr, const void *src, u32 len) {
    const u8 *p = (const u8 *)src;
    for (u32 i = 0; i < len; i++)
        SRAM[addr + i] = p[i];
}

static void sram_read(u32 addr, void *dst, u32 len) {
    u8 *p = (u8 *)dst;
    for (u32 i = 0; i < len; i++)
        p[i] = SRAM[addr + i];
}

void save_init(void) { /* Nothing needed */ }

void save_write(int slot) {
    if (slot < 0 || slot >= SAVE_SLOTS) return;
    SaveSlot s;
    s.magic      = SAVE_MAGIC;
    memcpy(s.grid, g_sim.cur, GRID_SIZE);
    s.generation = g_sim.generation;
    s.population = g_sim.population;
    s.type       = (int)g_sim.type;
    s.birth_mask = g_sim.birth_mask;
    s.surv_mask  = g_sim.surv_mask;
    s.theme      = g_theme;
    s.speed      = g_sim.speed;
    u32 off = (u32)slot * sizeof(SaveSlot);
    sram_write(off, &s, sizeof(s));
}

int save_read(int slot) {
    if (slot < 0 || slot >= SAVE_SLOTS) return 0;
    SaveSlot s;
    u32 off = (u32)slot * sizeof(SaveSlot);
    sram_read(off, &s, sizeof(s));
    if (s.magic != SAVE_MAGIC) return 0;
    memcpy(g_sim.cur, s.grid, GRID_SIZE);
    g_sim.generation = s.generation;
    g_sim.population = s.population;
    g_sim.type       = (AutomataType)s.type;
    g_sim.birth_mask = s.birth_mask;
    g_sim.surv_mask  = s.surv_mask;
    g_sim.speed      = s.speed;
    themes_apply(s.theme);
    return 1;
}

int save_slot_used(int slot) {
    if (slot < 0 || slot >= SAVE_SLOTS) return 0;
    u32 magic = 0;
    sram_read((u32)slot * sizeof(SaveSlot), &magic, 4);
    return (magic == SAVE_MAGIC);
}

void save_draw_menu(int selected) {
    renderer_fill_rect(20, 20, 200, 120, PAL_UI_BG);
    renderer_draw_string(60, 24, "SAVE / LOAD", PAL_UI_TXT);
    for (int i = 0; i < SAVE_SLOTS; i++) {
        int y = 40 + i * 20;
        u8 col = (i == selected) ? PAL_UI_HI : PAL_UI_TXT;
        renderer_draw_string(30, y, "Slot", col);
        renderer_draw_int(58, y, i + 1, col);
        if (save_slot_used(i))
            renderer_draw_string(80, y, "[USED]", col);
        else
            renderer_draw_string(80, y, "[EMPTY]", col);
    }
    renderer_draw_string(30, 120, "A=Save  B=Load  Start=Back", PAL_DARK);
}