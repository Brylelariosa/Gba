#include "ui.h"
#include "input.h"
#include "renderer.h"
#include "simulation.h"
#include "rules.h"
#include "patterns.h"
#include "themes.h"
#include "save.h"
#include "benchmark.h"
#include "statistics.h"
#include "audio.h"
#include <string.h>

AppState g_state = STATE_MENU;

/* ── Main menu ───────────────────────────────────── */
static const char *MENU_ITEMS[] = {
    "New Simulation",
    "Pattern Library",
    "Rule Editor",
    "Saved Worlds",
    "Benchmark",
    "Educational Mode",
    "Settings",
    "Credits",
};
#define MAIN_MENU_COUNT 8

static int s_menu_sel      = 0;
static int s_pattern_sel   = 0;
static int s_rule_cursor   = 0;    /* 0=birth 1=surv 2=type 3=wrap */
static int s_save_sel      = 0;
static int s_theme_sel     = 0;
static int s_edu_page      = 0;
static int s_automata_sel  = 0;
static int s_stats_visible = 0;
static int s_show_help     = 0;

/* ── Education texts ─────────────────────────────── */
static const char *EDU_TITLES[] = {
    "Neighborhoods",
    "Birth Rules",
    "Survival Rules",
    "Oscillators",
    "Still Lifes",
    "Spaceships",
    "Glider Guns",
    "Wireworld",
    "Brians Brain",
    "Langtons Ant",
};
#define EDU_PAGES 10

static const char *EDU_LINES[][4] = {
    { "Moore neighborhood:", "8 cells around each", "cell are checked each", "generation." },
    { "B=cells born when", "exactly N neighbors", "are alive. B3=alive", "with 3 neighbors." },
    { "S=cells survive when", "N neighbors alive.", "S23=survive with", "2 or 3 neighbors." },
    { "Oscillators repeat", "after N steps.", "Blinker period 2,", "Pulsar period 3." },
    { "Still lifes are", "patterns that do not", "change. Block and", "Beehive are common." },
    { "Spaceships move", "across the grid.", "LWSS is the smallest", "natural spaceship." },
    { "Glider guns produce", "gliders endlessly.", "Gosper gun fires", "every 30 steps." },
    { "4 states: Empty,", "Wire, Head, Tail.", "Electrons travel", "along wires." },
    { "3 states: Off,On,", "Dying. A cell fires", "with exactly 2 ON", "neighbors." },
    { "Ant turns right on", "white, left on black,", "flips cell, moves.", "Creates highway." },
};

/* ── Theme names ─────────────────────────────────── */
static const char *THEME_NAMES[] = {
    "Classic Green",
    "Monochrome",
    "Fire",
    "Ice",
    "Matrix",
    "Thermal",
    "Rainbow",
    "Retro LCD",
};

/* ─────────────────────────────────────────────────── */
void ui_init(void) {
    g_state    = STATE_MENU;
    s_menu_sel = 0;
}

/* ── Draw helpers ────────────────────────────────── */
static void draw_border(int x, int y, int w, int h, u8 col) {
    renderer_fill_rect(x,       y,       w, 1, col);
    renderer_fill_rect(x,       y+h-1,   w, 1, col);
    renderer_fill_rect(x,       y,       1, h, col);
    renderer_fill_rect(x+w-1,   y,       1, h, col);
}

static void draw_panel(int x, int y, int w, int h) {
    renderer_fill_rect(x, y, w, h, PAL_UI_BG);
    draw_border(x, y, w, h, PAL_BORDER);
}

/* ── Main menu draw ──────────────────────────────── */
static void draw_main_menu(void) {
    renderer_fill_rect(0, 0, SCREEN_W, SCREEN_H, PAL_DEAD);
    draw_panel(20, 10, 200, 140);

    renderer_draw_string(55, 16, "CELLULAR AUTOMATA LAB", PAL_UI_HI);
    renderer_fill_rect(20, 26, 200, 1, PAL_BORDER);

    for (int i = 0; i < MAIN_MENU_COUNT; i++) {
        int y = 32 + i * 13;
        if (i == s_menu_sel) {
            renderer_fill_rect(22, y - 1, 196, 11, PAL_UI_HI);
            renderer_draw_string(28, y, MENU_ITEMS[i], PAL_DEAD);
        } else {
            renderer_draw_string(28, y, MENU_ITEMS[i], PAL_UI_TXT);
        }
    }

    renderer_draw_string(30, 148, "D-Pad:navigate  A:select", PAL_DARK);
}

/* ── Sim draw overlay ────────────────────────────── */
static void draw_sim_help(void) {
    draw_panel(4, 4, 120, 70);
    renderer_draw_string(8,  8,  "D-Pad: move cursor",  PAL_UI_TXT);
    renderer_draw_string(8, 16,  "A: place cell",        PAL_UI_TXT);
    renderer_draw_string(8, 24,  "B: erase cell",        PAL_UI_TXT);
    renderer_draw_string(8, 32,  "L: randomize",         PAL_UI_TXT);
    renderer_draw_string(8, 40,  "R: pattern menu",      PAL_UI_TXT);
    renderer_draw_string(8, 48,  "Start: pause",         PAL_UI_TXT);
    renderer_draw_string(8, 56,  "Select: options",      PAL_UI_TXT);
    renderer_draw_string(8, 64,  "A+B: clear",           PAL_UI_TXT);
}

/* ── Pattern library ─────────────────────────────── */
static void draw_pattern_lib(void) {
    draw_panel(4, 4, SCREEN_W - 8, SCREEN_H - 8);
    renderer_draw_string(50, 8, "PATTERN LIBRARY", PAL_UI_HI);

    int items_per_page = 9;
    int page_start = (s_pattern_sel / items_per_page) * items_per_page;
    for (int i = 0; i < items_per_page; i++) {
        int idx = page_start + i;
        if (idx >= PATTERN_COUNT) break;
        int y = 22 + i * 12;
        u8 col = (idx == s_pattern_sel) ? PAL_UI_HI : PAL_UI_TXT;
        renderer_draw_string(14, y, PATTERNS[idx].name, col);
        renderer_draw_int(140, y, PATTERNS[idx].w, PAL_DARK);
        renderer_draw_char(154, y, 'x', PAL_DARK);
        renderer_draw_int(162, y, PATTERNS[idx].h, PAL_DARK);
    }
    renderer_draw_string(14, 142, "A:place  B:back  D-Pad:choose", PAL_DARK);
}

/* ── Rule editor ─────────────────────────────────── */
static void draw_rule_editor(void) {
    draw_panel(4, 4, SCREEN_W - 8, SCREEN_H - 8);
    renderer_draw_string(60, 8, "RULE EDITOR", PAL_UI_HI);

    /* Automata selector */
    renderer_draw_string(14, 22, "Automata:", PAL_UI_TXT);
    u8 ac = (s_rule_cursor == 0) ? PAL_UI_HI : PAL_UI_TXT;
    renderer_draw_string(80, 22, RULE_NAMES[g_sim.type], ac);

    /* Birth mask */
    renderer_draw_string(14, 36, "Birth:", PAL_UI_TXT);
    for (int i = 0; i <= 8; i++) {
        int bx = 60 + i * 14;
        u8 lit = (g_sim.birth_mask >> i) & 1;
        renderer_draw_char(bx, 36, (char)('0'+i), lit ? PAL_ALIVE : PAL_DARK);
        if (lit) renderer_fill_rect(bx, 44, 5, 1, PAL_ALIVE);
    }

    /* Surv mask */
    renderer_draw_string(14, 52, "Surv:", PAL_UI_TXT);
    for (int i = 0; i <= 8; i++) {
        int bx = 60 + i * 14;
        u8 lit = (g_sim.surv_mask >> i) & 1;
        renderer_draw_char(bx, 52, (char)('0'+i), lit ? PAL_STATE2 : PAL_DARK);
        if (lit) renderer_fill_rect(bx, 60, 5, 1, PAL_STATE2);
    }

    /* Wrap */
    renderer_draw_string(14, 68, "Wrap:", PAL_UI_TXT);
    renderer_draw_string(60, 68, g_sim.wrap ? "ON " : "OFF", PAL_UI_TXT);

    /* BS string */
    char bs[32];
    rule_get_bs_string(bs, sizeof(bs));
    renderer_draw_string(14, 82, "Rule: ", PAL_DARK);
    renderer_draw_string(60, 82, bs, PAL_ALIVE);

    renderer_draw_string(14, 100, "L/R:change A/B:toggle", PAL_DARK);
    renderer_draw_string(14, 112, "Start:apply  Select:back", PAL_DARK);
    renderer_draw_string(14, 126, "Rules: birth column 0-8", PAL_DARK);
    renderer_draw_string(14, 136, "       surv  column 0-8", PAL_DARK);
}

/* ── Education ───────────────────────────────────── */
static void draw_education(void) {
    draw_panel(4, 4, SCREEN_W - 8, SCREEN_H - 8);
    renderer_draw_string(60, 8, "EDUCATION", PAL_UI_HI);

    renderer_draw_string(14, 22, EDU_TITLES[s_edu_page], PAL_CYAN);
    for (int i = 0; i < 4; i++)
        renderer_draw_string(14, 36 + i * 14, EDU_LINES[s_edu_page][i], PAL_UI_TXT);

    renderer_draw_int(200, 8, s_edu_page + 1, PAL_DARK);
    renderer_draw_char(212, 8, '/', PAL_DARK);
    renderer_draw_int(220, 8, EDU_PAGES, PAL_DARK);

    renderer_draw_string(14, 120, "L/R:prev/next  B:back", PAL_DARK);
}

/* ── Settings ────────────────────────────────────── */
static void draw_settings(void) {
    draw_panel(4, 4, SCREEN_W - 8, SCREEN_H - 8);
    renderer_draw_string(65, 8, "SETTINGS", PAL_UI_HI);

    renderer_draw_string(14, 28, "Theme:", PAL_UI_TXT);
    renderer_draw_string(70, 28, THEME_NAMES[s_theme_sel], PAL_ALIVE);

    renderer_draw_string(14, 44, "Speed:", PAL_UI_TXT);
    renderer_draw_int(70, 44, g_sim.speed, PAL_ALIVE);
    renderer_draw_string(86, 44, "steps/frm", PAL_DARK);

    renderer_draw_string(14, 60, "Automata:", PAL_UI_TXT);
    renderer_draw_string(80, 60, RULE_NAMES[g_sim.type], PAL_ALIVE);

    renderer_draw_string(14, 76, "Wrap:", PAL_UI_TXT);
    renderer_draw_string(70, 76, g_sim.wrap ? "ON " : "OFF", PAL_ALIVE);

    renderer_draw_string(14, 92, "Density:", PAL_UI_TXT);
    renderer_draw_string(70, 92, "50%", PAL_ALIVE);

    renderer_draw_string(14, 130, "A:toggle  L/R:change  B:back", PAL_DARK);
}

/* ── Credits ─────────────────────────────────────── */
static void draw_credits(void) {
    draw_panel(20, 10, 200, 140);
    renderer_draw_string(55, 16, "CREDITS", PAL_UI_HI);
    renderer_fill_rect(20, 26, 200, 1, PAL_BORDER);
    renderer_draw_string(30, 36, "Cellular Automata Lab", PAL_ALIVE);
    renderer_draw_string(30, 50, "GBA Edition", PAL_UI_TXT);
    renderer_draw_string(30, 64, "Built with devkitARM", PAL_UI_TXT);
    renderer_draw_string(30, 78, "and libgba", PAL_UI_TXT);
    renderer_draw_string(30, 92, "All code, graphics,", PAL_UI_TXT);
    renderer_draw_string(30, 106, "and audio original.", PAL_UI_TXT);
    renderer_draw_string(30, 120, "B to return", PAL_DARK);
}

/* ═══════════════════════════════════════════════════ */
/* UPDATE                                              */
/* ═══════════════════════════════════════════════════ */

static void update_menu(void) {
    if (input_repeat(KEY_DOWN)) {
        s_menu_sel = (s_menu_sel + 1) % MAIN_MENU_COUNT;
        audio_play_sfx(SFX_CURSOR);
    }
    if (input_repeat(KEY_UP)) {
        s_menu_sel = (s_menu_sel + MAIN_MENU_COUNT - 1) % MAIN_MENU_COUNT;
        audio_play_sfx(SFX_CURSOR);
    }
    if (g_keys_pressed & KEY_A) {
        audio_play_sfx(SFX_MENU);
        switch (s_menu_sel) {
            case 0: g_state = STATE_SIM;       sim_clear(); break;
            case 1: g_state = STATE_PATTERN_LIB; break;
            case 2: g_state = STATE_RULE_EDITOR; break;
            case 3: g_state = STATE_SAVE_MENU;   break;
            case 4: g_state = STATE_BENCHMARK;
                    bench_run(); break;
            case 5: g_state = STATE_EDUCATION; s_edu_page = 0; break;
            case 6: g_state = STATE_SETTINGS;  break;
            case 7: g_state = STATE_CREDITS;   break;
        }
    }
}

static void update_sim(void) {
    /* Help toggle */
    if (g_keys_pressed & KEY_SELECT)
        s_show_help = !s_show_help;

    /* Back to menu */
    if ((g_keys_pressed & KEY_START) && (g_keys_held & KEY_SELECT)) {
        g_state = STATE_MENU;
        return;
    }

    /* Pause */
    if (g_keys_pressed & KEY_START) {
        g_sim.paused = !g_sim.paused;
        audio_play_sfx(SFX_PAUSE);
    }

    /* Clear */
    if ((g_keys_held & KEY_A) && (g_keys_held & KEY_B)) {
        sim_clear();
        return;
    }

    /* Cursor movement */
    if (input_repeat(KEY_LEFT))  { g_sim.cur_x--; if (g_sim.cur_x < 0) g_sim.cur_x = GRID_W-1; }
    if (input_repeat(KEY_RIGHT)) { g_sim.cur_x++; if (g_sim.cur_x >= GRID_W) g_sim.cur_x = 0; }
    if (input_repeat(KEY_UP))    { g_sim.cur_y--; if (g_sim.cur_y < 0) g_sim.cur_y = GRID_H-1; }
    if (input_repeat(KEY_DOWN))  { g_sim.cur_y++; if (g_sim.cur_y >= GRID_H) g_sim.cur_y = 0; }

    /* Place / erase */
    if (g_keys_held & KEY_A) {
        sim_set_cell(g_sim.cur_x, g_sim.cur_y, (u8)g_sim.draw_state);
        audio_play_sfx(SFX_PLACE);
    }
    if (g_keys_held & KEY_B) {
        sim_set_cell(g_sim.cur_x, g_sim.cur_y, 0);
        audio_play_sfx(SFX_ERASE);
    }

    /* Randomize */
    if (g_keys_pressed & KEY_L) {
        sim_randomize(40);
        audio_play_sfx(SFX_MENU);
    }

    /* Pattern menu */
    if (g_keys_pressed & KEY_R) {
        g_state = STATE_PATTERN_LIB;
        return;
    }

    /* Stats view */
    if (g_keys_pressed & KEY_SELECT)
        s_stats_visible = !s_stats_visible;
}

static void update_pattern_lib(void) {
    if (input_repeat(KEY_DOWN))
        s_pattern_sel = (s_pattern_sel + 1) % PATTERN_COUNT;
    if (input_repeat(KEY_UP))
        s_pattern_sel = (s_pattern_sel + PATTERN_COUNT - 1) % PATTERN_COUNT;

    if (g_keys_pressed & KEY_A) {
        audio_play_sfx(SFX_PLACE);
        pattern_place(s_pattern_sel, g_sim.cur_x, g_sim.cur_y);
        g_state = STATE_SIM;
    }
    if (g_keys_pressed & KEY_B) {
        audio_play_sfx(SFX_MENU);
        g_state = STATE_SIM;
    }
}

static void update_rule_editor(void) {
    /* Cycle automata type with L/R */
    if (g_keys_pressed & KEY_RIGHT) {
        int t = ((int)g_sim.type + 1) % AUTO_COUNT;
        sim_set_automata((AutomataType)t);
        audio_play_sfx(SFX_CURSOR);
    }
    if (g_keys_pressed & KEY_LEFT) {
        int t = ((int)g_sim.type + AUTO_COUNT - 1) % AUTO_COUNT;
        sim_set_automata((AutomataType)t);
        audio_play_sfx(SFX_CURSOR);
    }
    /* Toggle birth bits with A */
    if (g_keys_pressed & KEY_A) {
        g_sim.type = AUTO_CUSTOM;
        /* cycle through toggling birth bits 0-8 by pressing A */
        static int bbit = 0;
        g_sim.birth_mask ^= (1 << bbit);
        bbit = (bbit + 1) % 9;
    }
    /* Toggle surv bits with B */
    if (g_keys_pressed & KEY_B) {
        g_sim.type = AUTO_CUSTOM;
        static int sbit = 0;
        g_sim.surv_mask ^= (1 << sbit);
        sbit = (sbit + 1) % 9;
    }
    /* Toggle wrap with Up/Down */
    if (g_keys_pressed & KEY_UP || g_keys_pressed & KEY_DOWN)
        g_sim.wrap = !g_sim.wrap;

    if (g_keys_pressed & KEY_START) {
        audio_play_sfx(SFX_MENU);
        g_state = STATE_SIM;
    }
    if (g_keys_pressed & KEY_SELECT) {
        audio_play_sfx(SFX_MENU);
        g_state = STATE_MENU;
    }
}

static void update_education(void) {
    if (g_keys_pressed & KEY_RIGHT)
        s_edu_page = (s_edu_page + 1) % EDU_PAGES;
    if (g_keys_pressed & KEY_LEFT)
        s_edu_page = (s_edu_page + EDU_PAGES - 1) % EDU_PAGES;
    if (g_keys_pressed & KEY_B) {
        audio_play_sfx(SFX_MENU);
        g_state = STATE_MENU;
    }
}

static void update_settings(void) {
    /* Theme */
    if (g_keys_pressed & KEY_RIGHT) {
        s_theme_sel = (s_theme_sel + 1) % THEME_COUNT;
        themes_apply(s_theme_sel);
        audio_play_sfx(SFX_CURSOR);
    }
    if (g_keys_pressed & KEY_LEFT) {
        s_theme_sel = (s_theme_sel + THEME_COUNT - 1) % THEME_COUNT;
        themes_apply(s_theme_sel);
        audio_play_sfx(SFX_CURSOR);
    }
    /* Speed */
    if (g_keys_pressed & KEY_UP) {
        g_sim.speed++;
        if (g_sim.speed > 8) g_sim.speed = 8;
    }
    if (g_keys_pressed & KEY_DOWN) {
        g_sim.speed--;
        if (g_sim.speed < 1) g_sim.speed = 1;
    }
    /* Wrap */
    if (g_keys_pressed & KEY_A)
        g_sim.wrap = !g_sim.wrap;
    /* Automata */
    if (g_keys_pressed & KEY_L) {
        int t = ((int)g_sim.type + AUTO_COUNT - 1) % AUTO_COUNT;
        sim_set_automata((AutomataType)t);
    }
    if (g_keys_pressed & KEY_R) {
        int t = ((int)g_sim.type + 1) % AUTO_COUNT;
        sim_set_automata((AutomataType)t);
    }
    if (g_keys_pressed & KEY_B) {
        audio_play_sfx(SFX_MENU);
        g_state = STATE_MENU;
    }
}

static void update_save_menu(void) {
    if (input_repeat(KEY_DOWN))
        s_save_sel = (s_save_sel + 1) % SAVE_SLOTS;
    if (input_repeat(KEY_UP))
        s_save_sel = (s_save_sel + SAVE_SLOTS - 1) % SAVE_SLOTS;
    if (g_keys_pressed & KEY_A) {
        save_write(s_save_sel);
        audio_play_sfx(SFX_SAVE);
    }
    if (g_keys_pressed & KEY_B) {
        if (save_read(s_save_sel))
            audio_play_sfx(SFX_LOAD);
        g_state = STATE_SIM;
    }
    if (g_keys_pressed & KEY_START) {
        audio_play_sfx(SFX_MENU);
        g_state = STATE_MENU;
    }
}

/* ── Main update ─────────────────────────────────── */
void ui_update(void) {
    switch (g_state) {
        case STATE_MENU:        update_menu(); break;
        case STATE_SIM:         update_sim(); break;
        case STATE_PATTERN_LIB: update_pattern_lib(); break;
        case STATE_RULE_EDITOR: update_rule_editor(); break;
        case STATE_EDUCATION:   update_education(); break;
        case STATE_SETTINGS:    update_settings(); break;
        case STATE_SAVE_MENU:   update_save_menu(); break;
        case STATE_BENCHMARK:
            if (g_keys_pressed & KEY_B) {
                audio_play_sfx(SFX_MENU);
                g_state = STATE_MENU;
            }
            break;
        case STATE_CREDITS:
            if (g_keys_pressed & KEY_B) {
                audio_play_sfx(SFX_MENU);
                g_state = STATE_MENU;
            }
            break;
        default: break;
    }
}

/* ── Main draw ───────────────────────────────────── */
void ui_draw(void) {
    switch (g_state) {
        case STATE_MENU:
            draw_main_menu();
            break;
        case STATE_SIM:
            renderer_draw_grid();
            renderer_draw_cursor(g_sim.cur_x, g_sim.cur_y);
            renderer_draw_status();
            if (s_show_help) draw_sim_help();
            if (s_stats_visible) stats_draw();
            break;
        case STATE_PATTERN_LIB:
            draw_pattern_lib();
            break;
        case STATE_RULE_EDITOR:
            draw_rule_editor();
            break;
        case STATE_EDUCATION:
            draw_education();
            break;
        case STATE_SETTINGS:
            draw_settings();
            break;
        case STATE_SAVE_MENU:
            save_draw_menu(s_save_sel);
            break;
        case STATE_BENCHMARK:
            bench_draw();
            break;
        case STATE_CREDITS:
            draw_credits();
            break;
        default: break;
    }
}