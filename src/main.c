#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <stdio.h>
#include <stdlib.h>

#define SCREEN_W    30
#define SCREEN_H    20
#define BIRD_X      5
#define GAP_SIZE    5
#define NUM_PIPES   2
#define PIPE_SPACE  15

#define MOVE(x,y)   iprintf("\x1b[%d;%dH", (y)+1, (x)+1)
#define CLR()       iprintf("\x1b[2J")
#define HIDE_CURSOR iprintf("\x1b[?25l")

typedef struct { int x, gapTop; } Pipe;

static int  birdY;
static int  birdSub;
static int  birdVel;
static int  score;
static int  dead;
static int  frame;
static Pipe pipes[NUM_PIPES];

static unsigned int rng = 12345;
static int randGap() {
    rng = rng * 1664525u + 1013904223u;
    return 2 + (int)((rng >> 24) % (SCREEN_H - GAP_SIZE - 4));
}

static void initGame(void) {
    birdY   = SCREEN_H / 2;
    birdSub = 0;
    birdVel = -8;
    score   = 0;
    dead    = 0;
    frame   = 0;

    for (int i = 0; i < NUM_PIPES; i++) {
        pipes[i].x      = SCREEN_W + 2 + i * PIPE_SPACE;
        pipes[i].gapTop = randGap();
    }
}

static void drawPipe(int px, int gapTop) {
    if (px < 0 || px >= SCREEN_W) return;
    for (int row = 0; row < SCREEN_H - 1; row++) {
        MOVE(px, row);
        if (row >= gapTop && row < gapTop + GAP_SIZE)
            iprintf(" ");
        else
            iprintf("|");
    }
}

static void drawGame(void) {
    CLR();
    MOVE(0, SCREEN_H - 1);
    iprintf("Score:%-4d  [A]=Flap", score);

    for (int i = 0; i < NUM_PIPES; i++) {
        drawPipe(pipes[i].x,     pipes[i].gapTop);
        drawPipe(pipes[i].x + 1, pipes[i].gapTop);
    }

    if (birdY >= 0 && birdY < SCREEN_H - 1) {
        MOVE(BIRD_X, birdY);
        iprintf(">");
    }
}

static void drawDead(void) {
    CLR();
    MOVE(8,  7); iprintf("** GAME OVER **");
    MOVE(8,  9); iprintf("Score: %d", score);
    MOVE(6, 11); iprintf("Press START to retry");
}

static void drawTitle(void) {
    CLR();
    MOVE(7,  5); iprintf("=== FLAPPY BIRD ===");
    MOVE(8,  7); iprintf("(Text Edition)");
    MOVE(6, 10); iprintf("A Button  =  Flap");
    MOVE(4, 12); iprintf("Dodge the | pipes!");
    MOVE(6, 15); iprintf("Press START to play");
}

int main(void) {
    irqInit();
    irqEnable(IRQ_VBLANK);
    consoleDemoInit();
    SetMode(MODE_0 | BG0_ON);
    HIDE_CURSOR;

    drawTitle();
    while (1) {
        VBlankIntrWait();
        scanKeys();
        if (keysDown() & KEY_START) break;
    }

    initGame();

    while (1) {
        VBlankIntrWait();
        scanKeys();

        if (dead) {
            drawDead();
            if (keysDown() & KEY_START) initGame();
            continue;
        }

        if (keysDown() & KEY_A) {
            birdVel = -10;
        }

        birdSub += birdVel;
        birdVel += 2;
        if (birdVel > 12) birdVel = 12;

        birdY  += birdSub / 4;
        birdSub %= 4;

        if (frame % 2 == 0) {
            for (int i = 0; i < NUM_PIPES; i++) {
                pipes[i].x--;
                if (pipes[i].x < -2) {
                    pipes[i].x      = SCREEN_W + PIPE_SPACE - 2;
                    pipes[i].gapTop = randGap();
                    score++;
                }
            }
        }

        frame++;

        if (birdY < 0 || birdY >= SCREEN_H - 1) {
            dead = 1;
            continue;
        }

        for (int i = 0; i < NUM_PIPES; i++) {
            if (BIRD_X == pipes[i].x || BIRD_X == pipes[i].x + 1) {
                if (birdY < pipes[i].gapTop ||
                    birdY >= pipes[i].gapTop + GAP_SIZE) {
                    dead = 1;
                }
            }
        }

        drawGame();
    }

    return 0;
}
