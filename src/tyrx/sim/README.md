# sim: the C++ game's state, and game logic that owns its state

`sim/` is where the game's state goes as it moves out of the C globals into
structures, and the game logic that works on it.

## The game state

`state.hpp` has the C++ game's state, `State`: the state that C globals hold in
the C game, moved into structures, a subsystem at a time. So far it has:
- `Game`: the game's modes and settings (two players, arcade, Galaga, the
  difficulty, the speed, demos).
- `Cheats`: the cheats and debug modes.
- `Progress`: where the game is: the episode, the level, and the items and data
  cubes found.
- `Random`: the random number generator's state.
- `Explosions`: the explosions' slots, the repeating explosions (big explosions
  that set off explosions over several ticks), how far explosions move down
  each tick, and how far the player moved, for those that follow the player.
- `Superpixels`: sparks that move and fade.
- `Starfield`: the stars, and how much faster than their own speed they move.
- `Sound`: the samples to play at the end of the tick, and the music's fade.
- `Display`: the screen filter and its fades, the smoothies, flashes, warnings
  and texts, the boss bars, and the HUD's port config buttons.
- `Map`: where each of the three map layers is and how it moves, and how the
  layers are drawn. The map data stays in the C globals.
- `Level`: the level's events and where the level is in them, and what events
  set: random enemies, the level timer, Galaga mode's settings, and where the
  map screen goes next.
- `Ending`: how the level ends, game over, and which level comes next.
- `Input`: the buttons as the tick's input left them.
- `Players`: the players, their shots' timing, the generator's power and the
  shield's recharging, the charge weapon, two-player linking, the sidekicks'
  attachment and the ships' sprites.
- `Specials`: the special moves' timing and codes, the special weapon, and the
  specials that last a while (flares, the Zinglon and the astral).
- `Enemies`: the enemies' slots and what each holds, and how many there are on
  screen, in all and killed.
- `PlayerShots`: the player shots' slots, and how many ticks each has left.
- `EnemyShots`: the enemy shots' slots, and which are free.

The C++ level tick works on a `State` that the code running it provides
(`../port/README.md`): the shadow tick, or the C++ tick in place of the C
code's (`../cxx_tick.hpp`), which owns the game's `State` during a level.

While the C game runs, it is the reference, so its globals hold the state at a
tick boundary. `../gold/sim_state.hpp` lists the C global that holds each member
of each of `State`'s subsystems, and copies between them. The shadow tick
(`../check/README.md`) reads a `State` from the globals before each pass,
scrambles the globals, and writes the `State` back for the comparison, so code
that still used the globals would go wrong visibly. It also checks at startup
that each member has exactly one global. Pointers into the state (a player's
lives point into its items) are pointed into the copy when it is copied.

The `State` holds all the state that the registry (`../gold/registry.inc`)
classes as gameplay or presentation state. The C++ level tick still uses these
from the C globals:
- The level and episode data loaded from files (`enemyDat`, `weapons`, the map
  data and the like). Events 49 to 52 change `enemyDat[0]`, the data of the
  enemies they create, which the `State` doesn't hold yet.
- The raw input that the tick reads, the scratch globals that its code writes
  before reading (`temp`, `tempW` and the like), the screens and sprites, the
  settings and the sound.

## Ports

Each port here takes the state it works on explicitly: the subsystems it works
on, or the whole `State` for game logic that works on many (the enemies). The
level and episode data that it reads are still in the C globals (`enemyDat`,
`weapons`). `hidden.hpp` hides the C globals that the `State` holds, and the
random number generator's functions that use them, from code here and in
`../port/`, so using one by mistake doesn't compile.

The ports:
- `explosion.cpp`: `setup_explosion` (`JE_setupExplosion`) takes the explosion
  slots it fills in, and `setup_explosion_large` (`JE_setupExplosionLarge`) the
  `Explosions`. In the level loop, `update_repeating_explosions` sets off the
  repeating explosions' explosions, and `update_explosions` moves the
  explosions and returns the ones visible in the tick for its drawing.
- `superpixel.cpp`: `do_sp` (`JE_doSP`) creates superpixels, and
  `update_superpixels` moves and fades them and returns those it updated.
- `starfield.cpp`: `update_starfield` moves the stars.
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
- `enemy_shot.cpp`: `update_enemy_shots` moves the enemy shots and ends those
  that time out, leave the screen or hit a player. It returns the shots it
  drew.
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
- `hud.cpp`: the HUD's gauges and messages (`JE_drawShield`, `JE_drawArmor`
  and so on), which the game logic logs as it changes them (`hud.hpp`), for
  the tick's drawing. Some do game logic too: `hud_armor` (`JE_drawArmor`)
  limits the players' armor, `hud_text_window` (`JE_drawTextWindow`) sets when
  the message is erased, and `hud_options` (`JE_drawOptions`) sets up the
  sidekicks. The C code draws the shields and armor with `VGAScreen` set to
  the HUD's surface and leaves it at `game_screen` after, where the rest of
  the tick then draws; the log records where it does that, and the code
  running the tick applies it.

The rest of the level tick, in `../port/`, works on the `State` and the C
globals with the C code's names, so that it can be compared with it line by
line. Its code moves here a part at a time, and the shadow tick checks it here
as before. On the way, it takes its state explicitly, gets C++ names, and is
split into functions where the C code nests deeply or jumps (`goto`). It keeps
in local variables what the C code keeps in scratch globals (`temp`, `b`,
`tempW` and the like), when no code that runs after it reads them before
writing them.

A port in `sim/` can also be checked on its own, call by call, with `TYRX_DIFF`
(`../check/README.md`).

Ports that need random numbers call the reentrant C generator (`mt_rand_r` etc.)
on an `MtState` they are given; there is no C++ copy of it. The level tick gives
them the `State`'s, and `../port/`'s code draws from it too. A port must draw
the same values in the same order as the C function. Including the `MtState` in
the compared state checks that.
