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

#include "state.hpp"

#include "mirror.hpp"

namespace tyrx::c {

void read_state(sim::State& state) {
  for_each_binding(state, [](auto& cxx, auto const& c) { copy(cxx, c); });
}

void write_state(sim::State const& state) {
  for_each_binding(state, [](auto const& cxx, auto& c) { copy(c, cxx); });
}

}  // namespace tyrx::c
