// ============================================================
//  TERRARIA GBA
//  A Terraria-inspired 2D sandbox game for Game Boy Advance
// ============================================================
//
//  Controls:
//    D-Pad Left/Right  : Move
//    A                 : Jump (hold for higher jump)
//    B (hold)          : Mine block in front / below
//    R                 : Place selected block
//    L                 : Cycle selected block in inventory
//    START             : (reserved – pause)
//    SELECT            : Toggle HUD
//
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

// ─────────────────────────────────────────────────────────────
//  WORLD
// ─────────────────────────────────────────────────────────────
#define TILE_SIZE   8
#define WORLD_W   128
#define WORLD_H    64
#define SURFACE_Y  16   // tiles from top where ground starts

#define VIEW_W  (SCREEN_W / TILE_SIZE)   // 30 tiles wide
#define VIEW_H  (SCREEN_H / TILE_SIZE)   // 20 tiles tall

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
//  COLORS  (RGB555: 5 bits each, packed as 0bbbbbgggggrrrrr)
// ─────────────────────────────────────────────────────────────
#define RGB15(r,g,b)  ((u16)((r) | ((g)<<5) | ((b)<<10)))

static const u16 TILE_COL[TILE_COUNT] = {
    RGB15( 0,  0,  0),   // AIR
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
#define PLAYER_TOTAL_H  18  // body + head
#define WALK_SPEED    2
#define JUMP_VEL     -9
#define GRAVITY       1
#define MAX_FALL      9
#define MINE_DELAY    8   // frames between each mine tick

static int px, py;          // pixel position (world space)
static int pvx, pvy;        // velocity
static int on_ground;
static int facing;          // 0 = left, 1 = right

// ─────────────────────────────────────────────────────────────
//  INVENTORY
// ─────────────────────────────────────────────────────────────
static u8  inv[TILE_COUNT];   // count of each tile
static int hotbar_sel = 1;    // currently selected slot (1..TILE_COUNT-1)
static int show_hud   = 1;

// ─────────────────────────────────────────────────────────────
//  CAMERA
// ─────────────────────────────────────────────────────────────
static int cam_x, cam_y;

// ─────────────────────────────────────────────────────────────
//  RNG  (xorshift32)
// ─────────────────────────────────────────────────────────────
static u32 rng_state = 0xDEADBEEF;
static u32 rng(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state <<  5;
    return rng_state;
}

// ─────────────────────────────────────────────────────────────
//  COLOR HELPERS
// ─────────────────────────────────────────────────────────────
static u16 lighten(u16 c) {
    int r = (c & 0x1F) + 5;  if (r > 31) r = 31;
    int g = ((c >>  5) & 0x1F) + 5; if (g > 31) g = 31;
    int b = ((c >> 10) & 0x1F) + 5; if (b > 31) b = 31;
    return (u16)(r | (g<<5) | (b<<10));
}
static u16 darken(u16 c) {
    int r = (c & 0x1F) - 3;  if (r < 0) r = 0;
    int g = ((c >>  5) & 0x1F) - 3; if (g < 0) g = 0;
    int b = ((c >> 10) & 0x1F) - 3; if (b < 0) b = 0;
    return (u16)(r | (g<<5) | (b<<10));
}

// ─────────────────────────────────────────────────────────────
//  FAST PIXEL / FILL ROUTINES
// ─────────────────────────────────────────────────────────────
static inline void put_pixel(int x, int y, u16 color) {
    if ((unsigned)x < (unsigned)SCREEN_W && (unsigned)y < (unsigned)SCREEN_H)
        ((u16*)VRAM)[y * SCREEN_W + x] = color;
}

// 32-bit wide fill for speed
static void fill_row(int x, int y, int w, u16 color) {
    if ((unsigned)y >= (unsigned)SCREEN_H) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (w <= 0) return;

    u16 *dst = (u16*)VRAM + y * SCREEN_W + x;
    // Odd pixel alignment
    if ((int)dst & 2) { *dst++ = color; w--; }
    // 32-bit bulk fill
    u32  c32 = (u32)color | ((u32)color << 16);
    u32 *d32 = (u32*)dst;
    int  n32 = w >> 1;
    for (int i = 0; i < n32; i++) *d32++ = c32;
    if (w & 1) *(u16*)d32 = color;
}

static void fill_rect(int x, int y, int w, int h, u16 color) {
    for (int dy = 0; dy < h; dy++) fill_row(x, y + dy, w, color);
}

// ─────────────────────────────────────────────────────────────
//  TILE DRAW  (with 1-pixel bevelled border for depth)
// ─────────────────────────────────────────────────────────────
static void draw_tile(int sx, int sy, u16 color) {
    u16 hi = lighten(color);
    u16 lo = darken(color);

    for (int dy = 0; dy < TILE_SIZE; dy++) {
        int y = sy + dy;
        if ((unsigned)y >= (unsigned)SCREEN_H) continue;
        int x0 = sx < 0 ? -sx : 0;
        int x1 = (sx + TILE_SIZE > SCREEN_W) ? SCREEN_W - sx : TILE_SIZE;
        if (x0 >= x1) continue;

        u16 *row = (u16*)VRAM + y * SCREEN_W + sx;
        for (int dx = x0; dx < x1; dx++) {
            u16 c;
            if (dy == 0 || dx == 0)                         c = hi;
            else if (dy == TILE_SIZE-1 || dx == TILE_SIZE-1) c = lo;
            else                                              c = color;
            row[dx] = c;
        }
    }
}

// ─────────────────────────────────────────────────────────────
//  WORLD GENERATION
// ─────────────────────────────────────────────────────────────
static void generate_world(void) {
    memset(world, TILE_AIR, sizeof(world));

    // Build heightmap with a random walk
    int heights[WORLD_W];
    heights[0] = SURFACE_Y;
    for (int x = 1; x < WORLD_W; x++) {
        int d = (int)(rng() % 3) - 1;
        heights[x] = heights[x-1] + d;
        if (heights[x] < SURFACE_Y - 5) heights[x] = SURFACE_Y - 5;
        if (heights[x] > SURFACE_Y + 6) heights[x] = SURFACE_Y + 6;
    }

    // Fill columns
    for (int x = 0; x < WORLD_W; x++) {
        int h = heights[x];
        // Grass cap
        if (h >= 0 && h < WORLD_H)
            world[h][x] = TILE_GRASS;
        // Dirt band (5 tiles)
        for (int y = h+1; y < h+6 && y < WORLD_H; y++)
            world[y][x] = TILE_DIRT;
        // Stone band
        int stone_top = h + 6;
        int deep_top  = WORLD_H * 3 / 4;
        for (int y = stone_top; y < deep_top && y < WORLD_H; y++)
            world[y][x] = TILE_STONE;
        // Deep stone
        for (int y = deep_top; y < WORLD_H; y++)
            world[y][x] = TILE_DEEP_STONE;
    }

    // Scatter sand patches near surface
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

    // Gravel pockets (mid layer)
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

    // Caves (elliptical tunnels)
    for (int k = 0; k < 45; k++) {
        int cx = rng() % WORLD_W;
        int cy = (SURFACE_Y + 8) + rng() % (WORLD_H - SURFACE_Y - 14);
        int rx = 2 + rng() % 5;
        int ry = 1 + rng() % 3;
        for (int dy = -ry; dy <= ry; dy++)
            for (int dx = -rx; dx <= rx; dx++) {
                // Ellipse check: (dx/rx)^2 + (dy/ry)^2 <= 1
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
    if ((unsigned)tx >= (unsigned)WORLD_W) return 1;
    if (ty < 0) return 0;
    if ((unsigned)ty >= (unsigned)WORLD_H) return 1;
    return world[ty][tx] != TILE_AIR;
}

// Test whether a world-space rect overlaps any solid tile
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
    // Gravity
    pvy += GRAVITY;
    if (pvy > MAX_FALL) pvy = MAX_FALL;

    // Horizontal move
    if (pvx) {
        int nx = px + pvx;
        if (!rect_hits_world(nx, py, PLAYER_W, PLAYER_H))
            px = nx;
        else
            pvx = 0;
    }

    // Friction
    pvx = pvx * 3 / 4;

    // Vertical move
    {
        int ny = py + pvy;
        if (!rect_hits_world(px, ny, PLAYER_W, PLAYER_H)) {
            py = ny;
            if (pvy > 0) on_ground = 0;
        } else {
            if (pvy > 0) on_ground = 1;
            pvy = 0;
        }
    }

    // Clamp to world
    if (px < 0) { px = 0; pvx = 0; }
    if (px + PLAYER_W > WORLD_W * TILE_SIZE) { px = WORLD_W * TILE_SIZE - PLAYER_W; pvx = 0; }
    if (py < 0) { py = 0; pvy = 0; }

    // Smooth camera follow
    int tcx = px - SCREEN_W/2 + PLAYER_W/2;
    int tcy = py - SCREEN_H/2 + PLAYER_H/2 - 8;  // slight upward bias
    cam_x += (tcx - cam_x + 2) / 4;
    cam_y += (tcy - cam_y + 2) / 4;

    // Clamp camera
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

    // ── Movement ──────────────────────────────────────────────
    if (held & KEY_LEFT)  { pvx -= 2; facing = 0; }
    if (held & KEY_RIGHT) { pvx += 2; facing = 1; }
    if (pvx >  WALK_SPEED) pvx =  WALK_SPEED;
    if (pvx < -WALK_SPEED) pvx = -WALK_SPEED;

    // ── Jump ──────────────────────────────────────────────────
    if ((pressed & KEY_A) && on_ground) {
        pvy = JUMP_VEL;
        on_ground = 0;
    }

    // ── Mining (hold B) ───────────────────────────────────────
    if (held & KEY_B) {
        mine_timer++;
        if (mine_timer >= MINE_DELAY) {
            mine_timer = 0;
            int ptx = px / TILE_SIZE;
            int pty = py / TILE_SIZE;
            int dx  = facing ? 1 : -1;

            // Priority order of target blocks
            int order[8][2] = {
                {dx,  0}, {dx,  1}, {dx, -1},
                { 0,  1}, { 0,  0}, { 0, -1},
                {-dx, 0}, {-dx, 1}
            };
            for (int k = 0; k < 8; k++) {
                int tx = ptx + order[k][0];
                int ty = pty + order[k][1];
                if ((unsigned)tx < (unsigned)WORLD_W &&
                    (unsigned)ty < (unsigned)WORLD_H &&
                    world[ty][tx] != TILE_AIR) {
                    u8 t = world[ty][tx];
                    inv[t]++;
                    if (inv[t] > 99) inv[t] = 99;
                    world[ty][tx] = TILE_AIR;
                    break;
                }
            }
        }
    } else {
        mine_timer = 0;
    }

    // ── Place block (tap R) ───────────────────────────────────
    if (pressed & KEY_R) {
        // Skip air
        if (inv[hotbar_sel] > 0) {
            int ptx = px / TILE_SIZE;
            int pty = py / TILE_SIZE;
            int dx  = facing ? 1 : -1;

            int cands[6][2] = {
                {dx,  0}, {dx,  1}, {dx, -1},
                { 0,  1}, { 0, -1}, {-dx, 1}
            };
            for (int k = 0; k < 6; k++) {
                int tx = ptx + cands[k][0];
                int ty = pty + cands[k][1];
                if ((unsigned)tx < (unsigned)WORLD_W &&
                    (unsigned)ty < (unsigned)WORLD_H &&
                    world[ty][tx] == TILE_AIR) {
                    // Don't place inside player
                    int bpx = tx * TILE_SIZE, bpy = ty * TILE_SIZE;
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
    }

    // ── Cycle hotbar with L ───────────────────────────────────
    if (pressed & KEY_L) {
        hotbar_sel++;
        if (hotbar_sel >= TILE_COUNT) hotbar_sel = 1;
    }

    // ── Toggle HUD with SELECT ────────────────────────────────
    if (pressed & KEY_SELECT) show_hud ^= 1;
}

// ─────────────────────────────────────────────────────────────
//  RENDERING
// ─────────────────────────────────────────────────────────────
#define COL_SKY_TOP   RGB15( 5, 14, 27)
#define COL_SKY_BOT   RGB15(12, 20, 31)
#define COL_CAVE_SKY  RGB15( 2,  2,  4)
#define COL_PLAYER_HEAD RGB15(28, 20, 13)
#define COL_PLAYER_BODY RGB15( 4,  8, 22)
#define COL_PLAYER_LEGS RGB15( 3,  6, 16)
#define COL_PLAYER_EYE  RGB15( 0,  0,  0)
#define COL_MINE_ANIM   RGB15(31, 28,  0)
#define COL_HUD_BG      RGB15( 0,  0,  0)
#define COL_HUD_SEL     RGB15(28, 22,  0)
#define COL_HUD_TXT     RGB15(31, 31, 31)
#define COL_DEPTH_BAR   RGB15(20, 14,  4)

// Animated mining sparkle timer
static int spark_t = 0;

static void render_sky(void) {
    // Gradient sky: lighter at top, deeper blue at horizon
    // Underground: dark
    int cam_ty = cam_y / TILE_SIZE;
    for (int sy = 0; sy < SCREEN_H; sy++) {
        int world_ty = cam_ty + sy / TILE_SIZE;
        u16 color;
        if (world_ty >= SURFACE_Y + 6) {
            // Underground – very dark
            int darkness = world_ty - (SURFACE_Y + 6);
            int r = 2 - darkness; if (r < 0) r = 0;
            int g = 2 - darkness; if (g < 0) g = 0;
            int b = 5 - darkness; if (b < 0) b = 0;
            color = RGB15(r, g, b);
        } else {
            // Sky gradient
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

            TileType t = (TileType)world[ty][tx];
            if (t == TILE_AIR) continue;

            draw_tile(tdx * TILE_SIZE - offx, sy, TILE_COL[t]);
        }
    }
}

static void render_player(void) {
    int sx = px - cam_x;
    int sy = py - cam_y;

    // Head (centered on body)
    fill_rect(sx, sy - 7, 6, 7, COL_PLAYER_HEAD);

    // Eye (left or right depending on facing)
    if (facing)
        put_pixel(sx + 4, sy - 5, COL_PLAYER_EYE);
    else
        put_pixel(sx + 1, sy - 5, COL_PLAYER_EYE);

    // Body
    fill_rect(sx, sy, 6, 7, COL_PLAYER_BODY);

    // Legs (split, animated by pvx)
    int leg_anim = (pvx != 0 || !on_ground) ? ((spark_t / 4) & 1) : 0;
    if (leg_anim) {
        fill_rect(sx,     sy + 7, 3, 5, COL_PLAYER_LEGS);
        fill_rect(sx + 3, sy + 5, 3, 7, COL_PLAYER_LEGS);
    } else {
        fill_rect(sx,     sy + 7, 3, 5, COL_PLAYER_LEGS);
        fill_rect(sx + 3, sy + 7, 3, 5, COL_PLAYER_LEGS);
    }

    // Mining sparkle
    if (mine_timer > 2 && mine_timer < MINE_DELAY) {
        int blink = (mine_timer & 1);
        if (blink) {
            int mx = sx + (facing ? 8 : -4);
            int my = sy;
            put_pixel(mx,   my,   COL_MINE_ANIM);
            put_pixel(mx+1, my-1, COL_MINE_ANIM);
            put_pixel(mx,   my+2, COL_MINE_ANIM);
        }
    }
}

// Minimal 3x5 font for HUD (digits 0-9 only)
static const u8 FONT_3x5[10][5] = {
    {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
    {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
    {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
    {0b111, 0b001, 0b011, 0b001, 0b111}, // 3
    {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
    {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
    {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
    {0b111, 0b001, 0b011, 0b010, 0b010}, // 7
    {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
    {0b111, 0b101, 0b111, 0b001, 0b111}, // 9
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

    // ── Depth / height indicator (top-left) ──────────────────
    int depth = (py / TILE_SIZE) - SURFACE_Y;
    fill_rect(0, 0, 64, 10, COL_HUD_BG);

    if (depth >= 0) {
        // Underground depth bar
        int bar = depth * 3;
        if (bar > 60) bar = 60;
        fill_rect(2, 3, bar, 4, COL_DEPTH_BAR);
        draw_number(2, 2, depth, COL_HUD_TXT);
    } else {
        // Height above surface
        int ht = -depth;
        if (ht > 60) ht = 60;
        fill_rect(2, 3, ht, 4, RGB15(5, 22, 28));
        draw_number(2, 2, ht, COL_HUD_TXT);
    }

    // ── Hotbar (bottom of screen) ─────────────────────────────
    int bar_x = (SCREEN_W - (TILE_COUNT - 1) * 12) / 2;
    int bar_y = SCREEN_H - 12;

    // Background strip
    fill_rect(bar_x - 2, bar_y - 1, (TILE_COUNT - 1) * 12 + 2, 12, COL_HUD_BG);

    for (int i = 1; i < TILE_COUNT; i++) {
        int sx = bar_x + (i - 1) * 12;

        // Selection highlight
        if (i == hotbar_sel)
            fill_rect(sx - 1, bar_y - 1, 10, 11, COL_HUD_SEL);

        // Tile swatch
        if (inv[i] > 0)
            fill_rect(sx, bar_y, TILE_SIZE, TILE_SIZE, TILE_COL[i]);
        else
            fill_rect(sx, bar_y, TILE_SIZE, TILE_SIZE, RGB15(4, 4, 4));

        // Count
        if (inv[i] > 0)
            draw_number(sx, bar_y - 6, inv[i], COL_HUD_TXT);
    }

    // Currently selected tile name indicator (top-right)
    {
        int tx = SCREEN_W - 36;
        fill_rect(tx - 2, 0, 38, 8, COL_HUD_BG);
        // Draw tile swatch
        fill_rect(tx, 0, 7, 7, TILE_COL[hotbar_sel]);
    }
}

// ─────────────────────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────────────────────
int main(void) {
    irqInit();
    irqEnable(IRQ_VBLANK);

    // Mode 3: 240x160, 16bpp direct bitmap
    REG_DISPCNT = MODE_3 | BG2_ON;

    // Generate world
    generate_world();
    memset(inv, 0, sizeof(inv));

    // Give the player a few starter blocks
    inv[TILE_DIRT]  = 10;
    inv[TILE_STONE] = 5;
    hotbar_sel = TILE_DIRT;

    // Spawn player on surface
    px = (WORLD_W / 2) * TILE_SIZE;
    py = (SURFACE_Y - 3) * TILE_SIZE;
    pvx = pvy = 0;
    on_ground = 0;
    facing = 1;

    // Init camera
    cam_x = px - SCREEN_W/2;
    cam_y = py - SCREEN_H/2;

    while (1) {
    VBlankIntrWait();
    handle_input();
    update_physics();

    render_sky();
    render_world();
    render_player();
    render_hud();

    spark_t++;
}

    return 0;
}
// ============================================================
//  TERRARIA GBA
//  A Terraria-inspired 2D sandbox game for Game Boy Advance
// ============================================================
//
//  Controls:
//    D-Pad Left/Right  : Move
//    A                 : Jump (hold for higher jump)
//    B (hold)          : Mine block in front / below
//    R                 : Place selected block
//    L                 : Cycle selected block in inventory
//    START             : (reserved – pause)
//    SELECT            : Toggle HUD
//
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

// ─────────────────────────────────────────────────────────────
//  WORLD
// ─────────────────────────────────────────────────────────────
#define TILE_SIZE   8
#define WORLD_W   128
#define WORLD_H    64
#define SURFACE_Y  16   // tiles from top where ground starts

#define VIEW_W  (SCREEN_W / TILE_SIZE)   // 30 tiles wide
#define VIEW_H  (SCREEN_H / TILE_SIZE)   // 20 tiles tall

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
//  COLORS  (RGB555: 5 bits each, packed as 0bbbbbgggggrrrrr)
// ─────────────────────────────────────────────────────────────
#define RGB15(r,g,b)  ((u16)((r) | ((g)<<5) | ((b)<<10)))

static const u16 TILE_COL[TILE_COUNT] = {
    RGB15( 0,  0,  0),   // AIR
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
#define PLAYER_TOTAL_H  18  // body + head
#define WALK_SPEED    2
#define JUMP_VEL     -9
#define GRAVITY       1
#define MAX_FALL      9
#define MINE_DELAY    8   // frames between each mine tick

static int px, py;          // pixel position (world space)
static int pvx, pvy;        // velocity
static int on_ground;
static int facing;          // 0 = left, 1 = right

// ─────────────────────────────────────────────────────────────
//  INVENTORY
// ─────────────────────────────────────────────────────────────
static u8  inv[TILE_COUNT];   // count of each tile
static int hotbar_sel = 1;    // currently selected slot (1..TILE_COUNT-1)
static int show_hud   = 1;

// ─────────────────────────────────────────────────────────────
//  CAMERA
// ─────────────────────────────────────────────────────────────
static int cam_x, cam_y;

// ─────────────────────────────────────────────────────────────
//  RNG  (xorshift32)
// ─────────────────────────────────────────────────────────────
static u32 rng_state = 0xDEADBEEF;
static u32 rng(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state <<  5;
    return rng_state;
}

// ─────────────────────────────────────────────────────────────
//  COLOR HELPERS
// ─────────────────────────────────────────────────────────────
static u16 lighten(u16 c) {
    int r = (c & 0x1F) + 5;  if (r > 31) r = 31;
    int g = ((c >>  5) & 0x1F) + 5; if (g > 31) g = 31;
    int b = ((c >> 10) & 0x1F) + 5; if (b > 31) b = 31;
    return (u16)(r | (g<<5) | (b<<10));
}
static u16 darken(u16 c) {
    int r = (c & 0x1F) - 3;  if (r < 0) r = 0;
    int g = ((c >>  5) & 0x1F) - 3; if (g < 0) g = 0;
    int b = ((c >> 10) & 0x1F) - 3; if (b < 0) b = 0;
    return (u16)(r | (g<<5) | (b<<10));
}

// ─────────────────────────────────────────────────────────────
//  FAST PIXEL / FILL ROUTINES
// ─────────────────────────────────────────────────────────────
static inline void put_pixel(int x, int y, u16 color) {
    if ((unsigned)x < (unsigned)SCREEN_W && (unsigned)y < (unsigned)SCREEN_H)
        ((u16*)VRAM)[y * SCREEN_W + x] = color;
}

// 32-bit wide fill for speed
static void fill_row(int x, int y, int w, u16 color) {
    if ((unsigned)y >= (unsigned)SCREEN_H) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (w <= 0) return;

    u16 *dst = (u16*)VRAM + y * SCREEN_W + x;
    // Odd pixel alignment
    if ((int)dst & 2) { *dst++ = color; w--; }
    // 32-bit bulk fill
    u32  c32 = (u32)color | ((u32)color << 16);
    u32 *d32 = (u32*)dst;
    int  n32 = w >> 1;
    for (int i = 0; i < n32; i++) *d32++ = c32;
    if (w & 1) *(u16*)d32 = color;
}

static void fill_rect(int x, int y, int w, int h, u16 color) {
    for (int dy = 0; dy < h; dy++) fill_row(x, y + dy, w, color);
}

// ─────────────────────────────────────────────────────────────
//  TILE DRAW  (with 1-pixel bevelled border for depth)
// ─────────────────────────────────────────────────────────────
static void draw_tile(int sx, int sy, u16 color) {
    u16 hi = lighten(color);
    u16 lo = darken(color);

    for (int dy = 0; dy < TILE_SIZE; dy++) {
        int y = sy + dy;
        if ((unsigned)y >= (unsigned)SCREEN_H) continue;
        int x0 = sx < 0 ? -sx : 0;
        int x1 = (sx + TILE_SIZE > SCREEN_W) ? SCREEN_W - sx : TILE_SIZE;
        if (x0 >= x1) continue;

        u16 *row = (u16*)VRAM + y * SCREEN_W + sx;
        for (int dx = x0; dx < x1; dx++) {
            u16 c;
            if (dy == 0 || dx == 0)                         c = hi;
            else if (dy == TILE_SIZE-1 || dx == TILE_SIZE-1) c = lo;
            else                                              c = color;
            row[dx] = c;
        }
    }
}

// ─────────────────────────────────────────────────────────────
//  WORLD GENERATION
// ─────────────────────────────────────────────────────────────
static void generate_world(void) {
    memset(world, TILE_AIR, sizeof(world));

    // Build heightmap with a random walk
    int heights[WORLD_W];
    heights[0] = SURFACE_Y;
    for (int x = 1; x < WORLD_W; x++) {
        int d = (int)(rng() % 3) - 1;
        heights[x] = heights[x-1] + d;
        if (heights[x] < SURFACE_Y - 5) heights[x] = SURFACE_Y - 5;
        if (heights[x] > SURFACE_Y + 6) heights[x] = SURFACE_Y + 6;
    }

    // Fill columns
    for (int x = 0; x < WORLD_W; x++) {
        int h = heights[x];
        // Grass cap
        if (h >= 0 && h < WORLD_H)
            world[h][x] = TILE_GRASS;
        // Dirt band (5 tiles)
        for (int y = h+1; y < h+6 && y < WORLD_H; y++)
            world[y][x] = TILE_DIRT;
        // Stone band
        int stone_top = h + 6;
        int deep_top  = WORLD_H * 3 / 4;
        for (int y = stone_top; y < deep_top && y < WORLD_H; y++)
            world[y][x] = TILE_STONE;
        // Deep stone
        for (int y = deep_top; y < WORLD_H; y++)
            world[y][x] = TILE_DEEP_STONE;
    }

    // Scatter sand patches near surface
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

    // Gravel pockets (mid layer)
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

    // Caves (elliptical tunnels)
    for (int k = 0; k < 45; k++) {
        int cx = rng() % WORLD_W;
        int cy = (SURFACE_Y + 8) + rng() % (WORLD_H - SURFACE_Y - 14);
        int rx = 2 + rng() % 5;
        int ry = 1 + rng() % 3;
        for (int dy = -ry; dy <= ry; dy++)
            for (int dx = -rx; dx <= rx; dx++) {
                // Ellipse check: (dx/rx)^2 + (dy/ry)^2 <= 1
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
    if ((unsigned)tx >= (unsigned)WORLD_W) return 1;
    if (ty < 0) return 0;
    if ((unsigned)ty >= (unsigned)WORLD_H) return 1;
    return world[ty][tx] != TILE_AIR;
}

// Test whether a world-space rect overlaps any solid tile
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
    // Gravity
    pvy += GRAVITY;
    if (pvy > MAX_FALL) pvy = MAX_FALL;

    // Horizontal move
    if (pvx) {
        int nx = px + pvx;
        if (!rect_hits_world(nx, py, PLAYER_W, PLAYER_H))
            px = nx;
        else
            pvx = 0;
    }

    // Friction
    pvx = pvx * 3 / 4;

    // Vertical move
    {
        int ny = py + pvy;
        if (!rect_hits_world(px, ny, PLAYER_W, PLAYER_H)) {
            py = ny;
            if (pvy > 0) on_ground = 0;
        } else {
            if (pvy > 0) on_ground = 1;
            pvy = 0;
        }
    }

    // Clamp to world
    if (px < 0) { px = 0; pvx = 0; }
    if (px + PLAYER_W > WORLD_W * TILE_SIZE) { px = WORLD_W * TILE_SIZE - PLAYER_W; pvx = 0; }
    if (py < 0) { py = 0; pvy = 0; }

    // Smooth camera follow
    int tcx = px - SCREEN_W/2 + PLAYER_W/2;
    int tcy = py - SCREEN_H/2 + PLAYER_H/2 - 8;  // slight upward bias
    cam_x += (tcx - cam_x + 2) / 4;
    cam_y += (tcy - cam_y + 2) / 4;

    // Clamp camera
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

    // ── Movement ──────────────────────────────────────────────
    if (held & KEY_LEFT)  { pvx -= 2; facing = 0; }
    if (held & KEY_RIGHT) { pvx += 2; facing = 1; }
    if (pvx >  WALK_SPEED) pvx =  WALK_SPEED;
    if (pvx < -WALK_SPEED) pvx = -WALK_SPEED;

    // ── Jump ──────────────────────────────────────────────────
    if ((pressed & KEY_A) && on_ground) {
        pvy = JUMP_VEL;
        on_ground = 0;
    }

    // ── Mining (hold B) ───────────────────────────────────────
    if (held & KEY_B) {
        mine_timer++;
        if (mine_timer >= MINE_DELAY) {
            mine_timer = 0;
            int ptx = px / TILE_SIZE;
            int pty = py / TILE_SIZE;
            int dx  = facing ? 1 : -1;

            // Priority order of target blocks
            int order[8][2] = {
                {dx,  0}, {dx,  1}, {dx, -1},
                { 0,  1}, { 0,  0}, { 0, -1},
                {-dx, 0}, {-dx, 1}
            };
            for (int k = 0; k < 8; k++) {
                int tx = ptx + order[k][0];
                int ty = pty + order[k][1];
                if ((unsigned)tx < (unsigned)WORLD_W &&
                    (unsigned)ty < (unsigned)WORLD_H &&
                    world[ty][tx] != TILE_AIR) {
                    u8 t = world[ty][tx];
                    inv[t]++;
                    if (inv[t] > 99) inv[t] = 99;
                    world[ty][tx] = TILE_AIR;
                    break;
                }
            }
        }
    } else {
        mine_timer = 0;
    }

    // ── Place block (tap R) ───────────────────────────────────
    if (pressed & KEY_R) {
        // Skip air
        if (inv[hotbar_sel] > 0) {
            int ptx = px / TILE_SIZE;
            int pty = py / TILE_SIZE;
            int dx  = facing ? 1 : -1;

            int cands[6][2] = {
                {dx,  0}, {dx,  1}, {dx, -1},
                { 0,  1}, { 0, -1}, {-dx, 1}
            };
            for (int k = 0; k < 6; k++) {
                int tx = ptx + cands[k][0];
                int ty = pty + cands[k][1];
                if ((unsigned)tx < (unsigned)WORLD_W &&
                    (unsigned)ty < (unsigned)WORLD_H &&
                    world[ty][tx] == TILE_AIR) {
                    // Don't place inside player
                    int bpx = tx * TILE_SIZE, bpy = ty * TILE_SIZE;
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
    }

    // ── Cycle hotbar with L ───────────────────────────────────
    if (pressed & KEY_L) {
        hotbar_sel++;
        if (hotbar_sel >= TILE_COUNT) hotbar_sel = 1;
    }

    // ── Toggle HUD with SELECT ────────────────────────────────
    if (pressed & KEY_SELECT) show_hud ^= 1;
}

// ─────────────────────────────────────────────────────────────
//  RENDERING
// ─────────────────────────────────────────────────────────────
#define COL_SKY_TOP   RGB15( 5, 14, 27)
#define COL_SKY_BOT   RGB15(12, 20, 31)
#define COL_CAVE_SKY  RGB15( 2,  2,  4)
#define COL_PLAYER_HEAD RGB15(28, 20, 13)
#define COL_PLAYER_BODY RGB15( 4,  8, 22)
#define COL_PLAYER_LEGS RGB15( 3,  6, 16)
#define COL_PLAYER_EYE  RGB15( 0,  0,  0)
#define COL_MINE_ANIM   RGB15(31, 28,  0)
#define COL_HUD_BG      RGB15( 0,  0,  0)
#define COL_HUD_SEL     RGB15(28, 22,  0)
#define COL_HUD_TXT     RGB15(31, 31, 31)
#define COL_DEPTH_BAR   RGB15(20, 14,  4)

// Animated mining sparkle timer
static int spark_t = 0;

static void render_sky(void) {
    // Gradient sky: lighter at top, deeper blue at horizon
    // Underground: dark
    int cam_ty = cam_y / TILE_SIZE;
    for (int sy = 0; sy < SCREEN_H; sy++) {
        int world_ty = cam_ty + sy / TILE_SIZE;
        u16 color;
        if (world_ty >= SURFACE_Y + 6) {
            // Underground – very dark
            int darkness = world_ty - (SURFACE_Y + 6);
            int r = 2 - darkness; if (r < 0) r = 0;
            int g = 2 - darkness; if (g < 0) g = 0;
            int b = 5 - darkness; if (b < 0) b = 0;
            color = RGB15(r, g, b);
        } else {
            // Sky gradient
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

            TileType t = (TileType)world[ty][tx];
            if (t == TILE_AIR) continue;

            draw_tile(tdx * TILE_SIZE - offx, sy, TILE_COL[t]);
        }
    }
}

static void render_player(void) {
    int sx = px - cam_x;
    int sy = py - cam_y;

    // Head (centered on body)
    fill_rect(sx, sy - 7, 6, 7, COL_PLAYER_HEAD);

    // Eye (left or right depending on facing)
    if (facing)
        put_pixel(sx + 4, sy - 5, COL_PLAYER_EYE);
    else
        put_pixel(sx + 1, sy - 5, COL_PLAYER_EYE);

    // Body
    fill_rect(sx, sy, 6, 7, COL_PLAYER_BODY);

    // Legs (split, animated by pvx)
    int leg_anim = (pvx != 0 || !on_ground) ? ((spark_t / 4) & 1) : 0;
    if (leg_anim) {
        fill_rect(sx,     sy + 7, 3, 5, COL_PLAYER_LEGS);
        fill_rect(sx + 3, sy + 5, 3, 7, COL_PLAYER_LEGS);
    } else {
        fill_rect(sx,     sy + 7, 3, 5, COL_PLAYER_LEGS);
        fill_rect(sx + 3, sy + 7, 3, 5, COL_PLAYER_LEGS);
    }

    // Mining sparkle
    if (mine_timer > 2 && mine_timer < MINE_DELAY) {
        int blink = (mine_timer & 1);
        if (blink) {
            int mx = sx + (facing ? 8 : -4);
            int my = sy;
            put_pixel(mx,   my,   COL_MINE_ANIM);
            put_pixel(mx+1, my-1, COL_MINE_ANIM);
            put_pixel(mx,   my+2, COL_MINE_ANIM);
        }
    }
}

// Minimal 3x5 font for HUD (digits 0-9 only)
static const u8 FONT_3x5[10][5] = {
    {0b111, 0b101, 0b101, 0b101, 0b111}, // 0
    {0b010, 0b110, 0b010, 0b010, 0b111}, // 1
    {0b111, 0b001, 0b111, 0b100, 0b111}, // 2
    {0b111, 0b001, 0b011, 0b001, 0b111}, // 3
    {0b101, 0b101, 0b111, 0b001, 0b001}, // 4
    {0b111, 0b100, 0b111, 0b001, 0b111}, // 5
    {0b111, 0b100, 0b111, 0b101, 0b111}, // 6
    {0b111, 0b001, 0b011, 0b010, 0b010}, // 7
    {0b111, 0b101, 0b111, 0b101, 0b111}, // 8
    {0b111, 0b101, 0b111, 0b001, 0b111}, // 9
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

    // ── Depth / height indicator (top-left) ──────────────────
    int depth = (py / TILE_SIZE) - SURFACE_Y;
    fill_rect(0, 0, 64, 10, COL_HUD_BG);

    if (depth >= 0) {
        // Underground depth bar
        int bar = depth * 3;
        if (bar > 60) bar = 60;
        fill_rect(2, 3, bar, 4, COL_DEPTH_BAR);
        draw_number(2, 2, depth, COL_HUD_TXT);
    } else {
        // Height above surface
        int ht = -depth;
        if (ht > 60) ht = 60;
        fill_rect(2, 3, ht, 4, RGB15(5, 22, 28));
        draw_number(2, 2, ht, COL_HUD_TXT);
    }

    // ── Hotbar (bottom of screen) ─────────────────────────────
    int bar_x = (SCREEN_W - (TILE_COUNT - 1) * 12) / 2;
    int bar_y = SCREEN_H - 12;

    // Background strip
    fill_rect(bar_x - 2, bar_y - 1, (TILE_COUNT - 1) * 12 + 2, 12, COL_HUD_BG);

    for (int i = 1; i < TILE_COUNT; i++) {
        int sx = bar_x + (i - 1) * 12;

        // Selection highlight
        if (i == hotbar_sel)
            fill_rect(sx - 1, bar_y - 1, 10, 11, COL_HUD_SEL);

        // Tile swatch
        if (inv[i] > 0)
            fill_rect(sx, bar_y, TILE_SIZE, TILE_SIZE, TILE_COL[i]);
        else
            fill_rect(sx, bar_y, TILE_SIZE, TILE_SIZE, RGB15(4, 4, 4));

        // Count
        if (inv[i] > 0)
            draw_number(sx, bar_y - 6, inv[i], COL_HUD_TXT);
    }

    // Currently selected tile name indicator (top-right)
    {
        int tx = SCREEN_W - 36;
        fill_rect(tx - 2, 0, 38, 8, COL_HUD_BG);
        // Draw tile swatch
        fill_rect(tx, 0, 7, 7, TILE_COL[hotbar_sel]);
    }
}

// ─────────────────────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────────────────────
int main(void) {
    irqInit();
    irqEnable(IRQ_VBLANK);

    // Mode 3: 240x160, 16bpp direct bitmap
    REG_DISPCNT = MODE_3 | BG2_ON;

    // Generate world
    generate_world();
    memset(inv, 0, sizeof(inv));

    // Give the player a few starter blocks
    inv[TILE_DIRT]  = 10;
    inv[TILE_STONE] = 5;
    hotbar_sel = TILE_DIRT;

    // Spawn player on surface
    px = (WORLD_W / 2) * TILE_SIZE;
    py = (SURFACE_Y - 3) * TILE_SIZE;
    pvx = pvy = 0;
    on_ground = 0;
    facing = 1;

    // Init camera
    cam_x = px - SCREEN_W/2;
    cam_y = py - SCREEN_H/2;

    while (1) {
    VBlankIntrWait();
    handle_input();
    update_physics();

    render_sky();
    render_world();
    render_player();
    render_hud();

    spark_t++;
}

    return 0;
}
