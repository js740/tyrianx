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

// The C globals that hold the state of sim::State's members in the C game,
// and copying between the two.  Code that runs C++ code on a State the C game
// left (the shadow tick) reads the State from the C globals, scrambles them,
// runs the C++ code and writes the State back.  The scrambling makes code that
// still uses the globals, which no longer hold the state, go wrong visibly.

#pragma once

#include "../sim/state.hpp"

namespace tyrx::gold {

void read_sim_state(sim::State& state);
void write_sim_state(sim::State const& state);
void scramble_sim_state_globals();

// Whether each member of each of State's subsystems has a C global, exactly
// once.  Prints the members that don't.
bool sim_state_bound();

}  // namespace tyrx::gold
