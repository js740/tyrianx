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
