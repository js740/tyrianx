# Tyrian X: OpenTyrian's levels in C++

Tyrian X runs the game's levels in C++, in namespace `tyrx`, on a game state of
its own, with the same results as OpenTyrian's C code, tick for tick and pixel
for pixel. The C code still runs everything outside the levels (the menus, the
shop, loading), on its own globals.

New features work on the C++ game's state: rewind, so far.

There are two builds:
- `make`: the game, C and C++23.
  The C++ game runs the levels.
- `make TYRX_VALIDATE=on`: the game and its validation (`validate/`, C++26,
  GCC 16+ for reflection).
  The C code runs the levels unless `TYRX_CXX_TICK=1`.

Every C object is the same in both builds. The C code's changes for Tyrian X
are the hooks below.

C++ files are formatted with clang-format 15+ using `.clang-format` in this
directory: `clang-format -i $(find src/tyrx -name '*.[ch]pp')`.

## Layout

| Directory | Namespace | What it holds |
|---|---|---|
| `.` | `tyrx` | `game.cpp`: the C++ game, which runs the levels in place of the C code's level loop; `level.cpp`: one pass of the level loop, its simulation, drawing and frame; `rewind.cpp`: rewind on its state |
| [`c/`](c/README.md) | `tyrx::c` | The C game as seen from C++: its declarations, and copying the C++ game's state to and from the C globals |
| [`sim/`](sim/README.md) | `tyrx::sim` | The C++ game's state, and the game logic: the level tick's simulation |
| [`render/`](render/README.md) | `tyrx::render` | Drawing from explicit inputs, draw lists, and the level tick's drawing from its draw record |
| [`validate/`](validate/README.md) | `tyrx::validate` | With `TYRX_VALIDATE=on`: the state registry, hashing and snapshots of the C game, deterministic runs, and the checks of the C++ game against the C game |

The game code doesn't depend on `validate/`: the validation attaches itself
to the game (`Validation`, in `game.hpp`) during static initialization.

## How it attaches to the C code

- **Hooks**: `src/hooks.h` declares `game_hooks`, callbacks that the game
  calls once a level has loaded, at the top of each pass through
  `level_loop`, and when the level ends. The one at the top of `level_loop`
  (`level_tick_substitute`) can also substitute for the tick, running it in
  place of the C code's, and says where the C code goes next. They are NULL by default, and `game.cpp` sets
  them during static initialization, so linking the C++ objects is all it
  takes.
- **The C changes** are this branch's first commits, and none of them
  changes how the game plays: these hooks, the random number generator's
  state in a struct, external linkage for the state, and polling events only
  once per tick.
- **State access**: the C++ code reads and writes the C globals directly,
  through their declarations (`c/api.hpp`).
  The state that the C++ game keeps in its own `sim::State` is copied from the
  C globals when a level starts and back after each pass (`c/state.hpp`).
- **Link-time wrapping** (validation only): `-Wl,--wrap=SYMBOL` sends the C
  code's calls to `__wrap_SYMBOL` in C++, which can call `__real_SYMBOL`. It
  works for any function called across object files and needs no source
  changes. The validation wraps:
  - `SDL_GetTicks`, `SDL_Delay` and `time`, for the virtual clock;
  - `JE_itemScreen`, `gameplaySelect` and `init_joysticks`, for the synthetic
    input;
  - the drawing primitives, for the draw lists;
  - the functions whose effects the shadow tick replays or suppresses.

## Runtime controls (environment variables)

| Variable | Effect |
|---|---|
| `TYRX_REWIND=TICKS` | Rewind by holding a key (see below) |
| `TYRX_REWIND_KEY=NAME` | The rewind key, as an SDL scancode name (default Backspace) |

With `TYRX_VALIDATE=on`, the validation adds its own
([`validate/README.md`](validate/README.md)): `TYRX_CXX_TICK=1` runs the
levels with the C++ game, which rewinds only then, and the others run the
checks.

## Rewind

With `TYRX_REWIND=TICKS`, the C++ game keeps its `State` from the start of each
of the last TICKS passes (`rewind.hpp`), about 64 KB each. 350 ticks is about
10 seconds at Normal speed. Holding the rewind key steps back one tick per
frame: each pass drops the newest state and restores the one before it, and
then simulates and draws one tick from there. Input isn't restored, so
releasing the key resumes play with the keys actually held. Rewind is off in
demos and network games, and the states are dropped when a level starts.

```sh
TYRX_REWIND=350 ./opentyrian
```

Backspace is also the modifier for the game's debug key combinations (e.g.
Backspace+1 changes the game speed); those still work while rewinding.

The `State` doesn't hold `enemyDat[0]`, the level data that events 49 to 52
change for the enemies they create (`sim/README.md`), but each of those events
changes it again before using it. Round trips with the C++ game
(`validate/README.md`) check that the `State`, the level data and the input
are all that a tick depends on.
