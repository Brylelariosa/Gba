#include "input.h"

u16 g_keys_held     = 0;
u16 g_keys_pressed  = 0;
u16 g_keys_released = 0;
int g_repeat_timer  = 0;

#define REPEAT_DELAY  20
#define REPEAT_RATE    4

void input_update(void) {
    scanKeys();
    g_keys_held     = keysHeld();
    g_keys_pressed  = keysDown();
    g_keys_released = keysUp();
    if (g_keys_held) g_repeat_timer++;
    else              g_repeat_timer = 0;
}

int input_repeat(u16 key) {
    if (g_keys_pressed & key) return 1;
    if ((g_keys_held & key) && g_repeat_timer > REPEAT_DELAY &&
        (g_repeat_timer % REPEAT_RATE) == 0)
        return 1;
    return 0;
}