#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>

// ================================================================
// MODE 4 DOUBLE BUFFER
// ================================================================
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

// ---- Palette indices ----
#define C_BG     0
#define C_SKY1   1
#define C_SKY2   2
#define C_CLOUD  3
#define C_PIPE   4
#define C_PIPED  5
#define C_PIPECAP 6
#define C_PIPELN 7
#define C_GND    8
#define C_GRASS  9
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

static void initPal(void){
    PAL[C_BG]    =RGB15(0,0,0);
    PAL[C_SKY1]  =RGB15(7,18,30);
    PAL[C_SKY2]  =RGB15(12,22,31);
    PAL[C_CLOUD] =RGB15(28,30,31);
    PAL[C_PIPE]  =RGB15(4,20,4);
    PAL[C_PIPED] =RGB15(2,12,2);
    PAL[C_PIPECAP]=RGB15(7,25,7);
    PAL[C_PIPELN]=RGB15(1,5,1);
    PAL[C_GND]   =RGB15(19,12,3);
    PAL[C_GRASS] =RGB15(5,21,3);
    PAL[C_GNDLN] =RGB15(13,8,2);
    PAL[C_WHITE] =RGB15(31,31,31);
    PAL[C_EYEW]  =RGB15(31,31,31);
    PAL[C_EYE]   =RGB15(1,1,4);
    PAL[C_BEAK]  =RGB15(31,14,0);
    PAL[C_ORANGE]=RGB15(31,18,2);
    PAL[C_WING1] =RGB15(31,20,0);
    PAL[C_WING2] =RGB15(28,13,0);
    PAL[C_YEL1]  =RGB15(31,28,1);
    PAL[C_YEL2]  =RGB15(24,20,0);
    PAL[C_YEL3]  =RGB15(31,31,14);
    PAL[C_RED1]  =RGB15(27,4,4);
    PAL[C_RED2]  =RGB15(19,2,2);
    PAL[C_RED3]  =RGB15(31,16,16);
    PAL[C_BLU1]  =RGB15(4,14,28);
    PAL[C_BLU2]  =RGB15(2,8,20);
    PAL[C_BLU3]  =RGB15(16,24,31);
    PAL[C_SCORE] =RGB15(31,31,10);
    PAL[C_PANEL] =RGB15(3,11,23);
    PAL[C_PANELD]=RGB15(1,5,13);
    PAL[C_PANBDR]=RGB15(5,18,31);
    PAL[C_GOLD]  =RGB15(31,26,2);
    PAL[C_SILVER]=RGB15(22,22,23);
    PAL[C_BRONZE]=RGB15(22,12,4);
    PAL[C_RED]   =RGB15(27,3,3);
    PAL[C_DARK]  =RGB15(1,1,3);
}

// ---- Draw primitives ----
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

// ---- 5x7 font ----
static const unsigned char F5[36][7]={
    {0x0E,0x11,0x13,0x15,0x19,0x11,0x0E},{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E},
    {0x0E,0x11,0x01,0x06,0x08,0x10,0x1F},{0x1F,0x01,0x02,0x06,0x01,0x11,0x0E},
    {0x02,0x06,0x0A,0x12,0x1F,0x02,0x02},{0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E},
    {0x06,0x08,0x10,0x1E,0x11,0x11,0x0E},{0x1F,0x01,0x02,0x04,0x08,0x08,0x08},
    {0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E},{0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C},
    // A-Z
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
    while(*s){
        if(*s==' '){x+=(5+1)*sc;s++;continue;}
        dchar(x,y,*s,col,sc); x+=(5+1)*sc; s++;
    }
}
static int strW(const char*s,int sc){
    int w=0;
    while(*s){w+=(5+1)*sc;s++;}
    return w-(sc); // remove trailing space
}
static void dstrC(int y,const char*s,unsigned char col,int sc){
    dstr((SW-strW(s,sc))/2,y,s,col,sc);
}
static void dnum(int x,int y,int n,unsigned char col,int sc){
    char buf[8]; int i=7; buf[i]=0;
    if(n==0){buf[--i]='0';}
    while(n>0){buf[--i]='0'+n%10;n/=10;}
    dstr(x,y,buf+i,col,sc);
}
static void dnumC(int y,int n,unsigned char col,int sc){
    char buf[8]; int i=7; buf[i]=0;
    if(n==0){buf[--i]='0';}
    while(n>0){buf[--i]='0'+n%10;n/=10;}
    dstrC(y,buf+i,col,sc);
}

// ================================================================
// GAME CONSTANTS — exact Flappy Bird physics scaled to 160px
// ================================================================
#define FP       256
// Original: gravity=1.25px/frame², flap=-12.5px/frame at 60fps
// GBA screen 160px vs original ~512px => scale ~0.31
// Scaled: gravity=0.39 => FP*10/26, flap=-3.9 => -FP*10/6.5
// Tuned: gravity slightly stronger feels better on small screen
#define GRAVITY  (FP*5/12)   // ~0.42 px/frame²
#define FLAP_V   (-FP*35/8)  // always SET to -4.375 px/frame on tap
#define TERM_V   (FP*7)      // terminal velocity 7 px/frame
#define PIPE_SPD 2
#define GAP      44          // tight gap, scaled from original 140px
#define GROUND   142
#define NPIPES   2
#define PSPACE   138
#define PW       22
#define CAP_EX   4
#define CAP_H    8
#define BIRD_X   50
#define BIRD_W   17
#define BIRD_H   12

// ================================================================
// STATE
// ================================================================
typedef enum { ST_MENU, ST_CHARSEL, ST_COUNTDOWN, ST_GAME, ST_DEAD } State;
static State state;
static int birdYfp, birdVfp;
static int score, hiScore, totalGames;
static int frame, gndScroll, wingAnim, wingDir, charSel;
static int countTimer, deathTimer;
static int getReady; // flag: show get ready

typedef struct{int x,top;}Pipe;
static Pipe pipes[NPIPES];
static unsigned int rng=13337u;

#define NCLOUD 3
static int cldX[NCLOUD]={30,118,195};
static const int cldY[NCLOUD]={18,30,14};

// Bird color sets [main, dark, highlight]
static const unsigned char BC[3][3]={
    {C_YEL1,C_YEL2,C_YEL3},
    {C_RED1,C_RED2,C_RED3},
    {C_BLU1,C_BLU2,C_BLU3},
};
static const char *CNAMES[3]={"YELLOW","RED","BLUE"};

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

// ================================================================
// DRAW HELPERS
// ================================================================
static void drawSky(void){
    box(0,0,SW,GROUND/2,C_SKY1);
    box(0,GROUND/2,SW,GROUND-GROUND/2,C_SKY2);
}
static void drawCloud(int cx,int cy){
    // Rounded cloud
    box(cx+6,cy,16,3,C_CLOUD);
    box(cx+2,cy+2,24,4,C_CLOUD);
    box(cx,cy+5,28,5,C_CLOUD);
    box(cx+2,cy+9,24,3,C_CLOUD);
}
static void scrollClouds(void){
    int i;
    for(i=0;i<NCLOUD;i++){cldX[i]--;if(cldX[i]<-34)cldX[i]=SW+8;}
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
    // top body with highlight
    box(px,0,PW,top,C_PIPE);
    box(px+3,0,5,top,C_PIPECAP);
    box(px,0,1,top,C_PIPELN);
    box(px+PW-1,0,1,top,C_PIPELN);
    // top cap
    int cx=px-CAP_EX, cw=PW+CAP_EX*2;
    box(cx,top-CAP_H,cw,CAP_H,C_PIPECAP);
    box(cx+3,top-CAP_H,5,CAP_H,C_PIPE);
    box(cx,top-CAP_H,1,CAP_H,C_PIPELN);
    box(cx+cw-1,top-CAP_H,1,CAP_H,C_PIPELN);
    box(cx,top-CAP_H,cw,1,C_PIPELN);
    box(cx,top-1,cw,1,C_PIPELN);
    // bottom cap
    box(cx,bot,cw,CAP_H,C_PIPECAP);
    box(cx+3,bot,5,CAP_H,C_PIPE);
    box(cx,bot,1,CAP_H,C_PIPELN);
    box(cx+cw-1,bot,1,CAP_H,C_PIPELN);
    box(cx,bot,cw,1,C_PIPELN);
    box(cx,bot+CAP_H-1,cw,1,C_PIPELN);
    // bottom body
    int bb=bot+CAP_H;
    box(px,bb,PW,GROUND-bb,C_PIPE);
    box(px+3,bb,5,GROUND-bb,C_PIPECAP);
    box(px,bb,1,GROUND-bb,C_PIPELN);
    box(px+PW-1,bb,1,GROUND-bb,C_PIPELN);
}

static void drawBirdAt(int bx,int by,int vel,int wa,int ci){
    unsigned char b1=BC[ci][0],b2=BC[ci][1],b3=BC[ci][2];
    // tilt: shift beak+eye y based on velocity
    int tilt=vel/FP; // -4 to 7
    if(tilt<-2)tilt=-2;
    if(tilt> 3)tilt= 3;

    // Rounded body using layered boxes
    box(bx+3, by,        11,  1, b1);
    box(bx+1, by+1,      15,  1, b1);
    box(bx,   by+2,      17,  7, b1);
    box(bx+1, by+9,      15,  1, b2);
    box(bx+3, by+10,     11,  1, b2);
    // top highlight stripe
    box(bx+4, by+1,       7,  2, b3);
    box(bx+3, by+3,       3,  1, b3);
    // underbody shade
    box(bx+2, by+7,      13,  3, b2);
    // belly lighter spot
    box(bx+1, by+4,       5,  3, C_WHITE);
    box(bx+2, by+3,       4,  1, C_WHITE);
    // fix: overlay body color on belly edges
    pset(bx+1,by+4,b1); pset(bx+1,by+6,b1);

    // Wing (smooth up/down animation)
    int wingUp=(wa<8);
    if(wingUp){
        // wing up
        box(bx+3, by+1, 9, 2, C_WING1);
        box(bx+4, by+1, 7, 1, C_WING2);
        pset(bx+3,by+2,C_WING2);
    } else {
        // wing down
        box(bx+3, by+8, 9, 2, C_WING1);
        box(bx+4, by+9, 7, 1, C_WING2);
        pset(bx+3,by+8,C_WING2);
    }

    // Eye (rounded, with pupil + shine)
    int ey=by+2+tilt/2;
    box(bx+10, ey,   6, 6, C_EYEW);
    box(bx+9,  ey+1, 8, 4, C_EYEW);
    box(bx+10, ey+5, 6, 1, C_EYEW);
    // pupil
    box(bx+12, ey+1, 4, 4, C_EYE);
    box(bx+11, ey+2, 6, 2, C_EYE);
    // shine dots
    pset(bx+12,ey+1,C_WHITE);
    pset(bx+13,ey+1,C_WHITE);
    pset(bx+12,ey+2,C_WHITE);

    // Beak (angled based on tilt)
    int bky=by+3+tilt/2;
    // upper beak
    box(bx+17,bky,  6, 3, C_BEAK);
    box(bx+18,bky-1,4, 1, C_BEAK);
    // lower beak  
    box(bx+17,bky+3,5, 3, C_ORANGE);
    box(bx+18,bky+5,4, 1, C_BEAK);
    // mouth line
    box(bx+17,bky+2,6, 1, C_YEL2);
}

// ================================================================
// PANEL HELPERS
// ================================================================
static void panel(int x,int y,int w,int h){
    // border
    box(x,y,w,h,C_PANBDR);
    // inner dark
    box(x+2,y+2,w-4,h-4,C_PANELD);
    // inner fill
    box(x+3,y+3,w-6,h-6,C_PANEL);
}

// ================================================================
// SCREEN DRAW FUNCTIONS
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
    drawBirdAt(22,GROUND/2-4, -FP, wingAnim, charSel);

    // Title panel
    panel(20,14,200,50);
    dstrC(20,"FLAPPY",C_SCORE,3);
    dstrC(38,"BIRD",C_WHITE,3);

    // Play button
    panel(70,76,100,20);
    dstrC(82,"PLAY",C_WHITE,2);

    // Characters button
    panel(70,102,100,20);
    dstrC(108,"CHARACTERS",C_WHITE,1);

    // Blinking A button hint
    if((frame/20)%2==0){
        box(88,126,64,10,C_DARK);
        dstrC(128,"A TO START",C_PANBDR,1);
    }

    // Hi score
    if(hiScore>0){
        box(88,140,64,12,C_DARK);
        dstr(92,142,"BEST",C_GOLD,1);
        dnum(122,142,hiScore,C_WHITE,1);
    }
}

static void drawCharSelect(void){
    drawBg();
    panel(10,8,220,22);
    dstrC(13,"SELECT CHARACTER",C_SCORE,1);

    int i;
    for(i=0;i<3;i++){
        int cx=30+i*64;
        int cy=48;
        // Selection highlight
        if(i==charSel){
            box(cx-6,cy-6,44,54,C_GOLD);
            box(cx-4,cy-4,40,50,C_PANBDR);
        } else {
            box(cx-4,cy-4,40,50,C_PANELD);
        }
        box(cx-2,cy-2,36,46,C_PANEL);
        // Draw bird preview
        drawBirdAt(cx,cy+8,0,wingAnim,i);
        // Name
        int nw=strW(CNAMES[i],1);
        dstr(cx+(32-nw)/2,cy+34,CNAMES[i],
             i==charSel?C_GOLD:C_WHITE, 1);
    }

    // Controls hint
    panel(40,118,160,20);
    dstr(50,124,"L/R SELECT  A CONFIRM",C_WHITE,1);

    // Arrow indicators
    if(charSel>0){ dstr(18,78,"<",C_GOLD,2); }
    if(charSel<2){ dstr(214,78,">",C_GOLD,2); }
}

static void drawCountdown(void){
    drawBg();
    int i;
    for(i=0;i<NPIPES;i++) drawPipe(pipes[i].x,pipes[i].top);
    drawGround();
    drawBirdAt(BIRD_X,birdYfp/FP,0,wingAnim,charSel);

    // "GET READY" panel
    panel(44,38,152,24);
    dstrC(44,"GET READY",C_SCORE,2);

    // Countdown number
    int t=countTimer/60+1; // 3,2,1
    if(t>=1&&t<=3){
        panel(104,72,32,32);
        char buf[2]={'0'+t,0};
        dstrC(80,buf,C_WHITE,2);
    }
}

static void drawGameScene(void){
    drawBg();
    int i;
    for(i=0;i<NPIPES;i++) drawPipe(pipes[i].x,pipes[i].top);
    drawGround();
    drawBirdAt(BIRD_X,birdYfp/FP,birdVfp,wingAnim,charSel);
    // Score centered top
    dnumC(5,score,C_WHITE,2);
    // Score shadow
    dnumC(6,score,C_PANELD,2);
    dnumC(5,score,C_WHITE,2);
}

static void drawMedal(int x,int y,int s){
    unsigned char mc=(s>=30)?C_GOLD:(s>=20)?C_GOLD:(s>=10)?C_SILVER:C_BRONZE;
    const char *ml=(s>=30)?"GOLD":(s>=20)?"GOLD":(s>=10)?"SILV":"BRNZ";
    box(x,y,24,24,mc);
    box(x+2,y+2,20,20,C_DARK);
    box(x+4,y+4,16,16,mc);
    dstr(x+4,y+9,ml,C_WHITE,1);
}

static void drawDead(void){
    // Draw frozen game scene underneath
    drawBg();
    int i;
    for(i=0;i<NPIPES;i++) drawPipe(pipes[i].x,pipes[i].top);
    drawGround();
    drawBirdAt(BIRD_X,birdYfp/FP,FP*3,0,charSel); // nosedive pose

    // Stats panel
    panel(20,30,200,98);

    // GAME OVER
    box(52,36,136,18,C_RED);
    box(54,38,132,14,C_DARK);
    dstrC(40,"GAME OVER",C_RED,2);

    // Score row
    box(28,58,184,18,C_PANELD);
    dstr(34,62,"SCORE",C_SCORE,1);
    dnum(160-strW("00",1),62,score,C_WHITE,1);

    // Best row
    box(28,78,184,18,C_PANELD);
    dstr(34,82,"BEST",C_GOLD,1);
    dnum(160-strW("00",1),82,hiScore,C_WHITE,1);

    // Games row
    box(28,98,184,16,C_PANELD);
    dstr(34,100,"GAMES",C_PANBDR,1);
    dnum(160-strW("00",1),100,totalGames,C_WHITE,1);

    // Medal
    drawMedal(36,60,score);

    // Blink restart hint
    if(deathTimer>60&&(deathTimer/20)%2==0){
        box(60,132,120,16,C_DARK);
        dstrC(136,"A TO RETRY",C_WHITE,1);
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

    hiScore=0; totalGames=0; charSel=0;
    state=ST_MENU;
    initGame();

    while(1){
        VBlankIntrWait();
        flip();
        scanKeys();
        unsigned int kd=keysDown();
        unsigned int kh=keysHeld();
        frame++;

        // Animate common
        wingAnim=(wingAnim+wingDir+16)%16;
        if(wingAnim==0||wingAnim==15) wingDir=-wingDir;
        scrollClouds();

        switch(state){

        case ST_MENU:
            drawMenu();
            if(kd&KEY_A){state=ST_CHARSEL;}
            break;

        case ST_CHARSEL:
            drawCharSelect();
            if((kd&KEY_LEFT)||(kd&KEY_L)){if(charSel>0)charSel--;}
            if((kd&KEY_RIGHT)||(kd&KEY_R)){if(charSel<2)charSel++;}
            if(kd&KEY_A){
                initGame();
                countTimer=3*60;
                state=ST_COUNTDOWN;
            }
            if(kd&KEY_B){state=ST_MENU;}
            break;

        case ST_COUNTDOWN:
            // Bird bobs gently
            birdYfp+=(int)(10*256*2*3/100); // gentle bob
            if(birdYfp>(GROUND/2+6)*FP||birdYfp<(GROUND/2-10)*FP)
                birdYfp=(GROUND/2-4)*FP;
            countTimer--;
            drawCountdown();
            if(countTimer<=0){
                birdYfp=(GROUND/2-4)*FP;
                birdVfp=0;
                state=ST_GAME;
                totalGames++;
            }
            // allow early start with A
            if(kd&KEY_A){
                birdVfp=FLAP_V; // immediate flap
                birdYfp=(GROUND/2-4)*FP;
                state=ST_GAME;
                totalGames++;
                wingAnim=0;
            }
            break;

        case ST_GAME:
            // Input: tap always SETS velocity (key Flappy Bird mechanic)
            if(kd&KEY_A){
                birdVfp=FLAP_V;
                wingAnim=0; wingDir=1;
            }

            // Physics every frame at 60fps
            birdVfp+=GRAVITY;
            if(birdVfp>TERM_V) birdVfp=TERM_V;
            birdYfp+=birdVfp;

            // Ground scroll
            gndScroll=(gndScroll+PIPE_SPD)%SW;

            // Pipes
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

            // Collision
            {
                int by=birdYfp/FP;
                if(by<=1||by+BIRD_H>=GROUND){
                    state=ST_DEAD; deathTimer=0; break;
                }
                int i;
                for(i=0;i<NPIPES;i++){
                    if(BIRD_X+BIRD_W-2>pipes[i].x-CAP_EX &&
                       BIRD_X+2       <pipes[i].x+PW+CAP_EX){
                        if(by+1<pipes[i].top||by+BIRD_H-1>pipes[i].top+GAP){
                            state=ST_DEAD; deathTimer=0; break;
                        }
                    }
                }
            }

            drawGameScene();
            break;

        case ST_DEAD:
            deathTimer++;
            drawDead();
            if(deathTimer>90&&(kd&KEY_A)){
                initGame();
                countTimer=3*60;
                state=ST_COUNTDOWN;
            }
            if(deathTimer>90&&(kd&KEY_START)){
                state=ST_MENU;
                initGame();
            }
            break;
        }
    }
    return 0;
}
