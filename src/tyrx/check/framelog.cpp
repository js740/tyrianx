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

#include "framelog.hpp"

#include "../gold/api.hpp"
#include "../gold/state.hpp"
#include "../runtime.hpp"

#include <cinttypes>
#include <cstdio>

namespace tyrx::check {

namespace {

struct FrameLog {
  std::FILE *out = nullptr;

  FrameLog() {
    if (char const *path = env("TYRX_FRAMELOG")) {
      out = std::fopen(path, "w");
      if (!out) {
        std::fprintf(stderr, "tyrx: cannot open frame log '%s'\n", path);
      }
    }
  }

  ~FrameLog() {
    if (out) {
      std::fclose(out);
    }
  }
};

FrameLog& framelog() {
  static FrameLog log;
  return log;
}

}  // namespace

void framelog_frame() {
  std::FILE *const out = framelog().out;
  if (out == nullptr) {
    return;
  }

  gold::Hasher h;
  for (int y = 0; y < VGAScreen->h; ++y) {
    h.bytes(static_cast<uint8_t const *>(VGAScreen->pixels) + y * VGAScreen->pitch, VGAScreen->w);
  }
  h.bytes(rgb_palette, sizeof rgb_palette);
  std::fprintf(out, "%" PRIu64 " %016" PRIx64 "\n", current_tick(), h.value());
}

}  // namespace tyrx::check
