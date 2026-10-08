#include "game.h"
#include "assets.h"
#include "audio_data.h"

/* ---- instrument tables ---- */
typedef struct { u8 duty, vol, step, vdep, vspd, vdelay; } SqInst;
static const SqInst sq_inst[6] = {
    { 2, 11, 4, 2, 6, 10 },   /* 0 lead  */
    { 1, 12, 2, 0, 0, 0 },    /* 1 pluck */
    { 2, 7, 0, 3, 5, 12 },    /* 2 pad (sustained) */
    { 1, 10, 3, 0, 0, 0 },    /* 3 harp  */
    { 2, 12, 6, 2, 5, 8 },    /* 4 brass */
    { 0, 8, 4, 0, 0, 0 },     /* 5 soft  */
};
typedef struct { u8 bank, vol; } WvInst;
static const WvInst wv_inst[4] = { { 0, 1 }, { 0, 2 }, { 1, 1 }, { 1, 2 } };
typedef struct { u8 vol, step, r, s, w; } NzInst;
static const NzInst nz_inst[7] = {
    { 0, 0, 0, 0, 0 },
    { 15, 2, 7, 8, 0 },   /* kick */
    { 13, 1, 3, 5, 0 },   /* snare */
    { 8, 1, 2, 3, 1 },    /* hat */
    { 8, 3, 2, 3, 0 },    /* open hat */
    { 12, 2, 5, 7, 1 },   /* tom */
    { 11, 5, 3, 3, 0 },   /* crash */
};

typedef struct {
    const u8 *ptr, *loop;
    u8 remain, inst, active, vph, vdel, note, vdep, vspd;
    u16 freq;
} Chan;

static Chan ch[4];
static const SongDef *song;
static int song_id = -1;
static u8 spd, tick_n;
static u8 chan_en[4];
static int master_vol = 7;
static int fade_left, fade_total;
int music_on = 1;

typedef struct { const SfxDef *d; u16 left; u8 prio, busy; } Pcm;
static Pcm pcm[2];

static void apply_cnt(void) {
    u16 en = 0;
    for (int i = 0; i < 4; i++)
        if (chan_en[i]) en |= (u16)((1 << (8 + i)) | (1 << (12 + i)));
    REG_SNDCNT_L = music_on ? (u16)(en | master_vol | (master_vol << 4)) : 0;
}

void snd_init(void) {
    REG_SNDCNT_X = 0x80;
    REG_SNDCNT_H = 0x3302;
    REG_SNDCNT_L = 0x0077;
    REG_SND1CNT_L = 0x08;
    REG_SND3CNT_L = 0x40;
    for (int i = 0; i < 8; i++) REG_WAVERAM(i) = (u16)(wave_ram0[i * 2] | (wave_ram0[i * 2 + 1] << 8));
    REG_SND3CNT_L = 0x00;
    for (int i = 0; i < 8; i++) REG_WAVERAM(i) = (u16)(wave_ram1[i * 2] | (wave_ram1[i * 2 + 1] << 8));
    REG_SND3CNT_L = 0x80;
    REG_TM0CNT_L = 0xFC00;
    REG_TM0CNT_H = 0x80;
    for (int i = 0; i < 4; i++) chan_en[i] = 0;
    apply_cnt();
}

static void trig(int c, int note) {
    Chan *h = &ch[c];
    h->note = (u8)note;
    if (c < 2) {
        const SqInst *in = &sq_inst[h->inst > 5 ? 0 : h->inst];
        u16 env = (u16)((in->vol << 12) | (in->step << 8) | (in->duty << 6));
        u16 f = sq_freq[note & 127];
        if (c == 0) { REG_SND1CNT_H = env; REG_SND1CNT_X = (u16)(f | 0x8000); }
        else { REG_SND2CNT_L = env; REG_SND2CNT_H = (u16)(f | 0x8000); }
        h->freq = f;
        h->vph = 0;
        h->vdel = in->vdelay;
        h->vdep = in->vdep;
        h->vspd = in->vspd;
    } else if (c == 2) {
        const WvInst *in = &wv_inst[h->inst & 3];
        REG_SND3CNT_L = (u16)(0x80 | (in->bank << 6));
        REG_SND3CNT_H = (u16)(in->vol << 13);
        REG_SND3CNT_X = (u16)(wv_freq[note & 127] | 0x8000);
    } else {
        const NzInst *in = &nz_inst[note > 6 ? 1 : note];
        REG_SND4CNT_L = (u16)((in->vol << 12) | (in->step << 8));
        REG_SND4CNT_H = (u16)(0x8000 | (in->s << 4) | (in->w << 3) | in->r);
    }
}

static void fetch(int c) {
    Chan *h = &ch[c];
    for (int guard = 0; guard < 64; guard++) {
        u8 b = *h->ptr++;
        if (b >= 0xE0 && b < 0xF0) { h->inst = b & 15; continue; }
        if (b == 0xFD) { h->loop = h->ptr; continue; }
        if (b == 0xFE) { h->ptr = h->loop; continue; }
        if (b == 0xFF) { h->active = 0; chan_en[c] = 0; return; }
        u8 dur = *h->ptr++;
        h->remain = dur ? dur : 1;
        if (b == 0) chan_en[c] = 0;
        else { chan_en[c] = 1; trig(c, b); }
        return;
    }
}

void snd_stop_music(void) {
    song = 0;
    song_id = -1;
    for (int i = 0; i < 4; i++) chan_en[i] = 0;
    fade_left = 0;
    master_vol = 7;
    apply_cnt();
}

void snd_play_song(int id) {
    if (id == song_id && song) return;
    snd_stop_music();
    if (id < 0 || id >= SONG_COUNT) return;
    song = &song_table[id];
    song_id = id;
    spd = song->spd;
    tick_n = (u8)(spd - 1);
    for (int i = 0; i < 4; i++) {
        ch[i].ptr = song->tr[i];
        ch[i].loop = song->tr[i];
        ch[i].remain = 1;
        ch[i].inst = 0;
        ch[i].active = 1;
        ch[i].vdep = 0;
    }
    master_vol = 7;
    apply_cnt();
}

int snd_song_playing(void) { return song != 0; }

void snd_fade_out(int frames) {
    if (!song) return;
    fade_total = frames < 1 ? 1 : frames;
    fade_left = fade_total;
}

void snd_set_music(int on) {
    music_on = on;
    apply_cnt();
}

static void pcm_stop(int c) {
#ifndef HOST_BUILD
    if (c == 0) { REG_DMA1CNT_H = 0; REG_SNDCNT_H = 0x3302 | 0x0800; }
    else { REG_DMA2CNT_H = 0; REG_SNDCNT_H = 0x3302 | 0x8000; }
#endif
    pcm[c].busy = 0;
}

static void pcm_start(int c, const SfxDef *d) {
#ifndef HOST_BUILD
    if (c == 0) {
        REG_DMA1CNT_H = 0;
        REG_SNDCNT_H = 0x3302 | 0x0800;
        REG_DMA1SAD = (u32)(uintptr_t)d->data;
        REG_DMA1DAD = 0x040000A0;
        REG_DMA1CNT_H = 0xB640;
    } else {
        REG_DMA2CNT_H = 0;
        REG_SNDCNT_H = 0x3302 | 0x8000;
        REG_DMA2SAD = (u32)(uintptr_t)d->data;
        REG_DMA2DAD = 0x040000A4;
        REG_DMA2CNT_H = 0xB640;
    }
#endif
    pcm[c].d = d;
    pcm[c].busy = 1;
    pcm[c].prio = d->prio;
    pcm[c].left = (u16)(((u32)d->len * 15 >> 12) + 2);
}

void snd_sfx(int id) {
    if (id < 0 || id >= SFX_COUNT) return;
    const SfxDef *d = &sfx_table[id];
    int c = -1;
    if (!pcm[0].busy) c = 0;
    else if (!pcm[1].busy) c = 1;
    else {
        int lo = pcm[0].prio <= pcm[1].prio ? 0 : 1;
        if (pcm[lo].prio <= d->prio) c = lo;
    }
    if (c >= 0) pcm_start(c, d);
}

void snd_update(void) {
    for (int c = 0; c < 2; c++)
        if (pcm[c].busy && --pcm[c].left == 0) pcm_stop(c);
    if (!song) return;
    if (fade_left > 0) {
        fade_left--;
        master_vol = (fade_left * 7) / fade_total;
        if (fade_left == 0) { snd_stop_music(); return; }
        apply_cnt();
    }
    if (++tick_n >= spd) {
        tick_n = 0;
        for (int c = 0; c < 4; c++)
            if (ch[c].active && --ch[c].remain == 0) fetch(c);
        apply_cnt();
    }
    for (int c = 0; c < 2; c++) {
        Chan *h = &ch[c];
        if (h->active && chan_en[c] && h->vdep) {
            if (h->vdel) { h->vdel--; continue; }
            h->vph = (u8)(h->vph + h->vspd);
            int f = h->freq + ((isin(h->vph) * h->vdep) >> 8);
            if (c == 0) REG_SND1CNT_X = (u16)(f & 0x7FF);
            else REG_SND2CNT_H = (u16)(f & 0x7FF);
        }
    }
}
