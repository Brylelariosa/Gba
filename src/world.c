#include "game.h"
#include "assets.h"

static void apply_area_look(int area);
static void npc_assign_palettes(void);

/* Monster and boss sprites share VRAM slots between areas: the slime, bat, wolf and skeleton sheets are one contiguous
   block, and the Warden's boss sheet is shared with the Tyrant. The Cinder Hollows overwrite them with their own art. */
static void load_area_sheets(int area) {
    int ember = area == AREA_EMBER;
    copy32(OBJ_VRAM32 + OT_SLIME * 8, ember ? ember_mon_tiles : obj_common_tiles + (OT_SLIME - OT_COMMON) * 8, EMBER_MON_TILES * 8);
    copy32(OBJ_VRAM32 + OT_WARDEN * 8, ember ? tyrant_tiles : obj_common_tiles + (OT_WARDEN - OT_COMMON) * 8, TYRANT_TILES * 8);
}

#define SB_GROUND 16
#define SB_OVER 20

const AreaDef *cur_area;
const Tileset *cur_ts;
int cur_area_id;
u8 map_g[1024], map_o[1024];
Npc npcs[MAX_NPC];
u32 gflags, gchests, gchests2;

/* chest ids run 0..63: two 32-bit words */
int chest_taken(int id) { return ((id < 32 ? gchests : gchests2) >> (id & 31)) & 1; }
void chest_take(int id) { if (id < 32) gchests |= 1u << id; else gchests2 |= 1u << (id & 31); }

static u16 pal_orig[192];

static volatile u16 *map_entry(int sb, int tx, int ty) {
    int blk = sb + (tx >> 5) + ((ty >> 5) << 1);
    return VRAM16 + (blk * 0x800) / 2 + (ty & 31) * 32 + (tx & 31);
}

void world_set_meta(int layer, int tx, int ty, int id) {
    if (tx < 0 || ty < 0 || tx >= 32 || ty >= 32) return;
    if (layer == 0) map_g[ty * 32 + tx] = (u8)id;
    else map_o[ty * 32 + tx] = (u8)id;
    int sb = layer == 0 ? SB_GROUND : SB_OVER;
    const u16 *m = &cur_ts->meta[id * 4];
    u16 e0 = m[0], e1 = m[1], e2 = m[2], e3 = m[3];
    if (layer == 1 && id == 0) e0 = e1 = e2 = e3 = 0;   /* overlay id 0 = nothing */
    volatile u16 *p = map_entry(sb, tx * 2, ty * 2);
    p[0] = e0;
    p[1] = e1;
    p = map_entry(sb, tx * 2, ty * 2 + 1);
    p[0] = e2;
    p[1] = e3;
}

int tile_flags(int tx, int ty) {
    if (tx < 0 || ty < 0 || tx >= 32 || ty >= 32) return TF_SOLID;
    return cur_ts->flags[map_g[ty * 32 + tx]];
}

int pt_flags(int px, int py) { return tile_flags(px >> 4, py >> 4); }

int box_free(int x, int y, int hw, int hh) {
    int x0 = x - hw, x1 = x + hw - 1, y0 = y - hh, y1 = y - 1;
    if (pt_flags(x0, y0) & TF_SOLID) return 0;
    if (pt_flags(x1, y0) & TF_SOLID) return 0;
    if (pt_flags(x0, y1) & TF_SOLID) return 0;
    if (pt_flags(x1, y1) & TF_SOLID) return 0;
    return 1;
}

int proj_blocked(int px, int py) {
    int f = pt_flags(px, py);
    return (f & TF_SOLID) && !(f & TF_WATER);
}

void world_load(int area, int tx, int ty, int dir) {
    cur_area_id = area;
    cur_area = &areas[area];
    cur_ts = &tilesets[cur_area->tileset];
    gfx_blank(1);
    copy32(VRAM32, cur_ts->tiles, cur_ts->ntiles * 8);
    for (int i = 0; i < 192; i++) pal_orig[i] = cur_ts->pal[i];
    copy16(PAL_BG, cur_ts->pal, 192);
    apply_area_look(area);
    load_area_sheets(area);
    gfx_set_backdrop(cur_area->tileset == 1 || cur_area->tileset == 2 ? 0 : cur_ts->pal[1]);
    for (int i = 0; i < 1024; i++) {
        map_g[i] = cur_area->ground[i];
        map_o[i] = cur_area->overlay[i];
    }
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++) {
            world_set_meta(0, x, y, map_g[y * 32 + x]);
            world_set_meta(1, x, y, map_o[y * 32 + x]);
        }
    /* persistent state */
    for (int i = 0; i < cur_area->nchests; i++) {
        const ChestDef *c = &cur_area->chests[i];
        if (chest_taken(c->id)) world_set_meta(0, c->tx, c->ty, cur_ts->chest_open);
    }
    if (cur_area_id == AREA_ROAD) gflags |= GF_SEEN_ROAD;
    if (cur_area_id == AREA_AMBERFALL) gflags |= GF_SEEN_AMBERFALL;
    if ((gflags & GF_DOOR_OPEN) && cur_area_id == 2) world_unlock_door(TF_DOOR);
    if ((gflags & GF_SKULL_OPEN) && cur_area_id == 2) world_unlock_door(TF_DOOR2);
    if (cur_area_id == 2) gflags |= GF_SEEN_CRYPT;
    if (cur_area_id == 4) gflags |= GF_SEEN_EMBER;
    if ((gflags & GF_BOSS_DEAD) && cur_area_id == 2) {          /* the stair behind the throne opens once the king falls */
        world_set_meta(0, 15, 1, META_D_STAIRS_L);
        world_set_meta(0, 16, 1, META_D_STAIRS_R);
    }
    /* entities */
    enemies_clear();
    fx_clear();
    for (int i = 0; i < MAX_NPC; i++) npcs[i].active = 0;
    for (int i = 0; i < cur_area->nnpcs && i < MAX_NPC; i++) {
        const NpcDef *n = &cur_area->npcs[i];
        npcs[i].id = n->id;
        npcs[i].active = 1;
        npcs[i].dir = n->dir;
        npcs[i].x = (s16)(n->tx * 16 + 8);
        npcs[i].y = (s16)(n->ty * 16 + 15);
        npcs[i].anim = (u8)(i * 17);
        npcs[i].talk_t = 0;
    }
    npc_assign_palettes();
    for (int i = 0; i < cur_area->nspawns; i++) {
        const SpawnDef *s = &cur_area->spawns[i];
        if (s->type == EN_BOSS && (gflags & GF_BOSS_DEAD)) continue;
        if (s->type == EN_WARDEN && (gflags & GF_WARDEN_DEAD)) continue;
        if (s->type == EN_TYRANT && (gflags & GF_TYRANT_DEAD)) continue;
        enemy_spawn(s->type, s->tx, s->ty);
    }
    player_place(tx, ty, dir);
    cam_snap((int)(pl.x >> 8), (int)(pl.y >> 8));
    gfx_blank(0);
}

/* opens every door tile carrying `door_flag` (TF_DOOR = iron door, TF_DOOR2 = skull seal) */
void world_unlock_door(int door_flag) {
    const Tileset *ts = cur_ts;
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++)
            if (ts->flags[map_g[y * 32 + x]] & door_flag) world_set_meta(0, x, y, door_flag == TF_DOOR2 ? META_D_RUNE : META_D_FLOOR);
}

/* ---- area atmosphere: palette tint for the woods, darker/redder undead palette for the depths */
static u16 mix_rgb(u16 c, int r, int g, int b, int k16) {
    int cr = c & 31, cg = (c >> 5) & 31, cb = (c >> 10) & 31;
    cr = cr + (((r >> 3) - cr) * k16 >> 4);
    cg = cg + (((g >> 3) - cg) * k16 >> 4);
    cb = cb + (((b >> 3) - cb) * k16 >> 4);
    return (u16)(iclamp(cr, 0, 31) | (iclamp(cg, 0, 31) << 5) | (iclamp(cb, 0, 31) << 10));
}

static void apply_area_look(int area) {
    if (area == 1) {
        /* the woods: slightly cooler and darker than the sunny village */
        for (int i = 0; i < 192; i++) {
            u16 c = pal_orig[i];
            int r = (c & 31) * 9 / 10, g = ((c >> 5) & 31) * 15 / 16, b = ((c >> 10) & 31);
            pal_orig[i] = (u16)(r | (g << 5) | (iclamp(b + 1, 0, 31) << 10));
        }
        copy16(PAL_BG, pal_orig, 192);
    }
    static const u8 tinted[6] = { PB_SLIME, PB_SLIME2, PB_BAT, PB_WOLF, PB_SKEL, PB_BOSS };
    for (int k = 0; k < 6; k++) copy16(PAL_OBJ + tinted[k] * 16, obj_pal_init + tinted[k] * 16, 16);
    if (area == AREA_EMBER) {
        /* the Cinder Hollows have their own creatures now (fire imp, ember wisp, cinder boar, slag golem, and the
           Cinder Tyrant), drawn with their own palettes instead of tinted forest monsters */
        static const u8 ember_banks[5] = { PB_SLIME2, PB_BAT, PB_SKEL, PB_WOLF, PB_BOSS };
        for (int k = 0; k < 5; k++) copy16(PAL_OBJ + ember_banks[k] * 16, ember_pal[k], 16);
    }
    if (area == 3) {
        /* the warden's armour is crimson steel, not the king's purple */
        u16 p[16];
        for (int i = 0; i < 16; i++) p[i] = obj_pal_init[PB_BOSS * 16 + i];
        p[4] = mix_rgb(p[4], 0xa0, 0x28, 0x30, 15);
        p[5] = mix_rgb(p[5], 0xd8, 0x50, 0x58, 15);
        p[6] = mix_rgb(p[6], 0x58, 0x14, 0x20, 15);
        p[15] = mix_rgb(p[15], 0x30, 0x0c, 0x18, 15);
        copy16(PAL_OBJ + PB_BOSS * 16, p, 16);
    }
    if (area == 3) {
        /* the depths: bone turns ashen blue-grey, eyes glow red */
        u16 p[16];
        for (int i = 0; i < 16; i++) p[i] = obj_pal_init[PB_SKEL * 16 + i];
        for (int i = 1; i < 16; i++) {
            if (i == 12 || i == 13) { p[i] = mix_rgb(p[i], 0xff, 0x40, 0x40, 14); continue; }
            p[i] = mix_rgb(p[i], 0x70, 0x78, 0xa8, 9);
        }
        copy16(PAL_OBJ + PB_SKEL * 16, p, 16);
    }
}

int world_open_chest(int tx, int ty) {
    for (int i = 0; i < cur_area->nchests; i++) {
        const ChestDef *c = &cur_area->chests[i];
        if (c->tx == tx && c->ty == ty) {
            if (chest_taken(c->id)) return -1;
            chest_take(c->id);
            world_set_meta(0, tx, ty, cur_ts->chest_open);
            return i;
        }
    }
    return -1;
}

const char *world_sign_at(int tx, int ty) {
    for (int i = 0; i < cur_area->nsigns; i++)
        if (cur_area->signs[i].tx == tx && cur_area->signs[i].ty == ty) return cur_area->signs[i].text;
    return 0;
}

int world_check_warp(int px, int py) {
    int tx = px >> 4, ty = py >> 4;
    for (int i = 0; i < cur_area->nwarps; i++) {
        const WarpDef *w = &cur_area->warps[i];
        if (w->area == 4 && !(gflags & GF_BOSS_DEAD)) continue;      /* the Cinder Stair only exists once the king is dead */
        if (tx >= w->x0 && tx <= w->x1 && ty >= w->y0 && ty <= w->y1) return i;
    }
    return -1;
}

void world_anim(void) {
    for (int i = 0; i < cur_ts->ncycles; i++) {
        int bank = cur_ts->cycles[i * 4], start = cur_ts->cycles[i * 4 + 1];
        int n = cur_ts->cycles[i * 4 + 2], per = cur_ts->cycles[i * 4 + 3];
        int off = (int)((frame_count / (u32)per) % (u32)n);
        for (int k = 0; k < n; k++) PAL_BG[bank * 16 + start + k] = pal_orig[bank * 16 + start + (k + off) % n];
    }
}

/* ---------------------------------------------------------------- NPCs */
void world_update_npcs(void) {
    for (int i = 0; i < MAX_NPC; i++) {
        if (!npcs[i].active) continue;
        npcs[i].anim++;
        if (npcs[i].talk_t) npcs[i].talk_t--;
    }
}

Npc *world_npc_near(int px, int py, int r) {
    Npc *best = 0;
    int bd = r * 2;
    for (int i = 0; i < MAX_NPC; i++) {
        if (!npcs[i].active) continue;
        int d = idist(npcs[i].x - px, npcs[i].y - py);
        if (d < bd) { bd = d; best = &npcs[i]; }
    }
    return best;
}

/* Every NPC type has its own 15-colour palette. Villages and towns have no monsters, so they may borrow the monster
   palette banks (2-7) as well as the three NPC banks; areas with monsters make do with the three. */
static void npc_assign_palettes(void) {
    static const u8 banks[9] = { 8, 9, 10, 2, 3, 4, 5, 6, 7 };
    int nbanks = cur_area->nspawns ? 3 : 9, used = 0;
    u8 bank_of[NPC_COUNT];
    for (int i = 0; i < NPC_COUNT; i++) bank_of[i] = 0xFF;
    for (int i = 0; i < MAX_NPC; i++) {
        if (!npcs[i].active) continue;
        int id = npcs[i].id;
        if (id >= NPC_COUNT) id = 0;
        if (bank_of[id] == 0xFF) {
            bank_of[id] = banks[used % nbanks];
            used++;
            copy16(PAL_OBJ + bank_of[id] * 16, npc_pal[id], 16);
        }
        npcs[i].pal = bank_of[id];
    }
}

void world_draw_npcs(void) {
    for (int i = 0; i < MAX_NPC; i++) {
        if (!npcs[i].active) continue;
        Npc *n = &npcs[i];
        int sx = n->x - cam_x, sy = n->y - cam_y;
        if (sx < -24 || sx > 264 || sy < -40 || sy > 200) continue;
        int bob = ((n->anim >> 4) & 1) || n->talk_t ? (((n->anim >> 2) & 1) && n->talk_t ? 1 : 0) : 0;
        if (!n->talk_t) bob = (n->anim >> 5) & 1;
        int tile = OT_NPC + (n->id * 2 + bob) * 4;
        spr16(sx - 8, sy - 15, tile, n->pal, 0, n->y);
        spr_add(sx - 8, sy - 5, SH_WIDE, 0, OT_SHADOW, PB_FX, 2, SF_ALPHA, -2000 + n->y);
        int qm = quest_marker(n->id);                      /* ! = has a quest for you, ? = hand it in, ... = in progress */
        if (qm >= 0) {
            static const s8 hop[8] = { 0, -1, -2, -2, -1, 0, 0, 0 };
            spr_add(sx - 4, sy - 33 + (qm == QM_BUSY ? 0 : hop[(frame_count >> 2) & 7]), SH_TALL, 0, OT_QMARK + qm * 2, PB_ICON, 2, 0, 20000);
        }
    }
}
