#ifndef AUDIO_DATA_H
#define AUDIO_DATA_H
#include "hw.h"

typedef struct { const s8 *data; u16 len; u8 prio; } SfxDef;
typedef struct { u8 spd; const u8 *tr[4]; } SongDef;
#define SFX_MENU_MOVE 0
#define SFX_MENU_OK 1
#define SFX_MENU_BACK 2
#define SFX_ERROR 3
#define SFX_TEXT 4
#define SFX_TEXT2 5
#define SFX_TEXT3 6
#define SFX_SLASH 7
#define SFX_SLASH2 8
#define SFX_HIT 9
#define SFX_CRIT 10
#define SFX_HURT 11
#define SFX_ENEMY_DIE 12
#define SFX_ROLL 13
#define SFX_ARROW 14
#define SFX_MAGIC 15
#define SFX_FIRE 16
#define SFX_EXPLODE 17
#define SFX_COIN 18
#define SFX_HEART 19
#define SFX_MANA 20
#define SFX_CHEST 21
#define SFX_ITEM 22
#define SFX_LEVELUP 23
#define SFX_POTION 24
#define SFX_DOOR 25
#define SFX_WARP 26
#define SFX_BOSS_ROAR 27
#define SFX_BOSS_DIE 28
#define SFX_BUSH 29
#define SFX_SPIN 30
#define SFX_DASH 31
#define SFX_ALERT 32
#define SFX_VICTORY 33
#define SFX_GAMEOVER 34
#define SFX_EQUIP 35
#define SFX_SAVE 36
#define SFX_BAT 37
#define SFX_WOLF 38
#define SFX_SLIME 39
#define SFX_COUNT 40
#define SONG_TITLE 0
#define SONG_VILLAGE 1
#define SONG_FOREST 2
#define SONG_CRYPT 3
#define SONG_BOSS 4
#define SONG_GAMEOVER_NONE 255
#define SONG_COUNT 5

extern const SfxDef sfx_table[SFX_COUNT];
extern const SongDef song_table[SONG_COUNT];
extern const u16 sq_freq[128];
extern const u16 wv_freq[128];
extern const u8 wave_ram0[16];
extern const u8 wave_ram1[16];
#endif

