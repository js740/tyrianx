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

// Rewind on the C++ game's state (TYRX_REWIND=TICKS): the State at the start
// of each of the last TICKS passes of the C++ game (game.hpp).
// While the rewind key is held, each pass drops the newest and restores the one
// before it, and then simulates (and draws) one tick from there, so every frame
// steps back one tick.  Input is not restored, so releasing the key resumes
// play with the keys actually held.  It is off in demos and network games, and
// the states are dropped when a level starts.
//
//   TYRX_REWIND_KEY=NAME    rewind key, as an SDL scancode name (default Backspace)

#pragma once

#include "sim/sim.hpp"

#include <SDL.h>

namespace tyrx {

// The key that rewinds while held.
SDL_Scancode rewind_key();

// Called when a level starts.
void rewind_level_start();

// Called before each pass of the C++ game, with the State it will run on and
// the level loop's own state: keeps them, or while the rewind key is held,
// restores earlier ones.
void rewind_before_pass(sim::State& state, sim::LevelLocals& locals);

}  // namespace tyrx
