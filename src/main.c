#include "game.h"
#include "assets.h"
#include "audio_data.h"
#include "ui_util.h"

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

static char talk_buf[300];

/* the NPC's usual line, plus a reminder page while one of their quests is open */
static void say(const char *who, const char *text, const char *remind) {
    if (!remind) { play_dialogue(who, text); return; }
    int k = cat_str(talk_buf, 0, text);
    k = cat_str(talk_buf, k, "|Quest: ");
    cat_str(talk_buf, k, remind);
    play_dialogue(who, talk_buf);
}

static const char *pick(const char *const *lines, int n) { return lines[rnd_n(n)]; }

void npc_talk(Npc *n) {
    pending_action = 0;
    if (quest_talk(n->id)) return;                       /* a quest was offered or handed in */
    const char *remind = quest_reminder(n->id);
    switch (n->id) {
    case NPC_ELDER:
        if (!(gflags & GF_MET_ELDER)) {
            gflags |= GF_MET_ELDER;
            play_dialogue("Elder Maren",
                          "Ah, a hero at last! I am Maren, elder of Embervale.|Long ago King Aldric ruled the city of Emberhold. When the Ashfall buried it, he bound his soul to a crown rather than die.|Now the Hollow King wakes beneath the Hollow Crypt, past the Whispering Woods. The dead stir at his call.|Ranger Bram camps in the southwest of the woods and keeps the Crypt Key. Help him with his wolves and the key is yours.|Below the crypt, the Bone Warden guards the Skull Key, and the Skull Key opens the king's door.|Fight, grow strong, spend your stat and passive points in the menu. Levels matter here.|A attacks: tap or hold it to chain a combo. B skill, R roll, L potion, START menu. Watch for the ! over people's heads: that is a quest.");
        } else if (gflags & GF_TYRANT_DEAD) {
            say("Elder Maren", "The fire below is out, hero. Embervale sleeps soundly again.|Aldric and Corvin rest at last. Your tale ends here, but I think it is only the first of many.", remind);
        } else if (gflags & GF_BOSS_DEAD) {
            say("Elder Maren", "Aldric is gone, yet his crown still burns. Beneath his throne a stair leads to the Cinder Hollows.|That is where the crown was forged. Be level 14 or more before you go down.", remind);
        } else if (gflags & GF_SKULL_OPEN) {
            say("Elder Maren", "The skull seal is broken! The Hollow King awaits.|Rest with Lyra and stock up on potions first.", remind);
        } else if (pl.inv[ITEM_SKULL_KEY]) {
            say("Elder Maren", "You hold the Skull Key! Return to the crypt|and break the seal before the king's chamber.", remind);
        } else if (gflags & GF_DOOR_OPEN) {
            say("Elder Maren", "The iron door is open. Take the stairway down in the crypt's west room.|Defeat the Bone Warden below. Be level 11 or more.", remind);
        } else if (pl.inv[ITEM_CRYPT_KEY]) {
            say("Elder Maren", "You hold the Crypt Key! The iron door lies deep within the crypt.|Be level 7 at least.", remind);
        } else {
            say("Elder Maren", "Ranger Bram keeps the Crypt Key. Find his camp in the southwest of the woods.|Grow strong first, and heal at Lyra.", remind);
        }
        break;
    case NPC_MERCHANT: {
        static const char *const hello[3] = {
            "Welcome, traveler! Take a look at my wares. Gold buys survival.",
            "They say the Ashfall turned Emberhold to cinders overnight. Bad for business, good for fire sales!",
            "Sell me your spare gear and trophies! I pay half of what I'd charge. Fair, mostly." };
        say("Tobin", pick(hello, 3), remind);
        shop_id = 0;
        pending_action = 1;
        break;
    }
    case NPC_HEALER: {
        static const char *const hello[3] = {
            "You look weary, brave one. Shall I tend to your wounds?",
            "The crypt was not always cursed. Aldric loved his people. Grief twisted him, so they say. Shall I heal you?",
            "Rest, hero. The village hearths keep the cold dead away. Shall I tend to your wounds?" };
        say("Sister Lyra", pick(hello, 3), remind);
        rest_inn = 0;
        pending_action = 2;
        break;
    }
    case NPC_RANGER: {
        static const char *const hello[3] = {
            "The woods are quieter with those wolves gone. The Crypt Key should serve you well. Mind the dead beyond the iron door.",
            "I came here to hunt, and stayed for the quiet. Keep to the paths and the wolves keep to the trees.",
            "North lies the Hollow Crypt. South along the village road is Sunmeadow, and past it Amberwood and the town of Amberfall." };
        say("Ranger Bram", pick(hello, 3), remind);
        break;
    }
    case NPC_SMITH: {
        static const char *const hello[3] = {
            "Name's Garrick. Bring me trophies and I will forge you something sturdy. Or I can temper what you carry for more bite.",
            "Wolf pelts, bone, iron, amber... Every trophy is gear waiting to happen. Do not sell it all to Tobin!",
            "Tempering takes iron and patience. Each level makes a piece stronger, up to plus three. The last level needs a Cinder Heart." };
        say("Garrick", pick(hello, 3), remind);
        pending_action = 3;
        break;
    }
    case NPC_GUARD: {
        static const char *const hello[3] = {
            "South gate. The road leads to Amberfall, but wolves roam there. Be level 5 at least.",
            "I keep this gate. If the wolves ever reach the village they will go through me first.",
            "Amberwood is tougher than the woods behind us. Level 7 and up, and bring potions." };
        say("Dorn", pick(hello, 3), remind);
        break;
    }
    case NPC_KID: {
        static const char *const hello[3] = {
            "Are you a real hero? Have you seen the Hollow King? Is he scary?",
            "Mama says never go in the woods. I went once. Saw a wolf. Ran home. Do not tell her!",
            "When I grow up I will be a hero and have a sword and a dog and a hat." };
        say("Pip", pick(hello, 3), remind);
        break;
    }
    case NPC_FARMER: {
        static const char *const hello[3] = {
            "Good soil, this. Bone dust does wonders. The cabbages do not thank me, but they do grow.",
            "Slimes love my garden almost as much as I do. Almost.",
            "Fine weather for it. Pray it holds until harvest." };
        say("Hale", pick(hello, 3), remind);
        break;
    }
    case NPC_TRAVELER: {
        static const char *const hello[3] = {
            "Safe travels! Amberwood is just east, and past it lies Amberfall. They have a good inn and a better alchemist.",
            "The old waystone up north is where travelers leave amber for luck. Wolves take it. So much for luck.",
            "I walk this road to trade stories. The best one is still the Hollow King. Is it true he wears a crown of fire?" };
        say("Kess", pick(hello, 3), remind);
        break;
    }
    case NPC_INNKEEPER: {
        static const char *const hello[3] = {
            "Welcome to the Amberfall Inn! A warm bed heals every ache. Shall I tuck you in?",
            "Travelers always say my beds beat the road. Shall I tend to you?",
            "You look worn out, dear. Rest a while. Shall I tend to your wounds?" };
        say("Mara", pick(hello, 3), remind);
        rest_inn = 1;
        pending_action = 2;
        break;
    }
    case NPC_ALCHEMIST: {
        static const char *const hello[3] = {
            "Welcome to my bench! Bring me gel, wings, bone and amber and I will brew you potions and charms.",
            "Slime gel for potions, bat wings for ether. Simple chemistry, as long as nothing explodes.",
            "I can brew an Elixir if you bring bat wings, bone dust and Ember Cores. Rare stuff, rare results." };
        say("Wyn", pick(hello, 3), remind);
        pending_action = 4;
        break;
    }
    case NPC_SCHOLAR: {
        static const char *const hello[3] = {
            "Emberhold burned in a single night, the Ashfall they call it. Yet the crown King Aldric forged still burns beneath the crypt.",
            "Amber is fossil sap from forests older than the kingdom. It holds light the way old stories hold truth.",
            "Maren writes that you are a hero. I hope so. The Cinder Hollows hide secrets I would give a year of my life to read." };
        say("Orin", pick(hello, 3), remind);
        break;
    }
    case NPC_VENDOR: {
        static const char *const hello[3] = {
            "Welcome to Brea's stall! Fresh stock every day, fair prices, and I buy trophies too.",
            "Amberfall pays better than Embervale for loot. Do not tell Tobin I said so!",
            "Take a look! Elixirs, armor, charms. Everything a hero needs." };
        say("Brea", pick(hello, 3), remind);
        shop_id = 1;
        pending_action = 1;
        break;
    }
    case NPC_TOWNSMAN: {
        static const char *const hello[4] = {
            "They say the east road is blocked by a rockfall. The workers are slow, but we will be connected soon.",
            "The wolves keep to Amberwood these days. I keep to the town square.",
            "Brea's stall has the best prices. Wyn's potions work best, if you can stand the smell.",
            "Amberfall is small, but we look after each other." };
        say("Townsman", pick(hello, 4), remind);
        break;
    }
    case NPC_TOWNSWOMAN: {
        static const char *const hello[4] = {
            "The autumn light is wonderful here. It turns everything gold.",
            "My daughter wants to be a hero. I want her to be a baker. We will see.",
            "Mara's inn is the cleanest in the whole valley. Mind the stairs, though.",
            "Do you smell that? That is Wyn's cauldron again." };
        say("Townswoman", pick(hello, 4), remind);
        break;
    }
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
    hud_draw();
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
    gear_apply_look();
    bg2_darken(0);
    gfx_mode_world();
    gfx_blank(0);
}

/* after the credits the hero wakes in the silent throne room: the adventure goes on (and is saved) */
static void continue_after_victory(void) {
    setup_world_mode();
    hud_init();
    pl.hp = pl.maxhp;
    pl.mp = pl.maxmp;
    world_load(2, 15, 3, DIR_DOWN);
    snd_stop_music();
    enter_area_fx();
    gfx_set_fade(0, 0);
    game_state = ST_PLAY;
    end_delay = 0;
    save_write();
    play_dialogue("", "The throne room is silent. Behind the throne, a stair has opened in the stone.|Something still burns far below... (Progress saved.)");
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
        if ((keys_down & KEY_SELECT) && (pl.state == PS_IDLE)) {      /* SELECT: straight to the area map */
            menu_open_map();
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
            else if (pending_action == 3 || pending_action == 4) { craft_open(pending_action == 4); pending_action = 0; game_state = ST_CRAFT; }
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
    case ST_CRAFT:
        if (craft_update()) { close_overlay_ui(); game_state = ST_PLAY; }
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
        if (ending_update()) continue_after_victory();
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
