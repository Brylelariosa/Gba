# Embervale: The Hollow Crypt  (Game Boy Advance)

A Zelda-style action RPG: 4 classes, real-time combat, levels/stats, items & gear,
3 areas, a boss, saving, and a fully original chiptune soundtrack + sound effects.

## Build (your GitHub Actions workflow)
1. Delete the old `src/` folder in your repo, then copy this ZIP's contents over the repo root
   (`Makefile`, `src/`, `.github/workflows/build.yml`, `tools/`, ...).
2. Commit + push to `main`. The workflow runs `make` in the devkitARM container.
3. Download the `Embervale-ROM` artifact -> `Embervale.gba`.

Only `src/*.c` is compiled (the Makefile globs it). No external assets or libraries are needed:
all art, maps, music and sound effects are generated into `src/assets.c` / `src/audio_data.c`.

## Controls
| Button | Action |
|---|---|
| D-Pad | Move |
| A | Attack / talk / read / open chests |
| B | Class skill (costs MP) |
| R | Dodge roll (brief invincibility) |
| L | Quick potion |
| START | Menu: status, items, equipment, save, music on/off |

## Classes
* **Knight** - 3-hit sword combo. Skill: *Whirlwind*.
* **Mage** - arcane bolts. Skill: *Fireball* (explodes on impact).
* **Ranger** - fast arrows. Skill: *Multishot* (5-arrow fan).
* **Rogue** - rapid stabs, high luck/crit. Skill: *Shadow Step* (dash through enemies).

## Goal
Talk to Elder Maren in the village, find the **Crypt Key** (southwest of the Whispering Woods,
guarded by wolves), open the iron door deep in the Hollow Crypt and defeat the **Hollow King**.
Shop with Tobin (market stall), heal with Sister Lyra (chapel). Chests, bushes and enemies drop loot.
Enemies respawn when you re-enter an area, so you can grind levels (cap 20). Save from START > System.

## Tools
* `python3 tools/build_assets.py` regenerates every graphic/audio table (needs `numpy` and `Pillow`).
  Edit `tools/art_*.py` (sprites, tiles, maps), `tools/audio.py` (songs and sound effects).
* `host/` is a PC test harness (not part of the ROM build): `host/run.sh host/t2.txt out/shot`
  runs the real game code with scripted input, dumps VRAM and renders PNG screenshots with a tiny
  PPU emulator (`tools/ppu.py`).
