# check: running the C game deterministically and checking against it

This is the scaffolding around the C++ game: it runs the C game
deterministically, records what it does, and compares C++ code with it. It
shrinks as the C++ game takes over.

| File | What it does |
|---|---|
| `hashlog.cpp` | The per-tick state hash log and state dumps |
| `vclock.cpp` | The virtual clock |
| `input_driver.cpp` | Seeded synthetic input |
| `roundtrip.cpp` | The round-trip test of snapshots |

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
- Mouse input is not synthesized yet.
- Joysticks are read in the middle of a tick (`poll_joystick` in
  `JE_playerMovement`, and `push_joysticks_as_keyboard` in the game-over and
  demo-stop checks), with key repeat timed by the wall clock. The virtual clock
  only advances between ticks, so the repeat timer is deterministic here but
  not in real play.

## The round-trip test

`TYRX_ROUNDTRIP=K[,P]` tests whether snapshots (`../gold/README.md`) hold
everything a tick depends on. Every P ticks (default 10×K), outside demos, it
does the following:
1. Snapshots the game state and the input state.
2. Runs K ticks.
3. Restores the snapshot.
4. Replays those K ticks, requiring the same hashes as the first time, at the
   top of each tick and at the end.

Any state left out of the registry shows up as a mismatch that names the
differing blocks. Exits that Tyrian X initiates return status 5 if any replay
differed or no round trip completed. A run with round trips logs the same
hashes as one without:

```sh
tools/run_headless.sh ./opentyrian DATA plain.log input 1 20000
tools/run_headless.sh ./opentyrian DATA rt.log input 1 20000 TYRX_ROUNDTRIP=50,200
# tyrx roundtrip: 98 round trips of 50 ticks, 4900 ticks replayed, 0 mismatched, ...
tools/compare_hashlogs.py plain.log rt.log         # "identical: 20000 ticks"
```

Demos are skipped because their input comes from a file whose read position
isn't part of the snapshot.
