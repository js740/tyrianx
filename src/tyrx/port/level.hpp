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

//------------------------------------------------------------------------------
// The game's level loop in C++.  level_pass runs one pass of it: one level
// tick, from the level_loop label in JE_main (tyrian2.c) to the jump back to
// it, on the C code's globals.  It is checked against the C code by running it
// in the shadow of each C tick (check/shadow.hpp).
//------------------------------------------------------------------------------

#pragma once

namespace tyrx::port {

// How a pass ended: like the C code, back to level_loop for the next tick, or
// to start_level to end the level; or not at all, for network games.
enum class LevelPass { level_loop, start_level, unsupported };

LevelPass level_pass();

// State the C code keeps in JE_main's local variables, which outlive a pass.
struct LevelLocals {
  unsigned old_weapon_bar[2] = {0, 0};  // weapon power bars are only redrawn when they change
};

LevelLocals& level_locals();

// Provided by whatever runs the passes (check/shadow.cpp): present the frame,
// poll events, and a notice just before JE_mainKeyboardInput opens the in-game
// help.
void present();
void poll_events();
void in_game_help_ahead();

}  // namespace tyrx::port
