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
} QuestDef;

enum { Q_NONE, Q_WOLVES, Q_GEL, Q_WINGS, Q_CRYPT, Q_BONES, Q_WARDEN, Q_KING, Q_EMBER, Q_TYRANT, Q_CORES };

static const QuestDef quests[QUEST_COUNT] = {
    [Q_NONE] = { "", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "", "", "" },
    [Q_WOLVES] = { "Wolf Hunt", NPC_ELDER, QK_KILL, EN_WOLF, 5, 2, Q_NONE, 0, 100, 60, ITEM_HIPOTION, 2,
        "Wolves out of the Whispering Woods have grown bold and hungry. They bar the road to the crypt.|Cull five of them and the woods will breathe again.",
        "Five wolves fewer! The woods are quieter already. Take this, with my thanks.",
        "The wolves still prowl the woods. Hunt five of them." },
    [Q_GEL] = { "Sticky Gel", NPC_MERCHANT, QK_FETCH, ITEM_GEL, 6, 1, Q_NONE, 0, 80, 30, ITEM_ETHER, 2,
        "Hey, adventurer! Slimes drop a gel I boil into glue and lamp oil. Bring me six Slime Gel and I'll pay well!",
        "Sticky, smelly, perfect! Here's your coin, and a couple of Ethers on the house.",
        "Slimes in the woods carry the gel. I need six." },
    [Q_WINGS] = { "Bat Tonic", NPC_HEALER, QK_FETCH, ITEM_WING, 5, 3, Q_NONE, 0, 60, 50, ITEM_ELIXIR, 1,
        "Bat wings make a fine tonic against the crypt's chill. Could you bring me five Bat Wings?",
        "These will brew a fine tonic. Take this Elixir. May it save you in dark places.",
        "Bats nest in the woods and the crypt. Five wings, please." },
    [Q_CRYPT] = { "Into the Crypt", NPC_ELDER, QK_FLAG, 0, 0, 5, Q_WOLVES, GF_SEEN_CRYPT, 150, 120, ITEM_CHARM_GUARD, 1,
        "Now to the Hollow Crypt itself. Step inside and see what stirs there. Do not linger yet.|Long ago the Ember King, Aldric, bound his soul to a crown to cheat death. The crypt is his tomb.",
        "You stood in Aldric's tomb and lived. Then you are ready. This charm was my husband's.",
        "Find the Hollow Crypt beyond the woods and step inside." },
    [Q_BONES] = { "Bone Dust", NPC_MERCHANT, QK_FETCH, ITEM_BONE, 8, 7, Q_GEL, 0, 250, 90, ITEM_HIPOTION, 3,
        "Skeletons crumble into fine bone dust. Farmers swear by it as fertilizer! Eight Bone Dust, please.",
        "Eight! Rattling fine work. The farmers will pay me double. Here's your cut.",
        "Skeletons in the crypt drop Bone Dust. I need eight." },
    [Q_WARDEN] = { "The Warden", NPC_ELDER, QK_FLAG, 0, 0, 9, Q_CRYPT, GF_WARDEN_DEAD, 300, 200, ITEM_ELIXIR, 1,
        "Below the crypt stands Sir Corvin, the Bone Warden. Aldric's oldest captain, he guards the Skull Key.|He was a good man once. Free him from that duty.",
        "You freed Corvin. Rest, old friend... You have my deepest thanks, hero.",
        "Take the crypt's west stairs down and defeat the Bone Warden." },
    [Q_KING] = { "Hollow King", NPC_ELDER, QK_FLAG, 0, 0, 12, Q_WARDEN, GF_BOSS_DEAD, 600, 400, ITEM_ELIXIR, 2,
        "The Skull Key opens the seal to Aldric's throne. End his hollow reign, hero. Embervale is counting on you.",
        "The Hollow King is gone, truly gone. Embervale will sing of you for a hundred years!",
        "Break the skull seal and face the Hollow King." },
    [Q_EMBER] = { "Cinder Stair", NPC_ELDER, QK_FLAG, 0, 0, 12, Q_KING, GF_SEEN_EMBER, 400, 300, ITEM_CHARM_PHOENIX, 1,
        "But the crown still burns. It was forged below the old capital, in the Cinder Hollows.|A stair behind Aldric's throne now stands open. Go down, hero, and see what feeds the fire.",
        "You walked the Cinder Hollows? Then you know what lurks there... a tyrant of living flame.",
        "Take the stair behind the king's throne down to the Cinder Hollows." },
    [Q_TYRANT] = { "Cinder Tyrant", NPC_ELDER, QK_FLAG, 0, 0, 14, Q_EMBER, GF_TYRANT_DEAD, 1000, 600, ITEM_CHARM_EMBER, 1,
        "The Cinder Tyrant is the forge-spirit that kept Aldric's crown alight. Quench it and the curse ends for good.",
        "The fire is out. The curse is broken. The Hollow King sleeps at last. You are Embervale's greatest hero!",
        "Defeat the Cinder Tyrant in the heart of the Hollows." },
    [Q_CORES] = { "Ember Cores", NPC_MERCHANT, QK_FETCH, ITEM_EMBER, 4, 13, Q_EMBER, 0, 700, 300, ITEM_ELIXIR, 2,
        "Ember Cores! Rumor says they burn forever. Bring me four from the Cinder Hollows and I'll pay a king's ransom.",
        "Marvelous! Warm as a hearth! Take this, and my finest thanks.",
        "Creatures of the Cinder Hollows carry Ember Cores. Four, please." },
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
    for (int q = 1; q < QUEST_COUNT; q++) {
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
    for (int q = 1; q < QUEST_COUNT; q++)
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
    for (int q = 1; q < QUEST_COUNT; q++) {
        if (quests[q].giver != npc || quest_state[q] != QS_READY) continue;
        quest_state[q] = QS_DONE;
        reward(q);
        play_dialogue(npc == NPC_ELDER ? "Elder Maren" : (npc == NPC_MERCHANT ? "Tobin" : "Sister Lyra"), quests[q].ready);
        return 1;
    }
    for (int q = 1; q < QUEST_COUNT; q++) {
        if (quests[q].giver != npc || !available(q)) continue;
        quest_state[q] = QS_ACTIVE;
        snd_sfx(SFX_MENU_OK);
        toast_show("New quest!");
        play_dialogue(npc == NPC_ELDER ? "Elder Maren" : (npc == NPC_MERCHANT ? "Tobin" : "Sister Lyra"), quests[q].offer);
        quest_refresh();
        return 1;
    }
    return 0;
}

/* reminder line for an NPC with an open quest (appended to their normal talk) */
const char *quest_reminder(int npc) {
    for (int q = 1; q < QUEST_COUNT; q++)
        if (quests[q].giver == npc && quest_state[q] == QS_ACTIVE) return quests[q].busy;
    return 0;
}

/* ---- quest log (Status tab, second page) */
int quest_log_rows(int *ids, int max) {
    int n = 0;
    for (int q = 1; q < QUEST_COUNT && n < max; q++)
        if (quest_state[q] == QS_ACTIVE || quest_state[q] == QS_READY) ids[n++] = q;
    return n;
}

int quest_done_count(void) {
    int n = 0;
    for (int q = 1; q < QUEST_COUNT; q++) n += quest_state[q] == QS_DONE;
    return n;
}

/* how many of an item open fetch quests still need (bulk selling leaves these alone) */
int quest_keep_count(int item) {
    int keep = 0;
    for (int q = 1; q < QUEST_COUNT; q++)
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
