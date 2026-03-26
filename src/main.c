// ============================================================
//  TERRARIA GBA COMPLETE BUILD
// ============================================================

#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <gba_sound.h>
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
#define SURFACE_Y  16
#define VIEW_W  (SCREEN_W / TILE_SIZE)
#define VIEW_H  (SCREEN_H / TILE_SIZE)

typedef enum {
    TILE_AIR = 0,
    TILE_GRASS, TILE_DIRT, TILE_STONE,
    TILE_DEEP_STONE, TILE_GRAVEL, TILE_SAND,
    TILE_COUNT
} TileType;

#define RGB15(r,g,b)  ((u16)((r) | ((g)<<5) | ((b)<<10)))
static const u16 TILE_COL[TILE_COUNT] = {
    RGB15(0,0,0), RGB15(6,22,5), RGB15(16,11,6),
    RGB15(14,14,15), RGB15(8,8,10), RGB15(17,15,10),
    RGB15(26,22,8)
};

static u8 world[WORLD_H][WORLD_W];

// ─────────────────────────────────────────────────────────────
//  PLAYER
// ─────────────────────────────────────────────────────────────
#define PLAYER_W 6
#define PLAYER_H 12
#define PLAYER_TOTAL_H 18
#define WALK_SPEED 2
#define JUMP_VEL -9
#define GRAVITY 1
#define MAX_FALL 9
#define MINE_DELAY 8

static int px, py;
static int pvx, pvy;
static int on_ground;
static int facing;   // 0 = left, 1 = right

// Inventory
static u8 inv[TILE_COUNT];
static int hotbar_sel = 1;
static int show_hud = 1;

// Camera
static int cam_x, cam_y;

// RNG
static u32 rng_state = 0xDEADBEEF;
static u32 rng(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

// ─────────────────────────────────────────────────────────────
//  SOUND & MUSIC
// ─────────────────────────────────────────────────────────────
static int music_timer = 0;
static int music_step = 0;
static const u16 music_notes[] = {880,988,1046,1174,1046,988,880,784};

static void audio_init(void){
    REG_SOUNDCNT_X = SND_ENABLED;
    REG_SOUNDCNT_L = 0x1177;
    REG_SOUNDCNT_H = SND_OUTPUT_RATIO_100 | SND_OUTPUT_A_TO_BOTH | SND_OUTPUT_B_TO_BOTH;
}

static void audio_pan(int x){
    int left = 7, right = 7;
    if(x < 120) right = 3; else left = 3;
    REG_SOUNDCNT_L = (left<<8)|(right)|(left<<12)|(right<<4);
}

static void sfx_jump(void){
    REG_SOUND1CNT_L = 0;
    REG_SOUND1CNT_H = SOUND_DUTY_50 | SOUND_ENV_INIT(12);
    REG_SOUND1CNT_X = SOUND_FREQ(900) | SOUND_PLAY;
}

static void sfx_mine(void){
    REG_SOUND4CNT_L = SOUND_ENV_INIT(10);
    REG_SOUND4CNT_H = SOUND_RNG_7 | SOUND_FREQ(3) | SOUND_PLAY;
}

static void sfx_hurt(void){
    REG_SOUND2CNT_L = SOUND_DUTY_75 | SOUND_ENV_INIT(15);
    REG_SOUND2CNT_H = SOUND_FREQ(200) | SOUND_PLAY;
}

static void music_update(void){
    music_timer++;
    if(music_timer > 20){
        music_timer = 0;
        u16 freq = music_notes[music_step];
        REG_SOUND1CNT_L = 0;
        REG_SOUND1CNT_H = SOUND_DUTY_50 | SOUND_ENV_INIT(8);
        REG_SOUND1CNT_X = SOUND_FREQ(freq) | SOUND_PLAY;
        music_step++; if(music_step>=8) music_step=0;
    }
}

// ─────────────────────────────────────────────────────────────
//  PIXEL / DRAW
// ─────────────────────────────────────────────────────────────
static inline void put_pixel(int x,int y,u16 color){
    if((unsigned)x<SCREEN_W && (unsigned)y<SCREEN_H)
        ((u16*)VRAM)[y*SCREEN_W+x] = color;
}
static void fill_row(int x,int y,int w,u16 color){
    if((unsigned)y>=SCREEN_H) return;
    if(x<0){ w+=x; x=0; }
    if(x+w>SCREEN_W) w=SCREEN_W-x;
    if(w<=0) return;
    u16 *dst = (u16*)VRAM + y*SCREEN_W + x;
    if((int)dst&2){ *dst++=color; w--; }
    u32 c32 = (u32)color|((u32)color<<16);
    u32 *d32 = (u32*)dst;
    int n32 = w>>1;
    for(int i=0;i<n32;i++) *d32++=c32;
    if(w&1) *(u16*)d32=color;
}
static void fill_rect(int x,int y,int w,int h,u16 color){
    for(int dy=0;dy<h;dy++) fill_row(x,y+dy,w,color);
}
static void draw_tile(int sx,int sy,u16 color){
    u16 hi = (color&0x1F)+5; if(hi>31) hi=31;
    u16 lo = ((color>>5)&0x1F)+5; if(lo>31) lo=31;
    for(int dy=0;dy<TILE_SIZE;dy++){
        int y=sy+dy; if((unsigned)y>=SCREEN_H) continue;
        int x0 = sx<0?-sx:0;
        int x1 = (sx+TILE_SIZE>SCREEN_W)?SCREEN_W-sx:TILE_SIZE;
        if(x0>=x1) continue;
        u16 *row = (u16*)VRAM + y*SCREEN_W + sx;
        for(int dx=x0;dx<x1;dx++){
            u16 c = (dy==0||dx==0)?hi:(dy==TILE_SIZE-1||dx==TILE_SIZE-1)?lo:color;
            row[dx] = c;
        }
    }
}

// ─────────────────────────────────────────────────────────────
//  WORLD GENERATION
// ─────────────────────────────────────────────────────────────
static void generate_world(void){
    memset(world,TILE_AIR,sizeof(world));
    int heights[WORLD_W]; heights[0]=SURFACE_Y;
    for(int x=1;x<WORLD_W;x++){
        int d = (int)(rng()%3)-1;
        heights[x]=heights[x-1]+d;
        if(heights[x]<SURFACE_Y-5) heights[x]=SURFACE_Y-5;
        if(heights[x]>SURFACE_Y+6) heights[x]=SURFACE_Y+6;
    }
    for(int x=0;x<WORLD_W;x++){
        int h=heights[x];
        if(h>=0 && h<WORLD_H) world[h][x]=TILE_GRASS;
        for(int y=h+1;y<h+6 && y<WORLD_H;y++) world[y][x]=TILE_DIRT;
        int stone_top=h+6;
        int deep_top=WORLD_H*3/4;
        for(int y=stone_top;y<deep_top && y<WORLD_H;y++) world[y][x]=TILE_STONE;
        for(int y=deep_top;y<WORLD_H;y++) world[y][x]=TILE_DEEP_STONE;
    }
}

// ─────────────────────────────────────────────────────────────
//  COLLISION
// ─────────────────────────────────────────────────────────────
static int tile_solid(int tx,int ty){
    if((unsigned)tx>=WORLD_W) return 1;
    if(ty<0) return 0;
    if((unsigned)ty>=WORLD_H) return 1;
    return world[ty][tx]!=TILE_AIR;
}
static int rect_hits_world(int rx,int ry,int rw,int rh){
    int tx0=rx/TILE_SIZE; int ty0=ry/TILE_SIZE;
    int tx1=(rx+rw-1)/TILE_SIZE; int ty1=(ry+rh-1)/TILE_SIZE;
    for(int ty=ty0;ty<=ty1;ty++)
        for(int tx=tx0;tx<=tx1;tx++)
            if(tile_solid(tx,ty)) return 1;
    return 0;
}

// ─────────────────────────────────────────────────────────────
//  PHYSICS
// ─────────────────────────────────────────────────────────────
static void update_physics(void){
    pvy+=GRAVITY; if(pvy>MAX_FALL) pvy=MAX_FALL;
    if(pvx){ int nx=px+pvx; if(!rect_hits_world(nx,py,PLAYER_W,PLAYER_H)) px=nx; else pvx=0; }
    pvx=pvx*3/4;
    int ny=py+pvy; if(!rect_hits_world(px,ny,PLAYER_W,PLAYER_H)){ py=ny; if(pvy>0) on_ground=0;} else { if(pvy>0) on_ground=1; pvy=0; }
    if(px<0){px=0;pvx=0;} if(px+PLAYER_W>WORLD_W*TILE_SIZE){px=WORLD_W*TILE_SIZE-PLAYER_W;pvx=0;} if(py<0){py=0;pvy=0;}
    int tcx=px-SCREEN_W/2+PLAYER_W/2;
    int tcy=py-SCREEN_H/2+PLAYER_H/2-8;
    cam_x+=(tcx-cam_x+2)/4; cam_y+=(tcy-cam_y+2)/4;
    int max_cx=WORLD_W*TILE_SIZE-SCREEN_W;
    int max_cy=WORLD_H*TILE_SIZE-SCREEN_H;
    if(cam_x<0) cam_x=0; if(cam_y<0) cam_y=0; if(cam_x>max_cx) cam_x=max_cx; if(cam_y>max_cy) cam_y=max_cy;
}

// ─────────────────────────────────────────────────────────────
//  INPUT
// ─────────────────────────────────────────────────────────────
static int mine_timer=0;
static void handle_input(void){
    scanKeys();
    u32 held = keysHeld();
    u32 pressed = keysDown();
    if(held&KEY_LEFT){ pvx-=2; facing=0; }
    if(held&KEY_RIGHT){ pvx+=2; facing=1; }
    if(pvx>WALK_SPEED) pvx=WALK_SPEED; if(pvx<-WALK_SPEED) pvx=-WALK_SPEED;
    if((pressed&KEY_A)&&on_ground){ pvy=JUMP_VEL; on_ground=0; sfx_jump(); }
    if(held&KEY_B){ mine_timer++; if(mine_timer>=MINE_DELAY){ mine_timer=0; int ptx=px/TILE_SIZE; int pty=py/TILE_SIZE; int dx=facing?1:-1;
        int order[8][2]={{dx,0},{dx,1},{dx,-1},{0,1},{0,0},{0,-1},{-dx,0},{-dx,1}};
        for(int k=0;k<8;k++){ int tx=ptx+order[k][0]; int ty=pty+order[k][1];
            if((unsigned)tx<WORLD_W&&(unsigned)ty<WORLD_H&&world[ty][tx]!=TILE_AIR){ u8 t=world[ty][tx]; inv[t]++; if(inv[t]>99) inv[t]=99; world[ty][tx]=TILE_AIR; sfx_mine(); break; }
        }
    }} else mine_timer=0;
    if(pressed&KEY_R){ if(inv[hotbar_sel]>0){ int ptx=px/TILE_SIZE; int pty=py/TILE_SIZE; int dx=facing?1:-1; int cands[6][2]={{dx,0},{dx,1},{dx,-1},{0,1},{0,-1},{-dx,1}};
        for(int k=0;k<6;k++){ int tx=ptx+cands[k][0]; int ty=pty+cands[k][1]; if((unsigned)tx<WORLD_W&&(unsigned)ty<WORLD_H&&world[ty][tx]==TILE_AIR){ int bpx=tx*TILE_SIZE,bpy=ty*TILE_SIZE; if(!((bpx<px+PLAYER_W)&&(bpx+TILE_SIZE>px)&&(bpy<py+PLAYER_H)&&(bpy+TILE_SIZE>py))){ world[ty][tx]=(u8)hotbar_sel; inv[hotbar_sel]--; break; } } }
    }}
    if(pressed&KEY_L){ hotbar_sel++; if(hotbar_sel>=TILE_COUNT) hotbar_sel=1; }
    if(pressed&KEY_SELECT) show_hud^=1;
}

// ─────────────────────────────────────────────────────────────
//  RENDER
// ─────────────────────────────────────────────────────────────
static void render_sky(void){
    for(int sy=0;sy<SCREEN_H;sy++){
        u16 color = (sy<SCREEN_H/2)?RGB15(5+sy*7/SCREEN_H,14+sy*6/SCREEN_H,27+sy*4/SCREEN_H):RGB15(2,2,4);
        fill_row(0,sy,SCREEN_W,color);
    }
}
static void render_world(void){
    int tx0=cam_x/TILE_SIZE, ty0=cam_y/TILE_SIZE, offx=cam_x%TILE_SIZE, offy=cam_y%TILE_SIZE;
    for(int tdy=0;tdy<=VIEW_H+1;tdy++){
        int ty=ty0+tdy; if(ty<0||ty>=WORLD_H) continue; int sy=tdy*TILE_SIZE-offy;
        for(int tdx=0;tdx<=VIEW_W+1;tdx++){
            int tx=tx0+tdx; if(tx<0||tx>=WORLD_W) continue;
            TileType t=(TileType)world[ty][tx]; if(t==TILE_AIR) continue;
            draw_tile(tdx*TILE_SIZE-offx,sy,TILE_COL[t]);
        }
    }
}
static void render_player(void){
    int sx=px-cam_x, sy=py-cam_y;
    fill_rect(sx,sy-7,6,7,RGB15(28,20,13));
    put_pixel(facing?sx+4:sx+1,sy-5,RGB15(0,0,0));
    fill_rect(sx,sy,6,7,RGB15(4,8,22));
    fill_rect(sx,sy+7,6,5,RGB15(3,6,16));
}

// ─────────────────────────────────────────────────────────────
//  MAIN
// ─────────────────────────────────────────────────────────────
int main(void){
    irqInit(); irqEnable(IRQ_VBLANK);
    REG_DISPCNT=MODE_3|BG2_ON;
    audio_init(); generate_world(); memset(inv,0,sizeof(inv));
    inv[TILE_DIRT]=10; inv[TILE_STONE]=5; hotbar_sel=TILE_DIRT;
    px=(WORLD_W/2)*TILE_SIZE; py=(SURFACE_Y-3)*TILE_SIZE; pvx=pvy=0; on_ground=0; facing=1;
    cam_x=px-SCREEN_W/2; cam_y=py-SCREEN_H/2;
    while(1){
        handle_input(); update_physics(); music_update();
        render_sky(); render_world(); render_player();
        VBlankIntrWait();
    }
    return 0;
}