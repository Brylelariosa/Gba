#include "game.h"
#include "assets.h"

#define SB_GROUND 16
#define SB_OVER 20

const AreaDef *cur_area;
const Tileset *cur_ts;
int cur_area_id;
u8 map_g[1024], map_o[1024];
Npc npcs[MAX_NPC];
u32 gflags, gchests;

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
    gfx_set_backdrop(cur_area->tileset == 1 ? 0 : cur_ts->pal[1]);
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
        if (gchests & (1u << c->id)) world_set_meta(0, c->tx, c->ty, cur_ts->chest_open);
    }
    if ((gflags & GF_DOOR_OPEN) && cur_area_id == 2) world_unlock_door();
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
    for (int i = 0; i < cur_area->nspawns; i++) {
        const SpawnDef *s = &cur_area->spawns[i];
        if (s->type == EN_BOSS && (gflags & GF_BOSS_DEAD)) continue;
        enemy_spawn(s->type, s->tx, s->ty);
    }
    player_place(tx, ty, dir);
    cam_snap((int)(pl.x >> 8), (int)(pl.y >> 8));
    gfx_blank(0);
}

void world_unlock_door(void) {
    const Tileset *ts = cur_ts;
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++)
            if (ts->flags[map_g[y * 32 + x]] & TF_DOOR) world_set_meta(0, x, y, 0);
}

int world_open_chest(int tx, int ty) {
    for (int i = 0; i < cur_area->nchests; i++) {
        const ChestDef *c = &cur_area->chests[i];
        if (c->tx == tx && c->ty == ty) {
            if (gchests & (1u << c->id)) return -1;
            gchests |= (1u << c->id);
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

void world_draw_npcs(void) {
    for (int i = 0; i < MAX_NPC; i++) {
        if (!npcs[i].active) continue;
        Npc *n = &npcs[i];
        int sx = n->x - cam_x, sy = n->y - cam_y;
        int bob = ((n->anim >> 4) & 1) || n->talk_t ? (((n->anim >> 2) & 1) && n->talk_t ? 1 : 0) : 0;
        if (!n->talk_t) bob = (n->anim >> 5) & 1;
        int tile = OT_NPC + (n->id * 2 + bob) * 4;
        spr16(sx - 8, sy - 15, tile, PB_NPC0 + n->id, 0, n->y);
        spr_add(sx - 8, sy - 5, SH_WIDE, 0, OT_SHADOW, PB_FX, 2, SF_ALPHA, -2000 + n->y);
    }
}
