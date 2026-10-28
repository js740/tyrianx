# port: the level tick's drawing and frame

The C game's level tick in C++ is simulated by `../sim/` (`sim::level_tick`),
which records what the tick draws in a draw record (`../sim/draw_record.hpp`).
The code here draws the tick from that record, as the C code draws it, and runs
the tick's frame. The shadow tick (`../check/README.md`) checks both against
the C code tick by tick, and with `TYRX_CXX_TICK=1` they run the game's levels
in place of the C code (`../cxx_tick.hpp`).

The game's state is in a `sim::State` (`../sim/state.hpp`), which the code
running the tick provides (`state()`). `../sim/hidden.hpp` hides the C globals
that held it from code here and in `../sim/`, so using one by mistake doesn't
compile. The drawing reads the state at the end of the tick, and the C code's
sprites and surfaces.

## The level loop

`level.cpp` runs one pass of the game's level loop (`level_pass`, one level
tick), from the `level_loop` label in `JE_main` to the jump back to it, in
three steps:
1. The tick's simulation (`sim::level_tick`), in the C code's order, which
   draws nothing but records what the drawing needs. The C code makes a few
   changes after presenting the frame (`JE_starShowVGA`); they read no input
   and draw nothing, so the C++ code makes them before.
2. The drawing, from the draw record (`draw_tick`).
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
loop the `State` in the C globals and takes it back after.

The one piece of state the loop keeps outside the `State` is `old_weapon_bar`,
which the C code keeps in a local variable of `JE_main` and sets when a level
starts; the C++ tick does the same (`level_locals`, a `sim::LevelLocals`).

## The drawing

`draw_tick` (`draw_tick.cpp`) draws the tick from the game state at the end of
the tick and from the record, in the C code's order, with a function for each
part (`X_draw`, in the file of the same subject). The `draw::` functions
(`draw.hpp`) add the same calls as the C primitives record to a draw list
(`../render/draw_list.hpp`), which the code running the tick provides: the
shadow tick (`../check/shadow.cpp`) gives each pass a list of its own, and the
C++ tick in place of the C code's draws its list as the pass goes.

The record holds only what the state at the end of the tick no longer shows:
which parts were drawn, on which surface, and values that the simulation went on
to change after the C code drew them. It names the surfaces (`sim::Surface`:
`VGAScreenSeg`, `game_screen` and `VGAScreen2`), and keeps the one that the C
code's `VGAScreen` points at, which the simulation changes where the C code
changes `VGAScreen`; the drawing draws on the surfaces it names
(`draw_record.hpp`). The real `VGAScreen` is set from it for the C loops, which
save and restore it, and at the end of the pass. Each part of the playing area
is drawn at most once a tick, at a fixed point in the C code's order, so the
record has a place for each. The HUD is different: the C code draws its gauges
and messages as they change, wherever that happens in the tick, and they
overlap, so the record logs them in order. The HUD's surface is only copied onto
at the end of the tick, so `draw_tick` draws the logged HUD first.

| Part | Record |
|---|---|
| Backgrounds 1-3 (`background.cpp`) | The scroll position each was drawn at |
| Starfield, superpixels | Nothing: the drawing moves them back by their speed; for superpixels, which ones were active |
| Enemies (`enemy.cpp`, `JE_drawEnemy`'s drawing) | The enemies that their update returns: each drawn enemy's position before it moved, frame, filter, sprite sheet and size (damage changes the frame and filter, and the slot can be reused) |
| Player shots (`shots.cpp`, `player_shot_move_and_draw`'s drawing) | Each drawn shot's position and frame, and whether it has a shadow: a hit can free the slot for a chain-reaction shot |
| Players (`player.cpp`, `JE_playerMovement`'s and `JE_doSpecialShot`'s drawing) | The players that their update returns: the positions and ship and sidekick frames they were drawn with, and the charge, each with its surface |
| Explosions | The visible explosions that their update returns: which slots were drawn, and the position and frame of those that ended (a new explosion can take the slot) |
| Enemy shots | Which slots their update drew (drawn shots stay in their slots for the rest of the tick) |
| Boss bars (`hud.cpp`) | The bars' positions, armor and colours |
| Screen filter (`effects.cpp`) | The filter and brightness before the fade moves on |
| Smoothie filters, darkening | The surfaces, and the darkening's level |
| Warning, level timer, game over, in-game displays | Nothing but the surface; for the debug display and the bonus message, the texts and the flash |
| HUD (`hud.cpp`) | A log of the shield, armor, weapon power, power bar, sidekick, charge and ammo gauges and messages as drawn, from the level tick, the players, events and key commands |

To see which C++ lines the checks reach, build a copy with coverage and run
gcov on `port/` and `../sim/`:

```sh
make WITH_TYRX=true CXXFLAGS="-O1 -g --coverage" LDFLAGS="--coverage"
tools/run_headless.sh ./opentyrian DATA a.log input 1 20000 TYRX_INPUT_CHEATS=1 TYRX_SHADOW=1
gcov -o obj/tyrx/sim src/tyrx/sim/*.cpp   # writes *.gcov; ##### marks lines never run
```

Nothing is drawn during the simulation of a C++ level tick: `TYRX_SHADOW_CALLS`
counts no calls before the drawing begins in the shadow tick's runs.
