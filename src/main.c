#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <stdio.h>

#define SCREEN_W   30
#define SCREEN_H   19
#define BIRD_X      5
#define GAP_SIZE    5
#define NUM_PIPES   2
#define PIPE_SPACE  15

typedef struct { int x, gapTop; } Pipe;

static int  birdY;
static int  birdVel;
static int  score;
static int  dead;
static int  frame;
static Pipe pipes[NUM_PIPES];

static unsigned int rng = 12345;
static int randGap(void) {
    rng = rng * 1664525u + 1013904223u;
    return 2 + (int)((rng >> 24) % (SCREEN_H - GAP_SIZE - 4));
}

static void initGame(void) {
    birdY   = SCREEN_H / 2;
    birdVel = 0;
    score   = 0;
    dead    = 0;
    frame   = 0;
    int i;
    for (i = 0; i < NUM_PIPES; i++) {
        pipes[i].x      = SCREEN_W + 2 + i * PIPE_SPACE;
        pipes[i].gapTop = randGap();
    }
}

// Build and print one row at a time — no ANSI escapes
static void drawGame(void) {
    iprintf("\x1b[2J"); // just one clear at top
    int row, col;
    for (row = 0; row < SCREEN_H; row++) {
        char line[SCREEN_W + 2];
        int i;
        for (col = 0; col < SCREEN_W; col++) {
            char ch = ' ';

            // bird
            if (col == BIRD_X && row == birdY) {
                ch = '>';
            } else {
                // check pipes
                for (i = 0; i < NUM_PIPES; i++) {
                    if (col == pipes[i].x || col == pipes[i].x + 1) {
                        if (row < pipes[i].gapTop ||
                            row >= pipes[i].gapTop + GAP_SIZE) {
                            ch = '|';
                        }
                        break;
                    }
                }
            }
            line[col] = ch;
        }
        line[SCREEN_W]   = '\n';
        line[SCREEN_W+1] = '\0';
        iprintf("%s", line);
    }
    iprintf("Score:%-4d [A]=Flap", score);
}

static void drawScreen(const char *l1, const char *l2, const char *l3) {
    iprintf("\x1b[2J");
    int i;
    for (i = 0; i < 6; i++) iprintf("\n");
    iprintf("  %s\n\n", l1);
    iprintf("  %s\n\n", l2);
    iprintf("  %s\n",   l3);
}

int main(void) {
    irqInit();
    irqEnable(IRQ_VBLANK);
    consoleDemoInit();
    SetMode(MODE_0 | BG0_ON);

    drawScreen("=== FLAPPY BIRD ===",
               "A = Flap  |  pipes",
               "START to begin!");

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
            char buf[20];
            iprintf("\x1b[2J");
            int i;
            for (i = 0; i < 7; i++) iprintf("\n");
            iprintf("  ** GAME OVER **\n\n");
            iprintf("  Score: %d\n\n", score);
            iprintf("  START to retry");
            if (keysDown() & KEY_START) initGame();
            continue;
        }

        // Input
        if (keysDown() & KEY_A) {
            birdVel = -3;
        }

        // Physics every 2 frames
        if (frame % 2 == 0) {
            birdVel++;
            if (birdVel > 3) birdVel = 3;
            birdY += birdVel;
        }

        // Scroll pipes every 3 frames
        if (frame % 3 == 0) {
            int i;
            for (i = 0; i < NUM_PIPES; i++) {
                pipes[i].x--;
                if (pipes[i].x < -1) {
                    pipes[i].x      = SCREEN_W + PIPE_SPACE;
                    pipes[i].gapTop = randGap();
                    score++;
                }
            }
        }

        frame++;

        // Collisions
        if (birdY < 0 || birdY >= SCREEN_H) {
            dead = 1; continue;
        }
        int i;
        for (i = 0; i < NUM_PIPES; i++) {
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
