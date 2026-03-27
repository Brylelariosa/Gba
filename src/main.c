#include <gba_video.h>
#include <gba_interrupt.h>
#include <gba_systemcalls.h>
#include <gba_input.h>
#include <gba_dma.h>
#include "td.h"

int main(void) {
    irqInit();
    irqEnable(IRQ_VBLANK);

    /* Mode 3: 15-bit 240×160 direct-colour, BG2 enabled */
    SetMode(MODE_3 | BG2_ON);

    gameInit();

    while (1) {
        /*
         * 1. Wait for VBlank (display is in the blank period – safe to update).
         * 2. Run logic FIRST so it overlaps with the VBlank window.
         * 3. Draw everything into the EWRAM back-buffer (BUF), NOT into VRAM.
         * 4. DMA the completed buffer to VRAM in one atomic 32-bit burst.
         *    The GBA DMA is fast enough to finish well inside active display,
         *    and because we wait until after VBlank starts the copy always
         *    begins from a clean scanline boundary – no tearing, no flicker.
         */
        VBlankIntrWait();

        scanKeys();
        gameUpdate();
        gameDraw();

        /* 76 800 bytes = 19 200 32-bit words.
           DMA3 at 32-bit width copies the whole screen in ~19 200 cycles,
           comfortably before the first visible scanline. */
        dmaCopy(BUF, (void*)0x06000000, SCREEN_PIXELS * 2);
    }

    return 0;
}
