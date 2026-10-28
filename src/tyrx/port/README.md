# port: the level loop's passes

The C game's level tick in C++ is simulated by `../sim/` (`sim::level_tick`),
which records what the tick draws in a draw record (`../sim/draw_record.hpp`),
and drawn from that record by `../render/tick/`. The code here runs them: one
pass of the level loop, with its frame. The shadow tick (`../check/README.md`)
checks the passes against the C code tick by tick, and with `TYRX_CXX_TICK=1`
they run the game's levels in place of the C code (`../cxx_tick.hpp`).

The game's state is in a `sim::State` (`../sim/state.hpp`), which the code
running the passes provides (`state()`, in `port.hpp`).

`level.cpp` runs one pass of the game's level loop (`level_pass`, one level
tick), from the `level_loop` label in `JE_main` to the jump back to it, in
three steps:
1. The tick's simulation (`sim::level_tick`), in the C code's order, which
   draws nothing but records what the drawing needs. The C code makes a few
   changes after presenting the frame (`JE_starShowVGA`); they read no input
   and draw nothing, so the C++ code makes them before.
2. The drawing, from the draw record (`render::draw_tick`).
3. If the simulation reached the frame (it doesn't when the in-game menu or
   help opened), the wait for the frame deadline, the present and the event
   poll.

What runs the passes provides, besides the `State` and the draw list, a `Runner`
(`level.hpp`): it presents the frame, polls events, and runs the C loops that
have their own drawing (the pause screen, the in-game menu and help, through
`c_loop`), which the simulation asks for through its `sim::Host`. The shadow
tick presents and polls nothing, and abandons the pass at a C loop. The C++
tick in place of the C code's (`../cxx_tick.hpp`) draws the pass's list before
it presents. Before a C loop, it draws what the tick has recorded so far
(`draw_so_far`), as the C code has drawn the tick by then, and it gives the
loop the `State` in the C globals and takes it back after. The C loops draw on
the C code's `VGAScreen`, and save and restore it: it is set from the record's
surface before one, and the record's surface from it after, and at the end of
the pass.

The one piece of state the loop keeps outside the `State` is `old_weapon_bar`,
which the C code keeps in a local variable of `JE_main` and sets when a level
starts; the C++ tick does the same (`level_locals`, a `sim::LevelLocals`).

To see which C++ lines the checks reach, build a copy with coverage and run
gcov on `../sim/` and `../render/tick/`:

```sh
make WITH_TYRX=true CXXFLAGS="-O1 -g --coverage" LDFLAGS="--coverage"
tools/run_headless.sh ./opentyrian DATA a.log input 1 20000 TYRX_INPUT_CHEATS=1 TYRX_SHADOW=1
gcov -o obj/tyrx/sim src/tyrx/sim/*.cpp   # writes *.gcov; ##### marks lines never run
```
