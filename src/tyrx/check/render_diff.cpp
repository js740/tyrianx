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

#include "render_diff.hpp"

#include "../gold/api.hpp"
#include "../render/render.hpp"
#include "../runtime.hpp"

#include <cstdio>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace tyrx::check {

namespace {

struct SliceStats {
  std::uint64_t calls = 0;
  std::uint64_t mismatches = 0;
  std::uint64_t pixels = 0;  // differing pixels over all mismatching calls
};

struct RenderDiffState {
  bool enabled = env("TYRX_RENDER_DIFF") != nullptr;
  std::map<std::string, SliceStats, std::less<>> slices;
  std::uint64_t mismatches = 0;

  ~RenderDiffState() {
    if (!enabled) {
      return;
    }
    for (auto const& [name, s] : slices) {
      std::fprintf(stderr, "tyrx render diff: %-20s %10llu calls, %llu mismatches (%llu pixels)\n",
                   name.c_str(), (unsigned long long)s.calls, (unsigned long long)s.mismatches,
                   (unsigned long long)s.pixels);
    }
  }
};

RenderDiffState& render_diff_state() {
  static RenderDiffState s;
  return s;
}

constexpr std::uint64_t kMaxReported = 10;

// A copy of a surface's pixels for a C++ port to draw on.
class SurfaceCopy {
 public:
  explicit SurfaceCopy(SDL_Surface const *s)
      : w_(s->w),
        h_(s->h),
        pitch_(s->pitch),
        pixels_(static_cast<Uint8 const *>(s->pixels),
                static_cast<Uint8 const *>(s->pixels) + s->pitch * s->h) {}

  render::Canvas canvas() { return {pixels_.data(), pitch_, w_, h_}; }

 private:
  int w_, h_, pitch_;
  std::vector<Uint8> pixels_;
};

// Compares what the C function drew on `c` with what the port drew on `cxx`.
void render_diff_check(std::string_view slice, SDL_Surface const *c, render::Canvas cxx) {
  RenderDiffState& rd = render_diff_state();
  auto it = rd.slices.find(slice);
  if (it == rd.slices.end()) {
    it = rd.slices.emplace(std::string(slice), SliceStats{}).first;
  }
  SliceStats& stats = it->second;
  ++stats.calls;

  std::uint64_t differing = 0;
  int first_x = 0, first_y = 0;
  for (int y = 0; y < c->h; ++y) {
    Uint8 const *cr = static_cast<Uint8 const *>(c->pixels) + y * c->pitch;
    Uint8 const *xr = cxx.pixels + y * cxx.pitch;
    for (int x = 0; x < c->w; ++x) {
      if (cr[x] != xr[x] && differing++ == 0) {
        first_x = x;
        first_y = y;
      }
    }
  }
  if (differing == 0) {
    return;
  }
  ++stats.mismatches;
  stats.pixels += differing;
  if (++rd.mismatches > kMaxReported) {
    return;
  }

  Uint8 const c_pixel = static_cast<Uint8 const *>(c->pixels)[first_y * c->pitch + first_x];
  Uint8 const cxx_pixel = cxx.pixels[first_y * cxx.pitch + first_x];
  std::fprintf(stderr,
               "tyrx render diff: MISMATCH in %.*s (call %llu): %llu pixels differ, first at (%d, %d): C %u, "
               "C++ %u\n",
               (int)slice.size(), slice.data(), (unsigned long long)stats.calls,
               (unsigned long long)differing, first_x, first_y, c_pixel, cxx_pixel);
}

}  // namespace

bool render_diff_enabled() { return render_diff_state().enabled; }

std::uint64_t render_diff_mismatches() { return render_diff_state().mismatches; }

}  // namespace tyrx::check

//------------------------------------------------------------------------------
// Wrapped C functions
//------------------------------------------------------------------------------

extern "C" void __real_draw_background_1(SDL_Surface *surface);

extern "C" void __wrap_draw_background_1(SDL_Surface *surface) {
  if (!tyrx::check::render_diff_enabled()) {
    return __real_draw_background_1(surface);
  }

  tyrx::check::SurfaceCopy cxx(surface);
  tyrx::render::draw_background_1(cxx.canvas(), mapYPos, mapXbpPos, mapXPos, backPos);

  __real_draw_background_1(surface);

  tyrx::check::render_diff_check("draw_background_1", surface, cxx.canvas());
}
