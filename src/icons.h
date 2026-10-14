#ifndef ICONS_H
#define ICONS_H

#include "common.h"

/* 8x8 item icons drawn from small text bitmaps. */
typedef struct {
    const char *const *rows; /* 8 strings of 8 characters */
    u8 main, dark, light;    /* palette indices for 'H', 'h', 'W' */
} IconDef;

/* Icon for a non-block item id, or NULL when there is none. */
const IconDef *icon_for(u8 item);

/* Palette index for one icon character (0 = transparent). */
u8 icon_color(const IconDef *ic, char ch);

#endif
