# sim: the C++ game's state, and game logic that owns its state

`sim/` is where the game's state goes as it moves out of the C globals into
structures, and the game logic that works on it.

## The game state

`state.hpp` has the C++ game's state, `State`: the state that C globals hold in
the C game, moved into structures, a subsystem at a time. So far it has the
explosions: their slots, how far they move down each tick, and how far the
player moved, for those that follow the player. The C++ level tick works on a
`State` that the code running it provides (`../port/README.md`).

While the C game runs, it is the reference, so its globals hold the state at a
tick boundary. `../gold/sim_state.hpp` lists the C global that holds each
member of each of `State`'s subsystems, and copies between them. The shadow
tick (`../check/README.md`) reads a `State` from the globals before each pass,
scrambles the globals, and writes the `State` back for the comparison, so code
that still used the globals would go wrong visibly. It also checks at startup
that each member has exactly one global.

## Ports

Each port here takes the state it works on explicitly. So far there are the
explosions (`explosion.cpp`): `setup_explosion`, the port of
`JE_setupExplosion`, takes the explosion slots it fills in, and
`update_explosions`, the explosions' update in the level loop, moves the
`Explosions` and returns the ones visible in the tick for its drawing. The
level tick's ports in `../port/` work on the C globals and on the `State`,
with the C code's names, so that they can be compared with it line by line.
As state moves into `State`, the code that uses it moves from `../port/` to
`sim/`, and the shadow tick checks it there as before.

A port in `sim/` can also be checked on its own, call by call, with `TYRX_DIFF`
(`../check/README.md`).

Ports that need random numbers call the reentrant C generator (`mt_rand_r`
etc.) on an `MtState`; there is no C++ copy of it. A port must draw the same
values in the same order as the C function. Including the `MtState` in the
compared state checks that.
