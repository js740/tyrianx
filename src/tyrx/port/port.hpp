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

// The State that the level loop's passes (level.hpp) work on, which the code
// running them provides.

#pragma once

#include "../gold/api.hpp"
#include "../render/tick/tick.hpp"
#include "../sim/hidden.hpp"
#include "../sim/sim.hpp"

namespace tyrx::port {

// The State the tick works on, set by the code running the tick (the shadow
// tick, check/shadow.cpp, or the C++ tick, ../cxx_tick.cpp).
inline sim::State *& state_target() {
  static sim::State *state = nullptr;
  return state;
}

inline sim::State& state() { return *state_target(); }

// The C globals that sim::State holds are hidden here (../sim/hidden.hpp).
using namespace hidden;

using sim::DrawRecord, sim::Surface;

}  // namespace tyrx::port
