# validate: the C++ game checked against the C game

With `make TYRX_VALIDATE=on`, this directory is linked into the game. It is
C++26, for the reflection that it uses to walk the C and C++ types (GCC 16+).
It sees the C game as the reference, the "gold" version: it runs it
deterministically and hashes its state every tick.

It attaches itself to the game during static initialization, as the game's
`Validation` (`../game.hpp`, `runtime.cpp`), and the game's hooks run it.

| File | What it does |
|---|---|
| `runtime.cpp` | The validation's hooks, and the exits |
| `state.cpp` | The state registry (`registry.inc`, `untracked.txt`), hashing and dumping of the C state |
| `hashlog.cpp` | The per-tick state hash log and state dumps |
| `vclock.cpp` | The virtual clock |
| `input_driver.cpp` | Seeded synthetic input |

## Runtime controls (environment variables)

| Variable | Effect |
|---|---|
| `TYRX_HASHLOG=FILE` | Per-tick hash of every hashed block, plus level-start markers |
| `TYRX_DUMP_TICK=N`, `TYRX_DUMP=FILE` | Write every hashed field as `path = value` at tick N |
| `TYRX_VCLOCK=1` | Virtual clock: `SDL_Delay` advances time instantly, and `time()` starts at a fixed epoch |
| `TYRX_INPUT_SEED=S` | Deterministic synthetic keyboard input (needs `TYRX_VCLOCK`) |
| `TYRX_EXIT_AFTER_DEMOS=N`, `TYRX_EXIT_AFTER_TICKS=N` | End the run |

Exits that the validation initiates return a status that names the first
check that failed:
- 3: pointers that the hashes couldn't describe

## State registry

Every writable global and static of the C objects has a `StateClass`.
`registry.inc` lists the globals Tyrian X registers, as one X macro per class:
`TYRX_SIM(X)` expands to `X(symbol)` for every gameplay symbol, and so on.
`untracked.txt` lists the rest by object. `tools/check_c_state_registry.py`
runs at link time and fails if a symbol is in neither or if registered state is
`static`, so a newly added global can't silently go missing from the hashes.
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
as "unresolved", which could hide a change. The validation reports the number
of such pointers at exit, and exits it initiates (`TYRX_EXIT_AFTER_*`) return
status 3 if there were any, so CI fails until the target is registered.

The registry classification was done by hand and leans towards hashing too
much. A symbol hashed by mistake can only cause false divergences, never hide a
real one.

## Verifying that a C change preserves gameplay

```sh
make TYRX_VALIDATE=on && cp opentyrian /tmp/ot-before
# ...apply the change...
make TYRX_VALIDATE=on && cp opentyrian /tmp/ot-after

tools/run_headless.sh /tmp/ot-before DATA before.log demos
tools/run_headless.sh /tmp/ot-after  DATA after.log  demos
tools/compare_hashlogs.py before.log after.log     # "identical: 18538 ticks"

tools/run_headless.sh /tmp/ot-before DATA before.log input 1 20000
tools/run_headless.sh /tmp/ot-after  DATA after.log  input 1 20000
tools/compare_hashlogs.py before.log after.log
```

If the logs diverge, `compare_hashlogs.py` names the first diverging tick and
the blocks that differ. Rerun both builds with `TYRX_DUMP_TICK=<tick>` and
`diff` the dumps to see the exact fields.

`DATA` is a Tyrian 2.1 data directory (`./get_data.sh` fetches one).

## Synthetic input

With `TYRX_INPUT_SEED=S` (and `TYRX_VCLOCK`), `input_driver.cpp` plays the game
with seeded random keys.

### Caveats

- Synthetic input arrives only during frame-pacing sleeps, so a comparison
  shows that the simulation is unchanged for input that arrives between ticks.

