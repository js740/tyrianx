# render: drawing

`render.hpp` holds C++ ports of the C drawing primitives (`sprite.cpp`,
`vga256d.cpp`, `background.cpp`), which draw from explicit state onto an 8-bit
`Canvas`. They are the start of a renderer that can draw frames independently
of the C code, which changes game state while it draws. Every port must produce
exactly the C function's pixels.

`draw_list.hpp` describes a level tick's drawing as a *draw list*: calls to
those primitives, each with its arguments and, for primitives that draw from
game state (backgrounds, superpixels, the starfield), that state as it was at
the call. The level tick's drawing (`tick/`) builds the list of a tick
(`tick/draw.hpp`), and `execute.cpp` draws a call with the ports.

The ports are checked by replaying the C code's recorded draw lists with them
(`TYRX_DRAWLIST_CHECK`, `../check/draw/README.md`), and `draw_background_1` is
also checked call by call (`TYRX_RENDER_DIFF`, `../check/README.md`). Most of
the C drawing is called from inside `tyrian2.c`, where it can't be wrapped, so
the shadow tick checks whole frames instead: the pixels drawn from the C++
level tick's list against those the C code drew.

## The level tick's drawing

`tick/` draws a level tick after its simulation (`../sim/level.cpp`), as the C
code draws it: `draw_tick` (`tick/draw_tick.cpp`) draws it from the game state
at the end of the tick and from the draw record that the simulation keeps
(`../sim/draw_record.hpp`), in the C code's order, with a function for each
part (`X_draw`, in the file of the same subject). The `draw::` functions
(`tick/draw.hpp`) add the same calls as the C primitives record to a draw list,
which the code running the tick provides: the shadow tick
(`../check/shadow.cpp`) gives each pass a list of its own, and the C++ tick in
place of the C code's (`../cxx_tick.hpp`) draws its list as the pass goes.

The record holds only what the state at the end of the tick no longer shows:
which parts were drawn, on which surface, and values that the simulation went on
to change after the C code drew them. It names the surfaces (`sim::Surface`:
`VGAScreenSeg`, `game_screen` and `VGAScreen2`), and keeps the one that the C
code's `VGAScreen` points at, which the simulation changes where the C code
changes `VGAScreen`; the drawing draws on the surfaces it names (`surface`, in
`tick/tick.hpp`). Each part of the playing area is drawn at most once a tick,
at a fixed point in the C code's order, so the record has a place for each.
The HUD is different: the C code draws its gauges and messages as they change,
wherever that happens in the tick, and they overlap, so the record logs them in
order. The HUD's surface is only copied onto at the end of the tick, so
`draw_tick` draws the logged HUD first.

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

Nothing is drawn during the simulation of a C++ level tick: `TYRX_SHADOW_CALLS`
counts no calls before the drawing begins in the shadow tick's runs.
