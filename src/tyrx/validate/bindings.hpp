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

// Checks of the binding between sim::State and the C globals that hold its
// state in the C game (../c/state.hpp), with C++26's reflection: that each
// member of the State has a C global and each mirror lists every member of
// both structs (../c/mirror.hpp), and that no pointer in the State points
// into a C global it holds.  Code that runs the C++ game on a State read from
// the C globals scrambles them while it runs, so that code that still uses
// the globals, which no longer hold the state, goes wrong visibly.

#pragma once

#include "../sim/state.hpp"

namespace tyrx::validate {

// Whether each member of each of State's subsystems has a C global, exactly
// once.  Prints the members that don't.
bool bindings_complete();

// Overwrites the C globals that the State holds with a pattern.
void scramble_bound_globals();

// Aborts if a pointer in the State points into one of the C globals that the
// State holds: a copy of the state would have to point into itself instead.
void check_bound_pointers(sim::State const& state);

}  // namespace tyrx::validate
