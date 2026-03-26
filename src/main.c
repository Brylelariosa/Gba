// ============================================================
//  TERRARIA GBA - OPTIMIZED VERSION
//  A Terraria-inspired 2D sandbox game for Game Boy Advance
// ============================================================

#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <string.h>

// ─────────────────────────────────────────────────────────────
//  DISPLAY
// ─────────────────────────────────────────────────────────────
#define SCREEN_W  240
#define SCREEN_H  160
#define SCREEN_SIZE (SCREEN_W * SCREEN_H)

// Back buffer in EWRAM (must be declared outside main to be in .bss)
static u16 back_buffer[SCREEN_SIZE];

// ─────────────────────────────────────────────────────────────
//  WORLD
// ─────────────────────────────────────────────────────────────
#define TILE_SIZE   8
#define WORLD_W   128
#define WORLD_H    64
#define SURFACE_Y  16

#define VIEW_W  (SCREEN_W / TILE_SIZE)   // 30
#define VIEW_H  (SCREEN_H / TILE_SIZE)   // 20

// ─────────────────────────────────────────────────────────────
//  TILE TYPES
// ─────────────────────────────────────────────────────────────
typedef enum {
    TILE_AIR = 0,
    TILE_GRASS,
    TILE_DIRT,
    TILE_STONE,
    TILE_DEEP_STONE,
    TILE_GRAVEL,
    TILE_SAND,
    TILE_COUNT
} TileType;

// ─────────────────────────────────────────────────────────────
//  COLORS
// ─────────────────────────────────────────────────────────────
#define RGB15(r,g,b)  ((u16)((r) | ((g)<<5) | ((b)<<10)))

// Precomputed tile colors with light/dark edges
static u16 tile_colors[TILE_COUNT];      // base color
static u16 tile_light[TILE_COUNT];       // lighter edge
static u16 tile_dark[TILE_COUNT];        // darker edge

static const u16 TILE_COL_BASE[TILE_COUNT] = {
    RGB15( 0,  0,  0),   // AIR (unused)
    RGB15( 6, 22,  5),   // GRASS
    RGB15(16, 11,  6),   // DIRT
    RGB15(14, 14, 15),   // STONE
    RGB15( 8,  8, 10),   // DEEP STONE
    RGB15(17, 15, 10),   // GRAVEL
    RGB15(26, 22,  8),   // SAND
};

// ─────────────────────────────────────────────────────────────
//  WORLD DATA
// ─────────────────────────────────────────────────────────────
static u8 world[WORLD_H][WORLD_W];

// ─────────────────────────────────────────────────────────────
//  PLAYER
// ─────────────────────────────────────────────────────────────
#define PLAYER_W      6
#define PLAYER_H     12
#define WALK_SPEED    2
#define JUMP_VEL     -9
#define GRAVITY       1
#define MAX_FALL      9
#define MINE_DELAY    8

static int px, py;
static int pvx, pvy;
static int on_ground;
static int facing;

// ─────────────────────────────────────────────────────────────
//  INVENTORY
// ─────────────────────────────────────────────────────────────
static u8  inv[TILE_COUNT];
static int hotbar_sel = 1;
static int show_hud   = 1;

// ─────────────────────────────────────────────────────────────
//  CAMERA
// ─────────────────────────────────────────────────────────────
static int cam_x, cam_y;

// ─────────────────────────────────────────────────────────────
//  RNG
// ─────────────────────────────────────────────────────────────
static u32 rng_state = 0xDEADBEEF;
static u32 rng(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state <<  5;
    return rng_state;
}

// ─────────────────────────────────────────────────────────────
//  INIT COLOR TABLES
// ─────────────────────────────────────────────────────────────
static void init_colors(void) {
    for (int i = 0; i < TILE_COUNT; i++) {
        u16 base = TILE_COL_BASE[i];
        tile_colors[i] = base;
        
        int r = (base & 0x1F) + 5;  if (r > 31) r = 31;
        int g = ((base >>  5) & 0x1F) + 5; if (g > 31) g = 31;
        int b = ((base >> 10) & 0x1F) + 5; if (b > 31) b = 31;
        tile_light[i] = (u16)(r | (g<<5) | (b<<10));
        
        r = (base & 0x1F) - 3;  if (r < 0) r = 0;
        g = ((base >>  5) & 0x1F) - 3; if (g < 0) g = 0;
        b = ((base >> 10) & 0x1F) - 3; if (b < 0) b = 0;
        tile_dark[i] = (u16)(r | (g<<5) | (b<<10));
    }
}

// ─────────────────────────────────────────────────────────────
//  FAST FILL ROUTINES (using back buffer)
// ─────────────────────────────────────────────────────────────
static inline void put_pixel(int x, int y, u16 color) {
    if ((unsigned)x < SCREEN_W && (unsigned)y < SCREEN_H)
        back_buffer[y * SCREEN_W + x] = color;
}

static void fill_row(int x, int y, int w, u16 color) {
    if ((unsigned)y >= SCREEN_H) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (w <= 0) return;

    u16 *dst = back_buffer + y * SCREEN_W + x;
    u32 c32 = (u32)color | ((u32)color << 16);
    u32 *d32 = (u32*)dst;
    int n32 = w >> 1;
    for (int i = 0; i < n32; i++) *d32++ = c32;
    if (w & 1) *(u16*)d32 = color;
}

static void fill_rect(int x, int y, int w, int h, u16 color) {
    for (int dy = 0; dy < h; dy++) fill_row(x, y + dy, w, color);
}

// Optimized tile drawing: fill interior + borders
static void draw_tile(int sx, int sy, int tile_type) {
    if (tile_type == TILE_AIR) return;
    
    u16 base = tile_colors[tile_type];
    u16 light = tile_light[tile_type];
    u16 dark = tile_dark[tile_type];
    
    // Draw top border (full width)
    fill_row(sx, sy, TILE_SIZE, light);
    // Draw left border (full height)
    for (int y = 1; y < TILE_SIZE; y++)
        put_pixel(sx, sy + y, light);
    
    // Draw interior
    fill_rect(sx + 1, sy + 1, TILE_SIZE - 2, TILE_SIZE - 2, base);
    
    // Draw bottom border
    fill_row(sx, sy + TILE_SIZE - 1, TILE_SIZE, dark);
    // Draw right border
    for (int y = 1; y < TILE_SIZE - 1; y++)
        put_pixel(sx + TILE_SIZE - 1, sy + y, dark);
}

// ─────────────────────────────────────────────────────────────
//  WORLD GENERATION
// ─────────────────────────────────────────────────────────────
static void generate_world(void) {
    memset(world, TILE_AIR, sizeof(world));

    int heights[WORLD_W];
    heights[0] = SURFACE_Y;
    for (int x = 1; x < WORLD_W; x++) {
        int d = (int)(rng() % 3) - 1;
        heights[x] = heights[x-1] + d;
        if (heights[x] < SURFACE_Y - 5) heights[x] = SURFACE_Y - 5;
        if (heights[x] > SURFACE_Y + 6) heights[x] = SURFACE_Y + 6;
    }

    for (int x = 0; x < WORLD_W; x++) {
        int h = heights[x];
        if (h >= 0 && h < WORLD_H)
            world[h][x] = TILE_GRASS;
        for (int y = h+1; y < h+6 && y < WORLD_H; y++)
            world[y][x] = TILE_DIRT;
        
        int stone_top = h + 6;
        int deep_top  = WORLD_H * 3 / 4;
        for (int y = stone_top; y < deep_top && y < WORLD_H; y++)
            world[y][x] = TILE_STONE;
        for (int y = deep_top; y < WORLD_H; y++)
            world[y][x] = TILE_DEEP_STONE;
    }

    // Sand patches
    for (int k = 0; k < 20; k++) {
        int cx = rng() % WORLD_W;
        int cy = (rng() % 4) + SURFACE_Y;
        int r  = 1 + rng() % 3;
        for (int dy = -r; dy <= r; dy++)
            for (int dx = -r; dx <= r; dx++) {
                int tx = cx+dx, ty = cy+dy;
                if (tx > 0 && tx < WORLD_W-1 && ty > 0 && ty < WORLD_H-1)
                    if (world[ty][tx] == TILE_DIRT || world[ty][tx] == TILE_GRASS)
                        world[ty][tx] = TILE_SAND;
            }
    }

    // Gravel pockets
    for (int k = 0; k < 30; k++) {
        int cx = rng() % WORLD_W;
        int cy = (SURFACE_Y + 5) + rng() % (WORLD_H/2 - SURFACE_Y);
        int r  = 1 + rng() % 3;
        for (int dy = -r; dy <= r; dy++)
            for (int dx = -r; dx <= r; dx++) {
                int tx = cx+dx, ty = cy+dy;
                if (tx > 0 && tx < WORLD_W-1 && ty > 0 && ty < WORLD_H-1)
                    if (world[ty][tx] != TILE_AIR)
                        world[ty][tx] = TILE_GRAVEL;
            }
    }

    // Caves
    for (int k = 0; k < 45; k++) {
        int cx = rng() % WORLD_W;
        int cy = (SURFACE_Y + 8) + rng() % (WORLD_H - SURFACE_Y - 14);
        int rx = 2 + rng() % 5;
        int ry = 1 + rng() % 3;
        for (int dy = -ry; dy <= ry; dy++)
            for (int dx = -rx; dx <= rx; dx++) {
                if (dx*dx * ry*ry + dy*dy * rx*rx <= rx*rx * ry*ry) {
                    int tx = cx+dx, ty = cy+dy;
                    if (tx > 1 && tx < WORLD_W-2 && ty > SURFACE_Y+2 && ty < WORLD_H-2)
                        world[ty][tx] = TILE_AIR;
                }
            }
    }
}

// ─────────────────────────────────────────────────────────────
//  COLLISION HELPERS
// ─────────────────────────────────────────────────────────────
static int tile_solid(int tx, int ty) {
    if ((unsigned)tx >= WORLD_W) return 1;
    if (ty < 0) return 0;
    if ((unsigned)ty >= WORLD_H) return 1;
    return world[ty][tx] != TILE_AIR;
}

static int rect_hits_world(int rx, int ry, int rw, int rh) {
    int tx0 = rx / TILE_SIZE;
    int ty0 = ry / TILE_SIZE;
    int tx1 = (rx + rw - 1) / TILE_SIZE;
    int ty1 = (ry + rh - 1) / TILE_SIZE;
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++)
            if (tile_solid(tx, ty)) return 1;
    return 0;
}

// ─────────────────────────────────────────────────────────────
//  PHYSICS
// ─────────────────────────────────────────────────────────────
static void update_physics(void) {
    pvy += GRAVITY;
    if (pvy > MAX_FALL) pvy = MAX_FALL;

    if (pvx) {
        int nx = px + pvx;
        if (!rect_hits_world(nx, py, PLAYER_W, PLAYER_H))
            px = nx;
        else
            pvx = 0;
    }

    pvx = pvx * 3 / 4;

    int ny = py + pvy;
    if (!rect_hits_world(px, ny, PLAYER_W, PLAYER_H)) {
        py = ny;
        if (pvy > 0) on_ground = 0;
    } else {
        if (pvy > 0) on_ground = 1;
        pvy = 0;
    }

    if (px < 0) { px = 0; pvx = 0; }
    if (px + PLAYER_W > WORLD_W * TILE_SIZE) { px = WORLD_W * TILE_SIZE - PLAYER_W; pvx = 0; }
    if (py < 0) { py = 0; pvy = 0; }

    int tcx = px - SCREEN_W/2 + PLAYER_W/2;
    int tcy = py - SCREEN_H/2 + PLAYER_H/2 - 8;
    cam_x += (tcx - cam_x + 2) / 4;
    cam_y += (tcy - cam_y + 2) / 4;

    int max_cx = WORLD_W * TILE_SIZE - SCREEN_W;
    int max_cy = WORLD_H * TILE_SIZE - SCREEN_H;
    if (cam_x < 0) cam_x = 0;
    if (cam_y < 0) cam_y = 0;
    if (cam_x > max_cx) cam_x = max_cx;
    if (cam_y > max_cy) cam_y = max_cy;
}

// ─────────────────────────────────────────────────────────────
//  INPUT & GAME LOGIC
// ─────────────────────────────────────────────────────────────
static int mine_timer = 0;

static void handle_input(void) {
    scanKeys();
    u32 held    = keysHeld();
    u32 pressed = keysDown();

    if (held & KEY_LEFT)  { pvx -= 2; facing = 0; }
    if (held & KEY_RIGHT) { pvx += 2; facing = 1; }
    if (pvx >  WALK_SPEED) pvx =  WALK_SPEED;
    if (pvx < -WALK_SPEED) pvx = -WALK_SPEED;

    if ((pressed & KEY_A) && on_ground) {
        pvy = JUMP_VEL;
        on_ground = 0;
    }

    // Mining
    if (held & KEY_B) {
        mine_timer++;
        if (mine_timer >= MINE_DELAY) {
            mine_timer = 0;
            int ptx = px / TILE_SIZE;
            int pty = py / TILE_SIZE;
            int dx  = facing ? 1 : -1;

            int order[8][2] = {
                {dx,  0}, {dx, -1}, {dx,  1},
                {0,  -1}, {0,   1},
                {-dx, 0}, {-dx,-1}, {-dx, 1}
            };
            for (int k = 0; k < 8; k++) {
                int tx = ptx + order[k][0];
                int ty = pty + order[k][1];
                if ((unsigned)tx < WORLD_W && (unsigned)ty < WORLD_H &&
                    world[ty][tx] != TILE_AIR) {
                    u8 t = world[ty][tx];
                    if (++inv[t] > 99) inv[t] = 99;
                    world[ty][tx] = TILE_AIR;
                    break;
                }
            }
        }
    } else {
        mine_timer = 0;
    }

    // Block placement
    if (pressed & KEY_R) {
        if (inv[hotbar_sel] > 0) {
            int ptx = px / TILE_SIZE;
            int pty = py / TILE_SIZE;
            int dx = facing ? 1 : -1;
            
            int cands[8][2] = {
                {dx,  0}, {dx, -1}, {dx,  1},
                {0,  -1}, {0,   1},
                {-dx, 0}, {-dx,-1}, {-dx, 1}
            };
            
            for (int k = 0; k < 8; k++) {
                int tx = ptx + cands[k][0];
                int ty = pty + cands[k][1];
                if ((unsigned)tx >= WORLD_W || (unsigned)ty >= WORLD_H)
                    continue;
                if (world[ty][tx] != TILE_AIR)
                    continue;
                
                int bpx = tx * TILE_SIZE;
                int bpy = ty * TILE_SIZE;
                int ox = (bpx < px + PLAYER_W) && (bpx + TILE_SIZE > px);
                int oy = (bpy < py + PLAYER_H) && (bpy + TILE_SIZE > py);
                if (!(ox && oy)) {
                    world[ty][tx] = (u8)hotbar_sel;
                    inv[hotbar_sel]--;
                    break;
                }
            }
        }
    }

    if (pressed & KEY_L) {
        if (++hotbar_sel >= TILE_COUNT) hotbar_sel = 1;
    }

    if (pressed & KEY_SELECT) show_hud ^= 1;
}

// ─────────────────────────────────────────────────────────────
//  RENDERING
// ─────────────────────────────────────────────────────────────
#define COL_PLAYER_HEAD RGB15(28, 20, 13)
#define COL_PLAYER_BODY RGB15( 4,  8, 22)
#define COL_PLAYER_LEGS RGB15( 3,  6, 16)
#define COL_PLAYER_EYE  RGB15( 0,  0,  0)
#define COL_MINE_ANIM   RGB15(31, 28,  0)
#define COL_HUD_BG      RGB15( 0,  0,  0)
#define COL_HUD_SEL     RGB15(28, 22,  0)
#define COL_HUD_TXT     RGB15(31, 31, 31)
#define COL_DEPTH_BAR   RGB15(20, 14,  4)

static int spark_t = 0;

static void render_sky(void) {
    int cam_ty = cam_y / TILE_SIZE;
    for (int sy = 0; sy < SCREEN_H; sy++) {
        int world_ty = cam_ty + sy / TILE_SIZE;
        u16 color;
        if (world_ty >= SURFACE_Y + 6) {
            int darkness = world_ty - (SURFACE_Y + 6);
            int r = 2 - darkness; if (r < 0) r = 0;
            int g = 2 - darkness; if (g < 0) g = 0;
            int b = 5 - darkness; if (b < 0) b = 0;
            color = RGB15(r, g, b);
        } else {
            int t = sy * 16 / SCREEN_H;
            int r =  5 + t * 7 / 16;
            int g = 14 + t * 6 / 16;
            int b = 27 + t * 4 / 16;
            if (r > 31) r = 31;
            if (g > 31) g = 31;
            if (b > 31) b = 31;
            color = RGB15(r, g, b);
        }
        fill_row(0, sy, SCREEN_W, color);
    }
}

static void render_world(void) {
    int tx0  = cam_x / TILE_SIZE;
    int ty0  = cam_y / TILE_SIZE;
    int offx = cam_x % TILE_SIZE;
    int offy = cam_y % TILE_SIZE;

    for (int tdy = 0; tdy <= VIEW_H + 1; tdy++) {
        int ty = ty0 + tdy;
        if (ty < 0 || ty >= WORLD_H) continue;
        int sy = tdy * TILE_SIZE - offy;

        for (int tdx = 0; tdx <= VIEW_W + 1; tdx++) {
            int tx = tx0 + tdx;
            if (tx < 0 || tx >= WORLD_W) continue;

            int t = world[ty][tx];
            if (t == TILE_AIR) continue;

            draw_tile(tdx * TILE_SIZE - offx, sy, t);
        }
    }
}

static void render_player(void) {
    int sx = px - cam_x;
    int sy = py - cam_y;

    // Head
    fill_rect(sx, sy - 7, 6, 7, COL_PLAYER_HEAD);
    // Eye
    put_pixel(sx + (facing ? 4 : 1), sy - 5, COL_PLAYER_EYE);
    // Body
    fill_rect(sx, sy, 6, 7, COL_PLAYER_BODY);
    // Legs (animated)
    int leg_anim = (pvx != 0 || !on_ground) ? ((spark_t / 4) & 1) : 0;
    if (leg_anim) {
        fill_rect(sx,     sy + 7, 3, 5, COL_PLAYER_LEGS);
        fill_rect(sx + 3, sy + 5, 3, 7, COL_PLAYER_LEGS);
    } else {
        fill_rect(sx,     sy + 7, 3, 5, COL_PLAYER_LEGS);
        fill_rect(sx + 3, sy + 7, 3, 5, COL_PLAYER_LEGS);
    }
    // Mining spark
    if (mine_timer > 2 && mine_timer < MINE_DELAY && (mine_timer & 1)) {
        int mx = sx + (facing ? 8 : -4);
        int my = sy;
        put_pixel(mx,   my,   COL_MINE_ANIM);
        put_pixel(mx+1, my-1, COL_MINE_ANIM);
        put_pixel(mx,   my+2, COL_MINE_ANIM);
    }
}

// 3x5 font
static const u8 FONT_3x5[10][5] = {
    {0b111,0b101,0b101,0b101,0b111},
    {0b010,0b110,0b010,0b010,0b111},
    {0b111,0b001,0b111,0b100,0b111},
    {0b111,0b001,0b011,0b001,0b111},
    {0b101,0b101,0b111,0b001,0b001},
    {0b111,0b100,0b111,0b001,0b111},
    {0b111,0b100,0b111,0b101,0b111},
    {0b111,0b001,0b011,0b010,0b010},
    {0b111,0b101,0b111,0b101,0b111},
    {0b111,0b101,0b111,0b001,0b111},
};

static void draw_digit(int x, int y, int d, u16 color) {
    if (d < 0 || d > 9) return;
    const u8 *glyph = FONT_3x5[d];
    for (int row = 0; row < 5; row++) {
        for (int col = 0; col < 3; col++) {
            if (glyph[row] & (1 << (2 - col)))
                put_pixel(x + col, y + row, color);
        }
    }
}

static void draw_number(int x, int y, int n, u16 color) {
    if (n >= 10) draw_digit(x, y, n / 10, color);
    draw_digit(x + 4, y, n % 10, color);
}

static void render_hud(void) {
    if (!show_hud) return;

    int depth = (py / TILE_SIZE) - SURFACE_Y;
    fill_rect(0, 0, 64, 10, COL_HUD_BG);

    if (depth >= 0) {
        int bar = depth * 3;
        if (bar > 60) bar = 60;
        fill_rect(2, 3, bar, 4, COL_DEPTH_BAR);
        draw_number(2, 2, depth, COL_HUD_TXT);
    } else {
        int ht = -depth;
        if (ht > 60) ht = 60;
        fill_rect(2, 3, ht, 4, RGB15(5,22,28));
        draw_number(2, 2, ht, COL_HUD_TXT);
    }

    int bar_x = (SCREEN_W - (TILE_COUNT - 1) * 12) / 2;
    int bar_y = SCREEN_H - 12;
    fill_rect(bar_x - 2, bar_y - 1, (TILE_COUNT - 1) * 12 + 2, 12, COL_HUD_BG);

    for (int i = 1; i < TILE_COUNT; i++) {
        int sx = bar_x + (i - 1) * 12;
        if (i == hotbar_sel)
            fill_rect(sx - 1, bar_y - 1, 10, 11, COL_HUD_SEL);
        if (inv[i] > 0)
            fill_rect(sx, bar_y, TILE_SIZE, TILE_SIZE, tile_colors[i]);
        else
            fill_rect(sx, bar_y, TILE_SIZE, TILE_SIZE, RGB15(4,4,4));
        if (inv[i] > 0)
            draw_number(sx, bar_y - 6, inv[i], COL_HUD_TXT);
    }

    int tx = SCREEN_W - 36;
    fill_rect(tx - 2, 0, 38, 8, COL_HUD_BG);
    fill_rect(tx, 0, 7, 7, tile_colors[hotbar_sel]);
}

// ─────────────────────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────────────────────
int main(void) {
    irqInit();
    irqEnable(IRQ_VBLANK);
    REG_DISPCNT = MODE_3 | BG2_ON;

    init_colors();
    generate_world();
    memset(inv, 0, sizeof(inv));
    inv[TILE_DIRT]  = 10;
    inv[TILE_STONE] = 5;
    hotbar_sel = TILE_DIRT;

    px = (WORLD_W / 2) * TILE_SIZE;
    py = (SURFACE_Y - 3) * TILE_SIZE;
    pvx = pvy = 0;
    on_ground = 0;
    facing = 1;

    cam_x = px - SCREEN_W/2;
    cam_y = py - SCREEN_H/2;

    while (1) {
        VBlankIntrWait();          // Wait for VBlank

        handle_input();
        update_physics();

        // Draw to back buffer
        render_sky();      // fills entire screen
        render_world();
        render_player();
        render_hud();

        // Copy back buffer to VRAM (fast using DMA)
        // Using C memory copy is acceptable for 76800 bytes, but we can use DMA for speed
        // For simplicity, use memcpy; GBA's memcpy is optimized.
        memcpy((void*)VRAM, back_buffer, SCREEN_SIZE * 2);

        spark_t++;
    }

    return 0;
}