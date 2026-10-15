#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stddef.h>

typedef unsigned char  u8;
typedef signed char    s8;
typedef unsigned short u16;
typedef signed short   s16;
typedef unsigned int   u32;
typedef signed int     s32;

#define ARRAY_LEN(a)     ((int)(sizeof(a) / sizeof((a)[0])))
#define MIN(a, b)        ((a) < (b) ? (a) : (b))
#define MAX(a, b)        ((a) > (b) ? (a) : (b))
#define ABS(a)           ((a) < 0 ? -(a) : (a))
#define CLAMP(v, lo, hi) ((v) < (lo) ? (lo) : ((v) > (hi) ? (hi) : (v)))

#define ALIGN4 __attribute__((aligned(4)))

/* Section helpers. On the GBA, big buffers live in EWRAM (.sbss) and hot
 * code lives in IWRAM (.iwram). On the host test build they are no-ops. */
#ifdef HOST_TEST
#define IWRAM_CODE
#define EWRAM_BSS
#else
#define IWRAM_CODE __attribute__((section(".iwram"), long_call, noinline))
#define EWRAM_BSS  __attribute__((section(".sbss")))
#endif

#endif
