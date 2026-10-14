#include "rng.h"

Rng g_rng = { 0x1234567u };

void rng_seed(Rng *r, u32 seed)
{
    r->s = seed * 2654435761u + 0x9E3779B9u;
    if (r->s == 0) r->s = 1;
    rng_next(r);
    rng_next(r);
}
