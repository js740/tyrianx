# check: running the C game deterministically and checking against it

This is the scaffolding around the C++ game: it runs the C game
deterministically, records what it does, and compares C++ code with it. It
shrinks as the C++ game takes over.

| File | What it does |
|---|---|
| `hashlog.cpp` | The per-tick state hash log and state dumps |
| `vclock.cpp` | The virtual clock |
| `input_driver.cpp` | Seeded synthetic input |
| `shadow.cpp` | The shadow tick: the C++ level tick checked against the C tick |
| `diff.cpp` | Call-by-call checks of the ports in `../sim/` |
| `render_diff.cpp` | Call-by-call checks of the ports in `../render/` |
| `roundtrip.cpp` | The round-trip test of snapshots |
| [`draw/`](draw/README.md) | Recording the C code's draw lists, and checking them |

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

## Comparing frames

`TYRX_FRAMELOG=FILE` writes a line per frame presented (`JE_showVGA`): the
tick, and a hash of the screen's pixels and the palette. Two runs that show the
same frames write the same log, so `cmp` compares them. The logs of two builds
differ outside the levels: the title screen shows the build's version, and the
screens between the demos present more frames in a release build than in a
debug build.

## The C++ tick in place of the C code's

With `TYRX_CXX_TICK=1`, the C++ level tick runs the game's levels
(`../cxx_tick.hpp`). A run plays the same with it as without it if their hash
logs and frame logs are the same:

```sh
tools/run_headless.sh ./opentyrian DATA c.log demos TYRX_FRAMELOG=c.frames
tools/run_headless.sh ./opentyrian DATA x.log demos TYRX_FRAMELOG=x.frames TYRX_CXX_TICK=1
cmp c.log x.log && cmp c.frames x.frames
```

The demos play the same, and so do the synthetic runs of the shadow suite above
(seeds 1-7 with the cheats and with the commands, input during the tick, arcade
and two-player games) and six runs that open the pause screen, the help and the
menu (`TYRX_INPUT_SCREENS`), which the C++ tick runs with the `State` in the C
globals: 838,538 ticks and 967,108 frames.

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

`TYRX_INPUT_SCREENS=1` opens the screens that a level can open, each with its
own loop, in turn every 1000 ticks: the pause screen (P), the in-game help (F1)
and the in-game menu (Escape). The help closes at the next key; while the pause
screen or the menu is open (the wrappers of `JE_pauseGame` and
`JE_doInGameSetup` tell the driver), the driver presses only Escape, which
closes them. The shadow tick skips the ticks that open them. Round trips
(below) don't run with it: those keys go out as the virtual clock advances,
which a round trip doesn't restore.

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

## The shadow tick

`TYRX_SHADOW=1` (with `TYRX_VCLOCK`) checks the C++ level tick
(`../port/README.md`) against the C code tick by tick. At the top of each level
tick, after the C code has run the previous one, it:
1. Saves all of the C code's writable data (`../gold/memory.hpp`: the C
   objects' `.data` and `.bss`, which the linker places before the C++
   objects', up to the markers in `../gold/data_end.cpp`).
2. Restores the data from the start of the previous tick, and points the
   screen surfaces at copies of them as they were then.
3. Runs `level_pass` on it, with the side effects that the C code has already
   had suppressed: sound, music, presenting and polling. File reads (demo
   input) get the data the C tick read, and joystick polls the joystick state
   the C tick's polls left (the next tick's input has already arrived). The
   pass draws into a draw list of its own (`../port/draw.hpp`), and works on a
   `sim::State` read from the C globals, which are scrambled until the State
   is written back after the pass (`../sim/README.md`).
4. Compares the hashed state, the draw list and the pixels with the C tick's
   (`draw/`). The lists are compared surface by surface: for each surface, the
   calls that draw on it or read it (copies and filters) must be the same, in
   the same order. Calls that touch different surfaces commute. The pixels are
   drawn from the C++ pass's list by the C++ ports (`../render/`), on copies of
   the surfaces as they were at the start of the tick.
5. Restores the data saved in step 1.

Ticks in which the C code loaded sprites or opened the pause screen or the
in-game menu aren't checked, and a pass that would open one, or present or
poll in some other way, is abandoned. Network games aren't supported. A
summary goes to stderr at exit, and exits that Tyrian X initiates return
status 9 if a tick differed. The five demos pass (18,528 ticks checked, 5
skipped for sprite loading), and so do over 600,000 ticks of synthetic play:
with seeds 1-7 with `TYRX_INPUT_CHEATS` or `TYRX_INPUT_COMMANDS`, and in
one- and two-player arcade games, with the mouse moving player 2.

`TYRX_SHADOW_CALLS=1` also prints the calls of each kind that the passes drew,
and those drawn during the simulation.

## Call-by-call checks

`diff.cpp` and `render_diff.cpp` wrap a C function at link time and check a C++
port of it on every call. Only calls made from *other* object files go through
a wrapper: calls inside the defining file (e.g. varz.c calling
`JE_setupExplosion`) bind locally and are not checked.

With `TYRX_DIFF=1`, each call of a function ported to `../sim/` does the
following:
1. Copies the state the function owns.
2. Runs the C function, whose result always stands.
3. Runs the C++ port on the copy.
4. Compares the two results field by field.

Mismatches are printed with the differing fields. A per-function summary is
printed at exit, and exits that Tyrian X initiates return status 3. Checked so
far: `JE_setupExplosion`, `JE_setupExplosionLarge` and `JE_doSP`, whose
check includes the random number generator's state.

```sh
tools/run_headless.sh ./opentyrian DATA diff.log demos TYRX_DIFF=1
# tyrx diff: JE_doSP                     191 calls, 0 mismatches
# tyrx diff: JE_setupExplosion        36077 calls, 0 mismatches
# tyrx diff: JE_setupExplosionLarge    1893 calls, 0 mismatches
```

To check a new port this way:
1. Add `--wrap=NAME` to `EXTRA_LDFLAGS` in the Makefile.
2. Add a `__wrap_NAME` to `diff.cpp` that compares the owned state with
   `diff_check`. Reflected state structs must be at namespace scope.

With `TYRX_RENDER_DIFF=1`, each call of a drawing function ported to
`../render/` does the following:
1. Copies the surface the function draws on.
2. Has the C++ port draw on the copy, from the state before the call.
3. Runs the C function, whose result always stands.
4. Compares every pixel.

Mismatches are printed with the number of differing pixels and the first one. A
per-function summary is printed at exit, and exits that Tyrian X initiates
return status 7. Checked so far: `draw_background_1`; the other ports are
checked by replaying draw lists (`draw/`).

```sh
tools/run_headless.sh ./opentyrian DATA diff.log demos TYRX_RENDER_DIFF=1
# tyrx render diff: draw_background_1         18538 calls, 0 mismatches (0 pixels)
```

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

With `TYRX_CXX_TICK=1`, the C++ tick reads its `State` from the C globals that
a round trip restored, so the round trip also checks that the `State`, the level
data and the input are all that the C++ tick depends on. The runs above give
the same hash logs with it as without it.

## Rewind

Rewind (`../rewind.hpp`) works on the C++ tick's `State`. For testing,
`TYRX_INPUT_REWIND=1` makes the synthetic input hold the rewind key for 40 of
every 400 ticks. Rewind used to work on snapshots of the C state; with seed 2,
5,000 ticks and the same input, the C++ rewind presents the same frames as that
did, and ends in the same state.
