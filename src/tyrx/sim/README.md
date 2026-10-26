# sim: game logic with encapsulated state

`sim/` is where the game logic goes once the state it works on is encapsulated.
Each port there takes its state explicitly instead of using the C globals:
`setup_explosion`, the port of `JE_setupExplosion` and the only one so far,
takes the explosions it fills in. The level tick's ports in `../port/` still
work on the C globals, with the C code's names, so that they can be compared
with it line by line. As state moves out of the globals into structures, the
code that uses it moves from `../port/` to `sim/`, and the shadow tick checks
it there as before.

A port in `sim/` can also be checked on its own, call by call, with `TYRX_DIFF`
(`../check/README.md`).

Ports that need random numbers call the reentrant C generator (`mt_rand_r`
etc.) on an `MtState`; there is no C++ copy of it. A port must draw the same
values in the same order as the C function. Including the `MtState` in the
compared state checks that.
