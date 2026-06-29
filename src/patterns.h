#ifndef PATTERNS_H
#define PATTERNS_H

#include <gba.h>

typedef struct {
    const char *name;
    const u8   *data;
    int         w, h;
} Pattern;

#define PATTERN_COUNT 18

extern const Pattern PATTERNS[PATTERN_COUNT];

void pattern_place(int idx, int cx, int cy);

#endif