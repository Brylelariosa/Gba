#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>

#define REG_DISPCNT (*(volatile unsigned int*)0x04000000)
#define PAL   ((volatile unsigned short*)0x05000000)
#define PAGE0 ((volatile unsigned short*)0x06000000)
#define PAGE1 ((volatile unsigned short*)0x0600A000)
#define SW 240
#define SH 160
#define PITCH 120
#define RGB15(r,g,b) ((r)|((g)<<5)|((b)<<10))

static volatile unsigned short *back;
static int page=0;

#define C_BG      0
#define C_SKY1    1
#define C_SKY2    2
#define C_CLOUD   3
#define C_PIPE    4
#define C_PIPED   5
#define C_PIPECAP 6
#define C_PIPELN  7
#define C_GND     8
#define C_GRASS   9
#define C_GNDLN  10
#define C_WHITE  11
#define C_EYEW   12
#define C_EYE    13
#define C_BEAK   14
#define C_ORANGE 15
#define C_WING1  16
#define C_WING2  17
#define C_YEL1   18
#define C_YEL2   19
#define C_YEL3   20
#define C_RED1   21
#define C_RED2   22
#define C_RED3   23
#define C_BLU1   24
#define C_BLU2   25
#define C_BLU3   26
#define C_SCORE  27
#define C_PANEL  28
#define C_PANELD 29
#define C_PANBDR 30
#define C_GOLD   31
#define C_SILVER 32
#define C_BRONZE 33
#define C_RED    34
#define C_DARK   35
#define C_OUTLINE 36
#define C_BELLY  37

static void initPal(void){
    PAL[C_BG]    =RGB15( 0, 0, 0);
    PAL[C_SKY1]  =RGB15( 7,18,30);
    PAL[C_SKY2]  =RGB15(12,22,31);
    PAL[C_CLOUD] =RGB15(28,30,31);
    PAL[C_PIPE]  =RGB15( 4,20, 4);
    PAL[C_PIPED] =RGB15( 2,12, 2);
    PAL[C_PIPECAP]=RGB15(7,25, 7);
    PAL[C_PIPELN]=RGB15( 1, 5, 1);
    PAL[C_GND]   =RGB15(19,12, 3);
    PAL[C_GRASS] =RGB15( 5,21, 3);
    PAL[C_GNDLN] =RGB15(13, 8, 2);
    PAL[C_WHITE] =RGB15(31,31,31);
    PAL[C_EYEW]  =RGB15(31,31,31);
    PAL[C_EYE]   =RGB15( 1, 1, 4);
    PAL[C_BEAK]  =RGB15(31,14, 0);
    PAL[C_ORANGE]=RGB15(31,18, 2);
    PAL[C_WING1] =RGB15(31,20, 0);
    PAL[C_WING2] =RGB15(28,13, 0);
    PAL[C_YEL1]  =RGB15(31,28, 1);
    PAL[C_YEL2]  =RGB15(22,18, 0);
    PAL[C_YEL3]  =RGB15(31,31,16);
    PAL[C_RED1]  =RGB15(27, 4, 4);
    PAL[C_RED2]  =RGB15(19, 2, 2);
    PAL[C_RED3]  =RGB15(31,16,16);
    PAL[C_BLU1]  =RGB15( 4,14,28);
    PAL[C_BLU2]  =RGB15( 2, 8,20);
    PAL[C_BLU3]  =RGB15(16,24,31);
    PAL[C_SCORE] =RGB15(31,31,10);
    PAL[C_PANEL] =RGB15( 3,11,23);
    PAL[C_PANELD]=RGB15( 1, 5,13);
    PAL[C_PANBDR]=RGB15( 5,18,31);
    PAL[C_GOLD]  =RGB15(31,26, 2);
    PAL[C_SILVER]=RGB15(22,22,23);
    PAL[C_BRONZE]=RGB15(18,10, 3);
    PAL[C_RED]   =RGB15(27, 3, 3);
    PAL[C_DARK]  =RGB15( 1, 1, 3);
    PAL[C_OUTLINE]=RGB15( 0, 0, 0);
    PAL[C_BELLY] =RGB15(31,31,22);
}

static inline void pset(int x,int y,unsigned char c){
    if((unsigned)x>=SW||(unsigned)y>=SH)return;
    int o=y*PITCH+(x>>1);
    if(x&1) back[o]=(back[o]&0x00FF)|((unsigned short)c<<8);
    else     back[o]=(back[o]&0xFF00)|c;
}
static void box(int x,int y,int w,int h,unsigned char c){
    int j;
    unsigned short cc=(unsigned short)c|((unsigned short)c<<8);
    if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;}
    if(x+w>SW)w=SW-x;  if(y+h>SH)h=SH-y;
    if(w<=0||h<=0)return;
    for(j=y;j<y+h;j++){
        volatile unsigned short *row=back+j*PITCH;
        int ix=x,ex=x+w;
        if(ix&1){row[ix>>1]=(row[ix>>1]&0x00FF)|((unsigned short)c<<8);ix++;}
        volatile unsigned short *p=row+(ix>>1);
        int ex2=ex&~1;
        while(ix<ex2){*p++=cc;ix+=2;}
        if(ex&1){row[ex>>1]=(row[ex>>1]&0xFF00)|c;}
    }
}
static void flip(void){
    page^=1;
    if(page){REG_DISPCNT|=0x0010;back=PAGE0;}
    else    {REG_DISPCNT&=~0x0010;back=PAGE1;}
}

// ---- Font ----
static const unsigned char F5[36][7]={
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
    {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F},{0x1F,0x01,0x02,0x06,0x01,0x11,0x0E},
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},{0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},{0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},{0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
    {0x0E,0x11,0x11,0x1F,0x11,0x11,0x11},{0x1E,0x11,0x11,0x1E,0x11,0x11,0x1E},
    {0x0E,0x11,0x10,0x10,0x10,0x11,0x0E},{0x1C,0x12,0x11,0x11,0x11,0x12,0x1C},
    {0x1F,0x10,0x10,0x1E,0x10,0x10,0x1F},{0x1F,0x10,0x10,0x1E,0x10,0x10,0x10},
    {0x0E,0x11,0x10,0x10,0x17,0x11,0x0F},{0x11,0x11,0x11,0x1F,0x11,0x11,0x11},
    {0x0E,0x04,0x04,0x04,0x04,0x04,0x0E},{0x07,0x02,0x02,0x02,0x02,0x12,0x0C},
    {0x11,0x12,0x14,0x18,0x14,0x12,0x11},{0x10,0x10,0x10,0x10,0x10,0x10,0x1F},
    {0x11,0x1B,0x15,0x15,0x11,0x11,0x11},{0x11,0x19,0x15,0x13,0x11,0x11,0x11},
    {0x0E,0x11,0x11,0x11,0x11,0x11,0x0E},{0x1E,0x11,0x11,0x1E,0x10,0x10,0x10},
    {0x0E,0x11,0x11,0x11,0x15,0x12,0x0D},{0x1E,0x11,0x11,0x1E,0x14,0x12,0x11},
    {0x0F,0x10,0x10,0x0E,0x01,0x01,0x1E},{0x1F,0x04,0x04,0x04,0x04,0x04,0x04},
    {0x11,0x11,0x11,0x11,0x11,0x11,0x0E},{0x11,0x11,0x11,0x11,0x11,0x0A,0x04},
    {0x11,0x11,0x11,0x15,0x15,0x1B,0x11},{0x11,0x11,0x0A,0x04,0x0A,0x11,0x11},
    {0x11,0x11,0x0A,0x04,0x04,0x04,0x04},{0x1F,0x01,0x02,0x04,0x08,0x10,0x1F}
};
static int cidx(char c){
    if(c>='0'&&c<='9')return c-'0';
    if(c>='A'&&c<='Z')return c-'A'+10;
    if(c>='a'&&c<='z')return c-'a'+10;
    return -1;
}
static void dchar(int x,int y,char c,unsigned char col,int s){
    int i=cidx(c); if(i<0)return;
    int r,b;
    for(r=0;r<7;r++) for(b=0;b<5;b++)
        if(F5[i][r]&(0x10>>b)) box(x+b*s,y+r*s,s,s,col);
}
static void dstr(int x,int y,const char*s,unsigned char col,int sc){
    while(*s){ if(*s==' '){x+=(5+1)*sc;s++;continue;} dchar(x,y,*s,col,sc);x+=(5+1)*sc;s++; }
}
static int strW(const char*s,int sc){ int w=0; while(*s){w+=(5+1)*sc;s++;} return w>sc?w-sc:0; }
static void dstrC(int y,const char*s,unsigned char col,int sc){ dstr((SW-strW(s,sc))/2,y,s,col,sc); }
static void dnum(int x,int y,int n,unsigned char col,int sc){
    char buf[8]; int i=7; buf[i]=0;
    if(n==0){buf[--i]='0';} while(n>0){buf[--i]='0'+n%10;n/=10;}
    dstr(x,y,buf+i,col,sc);
}
static void dnumC(int y,int n,unsigned char col,int sc){
    char buf[8]; int i=7; buf[i]=0;
    if(n==0){buf[--i]='0';} while(n>0){buf[--i]='0'+n%10;n/=10;}
    dstrC(y,buf+i,col,sc);
}

// ================================================================
// BIRD SPRITE — pixel art, 20x16, modelled on real Flappy Bird
// Each row: 20 entries, value = palette index, 0=transparent(sky)
// ================================================================
// Colors per char: body, dark, highlight, belly
// We draw procedurally using pset for best shape

static void drawBirdAt(int bx,int by,int vel,int wa,int ci){
    unsigned char b1,b2,b3;
    switch(ci){
        case 1: b1=C_RED1;b2=C_RED2;b3=C_RED3;  break;
        case 2: b1=C_BLU1;b2=C_BLU2;b3=C_BLU3;  break;
        default:b1=C_YEL1;b2=C_YEL2;b3=C_YEL3;  break;
    }

    // tilt beak/eye based on velocity
    int tilt=vel/(256*2);
    if(tilt< -2)tilt=-2;
    if(tilt>  3)tilt= 3;

    // ---- Outline (black border around whole bird) ----
    // top arc
    box(bx+3, by-1,  14, 1, C_OUTLINE);
    box(bx+1, by,     2, 1, C_OUTLINE);
    box(bx+17,by,     2, 1, C_OUTLINE);
    // left side
    box(bx,   by+1,   1,10, C_OUTLINE);
    // right side (before beak)
    box(bx+19,by+1,   1, 4+tilt, C_OUTLINE);
    box(bx+19,by+8+tilt,1,3, C_OUTLINE);
    // bottom arc
    box(bx+1, by+11,  2, 1, C_OUTLINE);
    box(bx+3, by+12, 14, 1, C_OUTLINE);
    box(bx+17,by+11,  2, 1, C_OUTLINE);

    // ---- Body fill ----
    box(bx+1, by,    18, 1, b1);
    box(bx+1, by+1,  18, 1, b1);
    box(bx,   by+2,  19, 7, b1);
    box(bx+1, by+9,  18, 1, b2);
    box(bx+1, by+10, 17, 1, b2);
    box(bx+2, by+11, 15, 1, b2);

    // ---- Top highlight ----
    box(bx+4, by+1,  8, 1, b3);
    box(bx+3, by+2,  4, 1, b3);
    box(bx+2, by+3,  2, 1, b3);

    // ---- Belly (white/cream) ----
    box(bx+1, by+5,  5, 4, C_BELLY);
    box(bx+2, by+4,  4, 1, C_BELLY);
    box(bx+2, by+9,  4, 1, C_BELLY);
    box(bx+1, by+8,  1, 1, C_BELLY);
    // outline belly
    pset(bx,  by+5, C_OUTLINE);
    pset(bx,  by+8, C_OUTLINE);

    // ---- Wing ----
    int wingUp=(wa<8);
    if(wingUp){
        // Wing up: rises above body
        box(bx+5, by-1,  8, 1, b2);
        box(bx+4, by,    9, 2, b1);
        box(bx+5, by,    8, 1, b3);
        // wing outline
        box(bx+4, by-1,  1, 1, C_OUTLINE);
        box(bx+13,by-1,  1, 1, C_OUTLINE);
        box(bx+4, by+2,  9, 1, b2);
    } else if(wa<12){
        // Wing middle
        box(bx+4, by+5,  10, 2, b1);
        box(bx+5, by+5,  8,  1, b3);
        box(bx+4, by+7,  10, 1, b2);
    } else {
        // Wing down
        box(bx+5, by+11, 8,  1, b2);
        box(bx+4, by+9,  9,  2, b1);
        box(bx+5, by+9,  8,  1, b3);
        box(bx+4, by+11, 9,  1, b2);
        box(bx+5, by+12, 7,  1, C_OUTLINE);
    }

    // ---- Eye (big, round, expressive) ----
    int ey=by+1+tilt;
    // eye white (large rounded)
    box(bx+11,ey+0, 7, 1, C_EYEW);
    box(bx+10,ey+1, 9, 5, C_EYEW);
    box(bx+11,ey+6, 7, 1, C_EYEW);
    // eye outline
    box(bx+10,ey,   7, 1, C_OUTLINE);
    box(bx+9, ey+1, 1, 5, C_OUTLINE);
    box(bx+18,ey+1, 1, 5, C_OUTLINE);
    box(bx+10,ey+6, 7, 1, C_OUTLINE);
    // pupil (dark, slightly off-center forward)
    box(bx+14,ey+2, 4, 4, C_EYE);
    box(bx+13,ey+3, 5, 2, C_EYE);
    // shine (two dots top-left of pupil)
    pset(bx+12,ey+2,C_WHITE);
    pset(bx+13,ey+2,C_WHITE);
    pset(bx+12,ey+3,C_WHITE);

    // ---- Beak (two parts, angled) ----
    int bky=by+4+tilt;
    // beak outline
    box(bx+19,bky-1,  1, 1, C_OUTLINE);
    box(bx+20,bky,    1, 4, C_OUTLINE);
    box(bx+25,bky+1,  1, 3, C_OUTLINE);
    box(bx+19,bky+4,  1, 1, C_OUTLINE);
    box(bx+19,bky+7,  1, 1, C_OUTLINE);
    box(bx+20,bky+8,  1, 1, C_OUTLINE);
    box(bx+25,bky+5,  1, 2, C_OUTLINE);
    box(bx+19,bky+4,  6, 1, C_OUTLINE); // divider line
    // upper beak
    box(bx+21,bky,    4, 1, C_BEAK);
    box(bx+20,bky+1,  5, 2, C_BEAK);
    box(bx+20,bky+3,  4, 1, C_BEAK);
    // beak highlight
    box(bx+21,bky+1,  3, 1, C_ORANGE);
    // lower beak
    box(bx+20,bky+5,  5, 2, C_ORANGE);
    box(bx+21,bky+7,  4, 1, C_ORANGE);
    // lower beak shading
    box(bx+20,bky+7,  5, 1, C_BEAK);
}

// ================================================================
// GAME CONSTANTS
// ================================================================
#define FP       256
#define GRAVITY  (FP*5/12)
#define FLAP_V   (-FP*35/8)
#define TERM_V   (FP*7)
#define PIPE_SPD 2
#define GAP      44
#define GROUND   142
#define NPIPES   2
#define PSPACE   138
#define PW       22
#define CAP_EX   4
#define CAP_H    8
#define BIRD_X   44
#define BIRD_W   20
#define BIRD_H   13

typedef enum{ST_MENU,ST_CHARSEL,ST_COUNTDOWN,ST_GAME,ST_DEAD}State;
static State state;
static int birdYfp,birdVfp;
static int score,hiScore,totalGames;
static int frame,gndScroll,wingAnim,wingDir,charSel,menuSel;
static int countTimer,deathTimer;

typedef struct{int x,top;}Pipe;
static Pipe pipes[NPIPES];
static unsigned int rng=13337u;

#define NCLOUD 3
static int cldX[NCLOUD]={30,118,195};
static const int cldY[NCLOUD]={18,30,14};

static const char*CNAMES[3]={"YELLOW","RED","BLUE"};

static int randTop(void){
    rng=rng*1664525u+1013904223u;
    return 14+(int)((rng>>24)%(GROUND-GAP-28));
}
static void initGame(void){
    int i;
    birdYfp=(GROUND/2-6)*FP; birdVfp=0;
    score=0; frame=0; gndScroll=0;
    wingAnim=0; wingDir=1;
    for(i=0;i<NPIPES;i++){
        pipes[i].x=SW+20+i*PSPACE;
        pipes[i].top=randTop();
    }
}

// ---- Background ----
static void drawSky(void){
    box(0,0,SW,GROUND/2,C_SKY1);
    box(0,GROUND/2,SW,GROUND-GROUND/2,C_SKY2);
}
static void drawCloud(int cx,int cy){
    box(cx+6,cy,   16,3,C_CLOUD);
    box(cx+2,cy+2, 24,4,C_CLOUD);
    box(cx,  cy+5, 28,5,C_CLOUD);
    box(cx+2,cy+9, 24,3,C_CLOUD);
}
static void scrollClouds(void){
    int i; for(i=0;i<NCLOUD;i++){cldX[i]--;if(cldX[i]<-34)cldX[i]=SW+8;}
}
static void drawGround(void){
    box(0,GROUND,SW,SH-GROUND,C_GND);
    box(0,GROUND,SW,5,C_GRASS);
    int i;
    for(i=0;i<9;i++){
        int lx=((i*28)-gndScroll+280)%SW;
        box(lx,GROUND+7,16,3,C_GNDLN);
    }
}
static void drawPipe(int px,int top){
    int bot=top+GAP;
    box(px,0,PW,top,C_PIPE);
    box(px+3,0,5,top,C_PIPECAP);
    box(px,0,1,top,C_PIPELN);
    box(px+PW-1,0,1,top,C_PIPELN);
    int cx=px-CAP_EX,cw=PW+CAP_EX*2;
    box(cx,top-CAP_H,cw,CAP_H,C_PIPECAP);
    box(cx+3,top-CAP_H,5,CAP_H,C_PIPE);
    box(cx,top-CAP_H,1,CAP_H,C_PIPELN);
    box(cx+cw-1,top-CAP_H,1,CAP_H,C_PIPELN);
    box(cx,top-CAP_H,cw,1,C_PIPELN);
    box(cx,top-1,cw,1,C_PIPELN);
    box(cx,bot,cw,CAP_H,C_PIPECAP);
    box(cx+3,bot,5,CAP_H,C_PIPE);
    box(cx,bot,1,CAP_H,C_PIPELN);
    box(cx+cw-1,bot,1,CAP_H,C_PIPELN);
    box(cx,bot,cw,1,C_PIPELN);
    box(cx,bot+CAP_H-1,cw,1,C_PIPELN);
    int bb=bot+CAP_H;
    box(px,bb,PW,GROUND-bb,C_PIPE);
    box(px+3,bb,5,GROUND-bb,C_PIPECAP);
    box(px,bb,1,GROUND-bb,C_PIPELN);
    box(px+PW-1,bb,1,GROUND-bb,C_PIPELN);
}

static void panel(int x,int y,int w,int h){
    box(x,y,w,h,C_PANBDR);
    box(x+2,y+2,w-4,h-4,C_PANELD);
    box(x+3,y+3,w-6,h-6,C_PANEL);
}
static void panelHL(int x,int y,int w,int h){
    box(x,y,w,h,C_GOLD);
    box(x+2,y+2,w-4,h-4,C_PANELD);
    box(x+3,y+3,w-6,h-6,C_PANEL);
}

// ================================================================
// SCREENS
// ================================================================
static void drawBg(void){
    drawSky();
    int i;
    for(i=0;i<NCLOUD;i++) drawCloud(cldX[i],cldY[i]);
    drawGround();
}

static void drawMenu(void){
    drawBg();
    drawPipe(185,20);
    drawBirdAt(14,GROUND/2-4,-FP,wingAnim,charSel);

    // Title
    panel(18,12,204,46);
    dstrC(18,"FLAPPY",C_SCORE,3);
    dstrC(36,"BIRD",C_WHITE,3);

    // PLAY button (index 0)
    if(menuSel==0) panelHL(68,70,104,20);
    else           panel(68,70,104,20);
    dstrC(76,"PLAY",menuSel==0?C_GOLD:C_WHITE,2);

    // CHARACTERS button (index 1)
    if(menuSel==1) panelHL(68,96,104,20);
    else           panel(68,96,104,20);
    dstrC(102,"CHARACTERS",menuSel==1?C_GOLD:C_WHITE,1);

    // Arrow hint
    box(110,122,20,8,C_DARK);
    dstrC(123,"UP DOWN TO SELECT",C_PANBDR,1);

    // Hi score
    if(hiScore>0){
        box(84,134,72,14,C_DARK);
        dstr(88,137,"BEST",C_GOLD,1);
        dnum(120,137,hiScore,C_WHITE,1);
    }
}

static void drawCharSelect(void){
    drawBg();
    panel(10,6,220,24);
    dstrC(12,"SELECT CHARACTER",C_SCORE,1);

    int i;
    for(i=0;i<3;i++){
        int cx=22+i*72;
        int cy=42;
        if(i==charSel) panelHL(cx-4,cy-4,66,72);
        else           panel(cx-4,cy-4,66,72);
        box(cx-2,cy-2,62,68,C_PANEL);
        // Bird preview — centered in box
        drawBirdAt(cx+10,cy+8,0,wingAnim,i);
        int nw=strW(CNAMES[i],1);
        dstr(cx+(58-nw)/2,cy+52,CNAMES[i],
             i==charSel?C_GOLD:C_WHITE,1);
    }

    panel(34,122,172,22);
    dstrC(128,"L R SELECT   A CONFIRM   B BACK",C_WHITE,1);

    // arrows
    if(charSel>0)  dstr(10,72,"<",C_GOLD,2);
    if(charSel<2)  dstr(222,72,">",C_GOLD,2);
}

static void drawCountdown(void){
    drawBg();
    int i;
    for(i=0;i<NPIPES;i++) drawPipe(pipes[i].x,pipes[i].top);
    drawGround();
    drawBirdAt(BIRD_X,birdYfp/FP,0,wingAnim,charSel);
    panel(46,36,148,24);
    dstrC(42,"GET READY",C_SCORE,2);
    int t=countTimer/60+1;
    if(t>=1&&t<=3){
        panel(106,70,30,30);
        char buf[2]={'0'+t,0};
        dstrC(78,buf,C_WHITE,2);
    }
}

static void drawGameScene(void){
    drawBg();
    int i;
    for(i=0;i<NPIPES;i++) drawPipe(pipes[i].x,pipes[i].top);
    drawGround();
    drawBirdAt(BIRD_X,birdYfp/FP,birdVfp,wingAnim,charSel);
    // score with shadow
    dnumC(6,score,C_OUTLINE,2);
    dnumC(5,score,C_WHITE,2);
}

static void drawMedal(int x,int y,int s){
    unsigned char mc=(s>=20)?C_GOLD:(s>=10)?C_SILVER:C_BRONZE;
    const char*ml=(s>=20)?"GOLD":(s>=10)?"SILV":"BRNZ";
    box(x,y,24,24,C_OUTLINE);
    box(x+1,y+1,22,22,mc);
    box(x+3,y+3,18,18,C_DARK);
    box(x+5,y+5,14,14,mc);
    int nw=strW(ml,1);
    dstr(x+(24-nw)/2,y+9,ml,C_WHITE,1);
}

static void drawDead(void){
    drawBg();
    int i;
    for(i=0;i<NPIPES;i++) drawPipe(pipes[i].x,pipes[i].top);
    drawGround();
    // bird nosedive
    drawBirdAt(BIRD_X,birdYfp/FP,FP*5,0,charSel);

    panel(18,26,204,106);
    // GAME OVER header
    box(50,32,140,20,C_RED);
    box(52,34,136,16,C_DARK);
    dstrC(37,"GAME OVER",C_RED,2);

    // rows
    box(26,56,188,20,C_PANELD);
    dstr(32,61,"SCORE",C_SCORE,1);
    dnum(174,61,score,C_WHITE,1);

    box(26,78,188,20,C_PANELD);
    dstr(32,83,"BEST",C_GOLD,1);
    dnum(174,83,hiScore,C_WHITE,1);

    box(26,100,188,18,C_PANELD);
    dstr(32,104,"GAMES",C_PANBDR,1);
    dnum(174,104,totalGames,C_WHITE,1);

    drawMedal(34,58,score);

    // blink hint
    if(deathTimer>80&&(deathTimer/20)%2==0){
        box(54,126,132,14,C_DARK);
        dstrC(129,"A RETRY  START MENU",C_WHITE,1);
    }
}

// ================================================================
// MAIN
// ================================================================
int main(void){
    irqInit();
    irqEnable(IRQ_VBLANK);
    SetMode(MODE_4|BG2_ON);
    back=PAGE1; page=0;
    initPal();
    hiScore=0; totalGames=0; charSel=0; menuSel=0;
    state=ST_MENU;
    initGame();

    while(1){
        VBlankIntrWait();
        flip();
        scanKeys();
        unsigned int kd=keysDown();
        frame++;

        // shared animation
        wingAnim+=wingDir;
        if(wingAnim>=15||wingAnim<=0) wingDir=-wingDir;
        scrollClouds();

        switch(state){

        case ST_MENU:
            drawMenu();
            // UP/DOWN navigate between PLAY and CHARACTERS
            if(kd&KEY_DOWN) menuSel=1;
            if(kd&KEY_UP)   menuSel=0;
            if(kd&KEY_A){
                if(menuSel==0){
                    initGame(); countTimer=3*60;
                    totalGames++;
                    state=ST_COUNTDOWN;
                } else {
                    state=ST_CHARSEL;
                }
            }
            break;

        case ST_CHARSEL:
            drawCharSelect();
            if((kd&KEY_LEFT)||(kd&KEY_L))  {if(charSel>0)charSel--;}
            if((kd&KEY_RIGHT)||(kd&KEY_R)) {if(charSel<2)charSel++;}
            if(kd&KEY_A){ state=ST_MENU; menuSel=0; }
            if(kd&KEY_B){ state=ST_MENU; menuSel=0; }
            break;

        case ST_COUNTDOWN:
            // gentle hover bob
            {
                int by=birdYfp/FP;
                int target=(GROUND/2-6)*FP;
                if(by*FP<target) birdYfp+=FP/4;
                else             birdYfp-=FP/4;
            }
            countTimer--;
            drawCountdown();
            if(countTimer<=0){ birdYfp=(GROUND/2-6)*FP; birdVfp=0; state=ST_GAME; }
            if(kd&KEY_A){ birdVfp=FLAP_V; birdYfp=(GROUND/2-6)*FP; state=ST_GAME; }
            break;

        case ST_GAME:
            if(kd&KEY_A){ birdVfp=FLAP_V; wingAnim=0; wingDir=1; }
            birdVfp+=GRAVITY;
            if(birdVfp>TERM_V) birdVfp=TERM_V;
            birdYfp+=birdVfp;
            gndScroll=(gndScroll+PIPE_SPD)%SW;
            scrollClouds();
            {
                int i;
                for(i=0;i<NPIPES;i++){
                    pipes[i].x-=PIPE_SPD;
                    if(pipes[i].x<-PW-CAP_EX-2){
                        pipes[i].x=SW+PSPACE/2;
                        pipes[i].top=randTop();
                        score++;
                        if(score>hiScore)hiScore=score;
                    }
                }
            }
            {
                int by=birdYfp/FP;
                if(by<=1||by+BIRD_H>=GROUND){state=ST_DEAD;deathTimer=0;break;}
                int i;
                for(i=0;i<NPIPES;i++){
                    if(BIRD_X+BIRD_W-2>pipes[i].x-CAP_EX&&
                       BIRD_X+3       <pipes[i].x+PW+CAP_EX){
                        if(by+1<pipes[i].top||by+BIRD_H-1>pipes[i].top+GAP){
                            state=ST_DEAD;deathTimer=0;break;
                        }
                    }
                }
            }
            drawGameScene();
            break;

        case ST_DEAD:
            deathTimer++;
            drawDead();
            if(deathTimer>80&&(kd&KEY_A)){
                initGame(); countTimer=3*60; totalGames++;
                state=ST_COUNTDOWN;
            }
            if(deathTimer>80&&(kd&KEY_START)){
                menuSel=0; state=ST_MENU; initGame();
            }
            break;
        }
    }
    return 0;
}
