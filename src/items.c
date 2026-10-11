/* items.c - item table, equipment helpers, visible gear (palette) and shop stock.
   Everything about *what an item is* lives here; stats.c turns equipment into numbers. */
#include "game.h"
#include "assets.h"

const char *const slot_names[SLOT_COUNT] = { "Weapon", "Armor", "Charm", "Helm", "Boots" };

/* ------------------------------------------------------------------ item table */
#define BASE(nm, ds, ty, sl, cl, tr, ...) { .name = nm, .desc = ds, .type = ty, .slot = sl, .cls = cl, .tier = tr, __VA_ARGS__ }
#define USE(nm, ds, h, m, pr) BASE(nm, ds, IT_USE, 0, 0xFF, 0, .hp = h, .mp = m, .price = pr)
#define WPN(nm, ds, cl, tr, at, pr, ...) BASE(nm, ds, IT_EQUIP, SLOT_WEAPON, cl, tr, .atk = at, .price = pr, ##__VA_ARGS__)
#define GEAR(nm, ds, sl, tr, pr, ...) BASE(nm, ds, IT_EQUIP, sl, 0xFF, tr, .price = pr, ##__VA_ARGS__)

const ItemDef item_defs[ITEM_COUNT] = {
    [ITEM_NONE] = BASE("-", "", IT_USE, 0, 0xFF, 0),
    [ITEM_POTION] = USE("Potion", "Restores 40 HP.", 40, 0, 25),
    [ITEM_HIPOTION] = USE("Hi-Potion", "Restores 100 HP.", 100, 0, 70),
    [ITEM_ETHER] = USE("Ether", "Restores 20 MP.", 0, 20, 35),
    [ITEM_ELIXIR] = USE("Elixir", "Fully restores HP and MP.", 999, 999, 200),
    [ITEM_CRYPT_KEY] = BASE("Crypt Key", "Opens the iron door in the crypt.", IT_KEY, 0, 0xFF, 0),
    [ITEM_SKULL_KEY] = BASE("Skull Key", "Breaks the skull seal before the king.", IT_KEY, 0, 0xFF, 0),

    /* weapons: four tiers per class. Tier 0-2 ids are consecutive per class, tier 3 lives in the v1.2 block */
    [ITEM_SWORD1] = WPN("Wooden Sword", "A training blade.", CLS_KNIGHT, 0, 4, 30),
    [ITEM_SWORD2] = WPN("Iron Blade", "Sturdy and sharp.", CLS_KNIGHT, 1, 9, 160),
    [ITEM_SWORD3] = WPN("Emberbrand", "Glows with inner fire.", CLS_KNIGHT, 2, 16, 600),
    [ITEM_SWORD4] = WPN("Moonsteel Blade", "Forged from fallen starlight.", CLS_KNIGHT, 3, 26, 1100, .lck = 2, .hp = 20),
    [ITEM_STAFF1] = WPN("Oak Staff", "A simple focus.", CLS_MAGE, 0, 4, 30),
    [ITEM_STAFF2] = WPN("Crystal Rod", "Hums with magic.", CLS_MAGE, 1, 9, 160),
    [ITEM_STAFF3] = WPN("Arcane Scepter", "Crackling power.", CLS_MAGE, 2, 16, 600),
    [ITEM_STAFF4] = WPN("Starfall Staff", "Rains light on the foe.", CLS_MAGE, 3, 26, 1100, .mag = 3, .mp = 12),
    [ITEM_BOW1] = WPN("Short Bow", "Light and quick.", CLS_RANGER, 0, 4, 30),
    [ITEM_BOW2] = WPN("Long Bow", "Longer reach, harder hits.", CLS_RANGER, 1, 9, 160),
    [ITEM_BOW3] = WPN("Elven Bow", "Sings when drawn.", CLS_RANGER, 2, 16, 600),
    [ITEM_BOW4] = WPN("Windpiercer", "Arrows outrun their sound.", CLS_RANGER, 3, 26, 1100, .spd = 3, .crit = 3),
    [ITEM_DAGGER1] = WPN("Iron Dagger", "Small but nasty.", CLS_ROGUE, 0, 4, 30),
    [ITEM_DAGGER2] = WPN("Steel Dirk", "Balanced for speed.", CLS_ROGUE, 1, 9, 160),
    [ITEM_DAGGER3] = WPN("Shadow Fang", "Bites from the dark.", CLS_ROGUE, 2, 16, 600),
    [ITEM_DAGGER4] = WPN("Viper's Kiss", "One scratch is enough.", CLS_ROGUE, 3, 26, 1100, .crit = 6, .lck = 2),

    /* body armor */
    [ITEM_ARMOR1] = GEAR("Cloth Tunic", "Plain traveling clothes.", SLOT_ARMOR, 0, 20, .def = 2),
    [ITEM_ARMOR2] = GEAR("Leather Armor", "Boiled leather plates.", SLOT_ARMOR, 1, 120, .def = 5),
    [ITEM_ARMOR3] = GEAR("Chain Mail", "Thousands of tiny rings.", SLOT_ARMOR, 2, 400, .def = 9),
    [ITEM_ARMOR4] = GEAR("Plate Armor", "Heavy, polished, hard to hurt.", SLOT_ARMOR, 3, 800, .def = 15, .hp = 15),

    /* helms */
    [ITEM_HELM1] = GEAR("Leather Cap", "Better than nothing.", SLOT_HELM, 0, 50, .def = 1, .hp = 5),
    [ITEM_HELM2] = GEAR("Iron Helm", "Dented but dependable.", SLOT_HELM, 1, 180, .def = 3, .hp = 10),
    [ITEM_HELM3] = GEAR("Knight's Helm", "Worn by the old guard.", SLOT_HELM, 2, 450, .def = 5, .hp = 15),
    [ITEM_HELM4] = GEAR("Hollow Crown", "Whispers of a lost king.", SLOT_HELM, 3, 900, .def = 8, .hp = 25, .mp = 6),

    /* boots */
    [ITEM_BOOTS1] = GEAR("Leather Boots", "Soft soles, quiet steps.", SLOT_BOOTS, 0, 50, .def = 1, .spd = 1),
    [ITEM_BOOTS2] = GEAR("Iron Greaves", "Clank, clank, clank.", SLOT_BOOTS, 1, 180, .def = 3, .spd = 1),
    [ITEM_BOOTS3] = GEAR("Swift Boots", "Light as a feather.", SLOT_BOOTS, 2, 450, .def = 2, .spd = 3),
    [ITEM_BOOTS4] = GEAR("Windrunners", "The road bends to you.", SLOT_BOOTS, 3, 900, .def = 4, .spd = 5),

    /* charms */
    [ITEM_CHARM_VIGOR] = GEAR("Vigor Charm", "Hearty and hale.", SLOT_CHARM, 0, 150, .hp = 20),
    [ITEM_CHARM_MIGHT] = GEAR("Might Ring", "A warrior's ring.", SLOT_CHARM, 0, 180, .atk = 3),
    [ITEM_CHARM_MANA] = GEAR("Mana Pendant", "A drop of starlight.", SLOT_CHARM, 0, 150, .mp = 10),
    [ITEM_CHARM_SWIFT] = GEAR("Swift Charm", "Quickens hands and feet.", SLOT_CHARM, 0, 120, .spd = 2),
    [ITEM_CHARM_LUCK] = GEAR("Lucky Clover", "Four leaves, four wishes.", SLOT_CHARM, 0, 120, .lck = 5),
    [ITEM_CHARM_FOCUS] = GEAR("Focus Charm", "Sharpens the mind.", SLOT_CHARM, 0, 160, .mag = 4),
    [ITEM_CHARM_CRIT] = GEAR("Hawk Eye", "Finds the weak spot.", SLOT_CHARM, 0, 200, .crit = 6),
    [ITEM_CHARM_GUARD] = GEAR("Guard Charm", "Turns aside the blow.", SLOT_CHARM, 0, 160, .def = 4),
};

/* ------------------------------------------------------------------ helpers */
/* weapon of a class for a tier index 0..WEAPON_TIERS-1 */
int item_weapon(int cls, int tier) {
    if (tier < 0) tier = 0;
    if (tier >= WEAPON_TIERS) tier = WEAPON_TIERS - 1;
    if (tier < 3) return ITEM_SWORD1 + cls * 3 + tier;
    return ITEM_SWORD4 + cls;
}

/* chests store either an item id or a CH_WEAPONn placeholder */
int item_chest_resolve(int item, int cls) {
    if (item >= CH_WEAPON2) return item_weapon(cls, item - CH_WEAPON2 + 1);
    return item;
}

/* does `item` beat what is currently in that slot? (used for auto-equip on pickup / purchase) */
int item_is_better(int item, int equipped) {
    const ItemDef *it = &item_defs[item];
    if (equipped == ITEM_NONE) return 1;
    const ItemDef *cur = &item_defs[equipped];
    switch (it->slot) {
    case SLOT_WEAPON: return it->atk > cur->atk;
    case SLOT_ARMOR:
    case SLOT_HELM: return it->def > cur->def;
    case SLOT_BOOTS: return it->def + it->spd * 2 > cur->def + cur->spd * 2;
    default: return 0;                  /* charms are a matter of taste: only fill an empty slot */
    }
}

/* compact stat summary such as "ATK+9 LCK+2" (only non-zero entries). Returns the string length. */
int item_stat_line(char *buf, int item) {
    const ItemDef *it = &item_defs[item];
    static const char *const lab[8] = { "ATK", "DEF", "DEX", "LUK", "INT", "CRIT", "HP", "MP" };
    int v[8] = { it->atk, it->def, it->spd, it->lck, it->mag, it->crit, it->hp, it->mp };
    int n = 0;
    for (int i = 0; i < 8; i++) {
        if (!v[i] || n > 36) continue;                  /* callers pass 48+ byte buffers */
        if (n) buf[n++] = ' ';
        for (const char *s = lab[i]; *s; s++) buf[n++] = *s;
        buf[n++] = v[i] < 0 ? '-' : '+';
        int a = v[i] < 0 ? -v[i] : v[i];
        if (a >= 100) buf[n++] = (char)('0' + a / 100);
        if (a >= 10) buf[n++] = (char)('0' + (a / 10) % 10);
        buf[n++] = (char)('0' + a % 10);
    }
    buf[n] = 0;
    return n;
}

/* ------------------------------------------------------------------ shop */
/* entries >= 0x80 stand for "my class's weapon of tier (id - 0x80)" */
#define SHOP_WPN(t) (0x80 + (t))
typedef struct { u8 item, min_level; } ShopEntry;
static const ShopEntry shop_table[] = {
    { ITEM_POTION, 1 }, { ITEM_HIPOTION, 1 }, { ITEM_ETHER, 1 },
    { SHOP_WPN(1), 1 }, { SHOP_WPN(2), 6 }, { SHOP_WPN(3), 12 },
    { ITEM_ARMOR2, 1 }, { ITEM_ARMOR3, 5 }, { ITEM_ARMOR4, 11 },
    { ITEM_HELM1, 1 }, { ITEM_HELM2, 4 }, { ITEM_HELM3, 9 }, { ITEM_HELM4, 13 },
    { ITEM_BOOTS1, 1 }, { ITEM_BOOTS2, 4 }, { ITEM_BOOTS3, 9 }, { ITEM_BOOTS4, 13 },
    { ITEM_CHARM_VIGOR, 1 }, { ITEM_CHARM_SWIFT, 1 }, { ITEM_CHARM_LUCK, 1 },
    { ITEM_CHARM_MIGHT, 3 }, { ITEM_CHARM_MANA, 3 }, { ITEM_CHARM_GUARD, 5 },
    { ITEM_CHARM_FOCUS, 5 }, { ITEM_CHARM_CRIT, 7 },
};

/* fills `out` with the items Tobin currently offers (stock grows with the hero's level) */
int shop_build_stock(int *out, int max) {
    int n = 0;
    for (unsigned i = 0; i < sizeof(shop_table) / sizeof(shop_table[0]) && n < max; i++) {
        if (pl.level < shop_table[i].min_level) continue;
        int it = shop_table[i].item;
        if (it >= 0x80) it = item_weapon(pl.cls, it - 0x80);
        out[n++] = it;
    }
    return n;
}

/* ------------------------------------------------------------------ visible gear */
#define C15(r, g, b) ((u16)((((r) >> 3) & 31) | ((((g) >> 3) & 31) << 5) | ((((b) >> 3) & 31) << 10)))

static u16 mix15(u16 a, u16 b, int k16) {
    int r = (a & 31) + ((((b & 31) - (a & 31)) * k16) >> 4);
    int g = ((a >> 5) & 31) + (((((b >> 5) & 31) - ((a >> 5) & 31)) * k16) >> 4);
    int bl = ((a >> 10) & 31) + (((((b >> 10) & 31) - ((a >> 10) & 31)) * k16) >> 4);
    return (u16)(r | (g << 5) | (bl << 10));
}

/* body armor recolours the hero's cloth (palette 6-8) and trim (9-10) toward the armor's metal/leather */
typedef struct { u16 main, light, dark, trim, trim_light; u8 mix, trim_mix; } ArmorLook;
static const ArmorLook armor_look[4] = {
    { 0, 0, 0, 0, 0, 0, 0 },
    { C15(0x9a, 0x6a, 0x38), C15(0xc4, 0x8c, 0x54), C15(0x6a, 0x40, 0x20), 0, 0, 11, 0 },
    { C15(0x8c, 0x94, 0xa8), C15(0xc8, 0xd0, 0xe0), C15(0x5a, 0x62, 0x78), C15(0xb8, 0xb8, 0xc8), C15(0xe8, 0xe8, 0xf4), 13, 10 },
    { C15(0xc0, 0xc8, 0xd8), C15(0xf4, 0xf8, 0xff), C15(0x78, 0x80, 0xa0), C15(0xf0, 0xc8, 0x40), C15(0xff, 0xf0, 0x80), 14, 14 },
};
static const u16 boots_look[4] = { 0, C15(0x5a, 0x38, 0x1c), C15(0x58, 0x5c, 0x70), C15(0x98, 0x70, 0x28) };

void gfx_set_weapon_pal(int tier) {
    tier = iclamp(tier, 0, WEAPON_TIERS - 1);
    copy16(PAL_OBJ + PB_WEAPON * 16, weapon_tier_pal[tier], 16);
}

/* re-derive the hero palette + weapon palette from the current equipment */
void gear_apply_look(void) {
    u16 p[16];
    for (int i = 0; i < 16; i++) p[i] = hero_pal[pl.cls][i];
    const ArmorLook *a = &armor_look[item_defs[pl.equip[SLOT_ARMOR]].tier & 3];
    if (a->mix) {
        p[6] = mix15(p[6], a->main, a->mix);
        p[7] = mix15(p[7], a->light, a->mix);
        p[8] = mix15(p[8], a->dark, a->mix);
        if (a->trim_mix) {
            p[9] = mix15(p[9], a->trim, a->trim_mix);
            p[10] = mix15(p[10], a->trim_light, a->trim_mix);
        }
    }
    if (pl.equip[SLOT_BOOTS]) {
        int bt = item_defs[pl.equip[SLOT_BOOTS]].tier & 3;
        if (boots_look[bt]) p[12] = mix15(p[12], boots_look[bt], 12);
    }
    copy16(PAL_OBJ + PB_HERO * 16, p, 16);
    gfx_set_weapon_pal(item_defs[pl.equip[SLOT_WEAPON]].tier);
}
