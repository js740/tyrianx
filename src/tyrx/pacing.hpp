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
// The C code's frame pacing as seen from C++.  JE_starShowVGA waits for each
// level tick's deadline (delayUntilElapsed), sets the next one (setFrameCount),
// presents the frame (JE_showVGA) and polls events (handleSdlEvents).  The
// wraps of those calls, linked with -Wl,--wrap, live in pacing.cpp and pass
// them on to present timing and late sampling.
//
// The deadline is the C code's frameCountEnd, which is static in nortsong.c:
// the wraps of setFrameSpeed and setFrameCount repeat its fixed-point
// arithmetic exactly.  nortsong.c never calls them itself, so every change to
// the deadline comes through them.
//------------------------------------------------------------------------------

#pragma once

#include <SDL.h>

namespace tyrx {

// A copy of the C code's frame-pacing deadline, in its fixed-point
// milliseconds (UQ22.10).
struct FrameDeadline {
  Uint16 period = ((Uint64)0x4300 << 10) * 1000 * 88 * 3 / 315000000;
  Uint32 end = 0;
  bool moved = false;  // set when setFrameCount gives up on whole periods

  void set_speed(Uint16 speed, Uint32 now_ticks);
  void set_count(Uint16 count, Uint32 now_ticks);

  // What getFrameCountTicks returns at `now_ticks`.
  Uint32 remaining_ticks(Uint32 now_ticks) const;

  // The deadline in milliseconds on SDL_GetTicks' clock.
  double ms() const;
};

FrameDeadline& frame_deadline();

// Milliseconds on SDL_GetTicks' clock, with the resolution of
// SDL_GetPerformanceCounter: the counter, offset so that its whole
// milliseconds match SDL_GetTicks.
class PacingClock {
 public:
  PacingClock() { calibrate(); }

  double now() const { return (double)(SDL_GetPerformanceCounter() - epoch_) * 1000 / freq_; }

  // Whether the clock still agrees with SDL_GetTicks; if not, recalibrates.
  bool check();

  // Sleeps until about `t` (with SDL_Delay, which may overshoot), or spins
  // until exactly `t`.
  void sleep_until(double t) const;
  void spin_until(double t) const;

 private:
  void calibrate();

  Uint64 freq_ = 1;
  Uint64 epoch_ = 0;
};

PacingClock& pacing_clock();

}  // namespace tyrx
