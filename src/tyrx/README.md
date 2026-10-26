# Tyrian X: C++ extensions for OpenTyrian

Optional C++ code, in namespace `tyrx`, for verifying and eventually
extending OpenTyrian's game simulation without changing its behaviour.
It is built only with `make WITH_TYRX=true` (GCC 16+, `-std=c++26
-freflection`). The default build is the plain C99 game, and every C object
except those with deliberate changes is byte-identical to upstream's.

C++ files are formatted with clang-format 15+ using `.clang-format` in this
directory: `clang-format -i $(find src/tyrx -name '*.[ch]pp')`.

## How it attaches to the C code

- **Hooks**: `src/hooks.h` declares `game_hooks`, callbacks that the game
  calls once a level has loaded, at the top of each pass through
  `level_loop`, and when the level ends. They are NULL by default;
  `runtime.cpp` sets the ones it uses during static initialization, so
  linking the C++ objects is all it takes. The C
  changes are all meant for upstream: these hooks, the RNG state refactor,
  external linkage for some state, and polling events only once per tick.
- **Link-time wrapping**: `-Wl,--wrap=SYMBOL` sends the C code's calls to
  `__wrap_SYMBOL` in C++, which can call `__real_SYMBOL`. This currently
  covers `SDL_GetTicks`, `SDL_Delay` and `time` for the virtual clock,
  `JE_itemScreen` for the input driver, and `JE_setupExplosion` for the C++
  port check. It works for any function called across object files and needs
  no source changes.
- **State access**: the C++ code reads and writes the C globals directly,
  through their header declarations. All hashed state has external linkage.

## State registry

Every writable global and static of the C objects has a `StateClass`.
`c_state_registry.inc` lists the globals Tyrian X registers, as one X macro per
class: `TYRX_SIM(X)` expands to `X(symbol)` for every gameplay symbol, and so
on. `c_state_untracked.txt` lists the rest by object.
`tools/check_c_state_registry.py` runs at link time and fails if a symbol is in
neither or if registered state is `static`, so a newly added global can't
silently go missing from snapshots. With `--by-object`, it prints the
classification object by object.

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
Hashing and dumping (`c_state.hpp`) walk each block's C type with C++26
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

## Runtime controls (environment variables)

| Variable | Effect |
|---|---|
| `TYRX_HASHLOG=FILE` | Per-tick hash of every hashed block, plus level-start markers |
| `TYRX_DUMP_TICK=N`, `TYRX_DUMP=FILE` | Write every hashed field as `path = value` at tick N |
| `TYRX_VCLOCK=1` | Virtual clock: `SDL_Delay` advances time instantly, and `time()` starts at a fixed epoch |
| `TYRX_DIFF=1` | Check the C++ ports in `sim/` against the C functions they port (see below) |
| `TYRX_INPUT_SEED=S` | Deterministic synthetic keyboard input (needs `TYRX_VCLOCK`) |
| `TYRX_EXIT_AFTER_DEMOS=N`, `TYRX_EXIT_AFTER_TICKS=N` | End the run |

## Verifying that a C change preserves gameplay

```sh
make WITH_TYRX=true && cp opentyrian /tmp/ot-before
# ...apply the change...
make WITH_TYRX=true && cp opentyrian /tmp/ot-after

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

## C++ ports of the simulation (`sim/`)

`sim/` holds C++ ports of C game functions. Each port works on explicit
state instead of globals. `diff.cpp` wraps the C function at link time.
With `TYRX_DIFF=1`, each wrapped call does the following:
1. Copies the state the function owns.
2. Runs the C function, whose result always stands.
3. Runs the C++ port on the copy.
4. Compares the two results field by field.

Mismatches are printed with the differing fields. A per-function summary is
printed at exit, and exits that Tyrian X initiates return status 3.

```sh
tools/run_headless.sh ./opentyrian DATA diff.log demos TYRX_DIFF=1
# tyrx diff: JE_setupExplosion   36077 calls, 0 mismatches
```

Ported so far: `JE_setupExplosion`.
Only calls made from *other* object files go through the wrapper. Calls inside
the defining file (e.g. varz.c calling `JE_setupExplosion`) bind locally and
are not checked.

Ports that need random numbers call the reentrant C generator (`mt_rand_r`
etc.) on an `MtState`; there is no C++ copy of it. A port must draw the same
values in the same order as the C function. Including the `MtState` in the
compared state checks that.

To add a port:
1. Write it in `sim/`.
2. Add `--wrap=NAME` to `EXTRA_LDFLAGS` in the Makefile.
3. Add a `__wrap_NAME` to `diff.cpp` that compares the owned state with
   `diff_check`. Reflected state structs must be at namespace scope.

## Caveats

- Synthetic input arrives only while the virtual clock advances, i.e. during
  frame-pacing sleeps, never in the middle of a tick. The comparison therefore
  shows that the simulation is unchanged for tick-aligned input. It does not
  cover input that arrives mid-tick.
- Joystick and mouse input are not synthesized yet.
- The registry classification was done by hand and leans towards hashing too
  much. A symbol hashed by mistake can only cause false divergences, never hide
  a real one.
