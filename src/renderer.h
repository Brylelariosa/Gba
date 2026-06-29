---

## `src/renderer.h`

```c
#ifndef RENDERER_H
#define RENDERER_H

#include <gba.h>
#include "simulation.h"

#define CHAR_W  5
#define CHAR_H  6

void renderer_init(void);
void renderer_draw_grid(void);
void renderer_draw_status(void);
void renderer_draw_cursor(int x, int y);
void renderer_fill_rect(int x, int y, int w, int h, u8 pal_idx);
void renderer_draw_char(int x, int y, char c, u8 col);
void renderer_draw_string(int x, int y, const char *s, u8 col);
void renderer_draw_int(int x, int y, int n, u8 col);
void renderer_draw_uint(int x, int y, u32 n, u8 col);
void renderer_vsync(void);
void renderer_flip(void);

#endif