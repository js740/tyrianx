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

#include "vclock.hpp"

#include "input_driver.hpp"
#include "runtime.hpp"

#include <ctime>

namespace tyrx {

namespace {

bool const enabled = env("TYRX_VCLOCK") != nullptr;
Uint32 now_ms = 0;

}  // namespace

bool vclock_enabled() { return enabled; }

Uint32 vclock_now() { return now_ms; }

}  // namespace tyrx

// Linked with -Wl,--wrap=SDL_GetTicks,--wrap=SDL_Delay,--wrap=time so that the
// C code's calls come here.
extern "C" Uint32 __real_SDL_GetTicks(void);
extern "C" void __real_SDL_Delay(Uint32 ms);
extern "C" std::time_t __real_time(std::time_t *t);

extern "C" Uint32 __wrap_SDL_GetTicks(void) { return tyrx::enabled ? tyrx::now_ms : __real_SDL_GetTicks(); }

extern "C" void __wrap_SDL_Delay(Uint32 ms) {
  if (tyrx::enabled) {
    Uint32 const from = tyrx::now_ms;
    tyrx::now_ms += ms;
    tyrx::input_driver_advance(from, tyrx::now_ms);
  }
  else {
    __real_SDL_Delay(ms);
  }
}

extern "C" std::time_t __wrap_time(std::time_t *t) {
  if (!tyrx::enabled) {
    return __real_time(t);
  }
  std::time_t const now = 946684800 + tyrx::now_ms / 1000;
  if (t) {
    *t = now;
  }
  return now;
}
