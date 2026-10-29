# sim: the C++ game's state

`sim/` has the C++ game's state, the state that the C globals hold in the C
game, in structures.

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

The `State` holds all the state that the validation's registry
(`../validate/registry.inc`) classes as gameplay or presentation state. The
game logic uses these from the C globals:
- The level and episode data loaded from files (`enemyDat`, `weapons`, the map
  data and the like). Events 49 to 52 change `enemyDat[0]`, the data of the
  enemies they create, which the `State` doesn't hold.
- The raw input that the tick reads, the scratch globals that its code writes
  before reading (`temp`, `tempW` and the like), the screens and sprites, the
  settings and the sound.
