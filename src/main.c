#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>

// ================================================================
//  MODE 4 DOUBLE BUFFER
// ================================================================
#define REG_DISPCNT (*(volatile unsigned int*)0x04000000)
#define PAL   ((volatile unsigned short*)0x05000000)
#define PAGE0 ((volatile unsigned short*)0x06000000)
#define PAGE1 ((volatile unsigned short*)0x0600A000)
#define SW 240
#define SH 160
#define PITCH 120

static volatile unsigned short *back;
static int page=0;

#define RGB15(r,g,b) ((r)|((g)<<5)|((b)<<10))

// ---- Palette ----
#define C_TRANS   0
#define C_SKY1    1
#define C_SKY2    2
#define C_PIPE    3
#define C_PIPED   4
#define C_PIPECAP 5
#define C_PIPELN  6
#define C_BIRD1   7
#define C_BIRD2   8
#define C_BEAK    9
#define C_EYE     10
#define C_WHITE   11
#define C_GND     12
#define C_GRASS   13
#define C_CLOUD   14
#define C_WING    15
#define C_RED     16
#define C_YELLOW  17
#define C_DARK    18
#define C_DARKB   19
#define C_PANEL   20
#define C_GNDLN   21
#define C_EYEW    22
#define C_SCORE   23
#define C_ORANGE  24
#define C_BIRDD   25

static void initPal(void){
    PAL[C_TRANS]  =RGB15( 0, 0, 0);
    PAL[C_SKY1]   =RGB15( 8,19,30);
    PAL[C_SKY2]   =RGB15(12,23,31);
    PAL[C_PIPE]   =RGB15( 4,20, 4);
    PAL[C_PIPED]  =RGB15( 2,12, 2);
    PAL[C_PIPECAP]=RGB15( 6,25, 6);
    PAL[C_PIPELN] =RGB15( 1, 6, 1);
    PAL[C_BIRD1]  =RGB15(31,28, 0);
    PAL[C_BIRD2]  =RGB15(29,24, 0);
    PAL[C_BEAK]   =RGB15(31,14, 0);
    PAL[C_EYE]    =RGB15( 1, 1, 2);
    PAL[C_WHITE]  =RGB15(31,31,31);
    PAL[C_GND]    =RGB15(19,12, 3);
    PAL[C_GRASS]  =RGB15( 5,21, 3);
    PAL[C_CLOUD]  =RGB15(28,30,31);
    PAL[C_WING]   =RGB15(31,18, 0);
    PAL[C_RED]    =RGB15(27, 3, 3);
    PAL[C_YELLOW] =RGB15(31,29, 4);
    PAL[C_DARK]   =RGB15( 1, 1, 3);
    PAL[C_DARKB]  =RGB15( 2, 2, 8);
    PAL[C_PANEL]  =RGB15( 3,11,24);
    PAL[C_GNDLN]  =RGB15(13, 8, 2);
    PAL[C_EYEW]   =RGB15(31,31,31);
    PAL[C_SCORE]  =RGB15(31,31,12);
    PAL[C_ORANGE] =RGB15(31,16, 0);
    PAL[C_BIRDD]  =RGB15(24,18, 0);
}

// ---- Draw ----
static inline void pset(int x,int y,unsigned char c){
    if((unsigned)x>=SW||(unsigned)y>=SH)return;
    int o=y*PITCH+(x>>1);
    if(x&1) back[o]=(back[o]&0x00FF)|((unsigned short)c<<8);
    else     back[o]=(back[o]&0xFF00)|c;
}

static void box(int x,int y,int w,int h,unsigned char c){
    int j,i;
    unsigned short cc=(unsigned short)c|((unsigned short)c<<8);
    if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;}
    if(x+w>SW)w=SW-x;  if(y+h>SH)h=SH-y;
    if(w<=0||h<=0)return;
    for(j=y;j<y+h;j++){
        volatile unsigned short *row=back+j*PITCH;
        int ix=x,ex=x+w;
        if(ix&1){int o=ix>>1;row[o]=(row[o]&0x00FF)|((unsigned short)c<<8);ix++;}
        volatile unsigned short *p=row+(ix>>1);
        int ex2=ex&~1;
        while(ix<ex2){*p++=cc;ix+=2;}
        if(ex&1){int o=ex>>1;row[o]=(row[o]&0xFF00)|c;}
    }
}

static void flip(void){
    page^=1;
    if(page){REG_DISPCNT=(REG_DISPCNT&~0x0010)|0x0000;back=PAGE1;}
    else    {REG_DISPCNT=(REG_DISPCNT&~0x0000)|0x0010;back=PAGE0;}
}

// ---- 5x7 pixel font (A-Z, 0-9) ----
// Each char: 5 bits wide, 7 rows
static const unsigned char FONT5x7[36][7]={
    // 0-9
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}, // 0
    {0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}, // 1
    {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F}, // 2
    {0x1F,0x01,0x02,0x06,0x01,0x11,0x0E}, // 3
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}, // 4
    {0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}, // 5
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}, // 6
    {0x1F,0x01,0x02,0x04,0x08,0x08,0x08}, // 7
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}, // 8
    {0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}, // 9
    // A-Z (10-35)
    {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11}, // A
    {0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E}, // B
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E}, // C
    {0x1C,0x12,0x11,0x11,0x11,0x12,0x1C}, // D
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F}, // E
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x10}, // F
    {0x0E,0x11,0x10,0x10,0x17,0x11,0x0F}, // G
    {0x11,0x11,0x11,0x1F,0x11,0x11,0x11}, // H
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E}, // I
    {0x07,0x02,0x02,0x02,0x02,0x12,0x0C}, // J
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11}, // K
    {0x10,0x10,0x10,0x10,0x10,0x10,0x1F}, // L
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11}, // M
    {0x11,0x19,0x15,0x13,0x11,0x11,0x11}, // N
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E}, // O
    {0x1E,0x11,0x11,0x1E,0x10,0x10,0x10}, // P
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D}, // Q
    {0x1E,0x11,0x11,0x1E,0x14,0x12,0x11}, // R
    {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E}, // S
    {0x1F,0x04,0x04,0x04,0x04,0x04,0x04}, // T
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E}, // U
    {0x11,0x11,0x11,0x11,0x11,0x0A,0x04}, // V
    {0x11,0x11,0x11,0x15,0x15,0x1B,0x11}, // W
    {0x11,0x11,0x0A,0x04,0x0A,0x11,0x11}, // X
    {0x11,0x11,0x0A,0x04,0x04,0x04,0x04}, // Y
    {0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}, // Z
};

static int charIndex(char c){
    if(c>='0'&&c<='9') return c-'0';
    if(c>='A'&&c<='Z') return c-'A'+10;
    if(c>='a'&&c<='z') return c-'a'+10;
    return -1;
}

static void drawChar(int x,int y,char c,unsigned char col,int scale){
    int idx=charIndex(c);
    if(idx<0)return;
    int r,b;
    for(r=0;r<7;r++)
        for(b=0;b<5;b++)
            if(FONT5x7[idx][r]&(0x10>>b))
                box(x+b*scale,y+r*scale,scale,scale,col);
}

static void drawStr(int x,int y,const char*s,unsigned char col,int scale){
    while(*s){
        if(*s==' '){x+=(5+1)*scale;s++;continue;}
        drawChar(x,y,*s,col,scale);
        x+=(5+1)*scale; s++;
    }
}

static void drawNum(int x,int y,int n,unsigned char col,int scale){
    char buf[8]; int i=7;
    buf[i]=0;
    if(n==0){buf[--i]='0';}
    while(n>0){buf[--i]='0'+n%10;n/=10;}
    drawStr(x,y,buf+i,col,scale);
}

// ================================================================
//  GAME CONSTANTS — tuned to real Flappy Bird feel
// ================================================================
#define FP       256
#define GRAVITY  (FP*38/100)   // 0.38 px/frame
#define FLAP_V   (-FP*62/10)   // -6.2 px/frame on flap
#define TERM_V   (FP*10)       // terminal velocity
#define PIPE_SPD 2             // px per frame (real FB = 3 on phone, 2 feels right on GBA)
#define GAP      46            // tight gap like real FB scaled to 160px screen
#define GROUND   142
#define NPIPES   2
#define PSPACE   140
#define PW       20
#define CAP_EX   4
#define CAP_H    8
#define BIRD_X   48
#define BIRD_W   17
#define BIRD_H   12

// ================================================================
//  STATE
// ================================================================
static int birdYfp, birdVfp;
static int score, dead, frame, hiScore;
static int gndScroll, wingAnim, wingDir;

typedef struct{int x,top;}Pipe;
static Pipe pipes[NPIPES];
static unsigned int rng=13337u;

// Clouds
#define NCLOUD 3
static int cldX[NCLOUD]={30,120,195};
static const int cldY[NCLOUD]={18,32,12};

static int randTop(void){
    rng=rng*1664525u+1013904223u;
    return 14+(int)((rng>>24)%(GROUND-GAP-28));
}

static void initGame(void){
    int i;
    birdYfp=(SH/2-10)*FP; birdVfp=0;
    score=0; dead=0; frame=0;
    gndScroll=0; wingAnim=6; wingDir=1;
    for(i=0;i<NPIPES;i++){
        pipes[i].x=SW+20+i*PSPACE;
        pipes[i].top=randTop();
    }
}

// ================================================================
//  DRAW HELPERS
// ================================================================
static void drawSky(void){
    box(0,0,SW,GROUND/2,C_SKY1);
    box(0,GROUND/2,SW,GROUND-GROUND/2,C_SKY2);
}

static void drawCloud(int cx,int cy){
    box(cx+4,cy+5,26,8,C_CLOUD);
    box(cx+2,cy+3,30,5,C_CLOUD);
    box(cx+7,cy,18,4,C_CLOUD);
    box(cx+10,cy-2,12,3,C_CLOUD);
}

static void drawGround(void){
    box(0,GROUND,SW,SH-GROUND,C_GND);
    box(0,GROUND,SW,5,C_GRASS);
    int i;
    for(i=0;i<8;i++){
        int lx=((i*30)-gndScroll+240)%240;
        box(lx,GROUND+7,18,3,C_GNDLN);
    }
}

static void drawPipe(int px,int top){
    int bot=top+GAP;
    // top body
    box(px,0,PW,top,C_PIPE);
    box(px+3,0,4,top,C_PIPECAP); // highlight strip
    box(px,0,1,top,C_PIPELN);
    box(px+PW-1,0,1,top,C_PIPELN);
    // top cap
    box(px-CAP_EX,top-CAP_H,PW+CAP_EX*2,CAP_H,C_PIPECAP);
    box(px-CAP_EX,top-CAP_H,1,CAP_H,C_PIPELN);
    box(px-CAP_EX+PW+CAP_EX*2-1,top-CAP_H,1,CAP_H,C_PIPELN);
    box(px-CAP_EX,top-1,PW+CAP_EX*2,1,C_PIPELN);
    box(px-CAP_EX,top-CAP_H,PW+CAP_EX*2,1,C_PIPELN);
    // bottom cap
    box(px-CAP_EX,bot,PW+CAP_EX*2,CAP_H,C_PIPECAP);
    box(px-CAP_EX,bot,1,CAP_H,C_PIPELN);
    box(px-CAP_EX+PW+CAP_EX*2-1,bot,1,CAP_H,C_PIPELN);
    box(px-CAP_EX,bot,PW+CAP_EX*2,1,C_PIPELN);
    box(px-CAP_EX,bot+CAP_H-1,PW+CAP_EX*2,1,C_PIPELN);
    // bottom body
    int bb=bot+CAP_H;
    box(px,bb,PW,GROUND-bb,C_PIPE);
    box(px+3,bb,4,GROUND-bb,C_PIPECAP);
    box(px,bb,1,GROUND-bb,C_PIPELN);
    box(px+PW-1,bb,1,GROUND-bb,C_PIPELN);
}

static void drawBird(int by,int wa){
    // shadow on ground (only when near)
    if(by>GROUND-20) box(BIRD_X+2,GROUND+2,BIRD_W-2,3,C_PIPED);
    // body
    box(BIRD_X,   by,           BIRD_W,   BIRD_H,   C_BIRD1);
    box(BIRD_X,   by+BIRD_H-3,  BIRD_W-3, 3,        C_BIRDD);
    box(BIRD_X+3, by+1,         4,        2,        C_YELLOW);
    // wing (animated)
    int wy=(wa>0)?by+2:by+BIRD_H-4;
    box(BIRD_X+2,wy,10,3,C_WING);
    box(BIRD_X+3,wy+1,8,1,C_ORANGE);
    // beak
    box(BIRD_X+BIRD_W,  by+4, 6,4,C_BEAK);
    box(BIRD_X+BIRD_W,  by+5, 6,2,C_ORANGE);
    box(BIRD_X+BIRD_W+5,by+5, 1,1,C_BIRD1);
    // eye white
    box(BIRD_X+BIRD_W-6,by+1,7,6,C_EYEW);
    // pupil
    box(BIRD_X+BIRD_W-4,by+2,4,4,C_EYE);
    // shine
    pset(BIRD_X+BIRD_W-5,by+2,C_WHITE);
    pset(BIRD_X+BIRD_W-5,by+3,C_WHITE);
    // outline top/bottom
    box(BIRD_X,by,BIRD_W,1,C_BIRDD);
    box(BIRD_X,by+BIRD_H-1,BIRD_W,1,C_BIRDD);
}

static void drawScene(void){
    drawSky();
    int i;
    for(i=0;i<NCLOUD;i++) drawCloud(cldX[i],cldY[i]);
    for(i=0;i<NPIPES;i++) drawPipe(pipes[i].x,pipes[i].top);
    drawGround();
    drawBird(birdYfp/FP, wingAnim);
    // score top center
    int sw=score>=100?18:score>=10?12:6;
    drawNum(SW/2-sw,5,score,C_WHITE,2);
}

// ================================================================
//  SCREENS
// ================================================================
static void drawMenu(void){
    drawSky();
    int i;
    for(i=0;i<NCLOUD;i++) drawCloud(cldX[i],cldY[i]);
    drawPipe(178,22);
    drawGround();
    drawBird(72,wingAnim);

    // Title box
    box(18,18,204,44,C_PIPELN);
    box(20,20,200,40,C_DARK);
    box(22,22,196,36,C_PANEL);
    // FLAPPY
    drawStr(28,26,"FLAPPY",C_SCORE,3);
    // BIRD
    drawStr(72,26+22,"BIRD",C_WHITE,3);

    // START box (blinking)
    if((frame/18)%2==0){
        box(68,76,104,18,C_PIPELN);
        box(70,78,100,14,C_DARK);
        box(72,80,96,10,C_PANEL);
        drawStr(84,82,"START",C_WHITE,1);
    }

    // hi score
    if(hiScore>0){
        box(80,100,80,14,C_DARK);
        drawStr(84,103,"BEST",C_SCORE,1);
        drawNum(120,103,hiScore,C_WHITE,1);
    }
}

static void drawGameOver(void){
    // simple dark overlay box — no pixel manipulation needed
    box(40,44,160,72,C_PIPELN);
    box(42,46,156,68,C_DARK);
    box(44,48,152,64,C_PANEL);
    drawStr(70,54,"GAME OVER",C_RED,1);
    // score
    drawStr(52,68,"SCORE",C_SCORE,1);
    drawNum(104,68,score,C_WHITE,1);
    // best
    drawStr(52,80,"BEST",C_SCORE,1);
    drawNum(104,80,hiScore,C_WHITE,1);
    // restart
    if((frame/18)%2==0)
        drawStr(62,98,"PRESS START",C_WHITE,1);
}

// ================================================================
//  MAIN
// ================================================================
int main(void){
    irqInit();
    irqEnable(IRQ_VBLANK);
    SetMode(MODE_4|BG2_ON);
    back=PAGE1; page=0;
    initPal();
    hiScore=0;
    initGame();

    // MENU
    while(1){
        VBlankIntrWait();
        flip();
        scanKeys();
        frame++;
        // cloud scroll on menu
        int i;
        for(i=0;i<NCLOUD;i++){cldX[i]--;if(cldX[i]<-44)cldX[i]=SW+10;}
        // wing
        wingAnim+=wingDir;
        if(wingAnim>=8||wingAnim<=0)wingDir=-wingDir;
        drawMenu();
        if(keysDown()&(KEY_START|KEY_A)) break;
    }

    initGame();

    // GAME
    while(1){
        VBlankIntrWait();
        flip();
        scanKeys();

        if(dead){
            drawScene();
            drawGameOver();
            if(keysDown()&(KEY_START|KEY_A)){
                initGame();
                // back to menu
                while(1){
                    VBlankIntrWait();
                    flip();
                    scanKeys();
                    frame++;
                    int i;
                    for(i=0;i<NCLOUD;i++){cldX[i]--;if(cldX[i]<-44)cldX[i]=SW+10;}
                    wingAnim+=wingDir;
                    if(wingAnim>=8||wingAnim<=0)wingDir=-wingDir;
                    drawMenu();
                    if(keysDown()&(KEY_START|KEY_A)) break;
                }
                initGame();
            }
            frame++;
            continue;
        }

        // INPUT
        if(keysDown()&KEY_A){
            birdVfp=FLAP_V;
            wingAnim=8; wingDir=-1;
        }

        // PHYSICS every frame (60fps)
        birdVfp+=GRAVITY;
        if(birdVfp>TERM_V) birdVfp=TERM_V;
        birdYfp+=birdVfp;

        // wing animation
        wingAnim+=wingDir;
        if(wingAnim>=8||wingAnim<=0) wingDir=-wingDir;

        // SCROLL
        gndScroll=(gndScroll+PIPE_SPD)%240;
        int i;
        for(i=0;i<NCLOUD;i++){cldX[i]--;if(cldX[i]<-44)cldX[i]=SW+10;}

        // PIPES
        for(i=0;i<NPIPES;i++){
            pipes[i].x-=PIPE_SPD;
            if(pipes[i].x<-PW-CAP_EX-2){
                pipes[i].x=SW+PSPACE/2;
                pipes[i].top=randTop();
                score++;
                if(score>hiScore) hiScore=score;
            }
        }

        frame++;

        // COLLISION
        int by=birdYfp/FP;
        if(by<=0||by+BIRD_H>=GROUND){dead=1;continue;}

        for(i=0;i<NPIPES;i++){
            if(BIRD_X+BIRD_W-3>pipes[i].x-CAP_EX &&
               BIRD_X+3       <pipes[i].x+PW+CAP_EX){
                if(by+2<pipes[i].top||by+BIRD_H-2>pipes[i].top+GAP){
                    dead=1;
                }
            }
        }

        drawScene();
    }
    return 0;
}
