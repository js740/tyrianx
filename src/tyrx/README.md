# Tyrian X: OpenTyrian's levels in C++

Tyrian X is to run the game's levels in C++, in namespace `tyrx`, on a game
state of its own, with the same results as OpenTyrian's C code, tick for tick
and pixel for pixel.

There are two builds:
- `make`: the game, C and C++23.
- `make TYRX_VALIDATE=on`: the game and its validation (`validate/`, C++26,
  GCC 16+ for reflection).

Every C object is the same in both builds. The C code's changes for Tyrian X
are the hooks below.

C++ files are formatted with clang-format 15+ using `.clang-format` in this
directory: `clang-format -i $(find src/tyrx -name '*.[ch]pp')`.

## Layout

| Directory | Namespace | What it holds |
|---|---|---|
| `.` | `tyrx` | `game.cpp`: the C++ game's hooks into the C code |
| [`c/`](c/README.md) | `tyrx::c` | The C game as seen from C++: its declarations, and copying the C++ game's state to and from the C globals |
| [`sim/`](sim/README.md) | `tyrx::sim` | The C++ game's state |
| [`render/`](render/README.md) | `tyrx::render` | Drawing from explicit inputs, and draw lists |
| [`validate/`](validate/README.md) | `tyrx::validate` | With `TYRX_VALIDATE=on`: the state registry, hashing and snapshots of the C game, and deterministic runs |

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
- **Link-time wrapping** (validation only): `-Wl,--wrap=SYMBOL` sends the C
  code's calls to `__wrap_SYMBOL` in C++, which can call `__real_SYMBOL`. It
  works for any function called across object files and needs no source
  changes. The validation wraps:
  - `SDL_GetTicks`, `SDL_Delay` and `time`, for the virtual clock;
  - `JE_itemScreen`, `gameplaySelect` and `init_joysticks`, for the synthetic
    input;
  - the drawing primitives, for the draw lists.
