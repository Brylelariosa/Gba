#ifndef RNG_H
#define RNG_H

#include "common.h"

typedef struct { u32 s; } Rng;

static inline u32 rng_next(Rng *r)
{
    u32 x = r->s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    r->s = x;
    return x;
}

static inline int rng_range(Rng *r, int n)
{
    return (int)((rng_next(r) >> 4) % (u32)n);
}

extern Rng g_rng; /* gameplay RNG (mobs, drops) */
void rng_seed(Rng *r, u32 seed);

#endif
