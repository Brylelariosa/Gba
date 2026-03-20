#include <gba_console.h>
#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <stdio.h>

// Write directly to BG tilemap — no iprintf flickering
#define MAP   ((volatile unsigned short*)0x0600F800)
#define SW    30
#define SH    20

static void cls(void) {
    int i;
    for (i = 0; i < 32 * SH; i++) MAP[i] = ' ';
}

static void putch(int x, int y, char c) {
    if (x >= 0 && x < SW && y >= 0 && y < SH)
        MAP[y * 32 + x] = (unsigned short)c;
}

static void putstr(int x, int y, const char *s) {
    while (*s) putch(x++, y, *s++);
}

// itoa helper for score
static void putint(int x, int y, int n) {
    char buf[8];
    int i = 7;
    buf[i] = '\0';
    if (n == 0) { buf[--i] = '0'; }
    while (n > 0) { buf[--i] = '0' + (n % 10); n /= 10; }
    putstr(x, y, buf + i);
}

// ---- Game ----
#define BIRD_X    5
#define GAP       5
#define NPIPES    2
#define PSPACE   15

typedef struct { int x, top; } Pipe;

static int   birdY, birdVel, score, dead, frame;
static Pipe  pipes[NPIPES];
static unsigned int rng = 99991;

static int randTop(void) {
    rng = rng * 1664525u + 1013904223u;
    return 2 + (int)((rng >> 24) % (SH - GAP - 4));
}

static void init(void) {
    int i;
    birdY = SH / 2; birdVel = 0;
    score = 0; dead = 0; frame = 0;
    for (i = 0; i < NPIPES; i++) {
        pipes[i].x   = SW + 2 + i * PSPACE;
        pipes[i].top = randTop();
    }
}

static void draw(void) {
    int x, y, i;
    cls();

    // pipes
    for (i = 0; i < NPIPES; i++) {
        for (y = 0; y < SH - 1; y++) {
            if (y < pipes[i].top || y >= pipes[i].top + GAP) {
                putch(pipes[i].x,     y, '|');
                putch(pipes[i].x + 1, y, '|');
            }
        }
    }

    // bird
    putch(BIRD_X, birdY, '>');

    // hud
    putstr(0,  SH-1, "Score:");
    putint(6,  SH-1, score);
    putstr(20, SH-1, "[A]=Flap");
}

static void titleScreen(void) {
    cls();
    putstr(6,  6,  "== FLAPPY  BIRD ==");
    putstr(6,  9,  "A Button = Flap");
    putstr(4,  11, "Dodge the || pipes");
    putstr(4,  14, "START = Play");
}

static void deathScreen(void) {
    cls();
    putstr(7,  7,  "** GAME OVER **");
    putstr(8,  9,  "Score: ");
    putint(15, 9,  score);
    putstr(4,  12, "START = Retry");
}

int main(void) {
    irqInit();
    irqEnable(IRQ_VBLANK);
    consoleDemoInit();
    SetMode(MODE_0 | BG0_ON);

    titleScreen();
    while (1) {
        VBlankIntrWait();
        scanKeys();
        if (keysDown() & KEY_START) break;
    }

    init();

    while (1) {
        VBlankIntrWait();
        scanKeys();

        if (dead) {
            deathScreen();
            if (keysDown() & KEY_START) init();
            continue;
        }

        // Flap
        if (keysDown() & KEY_A) birdVel = -4;

        // Physics every 4 frames (~15 updates/sec)
        if (frame % 4 == 0) {
            birdVel++;
            if (birdVel >  4) birdVel =  4;
            if (birdVel < -4) birdVel = -4;
            birdY += birdVel;
        }

        // Scroll pipes every 5 frames (~12 moves/sec)
        if (frame % 5 == 0) {
            int i;
            for (i = 0; i < NPIPES; i++) {
                pipes[i].x--;
                if (pipes[i].x < -1) {
                    pipes[i].x   = SW + PSPACE;
                    pipes[i].top = randTop();
                    score++;
                }
            }
        }

        frame++;

        // Floor / ceiling
        if (birdY < 0 || birdY >= SH - 1) { dead = 1; continue; }

        // Pipe collision
        int i;
        for (i = 0; i < NPIPES; i++) {
            if (BIRD_X == pipes[i].x || BIRD_X == pipes[i].x + 1) {
                if (birdY < pipes[i].top ||
                    birdY >= pipes[i].top + GAP) {
                    dead = 1;
                }
            }
        }

        draw();
    }
    return 0;
}
