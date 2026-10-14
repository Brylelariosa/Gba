#include "game.h"

int main(void)
{
    game_init();
    for (;;) game_frame();
    return 0;
}
