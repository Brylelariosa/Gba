#include "themes.h"

int g_theme = 0;

/* Each theme: 32 colours for indices 0-31 */
static const u16 THEMES[THEME_COUNT][32] = {

    /* 0 – Classic Green */
    {
        RGB5( 0, 0, 0), RGB5( 0,28, 0), RGB5( 2, 4, 2), RGB5( 0,15, 0),
        /* cyclic 0-15 */
        RGB5(0,0,0),RGB5(0,8,0),RGB5(0,14,0),RGB5(0,20,0),
        RGB5(0,26,0),RGB5(4,28,0),RGB5(10,28,0),RGB5(16,26,0),
        RGB5(20,22,0),RGB5(24,18,0),RGB5(28,12,0),RGB5(28,6,0),
        RGB5(26,0,0),RGB5(20,0,4),RGB5(14,0,10),RGB5(8,0,16),
        /* UI */
        RGB5( 2, 4, 2),RGB5( 0,26, 0),RGB5( 0,31, 8),
        RGB5( 1, 3, 1),RGB5( 0,22, 0),RGB5( 0,20, 0),
        RGB5(31,31, 0),RGB5(31,31,31),RGB5(31, 0, 0),
        RGB5( 0,28,28),RGB5(31,28, 0),
    },

    /* 1 – Monochrome */
    {
        RGB5( 0, 0, 0),RGB5(28,28,28),RGB5(10,10,10),RGB5(18,18,18),
        RGB5(0,0,0),RGB5(3,3,3),RGB5(6,6,6),RGB5(9,9,9),
        RGB5(12,12,12),RGB5(15,15,15),RGB5(18,18,18),RGB5(21,21,21),
        RGB5(24,24,24),RGB5(26,26,26),RGB5(28,28,28),RGB5(31,31,31),
        RGB5(0,0,0),RGB5(0,0,0),RGB5(0,0,0),RGB5(0,0,0),
        RGB5( 4, 4, 4),RGB5(28,28,28),