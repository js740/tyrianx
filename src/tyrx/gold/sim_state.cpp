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

#include "sim_state.hpp"

#include "state.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <set>

namespace tyrx::gold {

namespace {

// Each member of each of State's subsystems, and the C global that holds its
// state in the C game.
template <typename State, typename F>
void for_each_binding(State& s, F f) {
  f(s.explosions.slots, ::explosions);
  f(s.explosions.repeating, ::rep_explosions);
  f(s.explosions.follow_x, ::explosionFollowAmountX);
  f(s.explosions.follow_y, ::explosionFollowAmountY);
  f(s.explosions.move, ::explodeMove);
  f(s.superpixels.slots, ::superpixels);
  f(s.superpixels.last, ::last_superpixel);
}

// Copies between a member and its global, which must have the same type, or
// be arrays of the same type and size.
template <typename T>
void copy(T& to, T const& from) {
  to = from;
}

template <typename T, std::size_t N>
void copy(std::array<T, N>& to, T const (&from)[N]) {
  std::ranges::copy(from, to.begin());
}

template <typename T, std::size_t N>
void copy(T (&to)[N], std::array<T, N> const& from) {
  std::ranges::copy(from, to);
}

}  // namespace

void read_sim_state(sim::State& state) {
  for_each_binding(state, [](auto& cxx, auto const& c) { copy(cxx, c); });
}

void write_sim_state(sim::State const& state) {
  for_each_binding(state, [](auto const& cxx, auto& c) { copy(c, cxx); });
}

void scramble_sim_state_globals() {
  sim::State state;
  for_each_binding(state, [](auto&, auto& c) { std::memset(&c, 0xa5, sizeof c); });
}

bool sim_state_bound() {
  sim::State state;
  std::set<void const *> bound;
  bool ok = true;
  for_each_binding(state, [&](auto const& cxx, auto&) {
    if (!bound.insert(&cxx).second) {
      std::fprintf(stderr, "tyrx: a member of sim::State is bound twice\n");
      ok = false;
    }
  });

  std::size_t found = 0;
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

}  // namespace tyrx::gold
