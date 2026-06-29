#ifndef INPUT_H
#define INPUT_H

#include <gba.h>

/* Held, pressed, released this frame */
extern u16 g_keys_held;
extern u16 g_keys_pressed;
extern u16 g_keys_released;
/* Repeat counter (cursor movement) */
extern int g_repeat_timer;

void input_update(void);
int  input_repeat(u16 key);

#endif