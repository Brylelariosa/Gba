#ifndef UI_H
#define UI_H

#include <gba.h>

typedef enum {
    STATE_MENU = 0,
    STATE_SIM,
    STATE_RULE_EDITOR,
    STATE_PATTERN_LIB,
    STATE_SAVE_MENU,
    STATE_BENCHMARK,
    STATE_EDUCATION,
    STATE_SETTINGS,
    STATE_CREDITS,
    STATE_STATS_VIEW,
    STATE_COUNT
} AppState;

extern AppState g_state;

void ui_init(void);
void ui_update(void);
void ui_draw(void);

#endif