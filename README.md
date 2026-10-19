# Embervale: The Hollow Crypt  (Game Boy Advance)

A Zelda-style action RPG: 4 classes, real-time combo combat, stat points (level cap 40), five equipment slots,
8 outdoor areas (two towns) and 7 house interiors, crafting, a mini-boss and two big bosses, saving, and a fully original
chiptune soundtrack + sound effects.

## What's new in this build (world art, houses, quests, bosses)
* **Main quest and side quests.** The story is one chain (Wolf Hunt -> Into the Crypt -> Bone Warden -> Hollow King ->
  Cinder Stair -> Cinder Tyrant -> To Amberfall); everything else is a side quest. `!` and `?` over a head are **gold for
  the main story and blue for side quests**. The Quest tab lists the main quest on top (or where the story goes next when
  none is open) and side quests below; **A pins the highlighted quest** to the HUD tracker (`*`). The tracker is yellow for
  the main quest, cyan for side quests, green when it is ready to hand in, and has **no plate behind it any more** (a thin
  outline keeps it readable).
* **The south road is sealed** by a wall of ember-lit thorns until the **Cinder Tyrant** is dead. Dorn the gate guard, the
  Elder, the sign at the gate and Bram all say so; the gate opens by itself once the Tyrant falls. Sunmeadow Road,
  Amberwood and Amberfall are now the late game (enemy tiers 4 and 5, recommended level 22+).
* **Level cap 40** (the XP curve is unchanged up to level 20, then grows in even steps). Two new enemy tiers scale the
  Amber lands; their behaviour speed stops scaling after the Cinder Hollows, only their stats grow.
* **Enter the houses.** Embervale: Elder Maren's house, Sister Lyra's chapel, **Tobin's shop** (green roof, striped
  awning) and **Garrick's forge** (slate roof, stone walls, glowing window). Amberfall: Mara's inn, Wyn's alchemy and Orin's
  study. Each is a one-screen interior with furniture, the owner inside, and a door back out. The market stall is gone.
* **Walk behind roofs and trees.** The top row of every roof and the crown of every big oak are drawn over the hero but no
  longer block him: only the trunk, the wall and the lower roof do. While he stands under a crown the leaves fade to
  see-through so he never disappears.
* **Redrawn trees**: round-leaf oaks (shaded leaf clusters with apples, three crown shapes), tiered pines, bare trees.
  The map borders are rings of **whole 2x2 trees** (no more half trees or mixed species), exits line up with them.
* **Redrawn props**: boulders (the old "bear trap" tile was a rock drawn as a ring with grass inside), fences, planter
  boxes, bushes. The fountain is **3x3 tiles** with a wider rim and a taller spout. Dungeon **pillars are gone**.
* **HUD**: the **coin counter sits at the end of the EXP bar**, the **minimap owns the top-right corner** and is bigger and
  sharper (3 px per tile, textured terrain, canopies and roofs shown, rounded frame). The Map tab uses the same drawing.
* **Items tab is split into four areas**: Heal, Weapons, Armor and Items (trophies, materials, keys); **Left/Right** changes
  area, A uses or equips. **Every weapon, armor, helm and boots tier has its own icon** (item icons now live in ROM and
  only the ones on screen occupy sprite tiles).
* **Fewer chests (15 instead of 30) and much less free gear** (5 gear chests: one weapon per dungeon plus the Cinder
  plate). Armor, helms and boots come from the shops, the forge and quests.
* **Enemies no longer get stuck.** Spawns are placed on reachable tiles with room on all sides (the checker enforces it),
  and wolves, skeletons and slimes now walk around a trunk or rock that blocks them instead of pushing into it.
* **Every boss has its own moves.** *Bone Warden*: shield bash, club smash with a shockwave, triple stomps, **volleys of bone
  spears**, whirlwind and skeleton helpers. *Hollow King*: orb fans and rings, jumping slam, charges, orb rain, and the new
  **spectral curtain** (walls of orbs from alternating sides, each with a gap). *Cinder Tyrant* (own AI now): **flame
  breath** (fireballs sweeping an arc), **cinder rush** (charge with a trail of eruptions), **geyser columns** (eruptions
  with a gap), **encircling meteors**, a fire stomp when you hug it, and ember bats; it enrages at 65% and 30% health.

## What was new in the previous builds (crafting, Amber lands, skills, menu and map)
* **Quest clarity**: a `!` floats over anyone who has a quest for you, a `?` over anyone waiting for you to hand one
  in, and `...` while one of theirs is in progress. The tracked quest always shows at the top left of the HUD
  (`Wolf Hunt 3/5`, `Hand in: Wolf Hunt`). The pause menu has its own **Quest** tab (second tab) with the goal and giver.
* **Minimap** (top right): terrain, doors and warps, closed chests, signs and people (gold = quest giver) with a
  blinking hero marker. Toggle it in START > System.
* **Item icons** beside every item in the Items, Gear, shop and crafting lists, and **rarity colours**
  (Common white, Uncommon green, Rare cyan, Epic purple, Legendary orange) for all gear, charms and trophies.
* **Blacksmith and alchemist**: *Garrick* (Embervale, north-east yard) forges gear from trophies and **tempers** any
  weapon, armor, helm or boots you own (+1 to +3, shown as `Iron Blade +2`; the last level needs a Cinder Heart).
  *Wyn* (Amberfall) brews potions, elixirs and the Amber Charm. New materials: Iron Ore, Amber Shard, Cinder Heart.
* **Dungeon key reworked**: the Crypt Key is no longer in a chest. Ranger Bram, in the south-west of the woods,
  gives it for clearing the wolves around his camp (the Elder points you to him, and the minimap shows him).
* **12 new people** and 11 new quests across the village and the new towns.
* **New areas**: south gate of Embervale -> *Sunmeadow Road* -> *Amberwood* -> the town of **Amberfall** (inn, stall,
  alchemist, scholar). The east road out of Amberfall is closed by a rockfall: to be continued.
* **Cinder Hollows have their own creatures**: fire imps, ember wisps, cinder boars and slag golems, and a brand new
  **Cinder Tyrant** (a magma colossus, no longer the Warden with a recolour).
* **Sword reach now matches the animation**: longer blades and slash arcs, and a pie-slice hit test instead of a box.
* Redrawn art: pillars (two tiles tall now), chests, signs, lampposts, oaks, houses, bushes and cattails, and a gentle
  water animation that no longer flashes.

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
| START | Menu: Hero, Quest (A pins a quest to the tracker), Map, Stats, Skills, Items (Left/Right: Heal / Weapons / Armor / Items), Gear, Sys |
| SELECT | Opens the menu on the **Map** tab (the whole area) |
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
Five slots: **Weapon, Helm, Armor, Boots, Charm**. Weapons have five tiers per class (their rarity follows the tier:
Common, Uncommon, Rare, Epic, Legendary), armor, helms and boots five tiers each, plus eleven charms. Gear is rare in chests (one weapon in the woods, the crypt, the depths and the Cinder Hollows, plus the Cinder
plate) and sold by Tobin; his stock **grows with your level**. Tobin also **buys**:
switch to the **Sell** tab with L/R in his shop (good gear asks for a second A press, SELECT sells every spare
trophy while keeping what open quests need). Common enemies drop **trophies** (Slime Gel, Bat Wing, Wolf Pelt,
Bone Dust, Ember Core) for selling and quests. **Auto-equip** puts better gear on as you pick it up (toggle it in
System); **SELECT in the Equip tab** equips the best piece you own in every slot. Equipped weapon and armor change
how the hero looks (blade/orb colour by tier, armor tints the tunic). The Equip tab previews every
stat change before you commit.

## Goal and progression
1. Talk to Elder Maren. Grow strong in the village outskirts and the **Whispering Woods**
   (recommended level 3+). Help **Ranger Bram** in the south-west of the woods to earn the **Crypt Key**.
2. Open the iron door in the **Hollow Crypt** (recommended level 7+). Its west room hides a stairway
   down to the **Crypt Depths** (recommended level 11+).
3. Defeat the **Bone Warden** in the north-west arena of the depths; he drops the **Skull Key**.
4. Break the skull seal in the crypt corridor and face the **Hollow King**:
   three phases, a shield-and-reinforcements break at 2/3 and 1/3 health, wall-to-wall charges and
   a rain of orbs in the final phase. Enemies, loot and gold scale with the area you are in.
5. The adventure continues after the ending: a stairway behind the king's throne leads to the
   **Cinder Hollows** (recommended level 14+, the Tyrant around 20): a lava forge with scorched enemies, cinder-forged tier-5 gear and
   the **Cinder Tyrant**. The area is saved automatically when the credits end.

**The road south** is sealed until the Cinder Tyrant is dead. Then the south gate, Sunmeadow Road (recommended level 22+)
and Amberwood (level 25+) lead to the town of **Amberfall**: Mara's inn (heals), Brea's stall (better gear, elixirs), Wyn's
bench and Orin the scholar, whose quests end with a Cinder Heart. Beyond Amberfall the road east is still closed.

**Quests** (21 of them, from the Elder, Tobin, Lyra, Bram, Garrick, Dorn, Pip, Hale, Kess, Mara, Wyn, Orin and Brea):
wolf hunts, trophy fetches, the crypt, the Warden, the king, the Cinder Stair and the Tyrant. Open quests are tracked
automatically (HUD tracker, `!`/`?` markers, the Quests tab) and handed in by talking to the giver again. Boss rooms were enlarged, both bosses have new telegraphed attacks (ring warnings,
a charge-lane warning, meteor rain, stomps, whirlwind) and new multi-pose animation sheets.

Enemies respawn when you re-enter an area, so you can grind (level cap 40). Save from START > System.

## Tools
* `python3 tools/build_assets.py` regenerates every graphic/audio table (needs `numpy` and `Pillow`).
  Edit `tools/art_*.py` (sprites, tiles, maps), `tools/audio.py` (songs and sound effects).
  `art_world2.py` holds the village/forest props, grass variants and house details; `art_village.py` the redrawn
  fountain, well, lampposts, barrels, plaza bench and leafy oak canopies; `python3 tools/preview_props.py out.png 5`
  renders those props close up; `art_props.py` the hand-drawn
  chests, signs, lampposts, pillars and shrubs; `art_icons.py` item icons, quest markers and minimap blips;
  `art_ember.py` the Cinder Hollows' creatures and the Cinder Tyrant; `art_maps2.py` the road, Amberwood and Amberfall.
* `art_dungeon.py` flagstone floors, wall faces/caps/ledges, stairways, pillars and lava. `art_bosses.py` the multi-pose
  Hollow King and Bone Warden sheets. While you are in the Cinder Hollows the monster and boss sprite slots are
  overwritten with the Hollows' own sheets (`ember_mon_tiles`, `tyrant_tiles`).
  `audio.py` now builds 32-40 bar songs from sections (walking basses, arpeggio styles, drum fills, bridges).
* `art_world3.py` the redrawn trees, boulders, fences, planters, the forge and shop walls, the 3x3 fountain and the gate
  seal, and `Tileset.compact()` (in `art_world.py`) which drops unused metatiles per area so every tileset stays within the
  480-tile / 256-metatile limits. `art_interior.py` the interior tileset and the seven rooms.
* `python3 tools/check_maps.py` verifies that every chest, sign, door, warp and spawn is reachable, that spawns have room
  on all eight sides, that the sealed gate leaves the rest of the village connected, and that every interior works.
* `host/` is a PC test harness (not part of the ROM build): `host/run.sh host/t11.txt out/shot`
  runs the real game code with scripted input, dumps VRAM and renders PNG screenshots with a tiny
  PPU emulator (`tools/ppu.py`). `t11`-`t24` cover combos, menus, shop, both bosses, the ending flow and the save
  format (`savetest`, including migration of v2/v3/v4 saves to the v5 layout); `b1/b2/e1/e2/m1` show the king, warden,
  Cinder Tyrant, the throne stair and the menu/shop screens; `host/fuzz.py SEED FRAMES GOD` generates random-input stress scripts. Extra script commands: `quest ID STATE PROG`,
  `craft 0|1`, `inv ITEM`, `ehp` (prints every enemy's HP and distance). `t25_map_menu` shows the HUD tracker, the Map tab
  and the Hero/Quest tabs; `t26_skill_range` checks the Knight skill's reach; `t27_village_art` renders the plaza, well and yard;
  `t30_menu_tabs` the Quest tab and the Items categories; `t31_interiors` walking behind a roof and into a house;
  `t32_gate` the sealed and the opened south gate; `t33_all_interiors` every interior; `t34_canopy` the hero under a crown.

## Source layout
* `player.c` movement, skills, rolling, interaction, drawing - `combat.c` the combo tables and attack state machine
* `stats.c` classes, derived stats, stat points - `items.c` item table, rarity, icons, tempering, gear look, shop stocks
* `craft.c` the forge (make + temper) and the alchemist's bench
* `menu.c` pause menu and shop - `screens.c` title/class/rest/game over/ending - `ui.c` HUD, dialogue, toasts
* `enemy.c` tiered enemy stats, AI, boss hazards/telegraphs, the Bone Warden, Cinder Tyrant and Hollow King - `fx.c` projectiles, particles, pickups
* `quest.c` quest table, main/side split, tracking and dialogue - `passives.c` passive skills and their hooks - `save.c` save format 5 (loads v2/v3/v4)
