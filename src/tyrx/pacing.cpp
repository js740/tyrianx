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

#include "pacing.hpp"

#include "c_api.hpp"
#include "late_sampling.hpp"
#include "present_timing.hpp"
#include "vclock.hpp"

extern "C" Uint32 __real_SDL_GetTicks(void);

namespace tyrx {

void FrameDeadline::set_speed(Uint16 speed, Uint32 now_ticks) {
  period = ((Uint64)speed << 10) * 1000 * 88 * 3 / 315000000;
  end = now_ticks << 10;
}

void FrameDeadline::set_count(Uint16 count, Uint32 now_ticks) {
  Uint32 const now = now_ticks << 10;
  Sint32 const diff = now - end;
  if (diff >= period) {
    end = now - (Uint32)diff % period;
    moved = true;
  }
  else if (-diff >= period) {
    end = now + (Uint32)-diff % period;
    moved = true;
  }
  end += count * period;
}

Uint32 FrameDeadline::remaining_ticks(Uint32 now_ticks) const {
  Sint32 const diff = end - (now_ticks << 10);
  return diff >= 0 ? ((Uint32)diff + (1 << 9)) >> 10 : 0;
}

double FrameDeadline::ms() const {
  // `end` wraps every 2^22 ms, so go via the (signed) time remaining.
  Uint32 const ticks = __real_SDL_GetTicks();
  return ticks + (Sint32)(end - (ticks << 10)) / 1024.0;
}

FrameDeadline& frame_deadline() {
  static FrameDeadline d;
  return d;
}

// Finds the counter value at which SDL_GetTicks ticks over.
void PacingClock::calibrate() {
  freq_ = SDL_GetPerformanceFrequency();
  Uint32 const t0 = __real_SDL_GetTicks();
  Uint64 before = SDL_GetPerformanceCounter();
  for (;;) {
    Uint32 const t = __real_SDL_GetTicks();
    Uint64 const after = SDL_GetPerformanceCounter();
    if (t != t0) {
      epoch_ = (before + after) / 2 - (Uint64)t * freq_ / 1000;
      return;
    }
    before = after;
  }
}

bool PacingClock::check() {
  double const t = now();
  Uint32 const ticks = __real_SDL_GetTicks();
  if (t > ticks - 0.05 && t < ticks + 1.05) {
    return true;
  }
  calibrate();
  return false;
}

void PacingClock::sleep_until(double t) const {
  double const remaining = t - now();
  if (remaining >= 1) {
    SDL_Delay((Uint32)remaining);
  }
}

void PacingClock::spin_until(double t) const {
  while (now() < t) {
  }
}

PacingClock& pacing_clock() {
  static PacingClock c;
  return c;
}

}  // namespace tyrx

extern "C" void __real_setFrameSpeed(Uint16 speed);
extern "C" void __real_setFrameCount(JE_word frameCount);
extern "C" void __real_delayUntilElapsed(void);
extern "C" void __real_JE_showVGA(void);
extern "C" void __real_handleSdlEvents(void);

extern "C" void __wrap_setFrameSpeed(Uint16 speed) {
  __real_setFrameSpeed(speed);
  tyrx::frame_deadline().set_speed(speed, tyrx::last_sdl_ticks());
}

extern "C" void __wrap_setFrameCount(JE_word frameCount) {
  __real_setFrameCount(frameCount);
  tyrx::frame_deadline().set_count(frameCount, tyrx::last_sdl_ticks());
  tyrx::present_timing_after_set_count(frameCount);
}

extern "C" void __wrap_delayUntilElapsed(void) {
  tyrx::present_timing_before_wait();
  if (!tyrx::late_sampling_wait()) {
    __real_delayUntilElapsed();
  }
  tyrx::present_timing_after_wait();
}

extern "C" void __wrap_JE_showVGA(void) {
  bool const timed = tyrx::present_timing_enabled();
  double const start = timed ? tyrx::pacing_clock().now() : 0;
  __real_JE_showVGA();
  if (timed) {
    tyrx::present_timing_presented(start, tyrx::pacing_clock().now());
  }
  tyrx::late_sampling_presented();
}

extern "C" void __wrap_handleSdlEvents(void) {
  tyrx::late_sampling_before_poll();
  tyrx::present_timing_polled();
  __real_handleSdlEvents();
}
