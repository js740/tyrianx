# validate: the C++ game checked against the C game

With `make TYRX_VALIDATE=on`, this directory is linked into the game. It is
C++26, for the reflection that it uses to walk the C and C++ types (GCC 16+).
It sees the C game as the reference, the "gold" version: it runs it
deterministically, hashes its state every tick, and records what it draws.

It attaches itself to the game during static initialization, as the game's
`Validation` (`../game.hpp`, `runtime.cpp`), and the game's hooks run it.

| File | What it does |
|---|---|
| `runtime.cpp` | The validation's hooks, and the exits |
| `state.cpp` | The state registry (`registry.inc`, `untracked.txt`), hashing, dumping and snapshots of the C state |
| `hashlog.cpp` | The per-tick state hash log and state dumps |
| `vclock.cpp` | The virtual clock |
| `input_driver.cpp` | Seeded synthetic input |
| `roundtrip.cpp` | The round-trip test of snapshots |
| [`draw/`](draw/README.md) | Recording the C code's draw lists, and checking them |
| `bindings.cpp` | The checks of the C++ game's `State` against the C globals that hold it (`../c/README.md`) |

## Runtime controls (environment variables)

| Variable | Effect |
|---|---|
| `TYRX_HASHLOG=FILE` | Per-tick hash of every hashed block, plus level-start markers |
| `TYRX_DUMP_TICK=N`, `TYRX_DUMP=FILE` | Write every hashed field as `path = value` at tick N |
| `TYRX_VCLOCK=1` | Virtual clock: `SDL_Delay` advances time instantly, and `time()` starts at a fixed epoch |
| `TYRX_INPUT_SEED=S` | Deterministic synthetic keyboard input (needs `TYRX_VCLOCK`) |
| `TYRX_INPUT_COMMANDS=1` | With `TYRX_INPUT_SEED`: also press a command every 300 ticks |
| `TYRX_INPUT_MIDTICK=1` | With `TYRX_INPUT_SEED`: send input while ticks run, not between them |
| `TYRX_INPUT_CHEATS=1` | With `TYRX_INPUT_SEED`: tour the game with its cheat keys |
| `TYRX_INPUT_TWO_PLAYER=1` | With `TYRX_INPUT_SEED`: play two-player arcade games |
| `TYRX_INPUT_ARCADE=1` | With `TYRX_INPUT_SEED`: play one-player arcade games |
| `TYRX_INPUT_MOUSE=1` | With `TYRX_INPUT_SEED`: also move the mouse and press its buttons |
| `TYRX_INPUT_JOYSTICK=1` | With `TYRX_INPUT_SEED`: also play with a virtual joystick |
| `TYRX_ROUNDTRIP=K[,P]` | Round-trip test of snapshots |
| `TYRX_DRAWLIST=FILE` | Write each level tick's draw calls to FILE ([draw](draw/README.md)) |
| `TYRX_DRAWLIST_CHECK=1` | Check that the draw lists are complete and reproduce each tick ([draw](draw/README.md)) |
| `TYRX_EXIT_AFTER_DEMOS=N`, `TYRX_EXIT_AFTER_TICKS=N` | End the run |

Exits that the validation initiates return a status that names the first
check that failed:
- 3: pointers that the hashes couldn't describe
- 4: round trips (`TYRX_ROUNDTRIP`)
- 5: draw lists (`TYRX_DRAWLIST_CHECK`)

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

### Input that arrives during a tick

Synthetic input normally arrives during the frame-pacing wait, between ticks.
With `TYRX_INPUT_MIDTICK=1`, the same input arrives at the top of the tick
before instead, after the game has polled events for that tick, which is where
real input lands while the game is busy. `TYRX_INPUT_COMMANDS=1` adds a command
every 300 ticks (Backspace+Minus, which cycles the level filter), so the run
also exercises the pause/menu/cheat handling (`JE_mainKeyboardInput`).

If the game reads input only once per tick, both runs log the same hashes:

```sh
tools/run_headless.sh ./opentyrian DATA a.log input 1 20000 TYRX_INPUT_COMMANDS=1
tools/run_headless.sh ./opentyrian DATA b.log input 1 20000 TYRX_INPUT_COMMANDS=1 TYRX_INPUT_MIDTICK=1
tools/compare_hashlogs.py a.log b.log
```

Upstream `level_loop` also polls events in the middle of each tick, before the
game-over, demo-stop and pause/menu/cheat checks, so there the logs diverge at
the first command. This branch removes those polls ("Poll events once per level
loop iteration"), and CI checks that the logs match.

### Touring more of the game

`TYRX_INPUT_CHEATS=1` uses the game's own key commands to show more of it than
random play, which rarely gets past the first level: in each level, it turns
on the debug display at the level's tick 30 and off at tick 230, toggles a
random smoothie (screen effect) every 100 ticks, and skips the level from tick
500. In about one level in four, it also gives the player the Nort ship, with
its weapons and the Astral Zone special, for the rest of the game. Between
levels it moves to the next detail level, as the setup menu would, which
changes what is drawn (blended backgrounds, darkening, transparent
explosions), and to the next difficulty, from Wimp to Nortaneous. In 20,000
ticks, a run visits about 25 levels.

`TYRX_INPUT_TWO_PLAYER=1` plays two-player arcade games instead of the full
game, and `TYRX_INPUT_ARCADE=1` one-player arcade games, in which pickups give
special weapons that fire with the main weapon: the game-mode menu makes its
choice right away (keys sent while it fades in can be lost, so the driver can't
reliably move to it), and between levels the driver plays the next level.
Player 2's controller is the mouse: with `TYRX_INPUT_MOUSE=1`, the driver moves
it and holds its left and right buttons in seeded runs, as it does the keys, so
player 2 moves, fires and uses its sidekicks; without it, player 2 doesn't
move, but its ship, gauges and sidekicks are still simulated and drawn. In
one-player games, the mouse moves player 1 along with the keys.

`TYRX_INPUT_JOYSTICK=1` replaces any real joysticks with one virtual joystick
(SDL's virtual joystick driver, with the default assignments) and drives its
stick, fire and sidekick buttons along with the keys, plus a fire-mode change
every 200 ticks. Joystick runs are deterministic and survive round trips, but
with `TYRX_INPUT_MIDTICK=1` they diverge at the first tick: `JE_playerMovement`
reads the joystick directly (`poll_joystick`) in the middle of the tick.

### Caveats

- Without `TYRX_INPUT_MIDTICK`, synthetic input arrives only during
  frame-pacing sleeps, so a comparison shows that the simulation is unchanged
  for input that arrives between ticks.
- Synthetic mouse input (`TYRX_INPUT_MOUSE`) is only relative motion and the
  left and right buttons.
- Joysticks are read in the middle of a tick (`poll_joystick` in
  `JE_playerMovement`, and `push_joysticks_as_keyboard` in the game-over and
  demo-stop checks), with key repeat timed by the wall clock. The virtual clock
  only advances between ticks, so the repeat timer is deterministic here but
  not in real play.

## Snapshots and the round-trip test

`Snapshot` (`state.hpp`) copies the registered blocks of chosen classes and
writes them back. A snapshot is valid only within the level it was taken in,
because pointers into per-level data (such as sprite sheets) are copied as-is.

The C code allocates the joystick array (`joystick`), which holds each
joystick's previous state and repeat timer, at startup. `register_heap_block`
adds it to snapshots as input state once `init_joysticks` has run.

`TYRX_ROUNDTRIP=K[,P]` tests whether snapshots hold everything a tick depends
on. Every P ticks (default 10×K), outside demos, it does the following:
1. Snapshots the game state and the input state.
2. Runs K ticks.
3. Restores the snapshot.
4. Replays those K ticks, requiring the same hashes as the first time, at the
   top of each tick and at the end.

Any state left out of the registry shows up as a mismatch that names the
differing blocks. Exits that the validation initiates return status 4 if any
replay differed or no round trip completed. A run with round trips logs the same
hashes as one without:

```sh
tools/run_headless.sh ./opentyrian DATA plain.log input 1 20000
tools/run_headless.sh ./opentyrian DATA rt.log input 1 20000 TYRX_ROUNDTRIP=50,200
# tyrx roundtrip: 98 round trips of 50 ticks, 4900 ticks replayed, 0 mismatched, ...
tools/compare_hashlogs.py plain.log rt.log         # "identical: 20000 ticks"
```

Demos are skipped because their input comes from a file whose read position
isn't part of the snapshot.

