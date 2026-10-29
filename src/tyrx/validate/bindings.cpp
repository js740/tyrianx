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

#include "bindings.hpp"

#include "../c/mirror.hpp"
#include "../c/state.hpp"
#include "state.hpp"

#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <span>
#include <string_view>
#include <vector>

namespace tyrx::validate {

namespace {

// Whether a mirror lists each member of both structs exactly once.
template <typename T>
consteval bool mirror_complete() {
  using C = typename c::Mirror<T>::C;
  T      cxx{};
  C      c{};
  void  *cxx_members[64] = {}, *c_members[64] = {};
  size_t n = 0;
  bool   distinct = true;
  c::Mirror<T>::members(cxx, c, [&](auto& a, auto& b, auto...) {
    for (size_t i = 0; i < n; ++i) {
      distinct = distinct && cxx_members[i] != &a && c_members[i] != &b;
    }
    cxx_members[n] = &a;
    c_members[n] = &b;
    ++n;
  });
  return distinct && n == detail::data_members<T>().size() && n == detail::data_members<C>().size();
}

static_assert(mirror_complete<sim::Event>());
static_assert(mirror_complete<sim::Explosion>());
static_assert(mirror_complete<sim::RepeatingExplosion>());
static_assert(mirror_complete<sim::Superpixel>());
static_assert(mirror_complete<sim::Star>());
static_assert(mirror_complete<sim::BossBar>());
static_assert(mirror_complete<sim::EnemyShot>());
static_assert(mirror_complete<sim::PlayerShot>());
static_assert(mirror_complete<sim::Enemy>());
static_assert(mirror_complete<sim::PlayerItems::Weapon>());
static_assert(mirror_complete<sim::PlayerItems>());
static_assert(mirror_complete<sim::Player::Sidekick>());
static_assert(mirror_complete<sim::Player>());

// Where a C global that the State holds is.
struct Bound {
  uintptr_t at;
  size_t    size;
};

std::vector<Bound> bound_globals() {
  sim::State         state;
  std::vector<Bound> b;
  c::for_each_binding(
      state, [&](auto const&, auto const& c) { b.push_back({reinterpret_cast<uintptr_t>(&c), sizeof c}); });
  return b;
}

template <typename T>
void check_pointers(T const& v, std::span<Bound const> bound) {
  using U = std::remove_cv_t<T>;
  if constexpr (detail::is_bytewise<std::remove_all_extents_t<U>>) {
    return;
  }
  else if constexpr (std::is_pointer_v<U>) {
    uintptr_t const p = reinterpret_cast<uintptr_t>(v);
    for (Bound const& g : bound) {
      if (p - g.at < g.size) {
        std::fprintf(stderr, "tyrx: a pointer in the State points into a C global that it holds\n");
        std::abort();
      }
    }
  }
  else if constexpr (std::is_array_v<U>) {
    for (auto const& e : v) {
      check_pointers(e, bound);
    }
  }
  else if constexpr (std::is_class_v<U>) {
    template for (constexpr auto m : detail::data_members<U>()) { check_pointers(v.[:m:], bound); }
  }
}

// Overwrites a global with a pattern.
template <typename T>
void scramble(T& c) {
  std::memset(&c, 0xa5, sizeof c);
}

// A generator keeps its indices in range, so that drawing from it gives wrong
// numbers instead of crashing.
void scramble(MtState& mt) {
  std::memset(&mt, 0xa5, sizeof mt);
  mt.p0 %= MT_RAND_N;
  mt.p1 %= MT_RAND_N;
  mt.pm %= MT_RAND_N;
}

}  // namespace

void scramble_bound_globals() {
  sim::State state;
  c::for_each_binding(state, [](auto&, auto& c) { scramble(c); });
}

void check_bound_pointers(sim::State const& state) {
  static std::vector<Bound> const bound = bound_globals();
  check_pointers(state, bound);
}

bool bindings_complete() {
  sim::State             state;
  std::set<void const *> bound;

  bool ok = true;
  c::for_each_binding(state, [&](auto const& cxx, auto&) {
    if (!bound.insert(&cxx).second) {
      std::fprintf(stderr, "tyrx: a member of sim::State is bound twice\n");
      ok = false;
    }
  });

  size_t found = 0;
  template for (constexpr auto sub : detail::data_members<sim::State>()) {
    auto& subsystem = state.[:sub:];
    template for (constexpr auto m : detail::data_members<std::remove_cvref_t<decltype(subsystem)>>()) {
      if (bound.contains(&subsystem.[:m:])) {
        ++found;
      }
      else {
        constexpr std::string_view sub_name = std::meta::identifier_of(sub),
                                   name = std::meta::identifier_of(m);
        std::fprintf(stderr, "tyrx: sim::State member %.*s.%.*s has no C global\n", (int)sub_name.size(),
                     sub_name.data(), (int)name.size(), name.data());
        ok = false;
      }
    }
  }
  if (found != bound.size()) {
    std::fprintf(stderr, "tyrx: some C globals are bound to something other than a sim::State member\n");
    ok = false;
  }
  return ok;
}

}  // namespace tyrx::validate
