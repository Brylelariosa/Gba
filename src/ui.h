#ifndef UI_H
#define UI_H

#include "common.h"

/* In-game HUD: hearts, hunger, hotbar, messages. */
void ui_hud(void);

/* Inventory / crafting / goals screen (SELECT). */
void ui_menu_open(void);
bool ui_menu_update(u16 held, u16 pressed, int near_mask); /* true = close */
void ui_menu_draw(int near_mask);

/* Pause menu (START). */
typedef enum { PAUSE_NONE, PAUSE_RESUME, PAUSE_SAVE, PAUSE_QUIT } PauseAction;
void ui_pause_open(void);
PauseAction ui_pause_update(u16 pressed);
void ui_pause_draw(void);

/* Title screen. */
typedef enum { TITLE_NONE, TITLE_NEW, TITLE_CONTINUE } TitleAction;
void ui_title_open(bool has_save);
TitleAction ui_title_update(u16 pressed);
void ui_title_draw(void);

void ui_dead_draw(void);

#endif
