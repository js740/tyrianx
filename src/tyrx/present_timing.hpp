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
// Present timing (TYRX_PRESENT_LOG=FILE, in real time, so without TYRX_VCLOCK):
// for each level tick, when the frame-pacing wait in JE_starShowVGA woke up and
// when JE_showVGA presented the frame, relative to the tick's deadline, and
// how long before the present the tick's input was read.  A summary goes to
// stderr at exit.  Times are in milliseconds (see pacing.hpp).
//------------------------------------------------------------------------------

#pragma once

#include <SDL.h>

namespace tyrx {

// Called at the top of each level tick, and when a level ends.
void present_timing_level_tick();
void present_timing_level_ended();

// Called by the pacing wraps (pacing.cpp).
bool present_timing_enabled();
void present_timing_before_wait();
void present_timing_after_wait();
void present_timing_after_set_count(Uint16 count);
void present_timing_presented(double start, double end);
void present_timing_polled();

}  // namespace tyrx
