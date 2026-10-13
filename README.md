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
| START | Menu: Status (A = quest log), Stats, Skills, Items, Equip, System (save, music, auto-equip) |
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

## Class skills (B) and passives
Every class has a four-stage B skill (6-7 MP):

| Class | Skill | What it does |
|---|---|---|
| Knight | **Blade Storm** | four spins around you, then a heavy ground slam with a shockwave |
| Mage | **Inferno** | a fire nova to push enemies off, then two fans of piercing fire bolts |
| Ranger | **Volley Step** | hop backwards, loose a fan of arrows, then a piercing triple shot |
| Rogue | **Shadow Strike** | dash through enemies (invulnerable), then two slashes with +40% crit |

You earn one **passive point every second level** and spend it in the **Skills** tab (3 ranks each):
Vitality, Iron Skin, Keen Edge, Quick Feet, Scavenger (gold and loot), Lifedrinker (kills heal),
Spirit Link (hits restore MP) and a **class passive** (Knight: Bulwark, Mage: Arcane Mind,
Ranger: Fleetfoot, Rogue: Shadow Veil). The Skills tab previews what the next rank changes.

## Equipment
Five slots: **Weapon, Helm, Armor, Boots, Charm**. Weapons have four tiers per class, armor four
tiers, helms and boots four tiers each, plus eight charms. Gear is found in chests (woods, crypt, the
depths, Cinder Hollows) and sold by Tobin; his stock **grows with your level**. Tobin also **buys**:
switch to the **Sell** tab with L/R in his shop (good gear asks for a second A press, SELECT sells every spare
trophy while keeping what open quests need). Common enemies drop **trophies** (Slime Gel, Bat Wing, Wolf Pelt,
Bone Dust, Ember Core) for selling and quests. **Auto-equip** puts better gear on as you pick it up (toggle it in
System); **SELECT in the Equip tab** equips the best piece you own in every slot. Equipped weapon and armor change
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
5. The adventure continues after the ending: a stairway behind the king's throne leads to the
   **Cinder Hollows** (recommended level 14+): a lava forge with scorched enemies, cinder-forged tier-5 gear and
   the **Cinder Tyrant**. The area is saved automatically when the credits end.

**Quests** (Elder Maren, Tobin and Sister Lyra): wolf hunts, trophy fetches, the crypt, the Warden, the king, the
Cinder Stair and the Tyrant. Open quests are tracked automatically (Status tab, press A for the quest log) and handed
in by talking to the giver again. Boss rooms were enlarged, both bosses have new telegraphed attacks (ring warnings,
a charge-lane warning, meteor rain, stomps, whirlwind) and new multi-pose animation sheets.

Enemies respawn when you re-enter an area, so you can grind (level cap 20). Save from START > System.

## Tools
* `python3 tools/build_assets.py` regenerates every graphic/audio table (needs `numpy` and `Pillow`).
  Edit `tools/art_*.py` (sprites, tiles, maps), `tools/audio.py` (songs and sound effects).
  `art_world2.py` holds the village/forest props, grass variants and house details.
* `art_dungeon.py` flagstone floors, wall faces/caps/ledges, stairways, pillars and lava. `art_bosses.py` the multi-pose
  Hollow King and Bone Warden sheets (the Cinder Tyrant reuses the Warden sheet with a molten palette).
  `audio.py` now builds 32-40 bar songs from sections (walking basses, arpeggio styles, drum fills, bridges).
* `python3 tools/check_maps.py` verifies that every chest, sign, warp and spawn is reachable.
* `host/` is a PC test harness (not part of the ROM build): `host/run.sh host/t11.txt out/shot`
  runs the real game code with scripted input, dumps VRAM and renders PNG screenshots with a tiny
  PPU emulator (`tools/ppu.py`). `t11`-`t24` cover combos, menus, shop, both bosses, the ending flow and the save
  format (`savetest`, including migration of v2/v3 saves to the v4 layout); `b1/b2/e1/e2/m1` show the king, warden,
  Cinder Tyrant, the throne stair and the menu/shop screens; `host/fuzz.py SEED FRAMES GOD` generates random-input stress scripts.

## Source layout
* `player.c` movement, skills, rolling, interaction, drawing - `combat.c` the combo tables and attack state machine
* `stats.c` classes, derived stats, stat points - `items.c` item table, gear look, shop stock
* `menu.c` pause menu and shop - `screens.c` title/class/rest/game over/ending - `ui.c` HUD, dialogue, toasts
* `enemy.c` tiered enemy stats, AI, boss hazards/telegraphs, the Bone Warden, Cinder Tyrant and Hollow King - `fx.c` projectiles, particles, pickups
* `quest.c` quest table, tracking and dialogue - `passives.c` passive skills and their hooks - `save.c` save format 4 (loads v2/v3)
