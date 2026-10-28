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

#include "hashlog.hpp"

#include "../gold/api.hpp"
#include "../gold/state.hpp"
#include "../runtime.hpp"

#include <cinttypes>
#include <cstdio>
#include <string>

namespace tyrx::check {

namespace {

struct HashLog {
  std::FILE *out = nullptr;
  long dump_tick = -1;
  std::string dump_path;

  HashLog() {
    if (char const *path = env("TYRX_HASHLOG")) {
      out = std::fopen(path, "w");
      if (!out) {
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

    if (out) {
      std::fprintf(out, "# tyrx hashlog v1\nblocks");
      for (gold::StateBlock const *b : gold::hashed_blocks()) {
        std::fprintf(out, " %.*s", (int)b->name.size(), b->name.data());
      }
      std::fputc('\n', out);
    }
  }

  ~HashLog() {
    if (out) {
      std::fclose(out);
    }
  }
};

HashLog& hashlog() {
  static HashLog log;
  return log;
}

}  // namespace

bool hashlog_enabled() { return hashlog().out != nullptr; }

void hashlog_level_start(unsigned level, uint64_t tick) {
  if (std::FILE *out = hashlog().out) {
    std::fprintf(out, "L %u %" PRIu64 " episode=%u level=%.10s demo=%d\n", level, tick, (unsigned)episodeNum,
                 levelName, (int)playDemo);
  }
}

void hashlog_tick(uint64_t tick, std::vector<uint64_t> const& hashes) {
  HashLog& log = hashlog();
  if (log.out) {
    gold::Hasher total;
    std::fprintf(log.out, "T %" PRIu64, tick);
    std::string line;
    char buf[24];
    for (uint64_t const h : hashes) {
      total.u64(h);
      std::snprintf(buf, sizeof buf, " %016" PRIx64, h);
      line += buf;
    }
    std::fprintf(log.out, " %016" PRIx64 "%s\n", total.value(), line.c_str());
  }
  if ((long)tick == log.dump_tick) {
    if (std::FILE *out = std::fopen(log.dump_path.c_str(), "w")) {
      gold::dump_state(out);
      std::fclose(out);
    }
  }
}

}  // namespace tyrx::check
