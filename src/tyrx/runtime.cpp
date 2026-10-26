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
// The hooks the C code calls (see ../hooks.h), which run the checks (check/),
// and exits.  Configured by environment variables:
//
//   TYRX_EXIT_AFTER_DEMOS=N exit when demo playback starts for the N+1th time
//   TYRX_EXIT_AFTER_TICKS=N exit at the top of level tick N
//
// Exits that Tyrian X initiates return status 4 if the hashes may be
// incomplete because some pointers could not be described.
//------------------------------------------------------------------------------

#include "runtime.hpp"

#include "check/hashlog.hpp"
#include "check/input_driver.hpp"
#include "gold/api.hpp"
#include "gold/state.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace tyrx {

namespace {

[[noreturn]] void exit_run() {
  std::fflush(nullptr);
  int status = EXIT_SUCCESS;
  if (gold::unresolved_pointers()) {
    status = 4;
  }
  std::exit(status);
}

struct Runtime {
  long exit_after_demos = -1;
  long exit_after_ticks = -1;

  std::uint64_t tick = 0;
  unsigned level_count = 0;
  long demo_count = 0;

  Runtime() {
    exit_after_demos = env_long("TYRX_EXIT_AFTER_DEMOS", -1);
    exit_after_ticks = env_long("TYRX_EXIT_AFTER_TICKS", -1);
  }

  ~Runtime() {
    if (std::uint64_t const n = gold::unresolved_pointers()) {
      std::fprintf(stderr,
                   "tyrx: %" PRIu64 " pointers in hashed state did not point into a registered block\n", n);
    }
  }

  void level_start() {
    ++level_count;
    if (playDemo && exit_after_demos >= 0 && ++demo_count > exit_after_demos) {
      exit_run();
    }
    check::hashlog_level_start(level_count, tick);
  }

  void level_tick() {
    if (exit_after_ticks >= 0 && (long)tick >= exit_after_ticks) {
      exit_run();
    }
    check::input_driver_level_tick();

    std::vector<std::uint64_t> hashes;
    if (check::hashlog_enabled()) {
      hashes = gold::block_hashes();
    }
    check::hashlog_tick(tick, hashes);
    ++tick;
  }
};

Runtime& runtime() {
  static Runtime rt;
  return rt;
}

}  // namespace

char const *env(char const *name) {
  char const *v = std::getenv(name);
  return v && *v ? v : nullptr;
}

long env_long(char const *name, long fallback) {
  char const *v = env(name);
  return v ? std::strtol(v, nullptr, 10) : fallback;
}

std::uint64_t current_tick() { return runtime().tick; }

namespace {

// Set during static initialization, before main() runs.
[[maybe_unused]] bool const hooks_installed = [] {
  game_hooks.level_initialized = [] { runtime().level_start(); };
  game_hooks.level_tick = [] { runtime().level_tick(); };
  return true;
}();

}  // namespace

}  // namespace tyrx
