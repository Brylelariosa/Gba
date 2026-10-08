#include "game.h"
#include "assets.h"
#include "audio_data.h"

u16 keys_held, keys_down, keys_up;
u32 frame_count;
int game_state;

extern int save_area, save_tx, save_ty;

static int trans_t, trans_area, trans_tx, trans_ty, trans_dir, trans_loaded;
static int pending_action;
static int end_delay;
static int first_banner;

static void input_poll(void) {
    u16 k = (u16)(~REG_KEYINPUT) & 0x3FF;
    keys_down = (u16)(k & ~keys_held);
    keys_up = (u16)(~k & keys_held);
    keys_held = k;
}

void play_dialogue(const char *speaker, const char *text) {
    dlg_open(speaker, text);
    game_state = ST_DIALOG;
}

void npc_talk(Npc *n) {
    pending_action = 0;
    switch (n->id) {
    case NPC_ELDER:
        if (!(gflags & GF_MET_ELDER)) {
            gflags |= GF_MET_ELDER;
            play_dialogue("Elder Maren",
                          "Ah, a hero at last! I am Maren, elder of Embervale.|"
                          "The dead stir in the Hollow Crypt, north of the Whispering Woods. A Hollow King wakes beneath it.|"
                          "Find the Crypt Key in the woods, open the iron door, and end this.|"
                          "A attacks or talks. B uses your skill. R rolls. L drinks a potion. START opens the menu.");
        } else if (gflags & GF_DOOR_OPEN) {
            play_dialogue("Elder Maren", "The iron door is open! Go, and may the light guide your blade.");
        } else if (pl.inv[ITEM_CRYPT_KEY]) {
            play_dialogue("Elder Maren", "You hold the Crypt Key! The iron door lies deep within the crypt. Be brave.");
        } else {
            play_dialogue("Elder Maren", "The Crypt Key rests in the southwest of the woods, guarded by wolves. Grow strong first, and heal at Lyra.");
        }
        break;
    case NPC_MERCHANT:
        play_dialogue("Tobin", "Welcome, traveler! Take a look at my wares. Gold buys survival.");
        pending_action = 1;
        break;
    case NPC_HEALER:
        play_dialogue("Sister Lyra", "You look weary, brave one. Shall I tend to your wounds?");
        pending_action = 2;
        break;
    }
}

/* ------------------------------------------------------------------ transitions */
void start_transition(int area, int tx, int ty, int dir) {
    trans_area = area;
    trans_tx = tx;
    trans_ty = ty;
    trans_dir = dir;
    trans_t = 0;
    trans_loaded = 0;
    game_state = ST_TRANS;
    snd_sfx(SFX_WARP);
    if (area != cur_area_id) snd_fade_out(24);
}

static void draw_world(void) {
    spr_begin();
    world_draw_npcs();
    player_draw();
    enemies_draw();
    fx_draw();
}

static void enter_area_fx(void) {
    banner_show(cur_area->name);
    snd_play_song(cur_area->music);
}

static void trans_update(void) {
    trans_t++;
    if (!trans_loaded) {
        int f = trans_t * 16 / 22;
        gfx_set_fade(f, trans_t * 8 / 22);
        if (trans_t >= 22) {
            world_load(trans_area, trans_tx, trans_ty, trans_dir);
            trans_loaded = 1;
            trans_t = 0;
            gfx_set_fade(16, 8);
            enter_area_fx();
        }
        fx_update();
        draw_world();
        return;
    }
    int f = 16 - trans_t * 16 / 22;
    gfx_set_fade(f, f / 2);
    world_anim();
    cam_update((int)(pl.x >> 8), (int)(pl.y >> 8));
    hud_update();
    draw_world();
    if (trans_t >= 22) {
        gfx_set_fade(0, 0);
        game_state = ST_PLAY;
    }
}

/* ------------------------------------------------------------------ game start */
static void setup_world_mode(void) {
    gfx_blank(1);
    gfx_load_obj_static();
    gfx_load_hero(pl.cls);
    bg2_darken(0);
    gfx_mode_world();
    gfx_blank(0);
}

void begin_new_game(int cls) {
    player_new(cls);
    setup_world_mode();
    hud_init();
    world_load(0, areas[0].startx, areas[0].starty, DIR_UP);
    snd_stop_music();
    enter_area_fx();
    gfx_set_fade(0, 0);
    first_banner = 1;
    game_state = ST_PLAY;
    end_delay = 0;
    play_dialogue("", "Embervale sleeps beneath the shadow of the Hollow Crypt.|Speak with Elder Maren, in the house north of the village square.");
}

void begin_game_from_save(void) {
    if (!save_read()) { begin_new_game(0); return; }
    setup_world_mode();
    hud_init();
    world_load(save_area, save_tx, save_ty, DIR_DOWN);
    snd_stop_music();
    enter_area_fx();
    gfx_set_fade(0, 0);
    game_state = ST_PLAY;
    end_delay = 0;
}

void game_init(void) {
    gfx_init();
    snd_init();
    ui_init();
    keys_held = 0;
    frame_count = 0;
    rnd_seed(0xC0FFEE);
    title_enter();
    game_state = ST_TITLE;
}

/* ------------------------------------------------------------------ play */
static void play_update(void) {
    int px = (int)(pl.x >> 8), py = (int)(pl.y >> 8);
    if (hitstop_t > 0) hitstop_t--;
    else {
        player_update();
        enemies_update();
        world_update_npcs();
    }
    fx_update();
    fx_ambient();
    world_anim();
    px = (int)(pl.x >> 8);
    py = (int)(pl.y >> 8);
    {
        int fx = px, fy = py;
        Enemy *bb = enemy_boss();
        if (bb && bb->active && (gflags & GF_BOSS_SEEN) && py < 12 * 16) {
            fy = (py + (int)(bb->y >> 8) - 18) / 2;
            fx = (px * 3 + (int)(bb->x >> 8)) / 4;
        }
        cam_update(fx, fy);
    }
    if (level_up_pending) { level_up_pending = 0; levelup_fx(); }
    hud_update();
    draw_world();

    if (pl.state == PS_DEAD && pl.t > 70) {
        gameover_enter();
        game_state = ST_GAMEOVER;
        return;
    }
    if (pl.state != PS_DEAD) {
        int w = world_check_warp(px, py);
        if (w >= 0) {
            const WarpDef *wd = &cur_area->warps[w];
            start_transition(wd->area, wd->dx, wd->dy, wd->dir);
            return;
        }
        if ((keys_down & KEY_START) && (pl.state == PS_IDLE)) {
            menu_open();
            game_state = ST_MENU;
            return;
        }
    }
    if (boss_dead_flag) {
        end_delay++;
        if (end_delay == 1) { snd_stop_music(); snd_sfx(SFX_VICTORY); banner_show("VICTORY!"); }
        if (end_delay > 170) gfx_set_fade((end_delay - 170) * 16 / 30, 0);
        if (end_delay >= 200) {
            ending_enter();
            game_state = ST_ENDING;
        }
    }
}

static void close_overlay_ui(void) {
    hud_init();
}

void game_frame(void) {
    input_poll();
    frame_count++;
    spr_begin();
    switch (game_state) {
    case ST_TITLE: {
        int r = title_update();
        if (r == 1) { class_enter(); game_state = ST_CLASS; }
        else if (r == 2) { begin_game_from_save(); }
        break;
    }
    case ST_CLASS: {
        int r = class_update();
        if (r > 0) { begin_new_game(r - 1); }
        else if (r < 0) { title_enter(); game_state = ST_TITLE; }
        break;
    }
    case ST_PLAY:
        play_update();
        break;
    case ST_DIALOG:
        fx_update();
        world_update_npcs();
        world_anim();
        hud_update();
        draw_world();
        if (dlg_update()) {
            if (pending_action == 1) { pending_action = 0; shop_open(); game_state = ST_SHOP; }
            else if (pending_action == 2) { pending_action = 0; rest_open(); game_state = ST_REST; }
            else game_state = ST_PLAY;
        }
        break;
    case ST_MENU: {
        int r = menu_update();
        if (r == 1) { close_overlay_ui(); game_state = ST_PLAY; }
        else if (r == 2) { title_enter(); game_state = ST_TITLE; }
        break;
    }
    case ST_SHOP:
        if (shop_update()) { close_overlay_ui(); game_state = ST_PLAY; }
        break;
    case ST_REST:
        fx_update();
        world_anim();
        hud_update();
        draw_world();
        if (rest_update()) game_state = ST_PLAY;
        break;
    case ST_TRANS:
        trans_update();
        break;
    case ST_GAMEOVER: {
        int r = gameover_update();
        if (r == 1) {
            pl.hp = pl.maxhp;
            pl.mp = pl.maxmp;
            pl.hp_trail = pl.hp;
            pl.gold = (u16)(pl.gold / 2);
            pl.state = PS_IDLE;
            gfx_set_fade(0, 0);
            setup_world_mode();
            hud_init();
            world_load(0, areas[0].startx, areas[0].starty, DIR_UP);
            snd_stop_music();
            enter_area_fx();
            game_state = ST_PLAY;
            end_delay = 0;
        } else if (r == 2) {
            gfx_set_fade(0, 0);
            title_enter();
            game_state = ST_TITLE;
        }
        break;
    }
    case ST_ENDING:
        if (ending_update()) { title_enter(); game_state = ST_TITLE; }
        break;
    }
    gfx_prepare();
    vsync();
    gfx_present();
    snd_update();
}

#ifndef HOST_BUILD
int main(void) {
    game_init();
    for (;;) game_frame();
    return 0;
}
#endif
