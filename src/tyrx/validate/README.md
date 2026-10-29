# validate: the C++ game checked against the C game

With `make TYRX_VALIDATE=on`, this directory is linked into the game. It is
C++26, for the reflection that it uses to walk the C and C++ types (GCC 16+).

It attaches itself to the game during static initialization, as the game's
`Validation` (`../game.hpp`, `runtime.cpp`), and the game's hooks run it.

| File | What it does |
|---|---|
| `runtime.cpp` | The validation's hooks, and the exits |

## Runtime controls (environment variables)

| Variable | Effect |
|---|---|
| `TYRX_EXIT_AFTER_DEMOS=N`, `TYRX_EXIT_AFTER_TICKS=N` | End the run |
