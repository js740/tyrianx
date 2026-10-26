# Tyrian X: C++ extensions for OpenTyrian

Optional C++ code, in namespace `tyrx`, for verifying and eventually
extending OpenTyrian's game simulation without changing its behaviour.
It is built only with `make WITH_TYRX=true` (GCC 16+, `-std=c++26
-freflection`). The default build is the plain C99 game, and every C object
except those with deliberate changes is byte-identical to upstream's.

C++ files are formatted with clang-format 15+ using `.clang-format` in this
directory: `clang-format -i $(find src/tyrx -name '*.[ch]pp')`.

## Layout

| Directory | Namespace | What it holds |
|---|---|---|
| `.` | `tyrx` | `runtime.cpp`: the hooks the C code calls, the environment controls and exits |
| [`gold/`](gold/README.md) | `tyrx::gold` | The C game, the reference, as seen from C++: its declarations, the state registry, hashing and snapshots |
| [`check/`](check/README.md) | `tyrx::check` | What runs the C game deterministically and compares C++ code with it: the hash log, synthetic input, the virtual clock, call-by-call checks and round trips |
| [`check/draw/`](check/draw/README.md) | `tyrx::check::draw` | Recording the C code's draw lists, and checking them |
| [`sim/`](sim/README.md) | `tyrx::sim` | Game logic whose state is encapsulated |
| [`render/`](render/README.md) | `tyrx::render` | Drawing from explicit inputs, and draw lists |

The C++ game is `sim/` and `render/`. `gold/` and `check/` are the scaffolding
that attaches the C++ game to the C game and checks it against it.

## How it attaches to the C code

- **Hooks**: `src/hooks.h` declares `game_hooks`, callbacks that the game
  calls once a level has loaded, at the top of each pass through
  `level_loop`, and when the level ends. They are NULL by default;
  `runtime.cpp` sets the ones it uses during static initialization, so
  linking the C++ objects is all it takes. The C
  changes are all meant for upstream: these hooks, the RNG state refactor,
  external linkage for some state, and polling events only once per tick.
- **Link-time wrapping**: `-Wl,--wrap=SYMBOL` sends the C code's calls to
  `__wrap_SYMBOL` in C++, which can call `__real_SYMBOL`. This currently covers
  `SDL_GetTicks`, `SDL_Delay` and `time` for the virtual clock,
  `JE_itemScreen`, `gameplaySelect` and `init_joysticks` for the input driver,
  `JE_setupExplosion` and `draw_background_1` for the call-by-call checks, and
  the drawing primitives for the draw lists. It works for any function called
  across object files and needs no source changes.
- **State access**: the C++ code reads and writes the C globals directly,
  through their declarations (`gold/api.hpp`). All hashed state has external
  linkage.

## Runtime controls (environment variables)

| Variable | Effect |
|---|---|
| `TYRX_HASHLOG=FILE` | Per-tick hash of every hashed block, plus level-start markers ([check](check/README.md)) |
| `TYRX_DUMP_TICK=N`, `TYRX_DUMP=FILE` | Write every hashed field as `path = value` at tick N ([check](check/README.md)) |
| `TYRX_VCLOCK=1` | Virtual clock: `SDL_Delay` advances time instantly, and `time()` starts at a fixed epoch |
| `TYRX_DIFF=1` | Check the C++ ports in `sim/` against the C functions they port ([check](check/README.md)) |
| `TYRX_RENDER_DIFF=1` | Check the C++ drawing ports in `render/` pixel by pixel ([check](check/README.md)) |
| `TYRX_DRAWLIST=FILE` | Write each level tick's draw calls to FILE ([check/draw](check/draw/README.md)) |
| `TYRX_DRAWLIST_CHECK=1` | Check that the draw lists are complete and reproduce each tick ([check/draw](check/draw/README.md)) |
| `TYRX_INPUT_SEED=S` | Deterministic synthetic keyboard input (needs `TYRX_VCLOCK`) |
| `TYRX_INPUT_COMMANDS=1` | With `TYRX_INPUT_SEED`: also press a command every 300 ticks ([check](check/README.md)) |
| `TYRX_INPUT_MIDTICK=1` | With `TYRX_INPUT_SEED`: send input while ticks run, not between them ([check](check/README.md)) |
| `TYRX_INPUT_CHEATS=1` | With `TYRX_INPUT_SEED`: tour the game with its cheat keys ([check](check/README.md)) |
| `TYRX_INPUT_TWO_PLAYER=1` | With `TYRX_INPUT_SEED`: play two-player arcade games ([check](check/README.md)) |
| `TYRX_INPUT_ARCADE=1` | With `TYRX_INPUT_SEED`: play one-player arcade games ([check](check/README.md)) |
| `TYRX_INPUT_MOUSE=1` | With `TYRX_INPUT_SEED`: also move the mouse and press its buttons ([check](check/README.md)) |
| `TYRX_INPUT_JOYSTICK=1` | With `TYRX_INPUT_SEED`: also play with a virtual joystick ([check](check/README.md)) |
| `TYRX_ROUNDTRIP=K[,P]` | Round-trip test of snapshots ([check](check/README.md)) |
| `TYRX_EXIT_AFTER_DEMOS=N`, `TYRX_EXIT_AFTER_TICKS=N` | End the run |

Exits that Tyrian X initiates return a status that names the first check that
failed: 3 for `TYRX_DIFF`, 4 for pointers the hashes couldn't describe, 5 for
round trips, 7 for `TYRX_RENDER_DIFF` and 8 for `TYRX_DRAWLIST_CHECK`.
