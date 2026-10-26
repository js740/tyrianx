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
// Late input sampling (TYRX_LATE_SAMPLING, in real time, so without
// TYRX_VCLOCK), a prototype of reading each tick's input shortly before its
// deadline instead of right after the previous present.  It changes only when
// things happen in real time, not what the game computes from its input.
//
//   TYRX_LATE_SAMPLING=precise  only make the wait before each present precise:
//                               sleep until about 1 ms before the deadline,
//                               then spin
//   TYRX_LATE_SAMPLING=auto     also wait before reading the next tick's
//                               input, until a budget before its deadline;
//                               the budget covers the slowest of the last 64
//                               ticks' work and sleep overshoot, plus 0.5 ms
//   TYRX_LATE_SAMPLING=MS       the same with a fixed budget of MS milliseconds
//
// A summary goes to stderr at exit.  TYRX_PRESENT_LOG measures the effect.
//------------------------------------------------------------------------------

#pragma once

namespace tyrx {

// Called at the top of each level tick, and when a level ends.
void late_sampling_level_tick();
void late_sampling_level_ended();

// Called by the pacing wraps (pacing.cpp).  late_sampling_wait replaces
// delayUntilElapsed in level ticks and returns false where the C code's wait
// should run instead.
bool late_sampling_wait();
void late_sampling_presented();
void late_sampling_before_poll();

}  // namespace tyrx
