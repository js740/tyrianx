# sim: the C++ game's state, and the game logic

`sim/` has the C++ game's state, the state that the C globals hold in the C
game, in structures, and the game logic that works on it.
Its level tick (`level_tick`) simulates a tick of the game's level loop, and
records the tick's drawing for `../render/tick/` to draw.

## The game state

`state.hpp` has the C++ game's state, `State`, by subsystem:
- `Game`: the game's modes and settings (two players, arcade, Galaga, the
  difficulty, the speed, demos).
- `Cheats`: the cheats and debug modes.
- `Progress`: where the game is: the episode, the level, the items and data
  cubes found, the level editor's items, and saved games.
- `Random`: the random number generator's state.
- `Explosions`: the explosions' slots, the repeating explosions (big explosions
  that set off explosions over several ticks), how far explosions move down
  each tick, and how far the player moved, for those that follow the player.
- `Superpixels`: sparks that move and fade.
- `Starfield`: the stars, and how much faster than their own speed they move.
- `Sound`: the samples to play at the end of the tick, and the music's fade.
- `Display`: the screen filter and its fades, the smoothies, the warnings,
  messages, the boss bars, and the HUD's port config buttons.
- `Map`: where each of the three map layers is and how it moves, layer by
  layer, and how the layers are drawn. The map data stays in the C globals.
- `Level`: the level's events and where the level is in them, the flags that
  destroyed enemies set for events, and what events set: random enemies, the
  level timer, Galaga mode's settings, and where the map screen goes next.
- `Ending`: how the level ends, game over, and which level comes next.
- `Input`: the buttons as the tick's input left them.
- `Players`: the players, their shots' timing, the generator's power and the
  shield's recharging, the charge weapon, two-player linking, the sidekicks'
  attachment and the ships' sprites.
- `Specials`: the special moves' timing and codes, the special weapon, and the
  specials that last a while (flares, the Zinglon and the astral).
- `Enemies`: the enemies' slots and what each holds, and how many there are on
  screen, in all and killed.
- `PlayerShots`: the player shots' slots, how many ticks each has left, and
  what the last hit on an enemy left (ice, and shots that pass through).
- `EnemyShots`: the enemy shots' slots, and which are free.

The slots of the events, explosions, superpixels, stars, boss bars, shots,
enemies and players are C++ structs (`Event`, `Explosion`,
`RepeatingExplosion`, `Superpixel`, `Star`, `BossBar`, `EnemyShot`,
`PlayerShot`, `Enemy`, `Player` with its `PlayerItems`) in place of the C
code's (`JE_EventRecType`, `rep_explosion_type` and so on). The C structs
left in the `State` are the random number generator's `MtState`, and the
sprite sheets that it points to.

The C code that runs between ticks and levels works on the C globals, which
`../c/state.hpp` copies the `State` from and back to (`../c/README.md`).
The C++ level tick works on the `State` that the code running it passes it.
The validation's shadow tick runs the level tick on a `State` of its own.

The `State` holds all the state that the validation's registry
(`../validate/registry.inc`) classes as gameplay or presentation state. The
game logic uses these from the C globals:
- The level and episode data loaded from files (`enemyDat`, `weapons`, the map
  data and the like). Events 49 to 52 change `enemyDat[0]`, the data of the
  enemies they create, which the `State` doesn't hold.
- The raw input that the tick reads, the scratch globals that its code writes
  before reading (`temp`, `tempW` and the like), the screens and sprites, the
  settings and the sound.

## The game logic

Each function here takes the state it works on explicitly: the subsystems it
works on, or the whole `State` for game logic that works on many (the
enemies). The level and episode data that it reads are in the C globals
(`enemyDat`, `weapons`). `hidden.hpp` hides the C globals that the `State`
holds, and the random number generator's functions that use them, from code
here and in `../render/`, so using one by mistake doesn't compile.

The files:
- `explosion.cpp`: `setup_explosion` (`JE_setupExplosion`) takes the explosion
  slots it fills in, and `setup_explosion_large` (`JE_setupExplosionLarge`) the
  `Explosions`. In the level loop, `update_repeating_explosions` sets off the
  repeating explosions' explosions, and `update_explosions` moves the
  explosions and returns the ones visible in the tick for its drawing.
- `superpixel.cpp`: `do_sp` (`JE_doSP`) creates superpixels, and
  `update_superpixels` moves and fades them and returns those it updated.
- `starfield.cpp`: `update_starfield` moves the stars.
- `hud.cpp`: the HUD's gauges and messages (`JE_drawShield`, `JE_drawArmor`
  and so on), which the game logic logs as it changes them (`hud.hpp`), for
  the tick's drawing. Some do game logic too: `hud_armor` (`JE_drawArmor`)
  limits the players' armor, `hud_text_window` (`JE_drawTextWindow`) sets when
  the message is erased, and `hud_options` (`JE_drawOptions`) sets up the
  sidekicks.
  The C code draws the shields and armor with `VGAScreen` set to the HUD's
  surface and leaves it at `game_screen` after, where the rest of the tick then
  draws; the game logic sets the draw record's surface there.
- `enemy.cpp`: `make_enemy` (`JE_makeEnemy`) makes an enemy in a slot, and
  `new_enemy` (`JE_newEnemy`) in the first free slot of a layer's.
  `update_enemies` (`JE_drawEnemy`, which "actually does a whole lot more than
  just drawing") updates a layer's enemies: each is animated, moves, fires and
  launches enemies. It returns the enemies it drew, as they were before they
  moved, for the tick's drawing. `hit_enemy` (from the level loop) is what a
  player shot does to the enemy it hits: it damages or destroys it, with the
  enemies linked to it.
- `events.cpp`: `update_events` runs the level's events that the level has
  reached (`JE_eventSystem`), and `event_jump` (`JE_eventJump`) jumps to a
  position in the level. Besides the `State`, the events change the C code's
  level data and resources, as the C code does: events 49 to 52 set the armor
  and graphic of enemy type 0 (`enemyDat[0]`) for the enemy they create, event
  5 loads the enemies' sprite sheets, and event 35 starts a song.
- `player_shot.cpp`: `create_player_shot` (`player_shot_create`) fires a
  weapon, and `set_player_shot_direction` (`player_shot_set_direction`) turns
  a linked weapon's shot. `update_player_shots` moves the player shots
  (`player_shot_move_and_draw`), and has them hit the enemies they collide
  with (`hit_enemy`) or set off chain reactions. It returns the shots it
  drew, for the tick's drawing.
- `player.cpp`: `update_players` runs the players' part of the level loop:
  their collisions with enemies (`JE_playerCollide`), which pick items up or
  damage both, then their movement, firing and sidekicks
  (`JE_mainGamePlayerFunctions` and `JE_playerMovement`), and the map's
  sideways scrolling, which follows them. It returns the players it drew, for
  the tick's drawing. It reads the raw input from the C globals and the demo
  file, as the C code does, in one place (`Movement::read_input`).
  `damage_player` (`JE_playerDamage`) damages a player's shield and armor, and
  `get_ship_info` (`JE_getShipInfo`) sets up the ships.
- `special.cpp`: the special moves: `sf_codes` (`JE_SFCodes`) follows the
  players' input through the ships' codes, and `do_special_shot`
  (`JE_doSpecialShot`) sets the specials off and runs those that last.
- `draw_record.hpp` and `visible.hpp`: what the tick's simulation records for
  its drawing (`DrawRecord`): which parts were drawn, on which surface, and the
  values that the simulation goes on to change after the C code drew them.
  The record names the surfaces (`Surface`), and keeps the one that the C
  code's `VGAScreen` points at.
- `enemy_shot.cpp`: `update_enemy_shots` moves the enemy shots and ends those
  that time out, leave the screen or hit a player. It returns the shots it
  drew.
- `level.cpp`: `level_tick` runs one tick of the level loop (from the
  `level_loop` label in `JE_main` to the jump back to it), in the C code's
  order, without its frame: the HUD's gauges, the events, the backgrounds and
  smoothies, the enemies, the player shots, the players, the enemy shots, the
  explosions, the warning, the sound routine, the texts, game over and the
  level's input, the superpixels, the filter and the boss bars, and the end of
  the level. It records the tick's drawing in the draw record, and asks the
  code running it (`Host`) for the C code's loops with their own drawing (the
  pause screen, the in-game menu and help). Besides the `State`, it changes
  the C code's sound and music, plays the sound queue, and reads the input for
  stopping demos, pausing, the menu and the key commands from the C globals.
  Network games' ticks run in C, so it has none of the C code's network
  play.
- `background.cpp`: the map's backgrounds (`background_1` to `background_3`),
  which `draw_background_2` and `draw_background_3` scroll as they draw, and
  `check_smoothies` (`JE_checkSmoothies`).
- `keyboard.cpp`: `main_keyboard_input` (`JE_mainKeyboardInput`), the key
  commands: edited ships, help, cheats, debug and screen settings, which read
  the keys from the C globals, and set some of the C code's settings (the debug
  display, the frame speed and the gamma).

The code here does what the C code does, in its order, but takes its state
explicitly, has C++ names, and is split into functions where the C code nests
deeply or jumps (`goto`). It keeps in local variables what the C code keeps
in scratch globals (`temp`, `b`, `tempW` and the like), as no code that runs
after it reads them before writing them, except for `temp2`, which carries a
value between functions and ticks, and is in the `State` (`Scratch`).
The validation's shadow tick checks it against the C code tick by tick
(`../validate/README.md`).

Functions that need random numbers call the reentrant C generator
(`mt_rand_r` etc.) on an `MtState` they are given; there is no C++ copy of it.
The level tick gives them the `State`'s. A function must draw the same values
in the same order as the C code. Including the `MtState` in the compared
state checks that.
