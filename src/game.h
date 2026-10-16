#ifndef GAME_H
#define GAME_H
#include "hw.h"

#define DIR_DOWN 0
#define DIR_UP 1
#define DIR_LEFT 2
#define DIR_RIGHT 3

enum { EN_NONE, EN_SLIME, EN_BAT, EN_WOLF, EN_SKELETON, EN_BOSS, EN_WARDEN, EN_TYRANT, EN_COUNT };
enum { NPC_ELDER, NPC_MERCHANT, NPC_HEALER, NPC_RANGER, NPC_SMITH, NPC_GUARD, NPC_KID, NPC_FARMER,
       NPC_TRAVELER, NPC_INNKEEPER, NPC_ALCHEMIST, NPC_SCHOLAR, NPC_VENDOR, NPC_TOWNSMAN, NPC_TOWNSWOMAN, NPC_COUNT };
enum { AREA_VILLAGE, AREA_FOREST, AREA_CRYPT, AREA_DEPTHS, AREA_EMBER, AREA_ROAD, AREA_AMBERWOOD, AREA_AMBERFALL, AREA_COUNT };
enum { CLS_KNIGHT, CLS_MAGE, CLS_RANGER, CLS_ROGUE, CLS_COUNT };

enum {
    ITEM_NONE,
    ITEM_POTION, ITEM_HIPOTION, ITEM_ETHER, ITEM_ELIXIR, ITEM_CRYPT_KEY,
    ITEM_SWORD1, ITEM_SWORD2, ITEM_SWORD3,
    ITEM_STAFF1, ITEM_STAFF2, ITEM_STAFF3,
    ITEM_BOW1, ITEM_BOW2, ITEM_BOW3,
    ITEM_DAGGER1, ITEM_DAGGER2, ITEM_DAGGER3,
    ITEM_ARMOR1, ITEM_ARMOR2, ITEM_ARMOR3,
    ITEM_CHARM_VIGOR, ITEM_CHARM_MIGHT, ITEM_CHARM_MANA, ITEM_CHARM_SWIFT, ITEM_CHARM_LUCK,
    /* ---- added in v1.2 (appended so ids in older saves stay valid) ---- */
    ITEM_SWORD4, ITEM_STAFF4, ITEM_BOW4, ITEM_DAGGER4,
    ITEM_ARMOR4,
    ITEM_HELM1, ITEM_HELM2, ITEM_HELM3, ITEM_HELM4,
    ITEM_BOOTS1, ITEM_BOOTS2, ITEM_BOOTS3, ITEM_BOOTS4,
    ITEM_CHARM_FOCUS, ITEM_CHARM_CRIT, ITEM_CHARM_GUARD,
    ITEM_SKULL_KEY,
    /* ---- added in v1.3 ---- */
    ITEM_GEL, ITEM_WING, ITEM_PELT, ITEM_BONE, ITEM_EMBER,                /* trophies */
    ITEM_SWORD5, ITEM_STAFF5, ITEM_BOW5, ITEM_DAGGER5,                    /* cinder-forged weapons */
    ITEM_ARMOR5, ITEM_HELM5, ITEM_BOOTS5,
    ITEM_CHARM_PHOENIX, ITEM_CHARM_EMBER,
    /* ---- added in v1.4 (crafting update) ---- */
    ITEM_IRON, ITEM_AMBER, ITEM_CINDER_HEART,                             /* crafting materials */
    ITEM_ARMOR_PELT, ITEM_HELM_BONE, ITEM_BOOTS_AMBER, ITEM_CHARM_AMBER,  /* gear made at the forge */
    ITEM_COUNT
};
#define ITEM_COUNT_V4 (ITEM_CHARM_EMBER + 1)   /* save format 4 stored exactly the ids up to the ember heart */
enum { RAR_COMMON, RAR_UNCOMMON, RAR_RARE, RAR_EPIC, RAR_LEGENDARY, RAR_COUNT };
#define TEMPER_MAX 3                 /* the blacksmith can temper a piece of gear up to +3 */
#define ITEM_COUNT_V3 (ITEM_SKULL_KEY + 1)   /* save format 3 stored exactly the ids up to the skull key */
#define ITEM_COUNT_V2 26            /* inventory size of save format 2 */
/* map chest placeholders: resolved to the opener's own class weapon (tier index = value - CH_WEAPON2 + 1) */
#define CH_WEAPON2 200
#define CH_WEAPON3 201
#define CH_WEAPON4 202
#define CH_WEAPON5 203
#define WEAPON_TIERS 5

enum { IT_USE, IT_KEY, IT_EQUIP, IT_LOOT };      /* IT_LOOT: trophies - sell them or hand them in for quests */
enum { SLOT_WEAPON, SLOT_ARMOR, SLOT_CHARM, SLOT_HELM, SLOT_BOOTS, SLOT_COUNT };
enum { STAT_STR, STAT_DEX, STAT_VIT, STAT_INT, STAT_LUK, STAT_COUNT };
#define STAT_PTS_PER_LEVEL 3
#define STAT_MAX 99
#define LEVEL_CAP 20
#define PASS_COUNT 8                 /* passive skills (slot 7 is the class passive) */
#define QUEST_COUNT 26
#define QUEST_COUNT_V4 11

#define TF_SOLID 1
#define TF_WATER 2
#define TF_CUT 4
#define TF_CHEST 8
#define TF_SIGN 16
#define TF_DOOR 32
#define TF_DOOR2 64

#define GF_BOSS_DEAD 1
#define GF_DOOR_OPEN 2
#define GF_MET_ELDER 4
#define GF_BOSS_SEEN 8
#define GF_GOT_KEY 16
#define GF_SKULL_OPEN 32
#define GF_WARDEN_DEAD 64
#define GF_WARDEN_SEEN 128
#define GF_SEEN_CRYPT 256
#define GF_SEEN_EMBER 512
#define GF_TYRANT_DEAD 1024
#define GF_TYRANT_SEEN 2048
#define GF_MET_BRAM 4096
#define GF_SEEN_ROAD 8192
#define GF_SEEN_AMBERFALL 16384

/* ---------------------------------------------------------------- data defs */
typedef struct { u8 type, tx, ty, aux; } SpawnDef;
typedef struct { u8 id, tx, ty, dir; } NpcDef;
typedef struct { u8 tx, ty, item, qty, id; } ChestDef;
typedef struct { u8 x0, y0, x1, y1, area, dx, dy, dir; } WarpDef;
typedef struct { u8 tx, ty; const char *text; } SignDef;

typedef struct {
    const u32 *tiles; u16 ntiles; const u16 *pal; const u16 *meta; const u8 *flags; u16 nmeta;
    const u8 *cycles; u8 ncycles; u8 chest_open;
    const u8 *mm;                       /* minimap colour class per metatile (BG palette bank 15) */
} Tileset;

typedef struct {
    const char *name;
    const u8 *ground; const u8 *overlay;
    const SpawnDef *spawns; const NpcDef *npcs; const ChestDef *chests; const WarpDef *warps; const SignDef *signs;
    u8 nspawns, nnpcs, nchests, nwarps, nsigns;
    u8 tileset, music, ambient, startx, starty;
} AreaDef;

typedef struct {
    const char *name; const char *desc;
    u8 hp, mp, str, mag, def, spd, lck;
    u8 ghp, gmp, gstr, gmag, gdef, gspd, glck;
} ClassDef;

typedef struct {
    const char *name; const char *desc;
    u8 type, slot, cls, tier;           /* cls 0xFF = any class; tier 0..3 drives look + shop gating */
    s8 atk, def, spd, lck, mag, crit;   /* spd = DEX, mag = INT, crit = crit chance % */
    s16 hp, mp;
    u16 price;
} ItemDef;

/* derived character stats (pure function of class, level, invested points and gear) */
typedef struct {
    s16 maxhp, maxmp;
    s16 str, spd, vit, mag, lck;        /* attribute totals: STR, DEX(spd), VIT, INT(mag), LUK */
    s16 atk, def, crit, aspd;           /* aspd: attack speed, 256 = 1.0x */
} Stats;

extern const AreaDef areas[AREA_COUNT];
extern const Tileset tilesets[5];
extern const ClassDef class_defs[CLS_COUNT];
extern const ItemDef item_defs[ITEM_COUNT];
extern const char *const stat_names[STAT_COUNT];
extern const char *const slot_names[SLOT_COUNT];

/* ---------------------------------------------------------------- runtime types */
enum { PS_IDLE, PS_ATTACK, PS_SKILL, PS_ROLL, PS_HURT, PS_DEAD, PS_ITEM };

typedef struct {
    s32 x, y;
    s16 kx, ky;
    u8 dir, state, anim, anim_t;
    u8 flash, invuln, roll_cd, skill_cd;
    u8 combo, atk_id, combo_t, chain_n;        /* combo step, swing id, chain window timer, hits landed in this chain */
    u8 cls, level, idle_t, mp_acc;
    u8 buffered, chain_flash, step_w, step_a;  /* queued A press; scaled windup/active frames of the current step */
    u8 step_r, step_c, stat_pts, pot_cd;       /* scaled recover / chain-cancel frames; unspent points; potion cooldown */
    u8 a_held, auto_equip, pass_pts, pad3;     /* frames A has been held; auto-equip option; unspent passive points */
    u16 t;
    u16 gold;
    u32 xp;
    s16 hp, mp, hp_trail;
    s16 maxhp, maxmp, atk, mag, def, spd, lck, str, vit, crit, aspd;
    s16 dash_x, dash_y;
    u8 inv[ITEM_COUNT];
    u8 equip[SLOT_COUNT];
    u8 temper[ITEM_COUNT];                     /* blacksmith upgrade level (+N) of every piece of gear */
    u8 pts[STAT_COUNT];                        /* invested stat points */
    u8 pass[PASS_COUNT];                       /* passive skill ranks */
    u32 playtime;
} Player;

#define MAX_EN 32
typedef struct {
    u8 type, state, dir, active;
    s32 x, y;
    s16 z, vz;
    s16 vx, vy;
    s16 kx, ky;
    s16 hp, maxhp;
    u16 t, t2;
    u8 anim, hurt, flash, stun;
    u8 hitid, aux, dying, alert;
    u8 tier, phase, pad0, pad1;           /* tier: area difficulty (0 forest, 1 crypt, 2 depths); phase: boss phase */
    s16 hx, hy;
} Enemy;

#define MAX_PROJ 28
enum { PJ_NONE, PJ_ARROW, PJ_BOLT, PJ_FIRE, PJ_ORB, PJ_SHARD };   /* PJ_SHARD = arcane blast (mage finisher) */
typedef struct {
    u8 type, owner, life, angle, frame, pierce;   /* pierce = extra enemies it may pass through */
    u8 hk, heavy, seen[3];                        /* hit kind, finisher flag, enemies already struck (index+1) */
    s32 x, y;
    s16 vx, vy;
    s16 dmg;
} Proj;

#define MAX_PICK 16
enum { PK_NONE, PK_COIN, PK_COIN5, PK_HEART, PK_MANA, PK_POTION, PK_COIN25 };
typedef struct {
    u8 type, life, anim, pad;
    s32 x, y;
    s16 vx, vy, z, vz;
} Pickup;

#define MAX_PART 48
enum { PK_SPARKLE, PK_SMOKE, PK_LEAF, PK_HIT, PK_DUST, PK_EMBER, PK_HEAL, PK_EXPL, PK_FIRE_TRAIL, PK_STAR, PK_GHOST, PK_BURST, PK_SHOCK };
typedef struct {
    u8 kind, frame, life, ftick, aux;
    s32 x, y;
    s16 vx, vy;
    s8 gy, fspd;
} Particle;

#define MAX_NUM 10
typedef struct { s16 x, y; s16 vy; u8 life, col, big, pad; u16 val; } DmgNum;

#define MAX_NPC 10
typedef struct { u8 id, active, dir, anim; s16 x, y; u8 talk_t, pal; } Npc;

/* ---------------------------------------------------------------- globals */
extern u16 keys_held, keys_down, keys_up;
extern u32 frame_count;
extern Player pl;
extern Enemy en[MAX_EN];
extern Npc npcs[MAX_NPC];
extern u32 gflags, gchests, gchests2;       /* gchests2: chest ids 32..63 */
int chest_taken(int id);
void chest_take(int id);
extern int cur_area_id;
extern const AreaDef *cur_area;
extern const Tileset *cur_ts;
extern u8 map_g[1024], map_o[1024];
extern int cam_x, cam_y, shake_t, hitstop_t;
extern int fade_level;
extern int music_on;
extern int boss_dead_flag;

/* util.c */
u32 rnd(void);
int rnd_n(int n);
void rnd_seed(u32 s);
int isin(int a);
int icos(int a);
int iabs(int v);
int imin(int a, int b);
int imax(int a, int b);
int iclamp(int v, int lo, int hi);
int dir_angle(int dir);
int vec_angle(int dx, int dy);
int idist(int dx, int dy);
int scale_div(int v, int num, int shift);

/* video.c */
#define SF_HFLIP 1
#define SF_VFLIP 2
#define SF_ALPHA 4
#define SF_DOUBLE 16
#define SH_SQUARE 0
#define SH_WIDE 1
#define SH_TALL 2
typedef struct { u16 base; u8 tx, ty, w, h, pal, bg; } TextArea;
void gfx_init(void);
void gfx_present(void);
void gfx_prepare(void);
void vsync(void);
void spr_begin(void);
void spr_add(int x, int y, int shape, int size, int tile, int pal, int prio, int flags, int key);
void spr_add_aff(int x, int y, int shape, int size, int tile, int pal, int prio, int flags, int key, int angle, int inv_scale);
void spr16(int x, int y, int tile, int pal, int flags, int key);
void spr8(int x, int y, int tile, int pal, int flags, int key);
void spr_ui8(int x, int y, int tile, int pal);
void spr_ui16(int x, int y, int tile, int pal);
void gfx_set_fade(int level, int mosaic);
void gfx_set_fade_mask(int m);
void gfx_blank(int on);
void gfx_mode_world(void);
void gfx_mode_ui(void);
void gfx_load_obj_static(void);
void gfx_load_hero(int cls);
void gfx_set_hero_pal(int cls);
void gfx_load_ui(void);
void gfx_load_title(int logo);
void gfx_clear_world_maps(void);
void gfx_set_backdrop(u16 color);
void bg2_darken(int amount);
void cam_snap(int px, int py);
void cam_update(int px, int py);
void shake_add(int n);
void text_pool_reset(int mark);
int text_pool_mark(void);
void ta_init(TextArea *t, int tx, int ty, int w, int h, int pal, int bg);
void ta_clear(TextArea *t);
void ta_remap(TextArea *t);
void hud_remap(void);
int ta_print(TextArea *t, int px, int py, const char *s, int col);
int ta_putc(TextArea *t, int px, int py, char c, int col);
int ta_width(const char *s);
int ta_num(TextArea *t, int px, int py, int v, int col);
void bg0_clear(void);
void bg0_set(int tx, int ty, u16 entry);
void ui_panel(int tx, int ty, int w, int h);
void bar_draw_thin(TextArea *t, int fill_px);
void bar_draw(TextArea *t, int fill_px, int trail_px, int cfill, int clight, int cdark, int ctrail);

/* world.c */
void world_load(int area, int tx, int ty, int dir);
int tile_flags(int tx, int ty);
int pt_flags(int px, int py);
int box_free(int x, int y, int hw, int hh);
int proj_blocked(int px, int py);
void world_set_meta(int layer, int tx, int ty, int id);
void world_anim(void);
void world_unlock_door(int door_flag);
int world_open_chest(int tx, int ty);
const char *world_sign_at(int tx, int ty);
int world_check_warp(int px, int py);
void world_draw_npcs(void);
void world_update_npcs(void);
Npc *world_npc_near(int px, int py, int r);

/* stats.c */
void stats_calc(int cls, int level, const u8 *pts, const u8 *equip, Stats *out);
void player_recalc(void);
int xp_for_level(int lvl);
int stats_spend(int stat);
int stats_reset_cost(void);
int stats_reset(void);
int stats_invested(void);
int player_roll_crit(void);
const char *stat_blurb(int stat);

/* items.c */
int item_weapon(int cls, int tier);
int item_chest_resolve(int item, int cls);
int item_is_better(int item, int equipped);
int item_stat_line(char *buf, int item);
int shop_build_stock(int *out, int max);
void gear_apply_look(void);
void gfx_set_weapon_pal(int tier);

/* player.c */
void player_new(int cls);
void player_place(int tx, int ty, int dir);
void player_update(void);
void player_draw(void);
void player_hurt(int dmg, int fx, int fy);
void player_add_xp(int xp);
void player_add_gold(int g);
int player_use_item(int item);
void player_equip(int item);
void player_give(int item, int qty);
int player_weapon_item(void);
int player_try_interact(void);
void player_heal(int hp, int mp);
int player_calc_dmg(int mult16, int *crit);
int player_base_dmg(int mult16);
int player_mitigate(int dmg);
void player_move_axis(int vx, int vy);
int player_body_x(void);
int player_body_y(void);
int player_try_roll(void);
extern int level_up_pending;

/* combat.c */
enum { HK_SLASH, HK_STAB, HK_SMASH, HK_ARROW, HK_MAGIC, HK_FIRE };
typedef struct { u8 kind, heavy, finisher, kb, shake, stop; } HitFx;   /* kb in 1/16 units */
#define COMBO_STEPS 4
#define HOLD_ATTACK_FRAMES 18       /* holding A this long keeps attacking on its own */
void combat_reset(void);
void combat_tick(void);
int combat_start(void);
void combat_update(void);
void combat_draw(int sx, int sy);
int combat_hero_frame(int *flip);
void combat_on_hit(const HitFx *fx);
int combat_melee(int reach, int arc, int mult, int radial, int id, const HitFx *fx);
void combat_fire_arrow(int angle, int mult, int spd);
int combat_step_is_finisher(void);

/* enemy.c */
void enemies_clear(void);
Enemy *enemy_spawn(int type, int tx, int ty);
void enemies_update(void);
void enemies_draw(void);
int enemy_damage(Enemy *e, int dmg, int kx, int ky, int crit, const HitFx *fx);
int enemies_alive(void);
Enemy *enemy_boss(void);
Enemy *enemy_big(void);
void enemy_aoe(int cx, int cy, int r, int dmg, int id);

/* fx.c */
void fx_clear(void);
void fx_update(void);
void fx_draw(void);
void part_spawn(int kind, int x, int y, int vx, int vy, int life, int gy);
void part_burst(int kind, int x, int y, int n, int speed, int life);
void dmgnum(int x, int y, int value, int color, int big);
void proj_spawn(int type, int owner, int x, int y, int angle, int speed, int dmg);
void proj_spawn_ex(int type, int owner, int x, int y, int angle, int speed, int dmg, int pierce, int hk, int heavy);
void part_spawn_dir(int kind, int x, int y, int dir, int life);
void pickup_spawn(int type, int x, int y);
void pickups_update(void);
void projs_update(void);
void fx_ambient(void);
void ghost_spawn(int x, int y, int frame, int dir);

/* sound.c */
void snd_init(void);
void snd_update(void);
void snd_play_song(int id);
void snd_stop_music(void);
void snd_fade_out(int frames);
void snd_sfx(int id);
void snd_set_music(int on);
int snd_song_playing(void);
void snd_impact(int hit_kind, int enemy_type, int crit, int heavy, int finisher);

/* ui.c */
void ui_init(void);
void hud_init(void);
void hud_update(void);
void hud_draw(void);
extern int hud_minimap;
void ta_pixel(TextArea *t, int x, int y, int c);
void ta_row(TextArea *t, int tx, int ty, int row, u32 word);
void hud_show(int on);
void banner_show(const char *text);
void toast_show(const char *text);
void ui_overlay_update(void);
void dlg_open(const char *speaker, const char *text);
int dlg_active(void);
int dlg_update(void);
void title_enter(void);
int title_update(void);
void class_enter(void);
int class_update(void);
void menu_open(void);
int menu_update(void);
void shop_open(void);
int shop_update(void);
extern int rest_inn;
void craft_open(int kind);        /* 0 = blacksmith (forge + temper), 1 = alchemist (brew) */
int craft_update(void);
void rest_open(void);
int rest_update(void);
void gameover_enter(void);
int gameover_update(void);
void ending_enter(void);
int ending_update(void);
void levelup_fx(void);
void item_get_show(int item, int qty);
void intro_enter(void);
int intro_update(void);
int has_save(void);

/* save.c */
int save_exists(void);
void save_write(void);
int save_read(void);
void save_erase(void);

/* main.c */
enum { ST_TITLE, ST_CLASS, ST_INTRO, ST_PLAY, ST_DIALOG, ST_MENU, ST_SHOP, ST_REST, ST_TRANS, ST_GAMEOVER, ST_ENDING, ST_CRAFT };
extern int game_state;
void game_init(void);
void game_frame(void);
void start_transition(int area, int tx, int ty, int dir);
void play_dialogue(const char *speaker, const char *text);
void begin_game_from_save(void);
void begin_new_game(int cls);
void npc_talk(Npc *n);


/* ---- v1.3: passives, quests, loot, selling (passives.c / quest.c / items.c) */
const char *pass_name(int i);
const char *pass_desc(int i);
int pass_max(int i);
int pass_spend(int i);
int pass_points_for(int from, int to);
void pass_apply_stats(int cls, Stats *o);
int pass_gold(int g);
int pass_loot_bonus(void);
int pass_dodge(void);
int pass_reduce(int dmg);
void pass_on_kill(void);
void pass_on_hit(void);
extern u8 quest_state[QUEST_COUNT], quest_prog[QUEST_COUNT];
void quests_reset(void);
void quest_refresh(void);
void quest_on_kill(int type);
int quest_talk(int npc);
int quest_marker(int npc);
int quest_tracked(void);
int quest_ready(int q);
int quest_total(void);
void quest_tracker_text(int q, char *buf);
const char *quest_giver_name(int q);
const char *quest_goal(int q);
const char *npc_name(int npc);
const char *quest_reminder(int npc);
int quest_log_rows(int *ids, int max);
int quest_done_count(void);
int quest_keep_count(int item);
const char *quest_title(int q);
const char *quest_hint(int q);
int quest_is_ready(int q);
void quest_progress_text(int q, char *buf);
int item_score(int item);
int item_sell_price(int item);
int loot_roll(int type, int tier, int slot);
const char *rarity_name(int r);
int item_rarity(int item);
int item_icon(int item);
int item_temper_atk(int item);
int item_temper_def(int item);
int item_temper_hp(int item);
int item_can_temper(int item);
int item_label(char *buf, int item);
extern int shop_id;
int player_auto_equip_best(void);
const char *class_skill_name(int cls);
int class_skill_cost(int cls);
const char *class_skill_desc(int cls, int line);
#endif
