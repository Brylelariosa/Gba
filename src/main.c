#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>

// ---- Screen ----
#define VRAM  ((volatile unsigned short*)0x06000000)
#define SW    240
#define SH    160

// ---- Colors (BGR555) ----
#define RGB15(r,g,b)  ((r)|((g)<<5)|((b)<<10))
#define COL_SKY    RGB15( 8,18,28)
#define COL_PIPE   RGB15( 4,22, 4)
#define COL_PIPEDARK RGB15(2,14,2)
#define COL_BIRD   RGB15(31,28, 0)
#define COL_BEAK   RGB15(31,16, 0)
#define COL_EYE    RGB15( 0, 0, 0)
#define COL_WHITE  RGB15(31,31,31)
#define COL_BLACK  RGB15( 0, 0, 0)
#define COL_GROUND RGB15(18,12, 4)
#define COL_GRASS  RGB15( 6,22, 4)
#define COL_RED    RGB15(28, 4, 4)

// ---- Draw primitives ----
static inline void pixel(int x, int y, unsigned short c) {
    if (x>=0 && x<SW && y>=0 && y<SH)
        VRAM[y*SW+x] = c;
}

static void rect(int x, int y, int w, int h, unsigned short c) {
    int i, j;
    for (j=y; j<y+h && j<SH; j++)
        for (i=x; i<x+w && i<SW; i++)
            if (i>=0 && j>=0)
                VRAM[j*SW+i] = c;
}

static void hline(int x, int y, int w, unsigned short c) {
    int i;
    for (i=x; i<x+w; i++) pixel(i,y,c);
}
static void vline(int x, int y, int h, unsigned short c) {
    int i;
    for (i=y; i<y+h; i++) pixel(x,i,c);
}

// ---- Tiny 4x5 pixel font (digits 0-9) ----
static const unsigned char FONT[10][5] = {
    {0x6,0x9,0x9,0x9,0x6}, // 0
    {0x2,0x6,0x2,0x2,0x7}, // 1
    {0x6,0x9,0x2,0x4,0xF}, // 2
    {0xE,0x1,0x6,0x1,0xE}, // 3
    {0x9,0x9,0xF,0x1,0x1}, // 4
    {0xF,0x8,0xE,0x1,0xE}, // 5
    {0x6,0x8,0xE,0x9,0x6}, // 6
    {0xF,0x1,0x2,0x4,0x4}, // 7
    {0x6,0x9,0x6,0x9,0x6}, // 8
    {0x6,0x9,0x7,0x1,0x6}, // 9
};

static void drawDigit(int x, int y, int d, unsigned short c) {
    int r, col;
    for (r=0; r<5; r++)
        for (col=0; col<4; col++)
            if (FONT[d][r] & (0x8>>col))
                rect(x+col*2, y+r*2, 2,2, c);
}

static void drawNum(int x, int y, int n, unsigned short c) {
    if (n>=100) drawDigit(x,y, n/100,    c), x+=10;
    if (n>=10)  drawDigit(x,y,(n/10)%10, c), x+=10;
    drawDigit(x,y,n%10,c);
}

// ---- Game constants ----
#define BIRD_X   40
#define BIRD_W    8
#define BIRD_H    8
#define PIPE_W   22
#define GAP      52
#define NPIPES    2
#define PSPACE  130
#define GROUND  148

// ---- Game state ----
static int  birdY, birdVel, score, dead, frame;

typedef struct { int x, top; } Pipe;
static Pipe pipes[NPIPES];

static unsigned int rng = 77771u;
static int randTop(void) {
    rng = rng*1664525u+1013904223u;
    return 12 + (int)((rng>>24) % (GROUND - GAP - 24));
}

static void init(void) {
    int i;
    birdY=SH/2; birdVel=0; score=0; dead=0; frame=0;
    for (i=0;i<NPIPES;i++) {
        pipes[i].x   = SW+10+i*PSPACE;
        pipes[i].top = randTop();
    }
}

// ---- Draw pipe with outline + cap ----
static void drawPipe(int px, int top) {
    int bot = top + GAP;
    // top pipe body
    rect(px, 0, PIPE_W, top, COL_PIPE);
    // top pipe cap (wider, darker)
    rect(px-3, top-8, PIPE_W+6, 8, COL_PIPEDARK);
    // bottom pipe cap
    rect(px-3, bot, PIPE_W+6, 8, COL_PIPE);
    // bottom pipe body
    rect(px, bot+8, PIPE_W, GROUND-(bot+8), COL_PIPE);
    // outlines
    vline(px,       0, top,          COL_BLACK);
    vline(px+PIPE_W-1, 0, top,       COL_BLACK);
    vline(px,       bot+8, GROUND-(bot+8), COL_BLACK);
    vline(px+PIPE_W-1, bot+8, GROUND-(bot+8), COL_BLACK);
}

// ---- Draw bird ----
static void drawBird(int by) {
    // body
    rect(BIRD_X, by, BIRD_W, BIRD_H, COL_BIRD);
    // beak
    rect(BIRD_X+BIRD_W, by+2, 3, 3, COL_BEAK);
    // eye
    rect(BIRD_X+5, by+1, 2, 2, COL_EYE);
    pixel(BIRD_X+6, by+1, COL_WHITE);
    // wing
    rect(BIRD_X+1, by+BIRD_H-3, 4, 2, RGB15(28,20,0));
}

// ---- Full scene draw ----
static void draw(void) {
    int i;
    // sky
    rect(0,0,SW,GROUND,COL_SKY);
    // pipes
    for (i=0;i<NPIPES;i++) drawPipe(pipes[i].x, pipes[i].top);
    // ground
    rect(0,GROUND,SW,SH-GROUND,COL_GROUND);
    rect(0,GROUND,SW,4,COL_GRASS);
    // bird
    drawBird(birdY);
    // score
    drawNum(4,4,score,COL_WHITE);
}

static void drawTitle(void) {
    int i;
    rect(0,0,SW,SH,COL_SKY);
    rect(0,GROUND,SW,SH-GROUND,COL_GROUND);
    rect(0,GROUND,SW,4,COL_GRASS);
    // title text as pixel blocks
    rect(55,40,130,3,COL_WHITE);
    rect(55,56,130,3,COL_WHITE);
    drawNum(108,44,8,COL_BIRD); // decorative
    // Bird demo
    drawBird(SH/2);
    // Pipe demo
    drawPipe(160,30);
    // instructions row
    rect(20,120,200,18,COL_BLACK);
    rect(22,122,196,14,RGB15(4,4,18));
    // A button indicator
    rect(30,125,12,8,COL_BIRD);
    rect(50,125,80,8,COL_WHITE);
    rect(140,125,50,8,RGB15(20,20,20));
    // score label
    drawNum(108,125,0,COL_WHITE);
}

static void drawDead(void) {
    // darken screen
    int i;
    for (i=0;i<SW*SH;i++) {
        unsigned short p = VRAM[i];
        unsigned short r = (p&0x1F)>>1;
        unsigned short g = ((p>>5)&0x1F)>>1;
        unsigned short b = ((p>>10)&0x1F)>>1;
        VRAM[i] = r|(g<<5)|(b<<10);
    }
    // panel
    rect(60,55,120,50,COL_BLACK);
    rect(62,57,116,46,COL_RED);
    rect(64,59,112,42,RGB15(20,4,4));
    // SCORE display
    drawNum(90,70,score,COL_WHITE);
    rect(65,88,110,3,COL_WHITE);
}

int main(void) {
    irqInit();
    irqEnable(IRQ_VBLANK);
    SetMode(MODE_3 | BG2_ON);

    drawTitle();
    while(1) {
        VBlankIntrWait();
        scanKeys();
        if (keysDown()&KEY_START) break;
    }

    init();

    while(1) {
        VBlankIntrWait();
        scanKeys();

        if (dead) {
            if (keysDown()&KEY_START) { init(); draw(); }
            continue;
        }

        // Flap
        if (keysDown()&KEY_A) birdVel = -3;

        // Physics every 4 frames
        if (frame%4==0) {
            birdVel++;
            if (birdVel> 4) birdVel= 4;
            if (birdVel<-4) birdVel=-4;
            birdY += birdVel;
        }

        // Scroll pipes every 4 frames
        if (frame%4==0) {
            int i;
            for (i=0;i<NPIPES;i++) {
                pipes[i].x--;
                if (pipes[i].x < -PIPE_W-6) {
                    pipes[i].x   = SW+PSPACE;
                    pipes[i].top = randTop();
                    score++;
                }
            }
        }

        frame++;

        // Floor/ceiling
        if (birdY<0 || birdY+BIRD_H>=GROUND) { dead=1; drawDead(); continue; }

        // Pipe collision
        int i;
        for (i=0;i<NPIPES;i++) {
            if (BIRD_X+BIRD_W > pipes[i].x-3 &&
                BIRD_X        < pipes[i].x+PIPE_W+3) {
                if (birdY        < pipes[i].top ||
                    birdY+BIRD_H > pipes[i].top+GAP) {
                    dead=1; drawDead(); break;
                }
            }
        }

        if (!dead) draw();
    }
    return 0;
}
