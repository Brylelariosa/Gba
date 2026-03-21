#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>

// =====================================================================
//  MODE 4 DOUBLE BUFFER  (8-bit palette, 240x160, two pages)
// =====================================================================
#define REG_DISPCNT  (*(volatile unsigned int*)0x04000000)
#define PAL          ((volatile unsigned short*)0x05000000)
#define PAGE0        ((volatile unsigned short*)0x06000000)
#define PAGE1        ((volatile unsigned short*)0x0600A000)
#define SW 240
#define SH 160
#define PITCH 120   // 240/2 shorts per row

static volatile unsigned short *back;
static int page = 0;

#define RGB15(r,g,b) ((r)|((g)<<5)|((b)<<10))

// ---- Palette indices ----
#define C_BLACK   0
#define C_SKY     1
#define C_SKYMID  2
#define C_PIPE    3
#define C_PIPED   4
#define C_PIPECAP 5
#define C_BIRD    6
#define C_BIRDD   7
#define C_BEAK    8
#define C_EYE     9
#define C_WHITE   10
#define C_GROUND  11
#define C_GRASS   12
#define C_CLOUD   13
#define C_WING    14
#define C_RED     15
#define C_SCORE   16
#define C_MENUBOX 17
#define C_GNDLINE 18
#define C_EYEWHITE 19
#define C_PIPELINE 20

static void initPal(void) {
    PAL[C_BLACK]   = RGB15( 0, 0, 0);
    PAL[C_SKY]     = RGB15(10,20,31);
    PAL[C_SKYMID]  = RGB15(14,24,31);
    PAL[C_PIPE]    = RGB15( 3,19, 3);
    PAL[C_PIPED]   = RGB15( 2,12, 2);
    PAL[C_PIPECAP] = RGB15( 5,24, 5);
    PAL[C_BIRD]    = RGB15(31,28, 0);
    PAL[C_BIRDD]   = RGB15(25,20, 0);
    PAL[C_BEAK]    = RGB15(31,15, 0);
    PAL[C_EYE]     = RGB15( 1, 1, 1);
    PAL[C_WHITE]   = RGB15(31,31,31);
    PAL[C_GROUND]  = RGB15(20,13, 4);
    PAL[C_GRASS]   = RGB15( 5,22, 3);
    PAL[C_CLOUD]   = RGB15(29,30,31);
    PAL[C_WING]    = RGB15(31,19, 0);
    PAL[C_RED]     = RGB15(28, 3, 3);
    PAL[C_SCORE]   = RGB15(31,31,18);
    PAL[C_MENUBOX] = RGB15( 3,10,22);
    PAL[C_GNDLINE] = RGB15(14, 9, 2);
    PAL[C_EYEWHITE]= RGB15(31,31,31);
    PAL[C_PIPELINE]= RGB15( 1, 7, 1);
}

// ---- Pixel + Rect ----
static inline void pset(int x, int y, unsigned char c) {
    if ((unsigned)x >= SW || (unsigned)y >= SH) return;
    int o = y*PITCH+(x>>1);
    if (x&1) back[o]=(back[o]&0x00FF)|((unsigned short)c<<8);
    else     back[o]=(back[o]&0xFF00)|c;
}

static void box(int x, int y, int w, int h, unsigned char c) {
    int j,i;
    unsigned short cc=(unsigned short)c|(unsigned short)c<<8;
    if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;}
    if(x+w>SW)w=SW-x;  if(y+h>SH)h=SH-y;
    if(w<=0||h<=0)return;
    for(j=y;j<y+h;j++){
        volatile unsigned short *row=back+j*PITCH;
        int ix=x;
        if(ix&1){volatile unsigned short*p=row+(ix>>1);*p=(*p&0x00FF)|((unsigned short)c<<8);ix++;}
        int ex=x+w; volatile unsigned short*p=row+(ix>>1);
        int ex2=ex&~1;
        while(ix<ex2){*p++=cc;ix+=2;}
        if(ex&1){*p=(*p&0xFF00)|c;}
    }
}

// ---- Flip pages ----
static void flip(void) {
    page^=1;
    if(page){ REG_DISPCNT|=0x0010; back=PAGE0; }
    else    { REG_DISPCNT&=~0x0010; back=PAGE1; }
}

// ---- Tiny 4x6 font for digits ----
static const unsigned char DGLYPH[10][6]={
    {0x6,0xB,0xB,0xD,0xD,0x6},{0x2,0x6,0x2,0x2,0x2,0x7},
    {0x6,0x9,0x1,0x2,0x4,0xF},{0xE,0x1,0x6,0x1,0x1,0xE},
    {0x9,0x9,0xF,0x1,0x1,0x1},{0xF,0x8,0xE,0x1,0x1,0xE},
    {0x6,0x8,0xE,0x9,0x9,0x6},{0xF,0x1,0x2,0x4,0x4,0x4},
    {0x6,0x9,0x6,0x9,0x9,0x6},{0x6,0x9,0x9,0x7,0x1,0x6}
};
static void drawDigit(int x,int y,int d,unsigned char c){
    int r,col;
    for(r=0;r<6;r++) for(col=0;col<4;col++)
        if(DGLYPH[d][r]&(8>>col)) box(x+col*2,y+r*2,2,2,c);
}
static void drawNum(int x,int y,int n,unsigned char c){
    if(n>=100){drawDigit(x,y,n/100,c);x+=10;}
    if(n>=10) {drawDigit(x,y,(n/10)%10,c);x+=10;}
    drawDigit(x,y,n%10,c);
}

// ---- Draw sky gradient ----
static void drawSky(int groundY) {
    int mid=groundY/2;
    box(0,0,SW,mid,C_SKY);
    box(0,mid,SW,groundY-mid,C_SKYMID);
}

// ---- Clouds ----
#define NCLOUDS 3
static int cloudX[NCLOUDS]={40,120,200};
static const int cloudY[NCLOUDS]={20,35,15};

static void scrollClouds(void) {
    int i;
    for(i=0;i<NCLOUDS;i++){
        cloudX[i]--;
        if(cloudX[i]<-40) cloudX[i]=SW+20;
    }
}

static void drawCloud(int cx,int cy){
    box(cx,   cy+6, 30,10,C_CLOUD);
    box(cx+4, cy+2, 22, 6,C_CLOUD);
    box(cx+8, cy,   14, 4,C_CLOUD);
}

// ---- Ground ----
#define GROUND 140
static int gndScroll=0;

static void drawGround(void){
    box(0,GROUND,SW,SH-GROUND,C_GROUND);
    box(0,GROUND,SW,5,C_GRASS);
    // scrolling ground lines
    int i;
    for(i=0;i<SW;i+=20){
        int lx=(i-gndScroll+200)%SW;
        box(lx,GROUND+6,8,2,C_GNDLINE);
    }
}

// ---- Pipe ----
#define PW    24
#define CAP_H  8
#define CAP_EX 4

static void drawPipe(int px,int top){
    int bot=top+64; // GAP=64px
    // top pipe body
    box(px,0,PW,top,C_PIPE);
    // top pipe highlight
    box(px+2,0,3,top,C_PIPECAP);
    // top pipe edge line
    box(px,0,1,top,C_PIPELINE);
    box(px+PW-1,0,1,top,C_PIPELINE);
    // top cap
    box(px-CAP_EX,top-CAP_H,PW+CAP_EX*2,CAP_H,C_PIPECAP);
    box(px-CAP_EX,top-CAP_H,1,CAP_H,C_PIPELINE);
    box(px-CAP_EX+PW+CAP_EX*2-1,top-CAP_H,1,CAP_H,C_PIPELINE);
    box(px-CAP_EX,top-1,PW+CAP_EX*2,1,C_PIPELINE);

    // bottom cap
    box(px-CAP_EX,bot,PW+CAP_EX*2,CAP_H,C_PIPECAP);
    box(px-CAP_EX,bot,1,CAP_H,C_PIPELINE);
    box(px-CAP_EX+PW+CAP_EX*2-1,bot,1,CAP_H,C_PIPELINE);
    box(px-CAP_EX,bot,PW+CAP_EX*2,1,C_PIPELINE);
    // bottom pipe body
    int bbot=bot+CAP_H;
    box(px,bbot,PW,GROUND-bbot,C_PIPE);
    box(px+2,bbot,3,GROUND-bbot,C_PIPECAP);
    box(px,bbot,1,GROUND-bbot,C_PIPELINE);
    box(px+PW-1,bbot,1,GROUND-bbot,C_PIPELINE);
}

// ---- Bird ----
// Fixed point: multiply by 256
#define FP 256
static int birdYfp, birdVfp;
static int flapAnim=0; // wing animation frame
#define BIRD_X  50
#define BIRD_W  16
#define BIRD_H  12

static void drawBird(int by,int anim){
    // shadow
    box(BIRD_X+2,by+BIRD_H+1,BIRD_W-2,2,C_PIPED);
    // body
    box(BIRD_X,by,BIRD_W,BIRD_H,C_BIRD);
    // darker bottom
    box(BIRD_X,by+BIRD_H-3,BIRD_W,3,C_BIRDD);
    // wing (animated: up or down)
    if(anim<8)
        box(BIRD_X+2,by+2,8,3,C_WING);     // wing up
    else
        box(BIRD_X+2,by+BIRD_H-4,8,3,C_WING); // wing down
    // beak
    box(BIRD_X+BIRD_W,by+3,5,4,C_BEAK);
    box(BIRD_X+BIRD_W,by+4,5,2,C_RED);
    // eye white
    box(BIRD_X+BIRD_W-5,by+1,6,5,C_EYEWHITE);
    // pupil
    box(BIRD_X+BIRD_W-3,by+2,3,3,C_EYE);
    // shine
    pset(BIRD_X+BIRD_W-5,by+2,C_WHITE);
}

// ---- Pipes ----
#define NPIPES 2
#define PSPACE 130
typedef struct{int x,top;}Pipe;
static Pipe pipes[NPIPES];
static unsigned int rng=55577u;

static int randTop(void){
    rng=rng*1664525u+1013904223u;
    return 20+(int)((rng>>24)%(GROUND-64-40));
}

// ---- Game state ----
static int score,dead,frame,hiScore;

static void initGame(void){
    int i;
    birdYfp=(SH/2)*FP; birdVfp=0;
    score=0; dead=0; frame=0; flapAnim=0;
    gndScroll=0;
    for(i=0;i<NPIPES;i++){
        pipes[i].x=SW+10+i*PSPACE;
        pipes[i].top=randTop();
    }
}

// ---- Full scene ----
static void drawScene(void){
    int by=birdYfp/FP;
    drawSky(GROUND);
    int i;
    for(i=0;i<NCLOUDS;i++) drawCloud(cloudX[i],cloudY[i]);
    for(i=0;i<NPIPES;i++) drawPipe(pipes[i].x,pipes[i].top);
    drawGround();
    drawBird(by,flapAnim);
    // score
    drawNum(SW/2-10,6,score,C_WHITE);
}

// ---- Tiny letter patterns for MENU text ----
// We'll draw "FLAPPY" and "BIRD" using boxes

static void drawTitle(void){
    drawSky(GROUND);
    int i;
    for(i=0;i<NCLOUDS;i++) drawCloud(cloudX[i],cloudY[i]);
    // demo pipe
    drawPipe(170,25);
    drawGround();
    // demo bird
    drawBird(70,(frame%16));

    // Title panel
    box(30,20,180,38,C_BLACK);
    box(32,22,176,34,C_MENUBOX);

    // FLAPPY text (big pixel blocks)
    // F
    box(40,26,3,18,C_SCORE); box(40,26,14,3,C_SCORE); box(40,34,10,3,C_SCORE);
    // L
    box(58,26,3,18,C_SCORE); box(58,41,14,3,C_SCORE);
    // A
    box(76,26,14,3,C_SCORE); box(76,26,3,18,C_SCORE); box(87,26,3,18,C_SCORE); box(76,34,14,3,C_SCORE);
    // P
    box(104,26,3,18,C_SCORE); box(104,26,12,3,C_SCORE); box(115,26,3,9,C_SCORE); box(104,34,12,3,C_SCORE);
    // P
    box(122,26,3,18,C_SCORE); box(122,26,12,3,C_SCORE); box(133,26,3,9,C_SCORE); box(122,34,12,3,C_SCORE);
    // Y
    box(140,26,3,9,C_SCORE); box(151,26,3,9,C_SCORE); box(145,34,3,10,C_SCORE); box(140,26,14,3,C_BIRD);

    // BIRD text
    // B
    box(70,48,3,18,C_WHITE); box(70,48,10,3,C_WHITE); box(79,48,3,7,C_WHITE); box(70,54,10,3,C_WHITE); box(79,54,3,7,C_WHITE); box(70,60,10,3,C_WHITE);
    // I
    box(90,48,10,3,C_WHITE); box(94,48,3,18,C_WHITE); box(90,63,10,3,C_WHITE);
    // R
    box(106,48,3,18,C_WHITE); box(106,48,12,3,C_WHITE); box(117,48,3,7,C_WHITE); box(106,54,12,3,C_WHITE); box(115,54,5,12,C_WHITE);
    // D
    box(124,48,3,18,C_WHITE); box(124,48,10,3,C_WHITE); box(133,52,3,10,C_WHITE); box(124,63,10,3,C_WHITE);

    // START button
    int blink=(frame/20)%2;
    unsigned char bc=blink?C_SCORE:C_WHITE;
    box(70,84,100,18,C_BLACK);
    box(72,86,96,14,bc);
    box(74,88,92,10,C_MENUBOX);
    // S T A R T text boxes
    box(80,90,3,8,bc); box(80,90,8,2,bc); box(80,94,8,2,bc); box(85,94,3,4,bc); box(80,96,8,2,bc);
    box(94,90,14,2,bc); box(100,90,3,8,bc);
    // best score
    if(hiScore>0){
        box(88,108,64,14,C_BLACK);
        box(90,110,60,10,C_MENUBOX);
        drawNum(116,112,hiScore,C_SCORE);
    }
}

static void drawDead(void){
    // darken
    int i;
    for(i=0;i<PITCH*SH;i++){
        unsigned short p=back[i];
        back[i]=((p>>1)&0x3DEF);
    }
    // panel
    box(50,50,140,60,C_BLACK);
    box(52,52,136,56,C_RED);
    box(54,54,132,52,C_MENUBOX);
    // score
    drawNum(SW/2-10,68,score,C_WHITE);
    // best
    drawNum(SW/2-10,82,hiScore,C_SCORE);
    // restart hint
    box(60,95,120,10,C_BLACK);
    box(62,97,116,6,C_WHITE);
}

// =====================================================================
//  MAIN
// =====================================================================
int main(void){
    irqInit();
    irqEnable(IRQ_VBLANK);
    SetMode(MODE_4|BG2_ON);
    back=PAGE1; page=0;
    initPal();

    hiScore=0;
    initGame();

    // ---- MENU ----
    while(1){
        VBlankIntrWait();
        flip();
        scanKeys();
        frame++;
        scrollClouds();
        drawTitle();
        if(keysDown()&KEY_START) break;
        if(keysDown()&KEY_A)     break;
    }

    initGame();

    // ---- GAME LOOP ----
    while(1){
        VBlankIntrWait();
        flip();
        scanKeys();

        if(dead){
            // draw dead screen on top of last frame
            drawScene();
            drawDead();
            if(keysDown()&(KEY_START|KEY_A)){
                initGame();
                // brief menu
                while(1){
                    VBlankIntrWait();
                    flip();
                    scanKeys();
                    frame++;
                    scrollClouds();
                    drawTitle();
                    if(keysDown()&(KEY_START|KEY_A)) break;
                }
                initGame();
            }
            continue;
        }

        // ---- INPUT ----
        if(keysDown()&KEY_A){
            birdVfp = -5*FP/3; // strong flap like real flappy bird
            flapAnim=0;
        }

        // ---- PHYSICS (every frame, 60fps) ----
        birdVfp += FP/5;           // gravity
        if(birdVfp > 4*FP) birdVfp=4*FP; // terminal velocity
        birdYfp += birdVfp;

        // wing animation
        flapAnim=(flapAnim+1)%16;

        // ---- SCROLL ----
        gndScroll=(gndScroll+2)%20;
        scrollClouds();

        // move pipes 2px/frame (same as real flappy bird)
        int i;
        for(i=0;i<NPIPES;i++){
            pipes[i].x-=2;
            if(pipes[i].x<-PW-CAP_EX){
                pipes[i].x=SW+PSPACE/2;
                pipes[i].top=randTop();
                score++;
                if(score>hiScore) hiScore=score;
            }
        }

        frame++;

        // ---- COLLISION ----
        int by=birdYfp/FP;
        if(by<0||by+BIRD_H>=GROUND){ dead=1; continue; }

        for(i=0;i<NPIPES;i++){
            int px=pipes[i].x;
            int ptop=pipes[i].top;
            int pbot=ptop+64;
            // check hitbox (slightly forgiving)
            if(BIRD_X+BIRD_W-2 > px-CAP_EX &&
               BIRD_X+2        < px+PW+CAP_EX){
                if(by+2 < ptop || by+BIRD_H-2 > pbot){
                    dead=1;
                }
            }
        }

        drawScene();
    }

    return 0;
}
