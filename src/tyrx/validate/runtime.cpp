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
  std::exit(status);
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

  void level_start() override {
    ++level_count_;
    round_trip_level_start();
    if (playDemo && exit_after_demos_ >= 0 && ++demo_count_ > exit_after_demos_) {
      exit_run();
    }
    hashlog_level_start(level_count_, tick_);
  }

  void level_tick() override {
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
    draw::drawlist_level_ended();
  }

  uint64_t tick() const { return tick_; }

 private:
  long exit_after_demos_ = -1;
  long exit_after_ticks_ = -1;

  uint64_t tick_ = 0;
  unsigned level_count_ = 0;
  long     demo_count_ = 0;
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

}  // namespace tyrx::validate
