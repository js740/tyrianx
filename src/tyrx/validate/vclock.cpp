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

#include "../env.hpp"
#include "input_driver.hpp"
#include "runtime.hpp"

#include <ctime>

namespace tyrx::validate {

namespace {

bool const enabled = env("TYRX_VCLOCK") != nullptr;
uint32_t   now_ms = 0;

}  // namespace

bool vclock_enabled() { return enabled; }

uint32_t vclock_now() { return now_ms; }

}  // namespace tyrx::validate

// Linked with -Wl,--wrap=SDL_GetTicks,--wrap=SDL_Delay,--wrap=time so that the
// C code's calls come here.
extern "C" uint32_t    __real_SDL_GetTicks(void);
extern "C" void        __real_SDL_Delay(uint32_t ms);
extern "C" std::time_t __real_time(std::time_t *t);

extern "C" uint32_t __wrap_SDL_GetTicks(void) {
  return tyrx::validate::enabled ? tyrx::validate::now_ms : __real_SDL_GetTicks();
}

extern "C" void __wrap_SDL_Delay(uint32_t ms) {
  if (tyrx::validate::enabled) {
    uint32_t const from = tyrx::validate::now_ms;
    tyrx::validate::now_ms += ms;
    tyrx::validate::input_driver_advance(from, tyrx::validate::now_ms);
  }
  else {
    __real_SDL_Delay(ms);
  }
}

extern "C" std::time_t __wrap_time(std::time_t *t) {
  if (!tyrx::validate::enabled) {
    return __real_time(t);
  }
  std::time_t const now = 946684800 + tyrx::validate::now_ms / 1000;
  if (t) {
    *t = now;
  }
  return now;
}
