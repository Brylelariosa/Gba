/* quest.c - story quests. A quest is offered by an NPC, tracked automatically (kills, items, story flags) and turned
   in by talking to the giver again. State lives in two small arrays that the save file stores. */
#include "game.h"
#include "assets.h"
#include "audio_data.h"

enum { QS_NEW, QS_ACTIVE, QS_READY, QS_DONE };
enum { QK_KILL, QK_FETCH, QK_FLAG };

typedef struct {
    const char *title;
    u8 giver, kind, target, need, min_level, prereq;
    u32 flag;                              /* QK_FLAG: story flag that completes it */
    u16 gold, xp;
    u8 item, qty;                          /* reward item */
    const char *offer, *ready, *busy;      /* dialogue; '|' separates pages */
    const char *goal;                      /* short objective for the on-screen tracker (story quests) */
} QuestDef;

enum { Q_NONE, Q_WOLVES, Q_GEL, Q_WINGS, Q_CRYPT, Q_BONES, Q_WARDEN, Q_KING, Q_EMBER, Q_TYRANT, Q_CORES,
       Q_PESTS, Q_PIP, Q_FORGE, Q_ROAD, Q_LETTER, Q_AMBER, Q_PELTS, Q_TONIC, Q_STOCK, Q_DIRE, Q_STUDY };   /* ids are saved: append only */
#define Q_LAST Q_STUDY            /* the table has room up to QUEST_COUNT, but only these are defined */

static const char *const npc_names[NPC_COUNT] = { "Elder Maren", "Tobin", "Sister Lyra", "Ranger Bram", "Garrick", "Dorn", "Pip", "Hale",
    "Kess", "Mara", "Wyn", "Orin", "Brea", "Townsfolk", "Townsfolk" };
const char *npc_name(int npc) { return npc_names[npc < NPC_COUNT ? npc : 0]; }

static const QuestDef quests[QUEST_COUNT] = {
    [Q_NONE] = { "", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "", "", "", "" },
    [Q_WOLVES] = { "Wolf Hunt", NPC_RANGER, QK_KILL, EN_WOLF, 5, 1, Q_NONE, 0, 100, 60, ITEM_CRYPT_KEY, 1,
        "Wolves have overrun my camp and I cannot reach my supplies. Cull five of them and the Crypt Key I keep for the Elder is yours.|They roam all through these woods. Mind their teeth.",
        "Five wolves fewer! You have earned this. The Crypt Key opens the iron door in the Hollow Crypt, north of here.",
        "Five wolves, friend. They prowl the woods around my camp.", "Hunt wolves" },
    [Q_GEL] = { "Sticky Gel", NPC_MERCHANT, QK_FETCH, ITEM_GEL, 6, 1, Q_NONE, 0, 80, 30, ITEM_ETHER, 2,
        "Hey, adventurer! Slimes drop a gel I boil into glue and lamp oil. Bring me six Slime Gel and I'll pay well!",
        "Sticky, smelly, perfect! Here's your coin, and a couple of Ethers on the house.",
        "Slimes in the woods carry the gel. I need six.", "Collect Slime Gel" },
    [Q_WINGS] = { "Bat Tonic", NPC_HEALER, QK_FETCH, ITEM_WING, 5, 3, Q_NONE, 0, 60, 50, ITEM_ELIXIR, 1,
        "Bat wings make a fine tonic against the crypt's chill. Could you bring me five Bat Wings?",
        "These will brew a fine tonic. Take this Elixir. May it save you in dark places.",
        "Bats nest in the woods and the crypt. Five wings, please.", "Collect Bat Wings" },
    [Q_CRYPT] = { "Into the Crypt", NPC_ELDER, QK_FLAG, 0, 0, 5, Q_WOLVES, GF_SEEN_CRYPT, 150, 120, ITEM_CHARM_GUARD, 1,
        "Now to the Hollow Crypt itself. Step inside and see what stirs there. Do not linger yet.|Long ago the Ember King, Aldric, bound his soul to a crown to cheat death. The crypt is his tomb.",
        "You stood in Aldric's tomb and lived. Then you are ready. This charm was my husband's.",
        "Find the Hollow Crypt beyond the woods and step inside.", "Enter the crypt" },
    [Q_BONES] = { "Bone Dust", NPC_MERCHANT, QK_FETCH, ITEM_BONE, 8, 7, Q_GEL, 0, 250, 90, ITEM_HIPOTION, 3,
        "Skeletons crumble into fine bone dust. Farmers swear by it as fertilizer! Eight Bone Dust, please.",
        "Eight! Rattling fine work. The farmers will pay me double. Here's your cut.",
        "Skeletons in the crypt drop Bone Dust. I need eight.", "Collect Bone Dust" },
    [Q_WARDEN] = { "The Warden", NPC_ELDER, QK_FLAG, 0, 0, 9, Q_CRYPT, GF_WARDEN_DEAD, 300, 200, ITEM_ELIXIR, 1,
        "Below the crypt stands Sir Corvin, the Bone Warden. Aldric's oldest captain, he guards the Skull Key.|He was a good man once. Free him from that duty.",
        "You freed Corvin. Rest, old friend... You have my deepest thanks, hero.",
        "Take the crypt's west stairs down and defeat the Bone Warden.", "Defeat the Warden" },
    [Q_KING] = { "Hollow King", NPC_ELDER, QK_FLAG, 0, 0, 12, Q_WARDEN, GF_BOSS_DEAD, 600, 400, ITEM_ELIXIR, 2,
        "The Skull Key opens the seal to Aldric's throne. End his hollow reign, hero. Embervale is counting on you.",
        "The Hollow King is gone, truly gone. Embervale will sing of you for a hundred years!",
        "Break the skull seal and face the Hollow King.", "Defeat the Hollow King" },
    [Q_EMBER] = { "Cinder Stair", NPC_ELDER, QK_FLAG, 0, 0, 12, Q_KING, GF_SEEN_EMBER, 400, 300, ITEM_CHARM_PHOENIX, 1,
        "But the crown still burns. It was forged below the old capital, in the Cinder Hollows.|A stair behind Aldric's throne now stands open. Go down, hero, and see what feeds the fire.",
        "You walked the Cinder Hollows? Then you know what lurks there... a tyrant of living flame.",
        "Take the stair behind the king's throne down to the Cinder Hollows.", "Descend to the Hollows" },
    [Q_TYRANT] = { "Cinder Tyrant", NPC_ELDER, QK_FLAG, 0, 0, 14, Q_EMBER, GF_TYRANT_DEAD, 1000, 600, ITEM_CHARM_EMBER, 1,
        "The Cinder Tyrant is the forge-spirit that kept Aldric's crown alight. Quench it and the curse ends for good.",
        "The fire is out. The curse is broken. The Hollow King sleeps at last. You are Embervale's greatest hero!",
        "Defeat the Cinder Tyrant in the heart of the Hollows.", "Defeat the Tyrant" },
    [Q_CORES] = { "Ember Cores", NPC_MERCHANT, QK_FETCH, ITEM_EMBER, 4, 13, Q_EMBER, 0, 700, 300, ITEM_ELIXIR, 2,
        "Ember Cores! Rumor says they burn forever. Bring me four from the Cinder Hollows and I'll pay a king's ransom.",
        "Marvelous! Warm as a hearth! Take this, and my finest thanks.",
        "Creatures of the Cinder Hollows carry Ember Cores. Four, please.", "Collect Ember Cores" },

    /* ---- the Amber lands update ---- */
    [Q_PESTS] = { "Pest Control", NPC_FARMER, QK_KILL, EN_SLIME, 6, 1, Q_NONE, 0, 60, 40, ITEM_POTION, 3,
        "Slimes keep creeping into my vegetable patch and slurping up everything. Squash six of them for me, would you?",
        "My cabbages thank you! Here, a little something for the trouble.",
        "Slimes by the woods and the road. Six should do it.", "Squash slimes" },
    [Q_PIP] = { "Pip's Pelts", NPC_KID, QK_FETCH, ITEM_PELT, 2, 2, Q_NONE, 0, 40, 30, ITEM_ETHER, 1,
        "Psst! I want a wolf-pelt cloak like a real ranger! Can you get me two Wolf Pelts? Please please please?",
        "Wow, real wolf pelts! I'm the coolest kid in Embervale now. Here, take my savings!",
        "Wolves drop pelts. I need two. Please hurry!", "Collect Wolf Pelts" },
    [Q_FORGE] = { "Fire the Forge", NPC_SMITH, QK_FETCH, ITEM_IRON, 3, 5, Q_NONE, 0, 150, 100, ITEM_HIPOTION, 2,
        "I can temper your gear and forge new pieces, but I am out of iron. Tough wolves and skeletons carry Iron Ore. Bring me three.|Then show me what you have gathered and I will make something of it.",
        "Good iron! Now we are in business. Bring me materials any time and I will forge or temper for you.",
        "Iron Ore drops from wolves and skeletons deeper in the wild. Three, please.", "Collect Iron Ore" },
    [Q_ROAD] = { "Clear the Road", NPC_GUARD, QK_KILL, EN_WOLF, 6, 4, Q_NONE, 0, 150, 120, ITEM_HIPOTION, 2,
        "Wolves on Sunmeadow Road are scaring off the traders. Cull six and the way south to Amberfall will be safe again.",
        "The traders are already asking about the road. Good work, hero. Take this.",
        "Wolves on the road south and in the woods. Six of them.", "Hunt wolves" },
    [Q_LETTER] = { "To Amberfall", NPC_ELDER, QK_FLAG, 0, 0, 6, Q_WOLVES, GF_SEEN_AMBERFALL, 150, 100, ITEM_HIPOTION, 2,
        "My old friend Orin the scholar lives in Amberfall. Take the south gate, follow Sunmeadow Road, then east through Amberwood.|He knows more of the old tales than I do. Seek him out, and trade in town.",
        "You made it to Amberfall? Then Orin has something to teach you. Take this for the road.",
        "South gate, Sunmeadow Road, then east through Amberwood to Amberfall.", "Travel to Amberfall" },
    [Q_AMBER] = { "Waystone Amber", NPC_TRAVELER, QK_FETCH, ITEM_AMBER, 3, 5, Q_NONE, 0, 120, 80, ITEM_ETHER, 3,
        "Hello, traveler! Amber shards wash up in these lands. Bring me three and I'll tell you the road's secrets.",
        "Beautiful! Warm as sunrise. Here is your share, and good luck on the road.",
        "Creatures on the road and in Amberwood carry Amber Shards. Three, please.", "Collect Amber Shards" },
    [Q_PELTS] = { "Cozy Beds", NPC_INNKEEPER, QK_FETCH, ITEM_PELT, 4, 6, Q_NONE, 0, 120, 80, ITEM_HIPOTION, 2,
        "Winter is coming and my guests want warm blankets. Four Wolf Pelts would make all of my beds cozy!",
        "Oh, these are lovely! Rest at my inn any time, hero. You have earned it.",
        "Wolves in Amberwood drop pelts. Four, please.", "Collect Wolf Pelts" },
    [Q_TONIC] = { "Wyn's Tonic", NPC_ALCHEMIST, QK_FETCH, ITEM_WING, 6, 6, Q_NONE, 0, 100, 90, ITEM_ELIXIR, 1,
        "My tonics need bat wings, fresh ones. Bring me six and I will pay in gold and in magic.",
        "Fresh and leathery, perfect. This Elixir is my own recipe. Use it wisely.",
        "Bats fly through Amberwood and the road. Six wings, please.", "Collect Bat Wings" },
    [Q_STOCK] = { "Brea's Stock", NPC_VENDOR, QK_FETCH, ITEM_IRON, 4, 7, Q_NONE, 0, 180, 100, ITEM_ETHER, 3,
        "My stall is low on iron fittings. Bring me four Iron Ore and I will make it worth your while.",
        "That is just what I needed! Business is booming. Here, for your trouble.",
        "Iron Ore comes from wolves in Amberwood and from skeletons. Four, please.", "Collect Iron Ore" },
    [Q_DIRE] = { "Dire Wolves", NPC_SCHOLAR, QK_KILL, EN_WOLF, 8, 7, Q_LETTER, 0, 200, 150, ITEM_CHARM_CRIT, 1,
        "I am cataloguing the wolf packs of Amberwood, but they will not hold still for sketching. Cull eight so I can work.",
        "Splendid! Now I can finish my notes in peace. Take this charm. It sharpens the eye.",
        "Wolves roam Amberwood and the roads. Eight of them.", "Hunt wolves" },
    [Q_STUDY] = { "Cinder Study", NPC_SCHOLAR, QK_FETCH, ITEM_EMBER, 3, 13, Q_EMBER, 0, 400, 300, ITEM_CINDER_HEART, 1,
        "I study the fire of the Cinder Hollows. Three Ember Cores, please, and I will trade you something rarer: a Cinder Heart.|Garrick the smith says it is the finest fuel for tempering legendary gear.",
        "Remarkable! The cores still hum. Here is the Cinder Heart I promised.",
        "Creatures of the Cinder Hollows carry Ember Cores. Three, please.", "Collect Ember Cores" },
};

u8 quest_state[QUEST_COUNT], quest_prog[QUEST_COUNT];

void quests_reset(void) {
    for (int i = 0; i < QUEST_COUNT; i++) { quest_state[i] = QS_NEW; quest_prog[i] = 0; }
}

static int objective_met(int q) {
    const QuestDef *d = &quests[q];
    switch (d->kind) {
    case QK_KILL: return quest_prog[q] >= d->need;
    case QK_FETCH: return pl.inv[d->target] >= d->need;
    default: return (gflags & d->flag) != 0;
    }
}

/* move ACTIVE quests to READY when their objective is met (and announce it once) */
void quest_refresh(void) {
    /* saves from before Ranger Bram: the key was found in a chest, so Wolf Hunt counts as done */
    if ((pl.inv[ITEM_CRYPT_KEY] || (gflags & GF_DOOR_OPEN)) && quest_state[Q_WOLVES] != QS_DONE) quest_state[Q_WOLVES] = QS_DONE;
    for (int q = 1; q <= Q_LAST; q++) {
        if (quest_state[q] == QS_ACTIVE && objective_met(q)) {
            quest_state[q] = QS_READY;
            snd_sfx(SFX_ITEM);
            toast_show("Quest ready to turn in!");
        } else if (quest_state[q] == QS_READY && quests[q].kind == QK_FETCH && !objective_met(q)) {
            quest_state[q] = QS_ACTIVE;                         /* sold the goods again */
        }
    }
}

void quest_on_kill(int type) {
    for (int q = 1; q <= Q_LAST; q++)
        if (quest_state[q] == QS_ACTIVE && quests[q].kind == QK_KILL && quests[q].target == type && quest_prog[q] < 250)
            quest_prog[q]++;
    quest_refresh();
}

static int available(int q) {
    const QuestDef *d = &quests[q];
    return quest_state[q] == QS_NEW && pl.level >= d->min_level && (d->prereq == Q_NONE || quest_state[d->prereq] == QS_DONE);
}

static void reward(int q) {
    const QuestDef *d = &quests[q];
    if (d->kind == QK_FETCH) pl.inv[d->target] = (u8)(pl.inv[d->target] - d->need);
    player_add_gold(d->gold);
    player_add_xp(d->xp);
    if (d->item) player_give(d->item, d->qty);
    snd_sfx(SFX_VICTORY);
    toast_show("Quest complete!");
}

/* Called when the hero talks to an NPC. Returns 1 when it started a quest dialogue (turn-in or new offer). */
int quest_talk(int npc) {
    quest_refresh();
    for (int q = 1; q <= Q_LAST; q++) {
        if (quests[q].giver != npc || quest_state[q] != QS_READY) continue;
        quest_state[q] = QS_DONE;
        reward(q);
        play_dialogue(npc_name(npc), quests[q].ready);
        return 1;
    }
    for (int q = 1; q <= Q_LAST; q++) {
        if (quests[q].giver != npc || !available(q)) continue;
        quest_state[q] = QS_ACTIVE;
        snd_sfx(SFX_MENU_OK);
        toast_show("New quest! See the tracker");
        play_dialogue(npc_name(npc), quests[q].offer);
        quest_refresh();
        return 1;
    }
    return 0;
}

/* what floats over an NPC's head: QM_READY (hand in), QM_NEW (offers a quest), QM_BUSY (waiting on you) or -1 */
int quest_marker(int npc) {
    int ready = 0, fresh = 0, busy = 0;
    for (int q = 1; q <= Q_LAST; q++) {
        if (quests[q].giver != npc) continue;
        if (quest_state[q] == QS_READY) ready = 1;
        else if (quest_state[q] == QS_ACTIVE) busy = 1;
        else if (available(q)) fresh = 1;
    }
    return ready ? QM_READY : (fresh ? QM_NEW : (busy ? QM_BUSY : -1));
}

/* reminder line for an NPC with an open quest (appended to their normal talk) */
const char *quest_reminder(int npc) {
    for (int q = 1; q <= Q_LAST; q++)
        if (quests[q].giver == npc && quest_state[q] == QS_ACTIVE) return quests[q].busy;
    return 0;
}

/* ---- quest log (Status tab, second page) */
int quest_log_rows(int *ids, int max) {
    int n = 0;
    for (int q = 1; q <= Q_LAST && n < max; q++)
        if (quest_state[q] == QS_ACTIVE || quest_state[q] == QS_READY) ids[n++] = q;
    return n;
}

int quest_done_count(void) {
    int n = 0;
    for (int q = 1; q <= Q_LAST; q++) n += quest_state[q] == QS_DONE;
    return n;
}

/* how many of an item open fetch quests still need (bulk selling leaves these alone) */
int quest_keep_count(int item) {
    int keep = 0;
    for (int q = 1; q <= Q_LAST; q++)
        if ((quest_state[q] == QS_ACTIVE || quest_state[q] == QS_READY) && quests[q].kind == QK_FETCH && quests[q].target == item && quests[q].need > keep)
            keep = quests[q].need;
    return keep;
}

const char *quest_title(int q) { return quests[q].title; }
const char *quest_hint(int q) { return quests[q].busy; }
int quest_is_ready(int q) { return quest_state[q] == QS_READY; }

/* "3/5" style progress; story quests show "Go!" until the objective is met */
void quest_progress_text(int q, char *buf) {
    const QuestDef *d = &quests[q];
    int cur = 0, need = d->need, k = 0;
    if (quest_state[q] == QS_READY) { buf[0] = 'O'; buf[1] = 'K'; buf[2] = 0; return; }
    if (d->kind == QK_KILL) cur = quest_prog[q];
    else if (d->kind == QK_FETCH) cur = pl.inv[d->target];
    else { buf[0] = '.'; buf[1] = '.'; buf[2] = '.'; buf[3] = 0; return; }
    if (cur > need) cur = need;
    if (cur >= 10) buf[k++] = (char)('0' + cur / 10);
    buf[k++] = (char)('0' + cur % 10);
    buf[k++] = '/';
    if (need >= 10) buf[k++] = (char)('0' + need / 10);
    buf[k++] = (char)('0' + need % 10);
    buf[k] = 0;
}


/* ---- on-screen tracker: the quest to follow (a quest waiting to be handed in first) */
int quest_tracked(void) {
    for (int q = 1; q <= Q_LAST; q++) if (quest_state[q] == QS_READY) return q;
    for (int q = 1; q <= Q_LAST; q++) if (quest_state[q] == QS_ACTIVE) return q;
    return 0;
}

/* "Wolf Hunt 3/5", "Hand in: Wolf Hunt", "Enter the crypt": fits the 24-tile-wide tracker line */
void quest_tracker_text(int q, char *buf) {
    const QuestDef *d = &quests[q];
    int k = 0;
    const char *src;
    char prog[8];
    if (quest_state[q] == QS_READY) {
        for (src = "Hand in: "; *src; src++) buf[k++] = *src;
        for (src = d->title; *src && k < 30; src++) buf[k++] = *src;
        buf[k] = 0;
        return;
    }
    src = d->kind == QK_FLAG ? d->goal : d->title;
    for (; *src && k < 24; src++) buf[k++] = *src;
    if (d->kind != QK_FLAG) {
        quest_progress_text(q, prog);
        buf[k++] = ' ';
        for (const char *p = prog; *p; p++) buf[k++] = *p;
    }
    buf[k] = 0;
}

const char *quest_giver_name(int q) { return npc_name(quests[q].giver); }

int quest_ready(int q) { return quest_state[q] == QS_READY; }
const char *quest_goal(int q) { return quests[q].goal; }

int quest_total(void) { return Q_LAST; }
