# Embervale: The Hollow Crypt  (Game Boy Advance)

A Zelda-style action RPG: 4 classes, real-time combo combat, stat points, five equipment slots,
4 areas, a mini-boss and a three-phase final boss, saving, and a fully original chiptune
soundtrack + sound effects.

## Build (your GitHub Actions workflow)
1. Delete the old `src/` folder in your repo, then copy this ZIP's contents over the repo root
   (`Makefile`, `src/`, `.github/workflows/build.yml`, `tools/`, ...).
2. Commit + push to `main`. The workflow runs `make` in the devkitARM container.
3. Download the `Embervale-ROM` artifact -> `Embervale.gba`.

Only `src/*.c` is compiled (the Makefile globs it). No external assets or libraries are needed:
all art, maps, music and sound effects are generated into `src/assets.c` / `src/audio_data.c`.
Old save files keep working: a version-2 save loads into this version and receives its stat points
retroactively (3 per level gained).

## Controls
| Button | Action |
|---|---|
| D-Pad | Move |
| A | Attack: **tap to chain a combo, or hold to keep attacking** / talk / read / open chests |
| B | Class skill (costs MP) |
| R | Dodge roll (brief invincibility; also cancels the end of an attack) |
| L | Quick potion (potions share a short cooldown) |
| START | Menu: Status, Stats, Items, Equip, System (save, music) |
| L / R in the menu | Switch tab |

## Combos
Every class chains four different attacks. A press made during a swing is remembered and fires as
soon as the strike is over, so mashing A flows from one move to the next; holding A does the same.
The last move of the chain is a finisher with a bigger effect, a heavier sound and a pause before
the next chain. Attack speed scales with DEX.

| Class | 1 | 2 | 3 | 4 (finisher) |
|---|---|---|---|---|
| Knight | slash | reverse slash | long thrust | overhead slam + shockwave |
| Rogue | stab | stab (other hand) | cross cut (2 hits) | spinning flurry (3 hits around you) |
| Mage | bolt | bolt | 3-way fan | piercing arcane blast |
| Ranger | arrow | arrow | twin arrows | piercing power shot |

A "N HITS" counter appears under the gold while a chain is going.

## Stats
Each level gives **3 stat points** (class growth is gentler than before; the points are the rest).
Spend them in START > Stats (hold A to repeat; the right-hand panel previews every change).
Reset all points for `12 x level` gold.

| Stat | Effect |
|---|---|
| STR | Attack power (Knight, Ranger, Rogue) and knockback |
| DEX | Attack, move and roll speed, crit chance |
| VIT | Max HP (+6 each) and defense |
| INT | Spell power (Mage attack), MP (+2 each) and regeneration |
| LUK | Crit chance and item drops |

Defense is a percentage now (`damage x 64 / (64 + 2 x DEF)`), so it never makes you immune.

## Equipment
Five slots: **Weapon, Helm, Armor, Boots, Charm**. Weapons have four tiers per class, armor four
tiers, helms and boots four tiers each, plus eight charms. Gear is found in chests (woods, crypt, the
depths) and sold by Tobin; his stock **grows with your level**. Equipped weapon and armor change
how the hero looks (blade/orb colour by tier, armor tints the tunic). The Equip tab previews every
stat change before you commit.

## Goal and progression
1. Talk to Elder Maren. Grow strong in the village outskirts and the **Whispering Woods**
   (recommended level 3+). Find the **Crypt Key** in the southwest of the woods.
2. Open the iron door in the **Hollow Crypt** (recommended level 7+). Its west room hides a stairway
   down to the **Crypt Depths** (recommended level 11+).
3. Defeat the **Bone Warden** in the north-west arena of the depths; he drops the **Skull Key**.
4. Break the skull seal in the crypt corridor and face the **Hollow King**:
   three phases, a shield-and-reinforcements break at 2/3 and 1/3 health, wall-to-wall charges and
   a rain of orbs in the final phase. Enemies, loot and gold scale with the area you are in.

Enemies respawn when you re-enter an area, so you can grind (level cap 20). Save from START > System.

## Tools
* `python3 tools/build_assets.py` regenerates every graphic/audio table (needs `numpy` and `Pillow`).
  Edit `tools/art_*.py` (sprites, tiles, maps), `tools/audio.py` (songs and sound effects).
  `art_world2.py` holds the village/forest props, grass variants and house details.
* `python3 tools/check_maps.py` verifies that every chest, sign, warp and spawn is reachable.
* `host/` is a PC test harness (not part of the ROM build): `host/run.sh host/t11.txt out/shot`
  runs the real game code with scripted input, dumps VRAM and renders PNG screenshots with a tiny
  PPU emulator (`tools/ppu.py`). `t11`-`t22` cover combos, menus, shop, both bosses and the save
  format (`savetest`); `host/fuzz.py SEED FRAMES GOD` generates random-input stress scripts.

## Source layout
* `player.c` movement, skills, rolling, interaction, drawing - `combat.c` the combo tables and attack state machine
* `stats.c` classes, derived stats, stat points - `items.c` item table, gear look, shop stock
* `menu.c` pause menu and shop - `screens.c` title/class/rest/game over/ending - `ui.c` HUD, dialogue, toasts
* `enemy.c` tiered enemy stats, AI, the Bone Warden and the Hollow King - `fx.c` projectiles, particles, pickups
