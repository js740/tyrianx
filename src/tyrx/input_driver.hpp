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
// Deterministic synthetic keyboard input (TYRX_INPUT_SEED=S, with TYRX_VCLOCK):
// outside levels, a key every 350 ms that heads for the next level through the
// menus; in levels, gameplay keys whose state is a function of the seed and the
// tick.
//
//   TYRX_INPUT_REWIND=1     also hold the rewind key for 40 of every 400 ticks
//   TYRX_INPUT_COMMANDS=1   also press a command for one of every 300 ticks
//                           (Backspace+Minus, which cycles the level filter)
//   TYRX_INPUT_MIDTICK=1    send each tick's input at the top of the tick
//                           before, while it runs, instead of during the
//                           frame-pacing wait between them
//------------------------------------------------------------------------------

#pragma once

#include <SDL.h>

namespace tyrx {

// Called as the virtual clock advances from `from` to `to` milliseconds.
void input_driver_advance(Uint32 from, Uint32 to);

// Called at the top of each level tick.
void input_driver_level_tick();

// Called when the game's key state has been restored from a snapshot.
void input_driver_snapshot_restored();

}  // namespace tyrx
