# port: the C level tick, ported as it is

The parts of the C game's level tick, ported to C++ as they are: they work on
the C globals (`../gold/api.hpp`), with the C code's names, so that they can be
compared with it line by line. The shadow tick (`../check/README.md`) checks
them against the C code tick by tick. As state moves out of the globals into
structures, the code that uses it moves to `../sim/`.

## The level loop

`level.cpp` has the game's level loop in C++: `level_pass` runs one pass of it
(one level tick), from the `level_loop` label in `JE_main` to the jump back to
it, as a function for each phase of the C code's loop, in its order. It is a
direct port of that code so far, which still calls the C functions for most of
the work (`JE_drawEnemy`, `JE_eventSystem`, `JE_mainGamePlayerFunctions` and so
on), so it still mixes simulation and drawing as the C code does. The one piece
of state it keeps itself is `old_weapon_bar`, which the C code keeps in a local
variable of `JE_main`.
