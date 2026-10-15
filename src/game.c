#include "game.h"
#include "config.h"
#include "craft.h"
#include "gfx.h"
#include "hw.h"
#include "inventory.h"
#include "items.h"
#include "mobs.h"
#include "palette.h"
#include "player.h"
#include "render.h"
#include "rng.h"
#include "save.h"
#include "sfx.h"
#include "sprites.h"
#include "texgen.h"
#include "ui.h"
#include "world.h"
#include <string.h>

Game g_game;

#ifdef HOST_TEST
Camera g_host_cam; /* camera used for the last gameplay frame (tests only) */
#endif

const char *const g_adv_name[ADV_COUNT] = {
    "GETTING WOOD", "BENCHMARKING", "TIME TO MINE", "STONE AGE",
    "GETTING AN UPGRADE", "HOT TOPIC", "ACQUIRE HARDWARE", "ISN'T IT IRON PICK",
    "DIAMONDS!", "SHINY TOOLS", "DELICIOUS DINNER", "HOME SWEET HOME",
    "MONSTER HUNTER", "SAVE " GAME_NAME,
};

const char *const g_adv_hint[ADV_COUNT] = {
    "PUNCH A TREE", "CRAFT A TABLE", "MAKE A WOOD PICKAXE", "MINE STONE",
    "MAKE A STONE PICKAXE", "BUILD A FURNACE", "SMELT IRON ORE", "MAKE AN IRON PICKAXE",
    "MINE A DIAMOND", "MAKE DIAMOND PICKAXE", "COOK RAW PORK", "PLACE 40 BLOCKS",
    "DEFEAT A ZOMBIE", "CRAFT + PLACE A BEACON",
};

typedef enum { GS_TITLE, GS_PLAY, GS_MENU, GS_PAUSE, GS_DEAD } GameState;

static GameState s_state;
static bool s_palette_dirty;
static int s_spawn_x, s_spawn_y, s_spawn_z;
static u16 s_ignore; /* buttons still held from a menu: ignored until released */

static void enter_play(void)
{
    s_state = GS_PLAY;
    s_ignore = g_keys_held;
}

/* ------------------------------------------------------------------ */
/* Goals and messages                                                  */
/* ------------------------------------------------------------------ */

void game_toast(const char *msg)
{
    int i = 0;
    for (; msg[i] && i < (int)sizeof g_game.toast - 1; i++) g_game.toast[i] = msg[i];
    g_game.toast[i] = 0;
    g_game.toast_t = 150;
}

bool game_has(int adv)
{
    return adv >= 0 && adv < ADV_COUNT && ((g_game.adv >> adv) & 1);
}

void game_unlock(int adv)
{
    if (adv < 0 || adv >= ADV_COUNT || game_has(adv)) return;
    g_game.adv |= (u16)(1u << adv);
    if (adv == ADV_BEACON) {
        game_toast("YOU WIN! " GAME_NAME " IS SAVED");
        g_game.toast_t = 250;
        sfx_play(SFX_ADV);
        return;
    }
    char buf[32] = "GOAL: ";
    int n = 6;
    for (const char *s = g_adv_name[adv]; *s && n < 31; s++) buf[n++] = *s;
    buf[n] = 0;
    game_toast(buf);
    sfx_play(SFX_ADV);
}

static void on_item_gained(u8 id)
{
    switch (id) {
    case B_LOG:           game_unlock(ADV_WOOD); break;
    case B_TABLE:         game_unlock(ADV_TABLE); break;
    case I_PICK_WOOD:     game_unlock(ADV_WPICK); break;
    case B_COBBLE:        game_unlock(ADV_STONE); break;
    case I_PICK_STONE:    game_unlock(ADV_SPICK); break;
    case B_FURNACE:       game_unlock(ADV_FURNACE); break;
    case I_IRON_INGOT:    game_unlock(ADV_IRON); break;
    case I_PICK_IRON:     game_unlock(ADV_IPICK); break;
    case I_DIAMOND:       game_unlock(ADV_DIAMOND); break;
    case I_PICK_DIAMOND:  game_unlock(ADV_DPICK); break;
    case I_PORK_COOKED:   game_unlock(ADV_COOK); break;
    default: break;
    }
}

/* ------------------------------------------------------------------ */
/* Time of day                                                         */
/* ------------------------------------------------------------------ */

int game_night_level(u16 tod)
{
    if (tod < 12000) return 0;
    if (tod < 14000) return (tod - 12000) * 8 / 2000;
    if (tod < 21000) return 8;
    if (tod < 23000) return 8 - (tod - 21000) * 8 / 2000;
    return 0;
}

static void update_time(void)
{
    if (++g_game.tod >= DAY_LEN) g_game.tod = 0;
    int lvl = game_night_level(g_game.tod);
    if (lvl != g_game.daylight) {
        g_game.daylight = (u8)lvl;
        s_palette_dirty = true;
    }
}

/* ------------------------------------------------------------------ */
/* Camera                                                              */
/* ------------------------------------------------------------------ */

/* Under a roof (anything but leaves) cut the world away at the player's level. */
static int roof_clip(void)
{
    for (int z = g_pl.z + 1; z < WZ; z++) {
        u8 b = world_get(g_pl.x, g_pl.y, z);
        if (b != B_AIR && b != B_LEAVES && b != B_DOORTOP) return g_pl.z; /* a doorway is no roof */
    }
    return WZ - 1;
}

/* Camera centred on the block the player is walking to (or standing on). */
static void dest_camera(Camera *c)
{
    c->cx = g_pl.x;
    c->cy = g_pl.y;
    c->cz = (s16)(g_pl.z - 1);
    c->clipz = (s16)roof_clip();
    c->offx = c->offy = 0;
}

/* Camera of the image being shown, slid towards the destination while the
 * player walks: the image is centred on the block we walk away from. */
static void display_camera(Camera *c)
{
    *c = *render_front();
    c->offx = c->offy = 0;
    if (g_pl.moving) {
        int dx = g_pl.x - g_pl.fx, dy = g_pl.y - g_pl.fy, dz = g_pl.z - g_pl.fz;
        int t = g_pl.step_t;
        int ox = ((dx - dy) * 8) * t / STEP_FRAMES;
        int oy = ((dx + dy) * 4 - dz * 8) * t / STEP_FRAMES;
        c->offx = (s16)CLAMP(ox & ~1, -MAX_OFFX, MAX_OFFX);
        c->offy = (s16)CLAMP(oy, -MAX_OFFY, MAX_OFFY);
    }
}

/* Keeps the shown world image in sync with the player. */
static void update_world_image(void)
{
    Camera dest;
    dest_camera(&dest);

    if (g_pl.arrived) { /* a step finished: show the image painted for it */
        if (render_busy()) render_step(true);
        if (render_ready()) render_swap();
    }
    if (g_pl.moving) {
        if (!render_busy() && !render_ready()) render_begin(&dest);
    } else {
        const Camera *f = render_front();
        bool stale = !render_has_front() || g_world_dirty || f->cx != dest.cx || f->cy != dest.cy ||
                     f->cz != dest.cz || f->clipz != dest.clipz;
        if (stale) {
            render_now(&dest);
            g_world_dirty = false;
        }
    }
    if (render_busy()) render_step(false);
}

/* ------------------------------------------------------------------ */
/* Sprites                                                             */
/* ------------------------------------------------------------------ */

static void draw_player(void)
{
    const Player *p = &g_pl;
    if (p->hurt_t & 2) return; /* blink while hurt */
    bool front = (p->dir == 1 || p->dir == 2);
    bool flip = (p->dir == 2 || p->dir == 3);
    int anim = p->moving ? ((p->step_t / 3) & 1) : (p->climb_t ? ((p->climb_t / 3) & 1) : 0);
    spr_put((front ? SP_PLAYER_F0 : SP_PLAYER_B0) + anim, 112, 65, flip, BANK_PLAYER);
}

static void draw_mobs(const Camera *cam)
{
    for (int i = 0; i < MAX_MOBS; i++) {
        const Mob *m = &g_mobs[i];
        if (!m->type || (m->flash & 2)) continue;
        int x0, y0, x1, y1;
        cam_cube_origin(cam, m->fx, m->fy, m->fz - 1, &x0, &y0);
        cam_cube_origin(cam, m->x, m->y, m->z - 1, &x1, &y1);
        int sx = x1, sy = y1;
        int anim = 0;
        if (m->moving) {
            sx = x0 + (x1 - x0) * m->step_t / MOB_STEP;
            sy = y0 + (y1 - y0) * m->step_t / MOB_STEP;
            anim = (m->step_t / 4) & 1;
        }
        sy -= 11;
        bool front = (m->dir == 1 || m->dir == 2);
        bool flip = (m->dir == 2 || m->dir == 3);
        if (m->type == MT_ZOMBIE)
            spr_put((front ? SP_ZOMBIE_F0 : SP_ZOMBIE_B0) + anim, sx, sy, flip, BANK_ZOMBIE);
        else
            spr_put(SP_PIG0 + anim, sx, sy, flip, BANK_PIG);
    }
}

static void draw_cursor(const Camera *cam)
{
    const Player *p = &g_pl;
    if (p->moving || !p->has_target) return;
    int sx, sy;
    cam_cube_origin(cam, p->tx, p->ty, p->tz, &sx, &sy);
    spr_put(SP_CURSOR, sx, sy, false, BANK_FX);
    if (p->mine_t > 0 && p->mine_need > 0) {
        int stage = p->mine_t * 4 / p->mine_need;
        spr_put(SP_CRACK0 + (stage > 3 ? 3 : stage), sx, sy, false, BANK_FX);
    }
}

/* ------------------------------------------------------------------ */
/* Frame presentation                                                  */
/* ------------------------------------------------------------------ */

/* Wait for VBlank, show the finished page, update sprites and palette. */
static void present_frame(void)
{
    hw_vsync();
    hw_flip();
    spr_end();
    if (s_palette_dirty) {
        palette_set_daylight(g_game.daylight);
        s_palette_dirty = false;
    }
}

static int near_stations(void)
{
    int mask = 0;
    for (int z = g_pl.z - 1; z <= g_pl.z + 1; z++)
        for (int y = g_pl.y - 2; y <= g_pl.y + 2; y++)
            for (int x = g_pl.x - 2; x <= g_pl.x + 2; x++) {
                u8 b = world_get(x, y, z);
                if (b == B_TABLE) mask |= NEAR_TABLE;
                else if (b == B_FURNACE) mask |= NEAR_FURNACE;
            }
    return mask;
}

/* ------------------------------------------------------------------ */
/* States                                                              */
/* ------------------------------------------------------------------ */

/* Fresh gameplay state for a freshly generated world. */
static void reset_world_state(void)
{
    memset(&g_game, 0, sizeof g_game);
    g_game.tod = START_TOD;
    palette_set_daylight(0);
    inv_clear();
    mobs_clear();
    world_find_spawn(&s_spawn_x, &s_spawn_y, &s_spawn_z);
    player_reset(s_spawn_x, s_spawn_y, s_spawn_z);
}

/* Shown over the last picture while something slow (world generation) runs. */
static void show_message(const char *msg)
{
    u16 *page = hw_back_page();
    render_present(page, 0, 0);
    gfx_begin(page);
    int w = gfx_text_w(msg, 1);
    gfx_rect(120 - w / 2 - 8, 70, w + 16, 20, C_BLACK);
    gfx_frame(120 - w / 2 - 8, 70, w + 16, 20, C_LGRAY);
    gfx_text(120 - w / 2, 77, msg, C_WHITE, 1);
    spr_begin();
    present_frame();
}

/* The title screen shows a real (throw-away) world as its backdrop. */
static void title_enter(void)
{
    world_generate(hw_seed());
    reset_world_state();

    Camera cam = { (s16)s_spawn_x, (s16)s_spawn_y, (s16)(s_spawn_z - 1), WZ - 1, 0, 0 };
    render_now(&cam);
    g_world_dirty = false;

    ui_title_open(save_exists());
    s_state = GS_TITLE;
}

static void start_new_game(void)
{
    show_message("CREATING WORLD...");
    world_generate(hw_seed());
    reset_world_state();
    rng_seed(&g_rng, hw_seed());
    mobs_populate();
    render_invalidate();
    game_toast("PUNCH A TREE: HOLD A");
    enter_play();
}

static void continue_game(void)
{
    show_message("LOADING...");
    if (!save_load()) { start_new_game(); return; }
    world_find_spawn(&s_spawn_x, &s_spawn_y, &s_spawn_z);
    rng_seed(&g_rng, hw_seed());
    g_game.daylight = (u8)game_night_level(g_game.tod);
    palette_set_daylight(g_game.daylight);
    render_invalidate();
    game_toast("WELCOME BACK");
    enter_play();
}

static void title_frame(void)
{
    TitleAction a = ui_title_update(g_keys_pressed);
    if (a == TITLE_NEW) { start_new_game(); return; }
    if (a == TITLE_CONTINUE) { continue_game(); return; }

    u16 *page = hw_back_page();
    render_present(page, 0, 0);
    gfx_begin(page);
    ui_title_draw();
    spr_begin();
    present_frame();
}

static void play_frame(void)
{
    if (g_keys_pressed & KEY_START) { ui_pause_open(); s_state = GS_PAUSE; return; }
    if (g_keys_pressed & KEY_SELECT) { ui_menu_open(); s_state = GS_MENU; return; }

    /* never finish a step before the image for the next block is painted */
    s_ignore &= g_keys_held;
    u16 held = (u16)(g_keys_held & ~s_ignore), pressed = (u16)(g_keys_pressed & ~s_ignore);
    bool hold = g_pl.moving && g_pl.step_t >= STEP_FRAMES - 1 && render_busy();
    if (!hold) player_update(held, pressed);
    mobs_update();
    update_time();
    if (g_game.toast_t) g_game.toast_t--;
    if (g_game.sel_t) g_game.sel_t--;
    if (g_pl.hp <= 0) { s_state = GS_DEAD; sfx_play(SFX_DEATH); }

    update_world_image();

    Camera cam;
    display_camera(&cam);
#ifdef HOST_TEST
    g_host_cam = cam;
#endif
    u16 *page = hw_back_page();
    render_present(page, cam.offx, cam.offy);
    gfx_begin(page);
    ui_hud();

    spr_begin();
    draw_player();
    draw_mobs(&cam);
    draw_cursor(&cam);
    present_frame();
}

static void menu_frame(void)
{
    int near = near_stations();
    if (ui_menu_update(g_keys_held, g_keys_pressed, near)) { enter_play(); return; }
    u16 *page = hw_back_page();
    gfx_begin(page);
    ui_menu_draw(near);
    spr_begin();
    present_frame();
}

static void pause_frame(void)
{
    switch (ui_pause_update(g_keys_pressed)) {
    case PAUSE_RESUME:
        enter_play();
        return;
    case PAUSE_SAVE:
        game_toast(save_write() ? "GAME SAVED" : "SAVED (EDITS TRIMMED)");
        enter_play();
        return;
    case PAUSE_QUIT:
        title_enter();
        return;
    default:
        break;
    }
    u16 *page = hw_back_page();
    render_present(page, 0, 0);
    gfx_begin(page);
    ui_pause_draw();
    spr_begin();
    present_frame();
}

static void dead_frame(void)
{
    if (g_keys_pressed & KEY_A) {
        player_reset(s_spawn_x, s_spawn_y, s_spawn_z);
        mobs_clear();
        mobs_populate();
        render_invalidate();
        enter_play();
        return;
    }
    u16 *page = hw_back_page();
    render_present(page, 0, 0);
    gfx_begin(page);
    ui_dead_draw();
    spr_begin();
    present_frame();
}

void game_init(void)
{
    hw_init();
    palette_init();
    texgen_init();
    spr_init();
    sfx_init();
    g_inv_hook = on_item_gained;
    rng_seed(&g_rng, hw_seed());
    title_enter();

    /* prepare the first frame while the screen is still blanked */
    u16 *page = hw_back_page();
    render_present(page, 0, 0);
    gfx_begin(page);
    ui_title_draw();
    spr_begin();
    present_frame();
    hw_show();
}

void game_frame(void)
{
    input_poll();
    switch (s_state) {
    case GS_TITLE: title_frame(); break;
    case GS_PLAY:  play_frame();  break;
    case GS_MENU:  menu_frame();  break;
    case GS_PAUSE: pause_frame(); break;
    case GS_DEAD:  dead_frame();  break;
    }
}
