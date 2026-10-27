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

// The C++ level tick in place of the C code's.  With TYRX_CXX_TICK=1, each
// pass of the level loop is port::level_pass, through the hook that only this
// fork's C code has (../hooks_tyrx.h), on a sim::State that this owns during
// the level: it reads it from the C globals when the level starts and after C
// code runs (the pause screen, the in-game menu and help), scrambles the
// globals while the pass runs, and writes it back to them after each pass, for
// what looks at them between ticks (the hash log) and after the level.

#pragma once

namespace tyrx {

// Whether TYRX_CXX_TICK is set.  It doesn't run with the checks that run the C
// code's ticks (TYRX_SHADOW, TYRX_DRAWLIST_CHECK), nor in network games, whose
// passes run in C.  It rewinds with TYRX_REWIND (rewind.hpp).
bool cxx_tick_enabled();

// The level has loaded, before its first tick.
void cxx_tick_level_start();

// The C globals have been restored to an earlier tick's state (a round trip,
// TYRX_ROUNDTRIP): the next pass reads the State from them.
void cxx_tick_c_state_restored();

}  // namespace tyrx
