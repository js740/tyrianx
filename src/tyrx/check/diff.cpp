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

#include "diff.hpp"

#include "../gold/api.hpp"
#include "../sim/sim.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <sstream>

namespace tyrx::check {

namespace {

struct SliceStats {
  std::uint64_t calls = 0;
  std::uint64_t mismatches = 0;
};

struct DiffState {
  bool enabled = [] {
    char const *v = std::getenv("TYRX_DIFF");
    return v && *v && *v != '0';
  }();
  std::map<std::string, SliceStats, std::less<>> slices;
  std::uint64_t mismatches = 0;

  ~DiffState() {
    if (!enabled) {
      return;
    }
    for (auto const& [name, s] : slices) {
      std::fprintf(stderr, "tyrx diff: %-20s %10llu calls, %llu mismatches\n", name.c_str(),
                   (unsigned long long)s.calls, (unsigned long long)s.mismatches);
    }
  }
};

DiffState& diff_state() {
  static DiffState s;
  return s;
}

constexpr std::uint64_t kMaxReported = 10;

}  // namespace

bool diff_enabled() { return diff_state().enabled; }

std::uint64_t diff_mismatches() { return diff_state().mismatches; }

namespace detail {

std::string dump_to_string(void (*dump)(gold::Dumper&, void const *), void const *value) {
  char *buf = nullptr;
  std::size_t size = 0;
  std::FILE *f = open_memstream(&buf, &size);
  {
    gold::Dumper d(f);
    d.push("");
    dump(d, value);
    d.pop();
  }
  std::fclose(f);
  std::string s(buf, size);
  std::free(buf);
  return s;
}

void diff_record(std::string_view slice, bool match, std::string const& c_dump, std::string const& cxx_dump) {
  DiffState& ds = diff_state();
  auto it = ds.slices.find(slice);
  if (it == ds.slices.end()) {
    it = ds.slices.emplace(std::string(slice), SliceStats{}).first;
  }
  ++it->second.calls;
  if (match) {
    return;
  }
  ++it->second.mismatches;
  if (++ds.mismatches > kMaxReported) {
    return;
  }

  std::fprintf(stderr, "tyrx diff: MISMATCH in %.*s (call %llu):\n", (int)slice.size(), slice.data(),
               (unsigned long long)it->second.calls);
  std::istringstream c(c_dump), x(cxx_dump);
  std::string lc, lx;
  int shown = 0;
  while (std::getline(c, lc) && std::getline(x, lx) && shown < 20) {
    if (lc != lx) {
      std::fprintf(stderr, "  C:   %s\n  C++: %s\n", lc.c_str(), lx.c_str());
      ++shown;
    }
  }
}

}  // namespace detail

namespace {

struct ExplosionsState {
  Explosion explosions[MAX_EXPLOSIONS];
};

struct ExplosionsLargeState {
  Explosion explosions[MAX_EXPLOSIONS];
  rep_explosion_type rep_explosions[MAX_REPEATING_EXPLOSIONS];
};

struct SuperpixelsState {
  superpixel_type superpixels[MAX_SUPERPIXELS];
  unsigned int last_superpixel;
  MtState mt;
};

}  // namespace

}  // namespace tyrx::check

//------------------------------------------------------------------------------
// Wrapped C functions
//------------------------------------------------------------------------------

extern "C" void __real_JE_setupExplosion(JE_integer x, JE_integer y, JE_integer deltaY, JE_integer type,
                                         bool fixedPosition, bool followPlayer);

extern "C" void __wrap_JE_setupExplosion(JE_integer x, JE_integer y, JE_integer deltaY, JE_integer type,
                                         bool fixedPosition, bool followPlayer) {
  if (!tyrx::check::diff_enabled()) {
    return __real_JE_setupExplosion(x, y, deltaY, type, fixedPosition, followPlayer);
  }

  tyrx::check::ExplosionsState cxx;
  std::ranges::copy(explosions, cxx.explosions);
  tyrx::sim::setup_explosion(cxx.explosions, x, y, deltaY, type, fixedPosition, followPlayer);

  __real_JE_setupExplosion(x, y, deltaY, type, fixedPosition, followPlayer);

  tyrx::check::ExplosionsState c;
  std::ranges::copy(explosions, c.explosions);
  tyrx::check::diff_check("JE_setupExplosion", c, cxx);
}

extern "C" void __real_JE_setupExplosionLarge(JE_boolean enemyGround, JE_byte exploNum, JE_integer x,
                                              JE_integer y);

extern "C" void __wrap_JE_setupExplosionLarge(JE_boolean enemyGround, JE_byte exploNum, JE_integer x,
                                              JE_integer y) {
  if (!tyrx::check::diff_enabled()) {
    return __real_JE_setupExplosionLarge(enemyGround, exploNum, x, y);
  }

  tyrx::sim::Explosions e{};
  std::ranges::copy(explosions, e.slots.begin());
  std::ranges::copy(rep_explosions, e.repeating.begin());
  tyrx::sim::setup_explosion_large(e, enemyGround, exploNum, x, y);

  __real_JE_setupExplosionLarge(enemyGround, exploNum, x, y);

  tyrx::check::ExplosionsLargeState c, cxx;
  std::ranges::copy(explosions, c.explosions);
  std::ranges::copy(rep_explosions, c.rep_explosions);
  std::ranges::copy(e.slots, cxx.explosions);
  std::ranges::copy(e.repeating, cxx.rep_explosions);
  tyrx::check::diff_check("JE_setupExplosionLarge", c, cxx);
}

extern "C" void __real_JE_doSP(JE_word x, JE_word y, JE_word num, JE_byte explowidth, JE_byte color);

extern "C" void __wrap_JE_doSP(JE_word x, JE_word y, JE_word num, JE_byte explowidth, JE_byte color) {
  if (!tyrx::check::diff_enabled()) {
    return __real_JE_doSP(x, y, num, explowidth, color);
  }

  tyrx::sim::Superpixels sp;
  std::ranges::copy(superpixels, sp.slots.begin());
  sp.last = last_superpixel;
  MtState mt = mt_global;
  tyrx::sim::do_sp(sp, mt, x, y, num, explowidth, color);

  __real_JE_doSP(x, y, num, explowidth, color);

  tyrx::check::SuperpixelsState c, cxx;
  std::ranges::copy(superpixels, c.superpixels);
  c.last_superpixel = last_superpixel;
  c.mt = mt_global;
  std::ranges::copy(sp.slots, cxx.superpixels);
  cxx.last_superpixel = sp.last;
  cxx.mt = mt;
  tyrx::check::diff_check("JE_doSP", c, cxx);
}
