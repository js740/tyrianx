# gold: the C game as seen from C++

The C game is the reference, the "gold" version, that the C++ code is checked
against. This directory is how C++ code sees it:

- `api.hpp`: the C game's declarations, for C++.
- `state.hpp`: the state registry, hashing, dumping and snapshots of the C
  state.
- `memory.hpp`: saving and restoring all of the C code's writable data at once
  (`.data` and `.bss`), for the shadow tick (`../check/shadow.hpp`).
  `data_end.cpp` marks where that data ends; the Makefile links it first among
  the C++ objects.
- `sim_state.hpp`: the C globals that hold the state of the C++ game's
  `sim::State` (`../sim/README.md`), and copying between the two.
- `mirror.hpp`: which member of a C struct each member of the C++ structs in
  `sim::State` maps to, and copying between them.

## State registry

Every writable global and static of the C objects has a `StateClass`.
`registry.inc` lists the globals Tyrian X registers, as one X macro per class:
`TYRX_SIM(X)` expands to `X(symbol)` for every gameplay symbol, and so on.
`untracked.txt` lists the rest by object. `tools/check_c_state_registry.py`
runs at link time and fails if a symbol is in neither or if registered state is
`static`, so a newly added global can't silently go missing from snapshots.
With `--by-object`, it prints the classification object by object.

The classes that matter most:
- `sim`: gameplay state that changes during play.
- `present`: state that changes during play but only affects what is shown or
  heard (explosions, flashes, palette filters, warning colours, sound queue).
  It is hashed like `sim`, but gameplay doesn't depend on it.
- `data`: level and episode data loaded from files, including the background
  maps (`megaData1`-`3`). A few events modify it (e.g. `enemyDat[0]`).
- `scratch`: Pascal-era temporaries (`temp`, `tempW`, `b`, `x`, ...), always
  written before being read within a tick, so not hashed.

Blocks of the `sim`, `present` and `data` classes form the hashed game state.
Hashing and dumping (`state.hpp`) walk each block's C type with C++26
reflection:
- struct padding is never read
- pointers are hashed as `block+offset` (e.g. `mapYPos = &megaData1+1234`),
  where the block is any registered global, so hashes are stable across ASLR
  and across builds. Globals that hashed state points into but that are not
  hashed themselves (e.g. the sprite sheets) are registered in `TYRX_POINTEES`
  just to be pointer targets.

A non-null pointer that doesn't point into any registered block is hashed only
as "unresolved", which could hide a change. Tyrian X reports the number of such
pointers at exit, and exits it initiates (`TYRX_EXIT_AFTER_*`) return status 4
if there were any, so CI fails until the target is registered.

The registry classification was done by hand and leans towards hashing too
much. A symbol hashed by mistake can only cause false divergences, never hide a
real one.

## Snapshots

`Snapshot` (`state.hpp`) copies the registered blocks of chosen classes and
writes them back. A snapshot is valid only within the level it was taken in,
because pointers into per-level data (such as sprite sheets) are copied as-is.
The round-trip test (`../check/README.md`) checks that snapshots hold
everything a tick depends on.

The C code allocates the joystick array (`joystick`), which holds each
joystick's previous state and repeat timer, at startup. `register_heap_block`
adds it to snapshots as input state once `init_joysticks` has run.
