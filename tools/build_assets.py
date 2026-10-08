"""Generate all game data into ../src (assets.c/.h, audio_data.c/.h).  Run: python3 tools/build_assets.py"""
import math
import os
import sys
import numpy as np

sys.path.insert(0, os.path.dirname(__file__))
from common import (Canvas, c15, hx, tiles_of, flat, u32arr, u16arr, u8arr, s16arr, pal16)
import art_chars as AC
import art_objs as AO
import art_font as AF
import art_world as AW
import art_maps as AM
import art_title as AT
import audio as AU

SRC = os.path.join(os.path.dirname(__file__), '..', 'src')
CLASSES = ['knight', 'mage', 'ranger', 'rogue']


def pal_words(p):
    return [c15(c) for c in p]


def main():
    H = []      # header lines
    C = ['#include "game.h"', '#include "assets.h"', '#include "audio_data.h"', '']

    # ---------------------------------------------------------------- hero frames per class
    hero_blobs = []
    for cn in CLASSES:
        hf = AC.hero_frames(cn)
        frames = []
        for d in (AC.DOWN, AC.UP, AC.SIDE):
            frames += hf['walk'][d]
        for d in (AC.DOWN, AC.UP, AC.SIDE):
            frames += hf['atk'][d]
        frames += hf['roll']
        for d in (AC.DOWN, AC.UP, AC.SIDE):
            frames.append(hf['hurt'][d])
        frames += hf['dead']
        assert len(frames) == 37
        hero_blobs.append(flat([t for f in frames for t in tiles_of(f.a)]))
    H.append('#define HERO_FRAMES 37')
    for nm, v in (('WALK_DOWN', 0), ('WALK_UP', 6), ('WALK_SIDE', 12), ('ATK_DOWN', 18), ('ATK_UP', 21), ('ATK_SIDE', 24),
                  ('ROLL', 27), ('HURT_DOWN', 31), ('HURT_UP', 32), ('HURT_SIDE', 33), ('DEAD', 34)):
        H.append('#define HF_%s %d' % (nm, v))
    for i, b in enumerate(hero_blobs):
        C.append(u32arr('hero_tiles_%d' % i, b))
    C.append('const u32 *const hero_tiles[4] = { hero_tiles_0, hero_tiles_1, hero_tiles_2, hero_tiles_3 };')
    C.append('const u16 hero_pal[4][16] = {')
    for cn in CLASSES:
        C.append('  {' + ','.join('0x%04x' % v for v in pal_words(AC.STYLES[cn]['pal'])) + '},')
    C.append('};')

    # ---------------------------------------------------------------- common OBJ tiles
    common = []          # list of tiles (each 8 words)
    OT = {}
    base = 148

    def alloc(name, frames, w=None):
        nonlocal base
        OT[name] = base
        n = 0
        for f in frames:
            ts = tiles_of(f.a)
            common.extend(ts)
            n += len(ts)
        base += n
        H.append('#define OT_%s %d' % (name, OT[name]))

    alloc('WEAPON', [AO.weapon(k) for k in ('sword', 'staff', 'bow', 'dagger')])
    alloc('ARC', [AO.arc_sprite()])
    alloc('SHADOW', [AO.shadow(True)])
    alloc('SHADOW_S', [AO.shadow(False)])
    alloc('SLIME', [AO.slime(i) for i in range(4)])
    alloc('BAT', [AO.bat(i) for i in range(3)])
    alloc('WOLF', [AO.wolf(i) for i in range(4)])
    sk = AC.skel_frames()
    sf = []
    for d in (AC.DOWN, AC.UP, AC.SIDE):
        sf += sk['walk'][d]
    sf += [sk['atk'][d] for d in (AC.DOWN, AC.UP, AC.SIDE)]
    alloc('SKEL', sf)
    alloc('BOSS', [AO.boss(i) for i in range(5)])
    npc = []
    for st in ('elder', 'merchant', 'healer'):
        npc += AC.npc_frames(st)
    alloc('NPC', npc)
    alloc('ARROW', [AO.arrow()])
    alloc('BOLT', [AO.bolt(0), AO.bolt(1)])
    alloc('FIRE', [AO.fireball(0), AO.fireball(1)])
    alloc('ORB', [AO.orb(0), AO.orb(1)])
    alloc('EXPL', [AO.explosion(i) for i in range(4)])
    parts = []
    pdefs = [('sparkle', 4), ('smoke', 4), ('leaf', 2), ('hit', 3), ('dust', 3), ('ember', 3), ('heal', 3)]
    off = 0
    for (k, n) in pdefs:
        H.append('#define PT_%s %d' % (k.upper(), off))
        H.append('#define PT_%s_N %d' % (k.upper(), n))
        for i in range(n):
            parts.append(AO.particle(k, i))
        off += n
    alloc('PART', parts)
    alloc('NUM', [AO.digit(str(i)) for i in range(10)])
    items = [AO.item('coin', i) for i in range(4)] + [AO.item(k) for k in ('heart', 'mana', 'potion', 'ether', 'key')]
    alloc('ITEM', items)
    for i, nm in enumerate(('HEART', 'MANA', 'POTION', 'ETHER', 'KEY')):
        H.append('#define IT_%s %d' % (nm, 4 + i))
    alloc('CURSOR', [AO.ui_sprite('cursor', 0), AO.ui_sprite('cursor', 1)])
    alloc('PROMPT', [AO.ui_sprite('prompt', 0), AO.ui_sprite('prompt', 1)])
    alloc('ALERT', [AO.ui_sprite('alert', 0), AO.ui_sprite('alert', 1)])
    alloc('STAR', [AO.ui_sprite('star', i) for i in range(4)])
    H.append('#define OT_TOTAL %d' % base)
    H.append('#define OT_COMMON 148')
    H.append('#define OT_COMMON_TILES %d' % (base - 148))
    C.append(u32arr('obj_common_tiles', flat(common)))

    # frame index constants
    for nm, v in (('SKF_WALK_D', 0), ('SKF_WALK_U', 4), ('SKF_WALK_S', 8), ('SKF_ATK_D', 12), ('SKF_ATK_U', 13), ('SKF_ATK_S', 14)):
        H.append('#define %s %d' % (nm, v))

    # ---------------------------------------------------------------- palettes
    obj_pal = [[0] * 16 for _ in range(16)]
    obj_pal[0] = pal_words(AC.STYLES['knight']['pal'])
    obj_pal[1] = pal_words(AO.PAL_WEAPON)
    obj_pal[2] = pal_words(AO.PAL_SLIME_G)
    obj_pal[3] = pal_words(AO.PAL_SLIME_B)
    obj_pal[4] = pal_words(AO.PAL_BAT)
    obj_pal[5] = pal_words(AC.STYLES['skeleton']['pal'])
    obj_pal[6] = pal_words(AO.PAL_WOLF)
    obj_pal[7] = pal_words(AO.PAL_BOSS)
    obj_pal[8] = pal_words(AC.STYLES['elder']['pal'])
    obj_pal[9] = pal_words(AC.STYLES['merchant']['pal'])
    obj_pal[10] = pal_words(AC.STYLES['healer']['pal'])
    obj_pal[11] = pal_words(AO.PAL_FX)
    obj_pal[12] = pal_words(AO.PAL_ITEM)
    obj_pal[13] = pal_words(AO.PAL_UI)
    obj_pal[14] = pal_words(pal16(['#ff5050', '#401020']))
    obj_pal[15] = pal_words(AO.PAL_FLASH)
    C.append(u16arr('obj_pal_init', [v for b in obj_pal for v in b], 16))
    H.append('#define PB_HERO 0\n#define PB_WEAPON 1\n#define PB_SLIME 2\n#define PB_SLIME2 3\n#define PB_BAT 4\n#define PB_SKEL 5\n#define PB_WOLF 6\n#define PB_BOSS 7')
    H.append('#define PB_NPC0 8\n#define PB_FX 11\n#define PB_ITEM 12\n#define PB_UI 13\n#define PB_NUMR 14\n#define PB_FLASH 15')

    # UI BG palettes (banks 12..15)
    ui12 = pal16(['#10101c', '#40405c', '#e04058', '#ff8090', '#901838', '#4080e8', '#80c0ff', '#2048a0', '#f0c040', '#fff080', '#e08030', '#ffffff', '#f8c820', '#a07010', '#a0a8d0'])
    ui13 = pal16(['#ffffff', '#202448', '#ffe060', '#ff6070', '#70e080', '#70d0ff', '#a0a8c0', '#1c2450', '#d0d8f8', '#7080c8', '#303870', '#ffa040', '#c090ff', '#6c7490', '#080818'])
    ui14 = pal16(['#ffffff', '#300818', '#ffe060', '#ff7080', '#70e080', '#70d0ff', '#a0a8c0', '#2c0c18', '#f8d0d8', '#c05068', '#601828', '#ffa040', '#c090ff', '#7c5060', '#080818'])
    ui15 = pal16(['#ffffff', '#101010', '#ffe060', '#ff6070', '#70e080', '#70d0ff', '#a0a8c0', '#000000', '#d0d8f8', '#7080c8', '#303870', '#ffa040', '#c090ff', '#6c7490', '#080818'])
    C.append(u16arr('ui_pal', pal_words(ui12) + pal_words(ui13) + pal_words(ui14) + pal_words(ui15), 16))

    # ---------------------------------------------------------------- UI tiles
    ui_tiles = [[0] * 8]
    UT = {}

    def uadd(name, arr):
        UT[name] = len(ui_tiles)
        ui_tiles.append(tiles_of(arr)[0])
        H.append('#define UT_%s %d' % (name, UT[name]))

    def frame_tile(kind):
        a = np.zeros((8, 8), np.uint8)
        a[:, :] = 8
        BD, BM, BL = 11, 10, 9   # dark, mid, light
        def edge_top():
            a[0, :] = BD
            a[1, :] = BM
            a[2, :] = BL
        if kind in ('T', 'TL', 'TR'):
            edge_top()
        if kind in ('B', 'BL', 'BR'):
            a[7, :] = BD
            a[6, :] = BM
            a[5, :] = BL
        if kind in ('L', 'TL', 'BL'):
            a[:, 0] = BD
            a[:, 1] = BM
            a[:, 2] = BL
        if kind in ('R', 'TR', 'BR'):
            a[:, 7] = BD
            a[:, 6] = BM
            a[:, 5] = BL
        if kind in ('T', 'TL', 'TR'):
            a[2, :] = BL
        # round corners
        if kind == 'TL':
            a[0, 0:3] = 0; a[1, 0:2] = 0; a[2, 0] = 0
        if kind == 'TR':
            a[0, 5:8] = 0; a[1, 6:8] = 0; a[2, 7] = 0
        if kind == 'BL':
            a[7, 0:3] = 0; a[6, 0:2] = 0; a[5, 0] = 0
        if kind == 'BR':
            a[7, 5:8] = 0; a[6, 6:8] = 0; a[5, 7] = 0
        return a
    uadd('FILL', frame_tile('F'))
    for k in ('TL', 'T', 'TR', 'L', 'R', 'BL', 'B', 'BR'):
        uadd('FRAME_' + k, frame_tile(k))
    # icons in bank 13: coin / key / potion / arrows
    def icon(rows, key):
        a = np.zeros((8, 8), np.uint8)
        for y, r in enumerate(rows):
            for x, ch in enumerate(r):
                if ch != '.':
                    a[y, x] = key[ch]
        return a
    uadd('COIN', icon(["..AAAA..", ".ABBBBA.", "ABBCBBBA", "ABBCBBBA", "ABBCBBBA", "ABBBBBBA", ".ABBBBA.", "..AAAA.."], {'A': 12, 'B': 3, 'C': 1}))
    uadd('KEY', icon(["........", ".AAA....", ".A.A....", ".AAA....", "..A.....", "..AA....", "..A.....", "..AA...."], {'A': 2}))
    uadd('POTION', icon(["...AA...", "...BB...", "..CCCC..", ".CDDDDC.", ".CDEDDC.", ".CDDDDC.", "..CCCC..", "........"], {'A': 14, 'B': 1, 'C': 1, 'D': 4, 'E': 1}))
    uadd('ARROW_UP', icon(["........", "...AA...", "..AAAA..", ".AAAAAA.", "........", "........", "........", "........"], {'A': 1}))
    uadd('ARROW_DN', icon(["........", "........", "........", "........", ".AAAAAA.", "..AAAA..", "...AA...", "........"], {'A': 1}))
    uadd('HEART', icon(["........", ".AA.AA..", "AAAAAAA.", "AAAAAAA.", ".AAAAA..", "..AAA...", "...A....", "........"], {'A': 4}))
    H.append('#define UT_POOL 32')
    pad = 32 - len(ui_tiles)
    ui_tiles += [[0] * 8] * pad
    C.append(u32arr('ui_tiles', flat(ui_tiles)))
    H.append('#define UI_TILES_N %d' % len(ui_tiles))

    # ---------------------------------------------------------------- font
    fw, fr = AF.font_tables()
    C.append(u8arr('font_w', fw))
    C.append(u8arr('font_rows', fr, 32))

    # ---------------------------------------------------------------- sin table
    C.append(s16arr('sin256', [int(round(math.sin(2 * math.pi * i / 256.0) * 256)) for i in range(256)]))

    # ---------------------------------------------------------------- title
    timg = AT.build_title()
    ttiles, tents = AT.tileize(timg)
    tmap = {tuple(t): i for i, t in enumerate(ttiles)}
    simg = AT.build_title(logo=False)
    ttiles, sents = AT.tileize(simg, ttiles, tmap)
    C.append(u32arr('title_tiles', flat(ttiles)))
    C.append(u16arr('title_map', tents))
    C.append(u16arr('scene_map', sents))
    C.append(u16arr('title_pal', pal_words(AT.TITLE_PAL)))
    H.append('#define TITLE_NTILES %d' % len(ttiles))

    # ---------------------------------------------------------------- tilesets & areas
    outdoor = AW.build_outdoor()
    dungeon = AW.build_dungeon()
    village = AM.build_village(outdoor)
    forest = AM.build_forest(outdoor)
    crypt = AM.build_crypt(dungeon)

    def emit_ts(ts, name):
        C.append(u32arr('ts_%s_tiles' % name, flat(ts.tiles)))
        C.append(u16arr('ts_%s_pal' % name, [c15(c) for c in ts.palette()], 16))
        C.append(u16arr('ts_%s_meta' % name, [e for m in ts.meta for e in m], 12))
        C.append(u8arr('ts_%s_flags' % name, ts.flags))
        cy = []
        for (b, s, n, per) in ts.cycles:
            cy += [b, s, n, per]
        C.append(u8arr('ts_%s_cycles' % name, cy or [0]))
        return len(ts.tiles), len(ts.meta), len(ts.cycles)
    nt0 = emit_ts(outdoor, 'outdoor')
    nt1 = emit_ts(dungeon, 'dungeon')
    assert nt0[0] <= 480 and nt1[0] <= 480, (nt0, nt1)
    C.append('const Tileset tilesets[2] = {')
    C.append('  { ts_outdoor_tiles, %d, ts_outdoor_pal, ts_outdoor_meta, ts_outdoor_flags, %d, ts_outdoor_cycles, %d, %d },' % (nt0 + (outdoor.id('chest_open'),)))
    C.append('  { ts_dungeon_tiles, %d, ts_dungeon_pal, ts_dungeon_meta, ts_dungeon_flags, %d, ts_dungeon_cycles, %d, %d },' % (nt1 + (dungeon.id('chest_open'),)))
    C.append('};')

    areas = [('village', village, 0, 'SONG_VILLAGE', 1, 'Embervale'), ('forest', forest, 0, 'SONG_FOREST', 2, 'Whispering Woods'), ('crypt', crypt, 1, 'SONG_CRYPT', 3, 'Hollow Crypt')]
    for (nm, m, tsi, song, amb, title) in areas:
        flatg = [v for row in m.ground for v in row]
        flato = [v for row in m.overlay for v in row]
        C.append(u8arr('map_%s_g' % nm, flatg, 32))
        C.append(u8arr('map_%s_o' % nm, flato, 32))
        C.append('static const SpawnDef sp_%s[] = {%s};' % (nm, ','.join('{%s,%d,%d,%d}' % s for s in m.spawns) or '{0,0,0,0}'))
        C.append('static const NpcDef np_%s[] = {%s};' % (nm, ','.join('{%s,%d,%d,%d}' % s for s in m.npcs) or '{0,0,0,0}'))
        C.append('static const ChestDef ch_%s[] = {%s};' % (nm, ','.join('{%d,%d,%s,%d,%d}' % (c[0], c[1], c[2], c[3], c[4]) for c in m.chests) or '{0,0,0,0,0}'))
        C.append('static const WarpDef wp_%s[] = {%s};' % (nm, ','.join('{%d,%d,%d,%d,%d,%d,%d,%d}' % w for w in m.warps) or '{0,0,0,0,0,0,0,0}'))
        C.append('static const SignDef sg_%s[] = {%s};' % (nm, ','.join('{%d,%d,"%s"}' % s for s in m.signs) or '{0,0,""}'))
    C.append('const AreaDef areas[3] = {')
    for (nm, m, tsi, song, amb, title) in areas:
        C.append('  { "%s", map_%s_g, map_%s_o, sp_%s, np_%s, ch_%s, wp_%s, sg_%s, %d,%d,%d,%d,%d, %d,%s,%d,%d,%d },' % (
            title, nm, nm, nm, nm, nm, nm, nm, len(m.spawns), len(m.npcs), len(m.chests), len(m.warps), len(m.signs), tsi, song, amb, m.start[0], m.start[1]))
    C.append('};')

    # ---------------------------------------------------------------- write
    hdr = ['#ifndef ASSETS_H', '#define ASSETS_H', '#include "hw.h"', '']
    hdr += H
    hdr += ['',
            'extern const u32 *const hero_tiles[4];', 'extern const u16 hero_pal[4][16];', 'extern const u32 obj_common_tiles[];',
            'extern const u16 obj_pal_init[256];', 'extern const u16 ui_pal[64];', 'extern const u32 ui_tiles[];',
            'extern const u8 font_w[95];', 'extern const u8 font_rows[95 * 8];', 'extern const s16 sin256[256];',
            'extern const u32 title_tiles[];', 'extern const u16 title_map[600];', 'extern const u16 scene_map[600];', 'extern const u16 title_pal[16];', '', '#endif', '']
    open(os.path.join(SRC, 'assets.h'), 'w').write('\n'.join(hdr))
    open(os.path.join(SRC, 'assets.c'), 'w').write('\n'.join(C) + '\n')
    tot = AU.emit_audio(os.path.join(SRC, 'audio_data.c'), os.path.join(SRC, 'audio_data.h'))
    print('outdoor: tiles %d meta %d | dungeon: tiles %d meta %d | obj tiles %d | title tiles %d | sfx bytes %d' % (
        nt0[0], nt0[1], nt1[0], nt1[1], base, len(ttiles), tot))
    # previews for debugging
    if '--preview' in sys.argv:
        AM.preview('/tmp/map_village.png', village, outdoor, 1)
        AM.preview('/tmp/map_forest.png', forest, outdoor, 1)
        AM.preview('/tmp/map_crypt.png', crypt, dungeon, 1)


if __name__ == '__main__':
    main()
