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

// The validation of the C++ game against the C game, as the game's
// Validation (../game.hpp): it runs the checks at the game's hooks, decides
// whether the C++ game runs the levels (TYRX_CXX_TICK=1) or the C code does,
// and ends runs.  Configured by environment variables:
//
//   TYRX_CXX_TICK=1          run the levels with the C++ game, in place of the
//                            C code's level loop
//   TYRX_EXIT_AFTER_DEMOS=N  exit when demo playback starts for the N+1th time
//   TYRX_EXIT_AFTER_TICKS=N  exit at the top of level tick N
//
// Exits that the validation initiates return a status that names the first
// check that failed:
//
//   3  the hashes may be incomplete: some pointers could not be described
//   4  a round trip failed (TYRX_ROUNDTRIP)
//   5  a draw list was incomplete or didn't reproduce its tick
//      (TYRX_DRAWLIST_CHECK)
//   6  a shadow pass differed from its C tick (TYRX_SHADOW)

#pragma once

#include <stdint.h>

namespace tyrx::validate {

// The number of level ticks run so far, i.e. the index of the next one.
uint64_t current_tick();

// Whether a pass of the C++ game, either the shadow tick's (shadow.hpp) or
// the C++ game's own, is running, outside the C loops it runs.
bool cxx_pass_active();

}  // namespace tyrx::validate
