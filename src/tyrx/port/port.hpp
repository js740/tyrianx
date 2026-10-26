//------------------------------------------------------------------------------
// Tyrian X: A modern C++ cross-platform port of Tyrian
// Copyright (C) The Tyrian X Development Team
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//------------------------------------------------------------------------------

// The parts of the level tick ported from the C code, as they are: they work on
// the C globals, with the C code's names.  Those that draw add their calls to
// the tick's draw list (draw.hpp) where the C code draws.

#pragma once

#include "../gold/api.hpp"

namespace tyrx::port {

// background.cpp
void background_2(SDL_Surface *screen, bool blend);   // draw_background_2, _blend
void background_3(SDL_Surface *screen);               // draw_background_3
void starfield(SDL_Surface *screen, int move_speed);  // update_and_draw_starfield

// effects.cpp
void draw_sp();                                               // JE_drawSP
void filter_screen(JE_shortint col, JE_shortint brightness);  // JE_filterScreen

// enemy.cpp
void draw_enemy(int enemyOffset);  // JE_drawEnemy

// hud.cpp
void draw_shield();                          // JE_drawShield
void draw_boss_bars();                       // draw_boss_bar
void in_game_displays(SDL_Surface *screen);  // JE_inGameDisplays

}  // namespace tyrx::port
