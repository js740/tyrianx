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

#include "roundtrip.hpp"

#include "../cxx_tick.hpp"
#include "../gold/api.hpp"
#include "../gold/state.hpp"
#include "../runtime.hpp"
#include "input_driver.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>

namespace tyrx::check {

namespace {

struct RoundTrip {
  long length = 0;  // ticks per round trip; 0 if disabled
  long period = 0;  // ticks between round trip starts

  bool           active = false;     // a snapshot has been taken ...
  bool           replaying = false;  // ... and restored
  uint64_t       start_tick = 0;
  gold::Snapshot snapshot;
  // Block hashes at the top of each tick of the first pass, and at its end.
  std::vector<std::vector<uint64_t>> recorded;

  uint64_t completed = 0;
  uint64_t cancelled = 0;  // by a level change
  uint64_t replayed_ticks = 0;
  uint64_t mismatched_ticks = 0;

  RoundTrip() {
    if (char const *v = env("TYRX_ROUNDTRIP")) {
      char *end = nullptr;
      length = std::strtol(v, &end, 10);
      period = *end == ',' ? std::strtol(end + 1, nullptr, 10) : 10 * length;
      if (length <= 0 || period < length) {
        std::fprintf(stderr, "tyrx: bad TYRX_ROUNDTRIP '%s'\n", v);
        length = 0;
      }
    }
  }

  ~RoundTrip() {
    if (length > 0) {
      std::fprintf(stderr,
                   "tyrx roundtrip: %" PRIu64 " round trips of %ld ticks, %" PRIu64
                   " ticks replayed, %" PRIu64 " mismatched, %" PRIu64 " cancelled by level changes\n",
                   completed, length, replayed_ticks, mismatched_ticks, cancelled);
    }
  }

  // Round trips snapshot the game state and the input state.
  static bool included(gold::StateClass c) { return gold::is_hashed(c) || c == gold::StateClass::input; }

  // Checks the replay's hashes at `tick` against the first pass's.
  void check(uint64_t tick, std::vector<uint64_t> const& hashes) {
    std::vector<uint64_t> const& expected = recorded[tick - start_tick];
    if (hashes == expected || ++mismatched_ticks > 10) {
      return;
    }
    std::fprintf(stderr, "tyrx roundtrip: tick %" PRIu64 " differs after restoring tick %" PRIu64 ":", tick,
                 start_tick);
    auto const blocks = gold::hashed_blocks();
    for (size_t i = 0; i < hashes.size(); ++i) {
      if (hashes[i] != expected[i]) {
        std::fprintf(stderr, " %.*s", (int)blocks[i]->name.size(), blocks[i]->name.data());
      }
    }
    std::fputc('\n', stderr);
  }
};

RoundTrip& round_trip() {
  static RoundTrip rt;
  return rt;
}

}  // namespace

bool round_trip_enabled() { return round_trip().length > 0; }

void round_trip_level_start() {
  if (RoundTrip& rt = round_trip(); rt.active) {
    rt.active = rt.replaying = false;
    ++rt.cancelled;
  }
}

uint64_t round_trip_begin_tick(uint64_t tick) {
  RoundTrip& rt = round_trip();
  if (rt.length == 0) {
    return tick;
  }
  if (rt.active && tick == rt.start_tick + rt.length) {
    if (!rt.replaying) {
      // End of the first pass: record the state it ended in, then go back and
      // replay it.
      rt.recorded.push_back(gold::block_hashes());
      rt.snapshot.restore();
      input_driver_snapshot_restored();
      cxx_tick_c_state_restored();
      rt.replaying = true;
      return rt.start_tick;
    }
    // End of the replay: it must have ended in the same state.
    rt.check(tick, gold::block_hashes());
    rt.active = rt.replaying = false;
    ++rt.completed;
  }
  if (!rt.active && !playDemo && tick % rt.period == 0) {
    // Demos are skipped: their input comes from a file whose read position
    // is not part of the snapshot.
    rt.snapshot = gold::Snapshot::capture(&RoundTrip::included);
    rt.recorded.clear();
    rt.start_tick = tick;
    rt.active = true;
  }
  return tick;
}

bool round_trip_active() { return round_trip().active; }

bool round_trip_end_tick(uint64_t tick, std::vector<uint64_t> const& hashes) {
  RoundTrip& rt = round_trip();
  if (!rt.active) {
    return false;
  }
  if (!rt.replaying) {
    rt.recorded.push_back(hashes);
    return false;
  }
  ++rt.replayed_ticks;
  rt.check(tick, hashes);
  return true;
}

bool round_trip_failed() {
  RoundTrip const& rt = round_trip();
  return rt.length > 0 && (rt.mismatched_ticks > 0 || rt.completed == 0);
}

}  // namespace tyrx::check
