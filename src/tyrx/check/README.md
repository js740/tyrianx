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

### Caveats

- Synthetic input arrives only while the virtual clock advances, i.e. during
  frame-pacing sleeps, never in the middle of a tick. The comparison therefore
  shows that the simulation is unchanged for tick-aligned input. It does not
  cover input that arrives mid-tick.
- Joystick and mouse input are not synthesized yet.

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
