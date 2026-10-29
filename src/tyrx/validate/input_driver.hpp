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
//------------------------------------------------------------------------------

#pragma once

#include <SDL.h>

namespace tyrx::validate {

// Called as the virtual clock advances from `from` to `to` milliseconds.
void input_driver_advance(uint32_t from, uint32_t to);

// Called at the top of each level tick.
void input_driver_level_tick();

}  // namespace tyrx::validate
