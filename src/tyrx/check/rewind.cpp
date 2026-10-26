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

#include "../gold/api.hpp"
#include "../gold/state.hpp"
#include "../runtime.hpp"
#include "roundtrip.hpp"

#include <cinttypes>
#include <cstdio>
#include <deque>

namespace tyrx::check {

namespace {

// Total hash of the hashed blocks.
std::uint64_t total_hash() {
  gold::Hasher total;
  for (std::uint64_t const h : gold::block_hashes()) {
    total.u64(h);
  }
  return total.value();
}

struct Rewind {
  long capacity = 0;  // 0 if disabled
  SDL_Scancode key = SDL_SCANCODE_BACKSPACE;
  bool verify = false;

  std::deque<gold::Snapshot> ring;
  std::deque<std::uint64_t> hashes;  // total hash of each snapshot, if verifying

  std::uint64_t rewound_ticks = 0;
  std::uint64_t verify_failures = 0;
  std::uint64_t captured = 0;
  std::uint64_t unshared_bytes = 0;

  Rewind() {
    if ((capacity = env_long("TYRX_REWIND", 0)) <= 0) {
      return;
    }
    if (round_trip_enabled()) {
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
    verify = env("TYRX_REWIND_VERIFY") != nullptr;
  }

  ~Rewind() {
    if (capacity > 0) {
      std::fprintf(stderr,
                   "tyrx rewind: %" PRIu64 " ticks rewound, %" PRIu64 " snapshots taken averaging %" PRIu64
                   " unshared bytes%s\n",
                   rewound_ticks, captured, captured ? unshared_bytes / captured : 0,
                   verify ? (verify_failures ? ", VERIFY FAILED" : ", verified") : "");
    }
  }

  static bool included(gold::StateClass c) { return gold::is_hashed(c); }

  void clear() {
    ring.clear();
    hashes.clear();
  }

  void tick() {
    if (keysactive[key] && ring.size() >= 2) {
      ring.pop_back();
      ring.back().restore();
      if (verify) {
        hashes.pop_back();
        if (total_hash() != hashes.back()) {
          ++verify_failures;
        }
      }
      ++rewound_ticks;
      return;
    }
    if (keysactive[key]) {
      return;  // nothing older to go back to; don't record while held
    }
    ring.push_back(gold::Snapshot::capture(&included, ring.empty() ? nullptr : &ring.back()));
    ++captured;
    unshared_bytes += ring.back().unshared_size();
    if (verify) {
      hashes.push_back(total_hash());
    }
    if ((long)ring.size() <= capacity) {
      return;
    }

    ring.pop_front();
    if (verify) {
      hashes.pop_front();
    }
  }
};

Rewind& rewind_state() {
  static Rewind rw;
  return rw;
}

}  // namespace

void rewind_level_start() { rewind_state().clear(); }

void rewind_tick() {
  if (Rewind& rw = rewind_state(); rw.capacity > 0 && !playDemo && !isNetworkGame) {
    rw.tick();
  }
}

SDL_Scancode rewind_key() { return rewind_state().key; }

bool rewind_verify_failed() { return rewind_state().verify_failures > 0; }

}  // namespace tyrx::check
