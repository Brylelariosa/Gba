#include "patterns.h"
#include "simulation.h"

/* ── Raw pattern data (1=alive 0=dead) ───────────── */
static const u8 PAT_GLIDER[] = {
    0,1,0,
    0,0,1,
    1,1,1,
};
static const u8 PAT_LWSS[] = {
    1,0,0,1,0,
    0,0,0,0,1,
    1,0,0,0,1,
    0,1,1,1,1,
};
static const u8 PAT_MWSS[] = {
    0,0,1,0,0,0,
    1,0,0,0,1,0,
    0,0,0,0,0,1,
    1,0,0,0,0,1,
    0,1,1,1,1,1,
};
static const u8 PAT_HWSS[] = {
    0,0,1,1,0,0,0,
    1,0,0,0,0,1,0,
    0,0,0,0,0,0,1,
    1,0,0,0,0,0,1,
    0,1,1,1,1,1,1,
};
static const u8 PAT_BEACON[] = {
    1,1,0,0,
    1,0,0,0,
    0,0,0,1,
    0,0,1,1,
};
static const u8 PAT_PULSAR[] = {
    0,0,1,1,1,0,0,0,1,1,1,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,0,0,0,0,1,0,1,0,0,0,0,1,
    1,0,0,0,0,1,0,1,0,0,0,0,1,
    1,0,0,0,0,1,0,1,0,0,0,0,1,
    0,0,1,1,1,0,0,0,1,1,1,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,1,1,1,0,0,0,1,1,1,0,0,
    1,0,0,0,0,1,0,1,0,0,0,0,1,
    1,0,0,0,0,1,0,1,0,0,0,0,1,
    1,0,0,0,0,1,0,1,0,0,0,0,1,
    0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,1,1,1,0,0,0,1,1,1,0,0,
};
static const u8 PAT_TOAD[] = {
    0,1,1,1,
    1,1,1,0,
};
static const u8 PAT_ACORN[] = {
    0,1,0,0,0,0,0,
    0,0,0,1,0,0,0,
    1,1,0,0,1,1,1,
};
static const u8 PAT_DIEHARD[] = {
    0,0,0,0,0,0,1,0,
    1,1,0,0,0,0,0,0,
    0,1,0,0,0,1,1,1,
};
static const u8 PAT_RPENTA[] = {
    0,1,1,
    1,1,0,
    0,1,0,
};
static const u8 PAT_GOSPER[] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,
    0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,
    1,1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,0,0,0,0,0,0,0,0,1,0,0,0,1,0,1,1,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
};
static const u8 PAT_BB_OSC[] = {   /* Brian's Brain simple oscillator */
    0,2,0,
    2,0,2,
    0,2,0,
};
static const u8 PAT_WW_CLOCK[] = {  /* Wireworld clock segment */
    0,0,1,1,0,0,
    0,1,0,0,1,0,
    1,0,0,0,0,1,
    1,0,0,0,0,1,
    0,1,0,0,1,0,
    0,0,1,1,0,0,
};
static const u8 PAT_WW_GATE[] = { /* AND gate in wireworld */
    2,1,0,0,0,
    0,0,1,1,1,
    3,1,0,0,0,
};
static const u8 PAT_SIMKIN[] = {
    1,1,0,0,0,1,1,
    1,1,0,0,0,1,1,
    0,0,0,0,0,0,0,
    0,0,0,1,1,0,0,
    0,0,0,1,1,0,0,
    0,0,0,0,0,0,0,
    0,0,0,0,0,1,1,
    0,0,0,0,0,1,1,
};
static const u8 PAT_BLINKER[] = { 1,1,1 };
static const u8 PAT_BLOCK[] = {
    1,1,
    1,1,
};
static const u8 PAT_RANDOM[] = {  /* Used as "random 8×8" seed */
    1,0,1,0,1,0,1,0,
    0,1,0,1,0,1,0,1,
    1,0,0,1,1,0,0,1,
    0,1,1,0,0,1,1,0,
    1,1,0,0,1,1,0,0,
    0,0,1,1,0,0,1,1,
    1,0,1,1,0,1,0,0,
    0,1,0,0,1,0,1,1,
};

/* ── Pattern table ───────────────────────────────── */
const Pattern PATTERNS[PATTERN_COUNT] = {
    { "Glider",       PAT_GLIDER,   3,  3 },
    { "LWSS",         PAT_LWSS,     5,  4 },
    { "MWSS",         PAT_MWSS,     6,  5 },
    { "HWSS",         PAT_HWSS,     7,  5 },
    { "Beacon",       PAT_BEACON,   4,  4 },
    { "Pulsar",       PAT_PULSAR,  13, 13 },
    { "Toad",         PAT_TOAD,     4,  2 },
    { "Acorn",        PAT_ACORN,    7,  3 },
    { "Diehard",      PAT_DIEHARD,  8,  3 },
    { "R-Pentomino",  PAT_RPENTA,   3,  3 },
    { "Gosper Gun",   PAT_GOSPER,  36,  9 },
    { "Simkin Gun",   PAT_SIMKIN,   7,  8 },
    { "Blinker",      PAT_BLINKER,  3,  1 },
    { "Block",        PAT_BLOCK,    2,  2 },
    { "Random 8x8",   PAT_RANDOM,   8,  8 },
    { "BB Osc",       PAT_BB_OSC,   3,  3 },
    { "WW Clock",     PAT_WW_CLOCK, 6,  6 },
    { "WW AND Gate",  PAT_WW_GATE,  5,  3 },
};

void pattern_place(int idx, int cx, int cy) {
    if (idx < 0 || idx >= PATTERN_COUNT) return;
    const Pattern *p = &PATTERNS[idx];
    int ox = cx - p->w / 2;
    int oy = cy - p->h / 2;
    sim_place_pattern(p->data, p->w, p->h, ox, oy);
}