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
// The hooks the C code calls (see ../hooks.h), the per-tick state hash log and
// dump, and exits.  Configured by environment variables:
//
//   TYRX_HASHLOG=FILE       write per-tick state hashes to FILE
//   TYRX_DUMP_TICK=N        dump the full hashed state at tick N ...
//   TYRX_DUMP=FILE          ... to FILE (default: tyrx-dump-N.txt)
//   TYRX_EXIT_AFTER_DEMOS=N exit when demo playback starts for the N+1th time
//   TYRX_EXIT_AFTER_TICKS=N exit at the top of level tick N
//
// Exits that Tyrian X initiates return status 3 if a C++ port mismatched
// (TYRX_DIFF), and 4 if the hashes may be incomplete because some pointers
// could not be described.
//------------------------------------------------------------------------------

#include "runtime.hpp"

#include "c_api.hpp"
#include "c_state.hpp"
#include "diff.hpp"
#include "input_driver.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace tyrx {

namespace {

[[noreturn]] void exit_run() {
  std::fflush(nullptr);
  int status = EXIT_SUCCESS;
  if (diff_mismatches()) {
    status = 3;
  }
  else if (unresolved_pointers()) {
    status = 4;
  }
  std::exit(status);
}

struct Runtime {
  std::FILE *hashlog = nullptr;
  long dump_tick = -1;
  std::string dump_path;
  long exit_after_demos = -1;
  long exit_after_ticks = -1;

  std::uint64_t tick = 0;
  unsigned level_count = 0;
  long demo_count = 0;

  Runtime() {
    if (char const *path = env("TYRX_HASHLOG")) {
      hashlog = std::fopen(path, "w");
      if (!hashlog) {
        std::fprintf(stderr, "tyrx: cannot open hash log '%s'\n", path);
      }
    }
    dump_tick = env_long("TYRX_DUMP_TICK", -1);
    if (char const *path = env("TYRX_DUMP")) {
      dump_path = path;
    }
    else if (dump_tick >= 0) {
      dump_path = "tyrx-dump-" + std::to_string(dump_tick) + ".txt";
    }
    exit_after_demos = env_long("TYRX_EXIT_AFTER_DEMOS", -1);
    exit_after_ticks = env_long("TYRX_EXIT_AFTER_TICKS", -1);

    if (hashlog) {
      std::fprintf(hashlog, "# tyrx hashlog v1\nblocks");
      for (StateBlock const *b : hashed_blocks()) {
        std::fprintf(hashlog, " %.*s", (int)b->name.size(), b->name.data());
      }
      std::fputc('\n', hashlog);
    }
  }

  ~Runtime() {
    if (hashlog) {
      std::fclose(hashlog);
    }
    if (std::uint64_t const n = unresolved_pointers()) {
      std::fprintf(stderr,
                   "tyrx: %" PRIu64 " pointers in hashed state did not point into a registered block\n", n);
    }
  }

  void level_start() {
    ++level_count;
    if (playDemo && exit_after_demos >= 0 && ++demo_count > exit_after_demos) {
      exit_run();
    }
    if (hashlog) {
      std::fprintf(hashlog, "L %u %" PRIu64 " episode=%u level=%.10s demo=%d\n", level_count, tick,
                   (unsigned)episodeNum, levelName, (int)playDemo);
    }
  }

  void level_tick() {
    if (exit_after_ticks >= 0 && (long)tick >= exit_after_ticks) {
      exit_run();
    }
    input_driver_level_tick();

    if (hashlog) {
      Hasher total;
      std::fprintf(hashlog, "T %" PRIu64, tick);
      std::string line;
      char buf[24];
      for (std::uint64_t const h : block_hashes()) {
        total.u64(h);
        std::snprintf(buf, sizeof buf, " %016" PRIx64, h);
        line += buf;
      }
      std::fprintf(hashlog, " %016" PRIx64 "%s\n", total.value(), line.c_str());
    }
    if ((long)tick == dump_tick) {
      if (std::FILE *out = std::fopen(dump_path.c_str(), "w")) {
        dump_state(out);
        std::fclose(out);
      }
    }
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

std::vector<std::uint64_t> block_hashes() {
  std::vector<std::uint64_t> hashes;
  for (StateBlock const *b : hashed_blocks()) {
    Hasher h;
    b->hash(h, b->addr);
    hashes.push_back(h.value());
  }
  return hashes;
}

namespace {

// Set during static initialization, before main() runs.
[[maybe_unused]] bool const hooks_installed = [] {
  game_hooks.level_initialized = [] { runtime().level_start(); };
  game_hooks.level_tick = [] { runtime().level_tick(); };
  return true;
}();

}  // namespace

}  // namespace tyrx
