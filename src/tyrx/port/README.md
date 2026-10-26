# port: the C level tick, ported as it is

The parts of the C game's level tick, ported to C++ as they are: they work on
the C globals (`../gold/api.hpp`), with the C code's names, so that they can be
compared with it line by line. The shadow tick (`../check/README.md`) checks
them against the C code tick by tick. As state moves out of the globals into
structures, the code that uses it moves to `../sim/`.

## The level loop

`level.cpp` has the game's level loop in C++: `level_pass` runs one pass of it
(one level tick), from the `level_loop` label in `JE_main` to the jump back to
it, as a function for each phase of the C code's loop, in its order. The one
piece of state it keeps itself is `old_weapon_bar`, which the C code keeps in a
local variable of `JE_main`.

## Drawing

The parts of the tick ported to C++ draw where the C code draws, with the
`draw::` functions (`draw.hpp`): each adds the call that the C primitive of the
same name records (`../check/draw/README.md`) to the tick's draw list
(`../render/draw_list.hpp`), instead of drawing. The code running the tick
provides the list (`draw::target`): the shadow tick (`../check/shadow.cpp`)
gives each pass a list of its own, which the C primitives that the pass still
calls add their calls to as well. So the list has the whole tick's drawing, in
the C code's order.

Ported so far:
- The drawing at the end of the tick: the superpixels (`effects.cpp`,
  `JE_drawSP`), the screen filter (`JE_filterScreen`), the boss bar (`hud.cpp`,
  `draw_boss_bar`), the in-game displays (`JE_inGameDisplays`) and the copy of
  the playing area to the screen (in `JE_starShowVGA`), with the text drawing
  of `fonthand.c` (`text.cpp`).
- The explosions, the enemy shots, the low armor warning, the debug display,
  the bonus message, the level timer and game over, which the level loop draws
  itself.
- The backgrounds and the starfield (`background.cpp`), the smoothie filters
  and the darkening.
- The HUD at the top of the tick: the message bar, the shield
  (`JE_drawShield`), the weapon power and the power bar.
- The enemies (`enemy.cpp`, `JE_drawEnemy`), which are drawn, then move, fire
  and launch enemies.
- The player shots (`shots.cpp`, `player_shot_move_and_draw`).
- The players (`player.cpp`, `JE_mainGamePlayerFunctions`, `JE_playerMovement`,
  `JE_doSpecialShot`): input, movement, firing and sidekicks, with the armor,
  shield and fire-mode HUD they draw (`JE_drawArmor`, `JE_wipeShieldArmorBars`,
  `JE_drawPortConfigButtons`).
- The player collisions and damage (`JE_playerCollide`, `JE_playerDamage`),
  with the message window and the sidekick HUD they draw (`JE_drawTextWindow`,
  `JE_drawOptions`, `JE_drawOptionLevel`).
- The events (`events.cpp`, `JE_eventSystem`), the key commands
  (`keyboard.cpp`, `JE_mainKeyboardInput`) and `JE_specialComplete`, which draw
  messages and the HUD.

With these, the C++ level tick makes all of its drawing calls itself. A call
graph of the C sources shows that none of the C functions it still calls can
draw, apart from the pause screen, the in-game menu and help, which the shadow
tick doesn't run.

The game logic that the tick calls is ported as well (creating explosions,
superpixels, enemies and player shots, event jumps, special-move codes, ship
info and power-ups), checked by the shadow tick's state comparison. What the
C++ tick still calls in C is the platform layer (input, sound, music, frame
pacing, sprite loading, the random number generator), the pause screen, the
in-game menu and help, which are separate loops with their own drawing, and a
few settings functions.
