#ifndef SAVE_H
#define SAVE_H

#include "common.h"

/* Battery (SRAM) save: world seed + list of player edits + player state. */
bool save_exists(void);
bool save_write(void);  /* returns false if the edit list had to be truncated */
bool save_load(void);   /* regenerates the world and applies the save         */

#endif
