#ifndef GAME_H
#define GAME_H

#include "common.h"

/* Goals ("advancements"), unlocked as you progress. */
enum {
    ADV_WOOD, ADV_TABLE, ADV_WPICK, ADV_STONE, ADV_SPICK, ADV_FURNACE,
    ADV_IRON, ADV_IPICK, ADV_DIAMOND, ADV_DPICK, ADV_COOK, ADV_HOUSE,
    ADV_ZOMBIE, ADV_BEACON, ADV_COUNT
};

typedef struct {
    u16 tod;        /* time of day, 0..DAY_LEN-1                      */
    u16 adv;        /* unlocked goals (bit mask)                      */
    u16 placed;     /* blocks placed so far                           */
    u8  won;        /* the Ember Beacon has been lit                  */
    u8  daylight;   /* current palette level 0 (day) .. 8 (night)     */
    u8  toast_t;    /* frames left for the toast message              */
    u8  sel_t;      /* frames left to show the selected item's name   */
    char toast[32];
} Game;

extern Game g_game;
extern const char *const g_adv_name[ADV_COUNT];
extern const char *const g_adv_hint[ADV_COUNT];

void game_toast(const char *msg);
void game_unlock(int adv);
bool game_has(int adv);
int  game_night_level(u16 tod);

void game_init(void);
void game_frame(void);

#endif
