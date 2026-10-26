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

// Pixel checking of the C++ drawing ports in render/ (TYRX_RENDER_DIFF=1).
//
// A C drawing function F that is called across object files can be wrapped at
// link time (-Wl,--wrap=F).  With TYRX_RENDER_DIFF=1, the wrapper copies the
// surface F draws on, has the C++ port draw on the copy from the state before
// the call, runs the C function, and compares every pixel.  The C result
// always stands, so the game is unaffected.
//
// See render_diff.cpp for the wrapped functions.

#pragma once

#include <cstdint>

namespace tyrx::check {

bool render_diff_enabled();

// Number of calls whose pixels differed so far (all slices).
std::uint64_t render_diff_mismatches();

}  // namespace tyrx::check
