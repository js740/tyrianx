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

#include "rewind.hpp"

#include "check/roundtrip.hpp"
#include "cxx_tick.hpp"
#include "gold/api.hpp"
#include "runtime.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <deque>

namespace tyrx {

namespace {

struct Rewind {
  struct Kept {
    sim::State state;
    port::LevelLocals locals;
  };

  long capacity = 0;  // 0 if disabled
  SDL_Scancode key = SDL_SCANCODE_BACKSPACE;
  std::deque<Kept> kept;
  uint64_t rewound_ticks = 0;

  Rewind() {
    if ((capacity = env_long("TYRX_REWIND", 0)) <= 0) {
      return;
    }
    if (!cxx_tick_enabled()) {
      std::fprintf(stderr, "tyrx: TYRX_REWIND needs TYRX_CXX_TICK\n");
      std::exit(EXIT_FAILURE);
    }
    if (check::round_trip_enabled()) {
      std::fprintf(stderr, "tyrx: TYRX_REWIND is ignored with TYRX_ROUNDTRIP\n");
      capacity = 0;
    }
    if (char const *name = env("TYRX_REWIND_KEY")) {
      key = SDL_GetScancodeFromName(name);
      if (key == SDL_SCANCODE_UNKNOWN) {
        std::fprintf(stderr, "tyrx: unknown TYRX_REWIND_KEY '%s'\n", name);
        capacity = 0;
      }
    }
  }

  ~Rewind() {
    if (capacity > 0) {
      std::fprintf(stderr, "tyrx rewind: %" PRIu64 " ticks rewound, up to %ld states of %zu bytes kept\n",
                   rewound_ticks, capacity, sizeof(Kept));
    }
  }

  void before_pass(sim::State& state, port::LevelLocals& locals) {
    if (keysactive[key]) {
      // Nothing older to go back to: the newest stays, and isn't kept again.
      if (kept.size() < 2) {
        return;
      }
      kept.pop_back();
      state = kept.back().state;
      locals = kept.back().locals;
      ++rewound_ticks;
      return;
    }
    kept.push_back({state, locals});
    if ((long)kept.size() > capacity) {
      kept.pop_front();
    }
  }
};

Rewind& rewind() {
  static Rewind rw;
  return rw;
}

}  // namespace

SDL_Scancode rewind_key() { return rewind().key; }

void rewind_level_start() { rewind().kept.clear(); }

void rewind_before_pass(sim::State& state, port::LevelLocals& locals) {
  if (Rewind& rw = rewind(); rw.capacity > 0 && !state.game.play_demo && !isNetworkGame) {
    rw.before_pass(state, locals);
  }
}

}  // namespace tyrx
