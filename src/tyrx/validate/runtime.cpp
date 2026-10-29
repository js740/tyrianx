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

#include "runtime.hpp"

#include "../c/api.hpp"
#include "../env.hpp"
#include "../game.hpp"
#include "bindings.hpp"
#include "draw/record.hpp"
#include "hashlog.hpp"
#include "input_driver.hpp"
#include "roundtrip.hpp"
#include "shadow.hpp"
#include "state.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace tyrx::validate {

namespace {

[[noreturn]] void exit_run() {
  std::fflush(nullptr);
  int status = EXIT_SUCCESS;
  if (unresolved_pointers()) {
    status = 3;
  }
  else if (round_trip_failed()) {
    status = 4;
  }
  else if (draw::drawlist_check_failures()) {
    status = 5;
  }
  else if (shadow_mismatches()) {
    status = 6;
  }
  std::exit(status);
}

[[noreturn]] void fail(char const *why) {
  std::fprintf(stderr, "tyrx: %s\n", why);
  std::exit(EXIT_FAILURE);
}

class Runtime final : public Validation {
 public:
  Runtime() {
    exit_after_demos_ = env_long("TYRX_EXIT_AFTER_DEMOS", -1);
    exit_after_ticks_ = env_long("TYRX_EXIT_AFTER_TICKS", -1);
    if (!bindings_complete()) {
      std::exit(EXIT_FAILURE);
    }
  }

  ~Runtime() {
    if (uint64_t const n = unresolved_pointers()) {
      std::fprintf(stderr,
                   "tyrx: %" PRIu64 " pointers in hashed state did not point into a registered block\n", n);
    }
  }

  bool cxx_game() override {
    if (env("TYRX_CXX_TICK") == nullptr) {
      return false;
    }
    if (env("TYRX_SHADOW") != nullptr || env("TYRX_DRAWLIST_CHECK") != nullptr) {
      fail("TYRX_CXX_TICK doesn't run with TYRX_SHADOW or TYRX_DRAWLIST_CHECK");
    }
    return true;
  }

  void level_start() override {
    shadow_level_start();
    ++level_count_;
    round_trip_level_start();
    if (playDemo && exit_after_demos_ >= 0 && ++demo_count_ > exit_after_demos_) {
      exit_run();
    }
    hashlog_level_start(level_count_, tick_);
  }

  void level_tick() override {
    shadow_level_tick();
    draw::drawlist_level_tick();
    if (exit_after_ticks_ >= 0 && (long)tick_ >= exit_after_ticks_) {
      exit_run();
    }
    tick_ = round_trip_begin_tick(tick_);  // may restore a snapshot and go back
    input_driver_level_tick();

    std::vector<uint64_t> hashes;
    if (hashlog_enabled() || round_trip_active()) {
      hashes = block_hashes();
    }
    if (round_trip_end_tick(tick_, hashes)) {
      ++tick_;  // replayed ticks are not logged or dumped again
      return;
    }

    hashlog_tick(tick_, hashes);
    ++tick_;
  }

  void level_ended() override {
    input_driver_level_ended();
    shadow_level_ended();
    draw::drawlist_level_ended();
  }

  // The C++ game's pass works on the State, and the C globals it holds the
  // state of are scrambled until it is written back.
  void pass_begins(sim::State const& state) override {
    check_bound_pointers(state);
    scramble_bound_globals();
    pass_active_ = true;
  }

  void pass_ends() override { pass_active_ = false; }

  uint64_t tick() const { return tick_; }
  bool     pass_active() const { return pass_active_; }

 private:
  long exit_after_demos_ = -1;
  long exit_after_ticks_ = -1;

  uint64_t tick_ = 0;
  unsigned level_count_ = 0;
  long     demo_count_ = 0;
  bool     pass_active_ = false;
};

Runtime& runtime() {
  static Runtime rt;
  return rt;
}

// Set during static initialization, before main() runs.
[[maybe_unused]] bool const installed = [] {
  validation() = &runtime();
  return true;
}();

}  // namespace

uint64_t current_tick() { return runtime().tick(); }

bool cxx_pass_active() { return runtime().pass_active() || shadow_active(); }

}  // namespace tyrx::validate
