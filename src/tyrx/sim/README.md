# sim: the C++ game's state, and game logic that owns its state

`sim/` is where the game's state goes as it moves out of the C globals into
structures, and the game logic that works on it.

## The game state

`state.hpp` has the C++ game's state, `State`: the state that C globals hold in
the C game, moved into structures, a subsystem at a time. So far it has:
- `Random`: the random number generator's state.
- `Explosions`: the explosions' slots, the repeating explosions (big explosions
  that set off explosions over several ticks), how far explosions move down
  each tick, and how far the player moved, for those that follow the player.
- `Superpixels`: sparks that move and fade.
- `Starfield`: the stars, and how much faster than their own speed they move.
- `Level`: the level's events and where the level is in them, and what events
  set: random enemies, the level timer, Galaga mode's settings, and where the
  map screen goes next.
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
(`../port/README.md`).

While the C game runs, it is the reference, so its globals hold the state at a
tick boundary. `../gold/sim_state.hpp` lists the C global that holds each member
of each of `State`'s subsystems, and copies between them. The shadow tick
(`../check/README.md`) reads a `State` from the globals before each pass,
scrambles the globals, and writes the `State` back for the comparison, so code
that still used the globals would go wrong visibly. It also checks at startup
that each member has exactly one global. Pointers into the state (a player's
lives point into its items) are pointed into the copy when it is copied.

## Ports

Each port here takes the state it works on explicitly:
- `explosion.cpp`: `setup_explosion` (`JE_setupExplosion`) takes the explosion
  slots it fills in, and `setup_explosion_large` (`JE_setupExplosionLarge`) the
  `Explosions`. In the level loop, `update_repeating_explosions` sets off the
  repeating explosions' explosions, and `update_explosions` moves the
  explosions and returns the ones visible in the tick for its drawing.
- `superpixel.cpp`: `do_sp` (`JE_doSP`) creates superpixels, and
  `update_superpixels` moves and fades them and returns those it updated.
- `starfield.cpp`: `update_starfield` moves the stars.

The level tick's ports in `../port/` work on the C globals and on the `State`,
with the C code's names, so that they can be compared with it line by line. As
state moves into `State`, the code that uses it moves from `../port/` to `sim/`,
and the shadow tick checks it there as before.

A port in `sim/` can also be checked on its own, call by call, with `TYRX_DIFF`
(`../check/README.md`).

Ports that need random numbers call the reentrant C generator (`mt_rand_r` etc.)
on an `MtState` they are given; there is no C++ copy of it. The level tick gives
them the `State`'s, and `../port/`'s code draws from it too. A port must draw
the same values in the same order as the C function. Including the `MtState` in
the compared state checks that.
