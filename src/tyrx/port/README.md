# port: the C level tick, ported as it is

The parts of the C game's level tick, ported to C++ as they are: they work on
the C globals (`../gold/api.hpp`), with the C code's names, so that they can be
compared with it line by line. The shadow tick (`../check/README.md`) checks
them against the C code tick by tick, and with `TYRX_CXX_TICK=1` they run the
game's levels in place of the C code (`../cxx_tick.hpp`). As state moves out of
the globals into structures, the code that uses it moves to `../sim/`.

The state moved so far is in a `sim::State` (`../sim/state.hpp`), which the code
running the tick provides (`state()`). `../sim/hidden.hpp` hides the C globals
that held it from code here and in `../sim/`, so using one by mistake doesn't
compile. Code here uses it in one of three ways:
- The explosions, the repeating explosions, the superpixels and the starfield,
  the enemies' creation and update (`JE_makeEnemy`, `JE_newEnemy` and
  `JE_drawEnemy`), the player shots (`player_shot_create` and the like) and
  what they do to the enemies they hit, the level's events (`JE_eventSystem`
  and the like), the enemy shots, the players (`JE_mainGamePlayerFunctions`,
  `JE_playerCollide`, `JE_playerDamage` and the like) and their specials, and
  the HUD's log: their code has moved to `../sim/`, which code here calls with
  `state()`.
- `port.hpp`'s `mt_rand` etc. draw from the random number generator's state.
- Functions here bind references with the C globals' names to it, and are
  otherwise unchanged: the game's modes, cheats and progress, the sound queue
  and the music's fade, the display effects, the map, the level's events and
  ending, the buttons, the players, the specials, the enemies, and the player
  and enemy shots. `COUNTOF` counts their arrays, and `port.hpp` has versions of
  `player.h`'s inline functions. The C functions that use them and that the
  tick runs are the pause screen, the in-game menu and help (`JE_pauseGame`,
  `JE_doInGameSetup` and `JE_inGameHelp`), which the shadow tick doesn't run,
  and `playDemoKeys` (the demo input, which moves player 1 and sets the
  buttons), which `../sim/player.cpp` gives player 1's position and the buttons
  in the C globals.

## The level loop

`level.cpp` has the game's level loop in C++: `level_pass` runs one pass of it
(one level tick), from the `level_loop` label in `JE_main` to the jump back to
it, with the parts of the C code that draw ported to C++ and split in two (see
"Separating the drawing" below). A pass runs in three steps:
1. The tick's simulation (`simulate`, a function for each phase of the C
   code's loop), in the C code's order, which draws nothing but records what
   the drawing needs. The C code makes a few changes after presenting the frame
   (`JE_starShowVGA`); they read no input and draw nothing, so the C++ code
   makes them before.
2. The drawing, from the draw record (`draw_tick`).
3. If the simulation reached the frame (it doesn't when the in-game menu or
   help opened), the wait for the frame deadline, the present and the event
   poll.

What runs the passes provides, besides the `State` and the draw list, a `Runner`
(`level.hpp`): it presents the frame, polls events, and runs the C loops that
have their own drawing (the pause screen, the in-game menu and help, through
`c_loop`). The shadow tick presents and polls nothing, and abandons the pass at
a C loop. The C++ tick in place of the C code's (`../cxx_tick.hpp`) draws the
pass's list before it presents. Before a C loop, it draws what the tick has
recorded so far (`draw_so_far`), as the C code has drawn the tick by then, and
it gives the loop the `State` in the C globals and takes it back after.

The one piece of state the loop keeps itself is `old_weapon_bar`, which the C
code keeps in a local variable of `JE_main` and sets when a level starts; the
C++ tick does the same (`level_locals`).

## Separating the drawing

Each part of the tick that is ported to C++ is split in two. Its simulation
(`X_tick`) runs where the C code runs it, and keeps what its drawing will need
in the tick's draw record (`draw_record.hpp`). Its drawing (`X_draw`) runs after
the whole simulation: `draw_tick` (`draw_tick.cpp`) draws the tick from the game
state at the end of the tick and from the record, in the C code's order. The
`draw::` functions (`draw.hpp`) add the same calls as the C primitives record to
a draw list (`../render/draw_list.hpp`), which the code running the tick
provides: the shadow tick (`../check/shadow.cpp`) gives each pass a list of its
own, and the C++ tick in place of the C code's draws its list as the pass goes.

The record holds only what the state at the end of the tick no longer shows:
which parts were drawn, on which surface, and values that the simulation went
on to change after the C code drew them. Each part of the playing area is
drawn at most once a tick, at a fixed point in the C code's order, so the
record has a place for each. The HUD is different: the C code draws its
gauges and messages as they change, wherever that happens in the tick, and
they overlap, so the record logs them in order. The HUD's surface is only
copied onto at the end of the tick, so `draw_tick` draws the logged HUD first.

| Part | Record |
|---|---|
| Backgrounds 1-3 (`background.cpp`) | The scroll position each was drawn at |
| Starfield, superpixels | Nothing: the drawing moves them back by their speed; for superpixels, which ones were active |
| Enemies (`enemy.cpp`, `JE_drawEnemy`'s drawing; the update is in `../sim/enemy.cpp`) | The enemies that their update returns: each drawn enemy's position before it moved, frame, filter, sprite sheet and size (damage changes the frame and filter, and the slot can be reused) |
| Player shots (`shots.cpp`, `player_shot_move_and_draw`'s drawing; the update is in `../sim/player_shot.cpp`) | Each drawn shot's position and frame, and whether it has a shadow: a hit can free the slot for a chain-reaction shot |
| Players (`player.cpp`, `JE_playerMovement`'s and `JE_doSpecialShot`'s drawing; the update is in `../sim/player.cpp` and `../sim/special.cpp`) | The players that their update returns: the positions and ship and sidekick frames they were drawn with, and the charge, each with whether the HUD's drawing had left `VGAScreen` at `game_screen` |
| Explosions (`../sim/explosion.cpp`) | The visible explosions that their update returns: which slots were drawn, and the position and frame of those that ended (a new explosion can take the slot) |
| Enemy shots (their update is in `../sim/enemy_shot.cpp`) | Which slots their update drew (drawn shots stay in their slots for the rest of the tick) |
| Boss bars | The bars' positions, armor and colours |
| Screen filter | The filter and brightness before the fade moves on |
| Smoothie filters, darkening | The surfaces, and the darkening's level |
| Warning, level timer, game over, in-game displays | Nothing but the surface; for the debug display and the bonus message, the texts and the flash |
| HUD (`hud.cpp`; the log is `../sim/hud.hpp`'s) | A log of the shield, armor, weapon power, power bar, sidekick, charge and ammo gauges and messages as drawn, from the level tick, the player functions, collisions and damage (`JE_playerCollide`, `JE_playerDamage`), events (`../sim/events.cpp`, `JE_eventSystem`) and key commands (`keyboard.cpp`, `JE_mainKeyboardInput`) |

To see which C++ lines the checks reach, build a copy with coverage and run
gcov on `port/`:

```sh
make WITH_TYRX=true CXXFLAGS="-O1 -g --coverage" LDFLAGS="--coverage"
tools/run_headless.sh ./opentyrian DATA a.log input 1 20000 TYRX_INPUT_CHEATS=1 TYRX_SHADOW=1
gcov -o obj/tyrx/port src/tyrx/port/*.cpp   # writes *.gcov; ##### marks lines never run
```

With the runs above, including arcade games, about four fifths of the lines of
`port/` run. Most of the rest is some special weapons, the edited ships, the
level timer, the higher difficulties, network play, and the pause screen and
menu (which the shadow tick doesn't run).

Nothing is drawn during the simulation of a C++ level tick: `TYRX_SHADOW_CALLS`
counts no calls before the drawing begins in the runs above. The game logic that
the tick calls is ported as well (creating explosions, superpixels, enemies and
player shots, event jumps, special-move codes, ship info and power-ups), checked
by the shadow tick's state comparison. What the C++ tick still calls in C is the
platform layer (input, sound, music, frame pacing, sprite loading, the random
number generator's functions, on the `State`'s generator), the pause screen, the
in-game menu and help, which are separate loops with their own drawing, and a
few settings functions.
