# sim: game logic with encapsulated state

`sim/` holds C++ ports of C game functions. Each port works on explicit state
instead of globals, and is checked against the C function it ports, call by
call, with `TYRX_DIFF` (`../check/README.md`).

Ports that need random numbers call the reentrant C generator (`mt_rand_r`
etc.) on an `MtState`; there is no C++ copy of it. A port must draw the same
values in the same order as the C function. Including the `MtState` in the
compared state checks that.
