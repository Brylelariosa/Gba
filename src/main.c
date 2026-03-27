#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include "td.h"

int main(void) {
    irqInit();
    irqEnable(IRQ_VBLANK);

    /* Mode 3: direct 15-bit 240×160 framebuffer, BG2 enabled */
    SetMode(MODE_3 | BG2_ON);

    gameInit();

    while (1) {
        VBlankIntrWait();
        scanKeys();
        gameUpdate();
        gameDraw();
    }

    return 0;
}
