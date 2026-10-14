#include "icons.h"
#include "items.h"
#include "palette.h"

static const char *const ART_STICK[8] = {
    "......S.", ".....S..", "....S...", "...S....", "..S.....", ".S......", "S.......", "........",
};
static const char *const ART_PICK[8] = {
    ".HHHHH..", "H....hH.", "....S..h", "...S....", "..S.....", ".S......", "S.......", "........",
};
static const char *const ART_AXE[8] = {
    "..HHH...", ".HHHHh..", ".HHHSh..", "..hhS...", "...S....", "..S.....", ".S......", "S.......",
};
static const char *const ART_SWORD[8] = {
    "......hH", ".....hH.", "....hH..", "...hH...", "..KKK...", "..SK....", ".S......", "S.......",
};
static const char *const ART_COAL[8] = {
    "..hhh...", ".hHHHh..", "hHWHHHh.", "hHHHHHh.", ".hHHHh..", "..hhh...", "........", "........",
};
static const char *const ART_INGOT[8] = {
    "........", "..hHHH..", ".hHWHHh.", "hHHHHHHh", ".hhHHHh.", "..hhhh..", "........", "........",
};
static const char *const ART_GEM[8] = {
    "..hHHh..", ".hHWHHh.", "hHHHHHHh", ".hHHHHh.", "..hHHh..", "...hh...", "........", "........",
};
static const char *const ART_APPLE[8] = {
    "....G...", "..hHHh..", ".hHWHHh.", ".hHHHHh.", ".hHHHHh.", "..hHHh..", "........", "........",
};
static const char *const ART_PORK[8] = {
    "..hhhh..", ".hHHHHh.", "hHWHHHHh", "hHHHHHHh", ".hhHHhh.", "...BB...", "...BB...", "........",
};

#define TIER(m) PAL_UI((m), 4), PAL_UI((m), 2), PAL_UI((m), 6)

static const IconDef ICON_STICK        = { ART_STICK, 0, 0, 0 };
static const IconDef ICON_PICK_WOOD    = { ART_PICK,  TIER(M_PLANK) };
static const IconDef ICON_PICK_STONE   = { ART_PICK,  TIER(M_STONE) };
static const IconDef ICON_PICK_IRON    = { ART_PICK,  TIER(M_IRON) };
static const IconDef ICON_PICK_DIAMOND = { ART_PICK,  TIER(M_DIAMOND) };
static const IconDef ICON_AXE_WOOD     = { ART_AXE,   TIER(M_PLANK) };
static const IconDef ICON_AXE_STONE    = { ART_AXE,   TIER(M_STONE) };
static const IconDef ICON_AXE_IRON     = { ART_AXE,   TIER(M_IRON) };
static const IconDef ICON_AXE_DIAMOND  = { ART_AXE,   TIER(M_DIAMOND) };
static const IconDef ICON_SWORD_WOOD   = { ART_SWORD, TIER(M_PLANK) };
static const IconDef ICON_SWORD_STONE  = { ART_SWORD, TIER(M_STONE) };
static const IconDef ICON_SWORD_IRON   = { ART_SWORD, TIER(M_IRON) };
static const IconDef ICON_SWORD_DIAMOND = { ART_SWORD, TIER(M_DIAMOND) };
static const IconDef ICON_COAL         = { ART_COAL,  PAL_UI(M_COAL, 3), PAL_UI(M_COAL, 1), PAL_UI(M_COAL, 6) };
static const IconDef ICON_IRON_INGOT   = { ART_INGOT, TIER(M_IRON) };
static const IconDef ICON_GOLD_INGOT   = { ART_INGOT, TIER(M_GOLD) };
static const IconDef ICON_DIAMOND      = { ART_GEM,   TIER(M_DIAMOND) };
static const IconDef ICON_APPLE        = { ART_APPLE, C_RED, C_DRED, C_WHITE };
static const IconDef ICON_PORK_RAW     = { ART_PORK,  C_PINK, C_RED, C_WHITE };
static const IconDef ICON_PORK_COOKED  = { ART_PORK,  C_BROWN, PAL_UI(M_BARK, 1), C_ORANGE };

const IconDef *icon_for(u8 item)
{
    switch (item) {
    case I_STICK:         return &ICON_STICK;
    case I_COAL:          return &ICON_COAL;
    case I_IRON_INGOT:    return &ICON_IRON_INGOT;
    case I_GOLD_INGOT:    return &ICON_GOLD_INGOT;
    case I_DIAMOND:       return &ICON_DIAMOND;
    case I_APPLE:         return &ICON_APPLE;
    case I_PORK_RAW:      return &ICON_PORK_RAW;
    case I_PORK_COOKED:   return &ICON_PORK_COOKED;
    case I_PICK_WOOD:     return &ICON_PICK_WOOD;
    case I_PICK_STONE:    return &ICON_PICK_STONE;
    case I_PICK_IRON:     return &ICON_PICK_IRON;
    case I_PICK_DIAMOND:  return &ICON_PICK_DIAMOND;
    case I_AXE_WOOD:      return &ICON_AXE_WOOD;
    case I_AXE_STONE:     return &ICON_AXE_STONE;
    case I_AXE_IRON:      return &ICON_AXE_IRON;
    case I_AXE_DIAMOND:   return &ICON_AXE_DIAMOND;
    case I_SWORD_WOOD:    return &ICON_SWORD_WOOD;
    case I_SWORD_STONE:   return &ICON_SWORD_STONE;
    case I_SWORD_IRON:    return &ICON_SWORD_IRON;
    case I_SWORD_DIAMOND: return &ICON_SWORD_DIAMOND;
    default:              return NULL;
    }
}

u8 icon_color(const IconDef *ic, char ch)
{
    switch (ch) {
    case 'H': return ic->main;
    case 'h': return ic->dark;
    case 'W': return ic->light;
    case 'S': return PAL_UI(M_BARK, 5);
    case 'K': return C_BLACK;
    case 'G': return C_GREEN;
    case 'B': return C_WHITE;
    default:  return 0;
    }
}
