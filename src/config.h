#ifndef CONFIG_H
#define CONFIG_H

/* The game's name: title screen and goal messages. Capital letters, digits
 * and spaces only. The title logo shrinks by itself if the name is long. */
#define GAME_NAME "DUSKHAVEN"

/* Tunables - tweak the feel of the game here. */
#define STEP_FRAMES   10     /* frames the player takes to walk one block   */
#define MOB_STEP      14     /* frames a mob takes to walk one block        */
#define DAY_LEN       24000  /* frames in a full day/night cycle            */
#define MAX_HP        20
#define MAX_FOOD      20
#define START_TOD     2000

/* Building. */
#define TALL_HOLD     14     /* frames B is held before the 2nd block of a wall */
#define CLIMB_HOLD_BUILT   20 /* frames you must push into a block YOU placed
                               * before you climb onto it (no instant hop)     */
#define CLIMB_HOLD_NATURAL 0  /* same for hills and ledges that were generated  */

#endif
