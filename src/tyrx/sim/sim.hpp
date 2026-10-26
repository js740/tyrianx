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

// C++ ports of the game logic whose state is encapsulated: each function
// takes the state it works on instead of using the C globals.  Code moves
// here from port/ as the globals it uses move into structures.
//
// Each function must match the C function it ports exactly.  The shadow tick
// checks that for the code the level tick runs, and check/diff.cpp checks the
// functions it wraps call by call (TYRX_DIFF).

#pragma once

#include "../gold/api.hpp"

#include <span>

namespace tyrx::sim {

// Random numbers: ports draw from an MtState with the reentrant C functions
// (mt_rand_r etc. in mtrand.h) rather than a C++ copy of the generator.

// varz.c
void setup_explosion(std::span<Explosion, MAX_EXPLOSIONS> explosions, JE_integer x, JE_integer y,
                     JE_integer delta_y, JE_integer type, bool fixed_position, bool follow_player);

}  // namespace tyrx::sim
