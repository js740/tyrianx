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
// The C++ structs that stand for C structs in sim::State (Event for
// JE_EventRecType and so on), member by member, and copying between the
// State's members and the C globals that hold them.  Each mirror lists each
// member of the C++ struct with the member of the C struct that holds it, so
// that the C++ struct can have its own names, types and order; a check at
// compile time makes sure that the list has every member of both.

#pragma once

#include "../sim/state.hpp"
#include "state.hpp"

#include <stddef.h>
#include <type_traits>

namespace tyrx::gold {

// Mirror<T>::C is the C struct that the C++ struct T stands for, and
// Mirror<T>::members(cxx, c, f) calls f(cxx member, c member) for each pair.
template <typename T>
struct Mirror;

template <>
struct Mirror<sim::Event> {
  using C = JE_EventRecType;
  static constexpr void members(auto& cxx, auto& c, auto f) {
    f(cxx.time, c.eventtime);
    f(cxx.type, c.eventtype);
    f(cxx.dat, c.eventdat);
    f(cxx.dat2, c.eventdat2);
    f(cxx.dat3, c.eventdat3);
    f(cxx.dat4, c.eventdat4);
    f(cxx.dat5, c.eventdat5);
    f(cxx.dat6, c.eventdat6);
  }
};

template <>
struct Mirror<sim::Explosion> {
  using C = ::Explosion;
  static constexpr void members(auto& cxx, auto& c, auto f) {
    f(cxx.x, c.x);
    f(cxx.y, c.y);
    f(cxx.delta_y, c.deltaY);
    f(cxx.sprite, c.sprite);
    f(cxx.ttl, c.ttl);
    f(cxx.fixed_position, c.fixedPosition);
    f(cxx.follow_player, c.followPlayer);
  }
};

template <>
struct Mirror<sim::RepeatingExplosion> {
  using C = rep_explosion_type;
  static constexpr void members(auto& cxx, auto& c, auto f) {
    f(cxx.x, c.x);
    f(cxx.y, c.y);
    f(cxx.ttl, c.ttl);
    f(cxx.delay, c.delay);
    f(cxx.big, c.big);
  }
};

template <>
struct Mirror<sim::Superpixel> {
  using C = superpixel_type;
  static constexpr void members(auto& cxx, auto& c, auto f) {
    f(cxx.x, c.x);
    f(cxx.y, c.y);
    f(cxx.delta_x, c.delta_x);
    f(cxx.delta_y, c.delta_y);
    f(cxx.color, c.color);
    f(cxx.z, c.z);
  }
};

template <>
struct Mirror<sim::Star> {
  using C = StarfieldStar;
  static constexpr void members(auto& cxx, auto& c, auto f) {
    f(cxx.position, c.position);
    f(cxx.speed, c.speed);
    f(cxx.color, c.color);
  }
};

template <>
struct Mirror<sim::BossBar> {
  using C = boss_bar_t;
  static constexpr void members(auto& cxx, auto& c, auto f) {
    f(cxx.link_num, c.link_num);
    f(cxx.armor, c.armor);
    f(cxx.color, c.color);
  }
};

template <>
struct Mirror<sim::EnemyShot> {
  using C = EnemyShotType;
  static constexpr void members(auto& cxx, auto& c, auto f) {
    f(cxx.x, c.sx);
    f(cxx.y, c.sy);
    f(cxx.x_speed, c.sxm);
    f(cxx.y_speed, c.sym);
    f(cxx.x_accel, c.sxc);
    f(cxx.y_accel, c.syc);
    f(cxx.x_track, c.tx);
    f(cxx.y_track, c.ty);
    f(cxx.damage, c.sdmg);
    f(cxx.duration, c.duration);
    f(cxx.sprite, c.sgr);
    f(cxx.frame, c.animate);
    f(cxx.frames, c.animax);
    f(cxx.c_fill, c.fill);
  }
};

template <>
struct Mirror<sim::PlayerShot> {
  using C = PlayerShotDataType;
  static constexpr void members(auto& cxx, auto& c, auto f) {
    f(cxx.x, c.shotX);
    f(cxx.y, c.shotY);
    f(cxx.x_speed, c.shotXM);
    f(cxx.y_speed, c.shotYM);
    f(cxx.x_accel, c.shotXC);
    f(cxx.y_accel, c.shotYC);
    f(cxx.circling, c.shotComplicated);
    f(cxx.x_circle, c.shotDevX);
    f(cxx.x_circle_dir, c.shotDirX);
    f(cxx.x_circle_size, c.shotCirSizeX);
    f(cxx.y_circle, c.shotDevY);
    f(cxx.y_circle_dir, c.shotDirY);
    f(cxx.y_circle_size, c.shotCirSizeY);
    f(cxx.trail, c.shotTrail);
    f(cxx.sprite, c.shotGr);
    f(cxx.frame, c.shotAni);
    f(cxx.frames, c.shotAniMax);
    f(cxx.damage, c.shotDmg);
    f(cxx.blast_filter, c.shotBlastFilter);
    f(cxx.chain_reaction, c.chainReaction);
    f(cxx.player_num, c.playerNumber);
    f(cxx.aim_at_enemy, c.aimAtEnemy);
    f(cxx.aim_delay, c.aimDelay);
    f(cxx.aim_delay_max, c.aimDelayMax);
  }
};

template <typename T>
concept Mirrored = requires { typename Mirror<T>::C; };

namespace detail {

// Whether a mirror lists each member of both structs exactly once.
template <typename T>
consteval bool mirror_complete() {
  using C = typename Mirror<T>::C;
  T      cxx{};
  C      c{};
  void  *cxx_members[64] = {}, *c_members[64] = {};
  size_t n = 0;
  bool   distinct = true;
  Mirror<T>::members(cxx, c, [&](auto& a, auto& b) {
    for (size_t i = 0; i < n; ++i) {
      distinct = distinct && cxx_members[i] != &a && c_members[i] != &b;
    }
    cxx_members[n] = &a;
    c_members[n] = &b;
    ++n;
  });
  return distinct && n == data_members<T>().size() && n == data_members<C>().size();
}

}  // namespace detail

static_assert(detail::mirror_complete<sim::Event>());
static_assert(detail::mirror_complete<sim::Explosion>());
static_assert(detail::mirror_complete<sim::RepeatingExplosion>());
static_assert(detail::mirror_complete<sim::Superpixel>());
static_assert(detail::mirror_complete<sim::Star>());
static_assert(detail::mirror_complete<sim::BossBar>());
static_assert(detail::mirror_complete<sim::EnemyShot>());
static_assert(detail::mirror_complete<sim::PlayerShot>());

// Copies between a State's member and the C global that holds it, which must
// have the same type, be arrays of the same size whose elements copy so, or
// be a C++ struct and the C struct it mirrors.  (Declared first, as each
// copies its parts with the others.)
template <typename T>
  requires(!std::is_array_v<T> && !Mirrored<T>)
void copy(T& to, T const& from);
template <Mirrored T>
void copy(T& to, typename Mirror<T>::C const& from);
template <Mirrored T>
void copy(typename Mirror<T>::C& to, T const& from);
template <typename T, typename U, size_t N>
void copy(T (&to)[N], U const (&from)[N]);

template <typename T>
  requires(!std::is_array_v<T> && !Mirrored<T>)
void copy(T& to, T const& from) {
  to = from;
}

template <Mirrored T>
void copy(T& to, typename Mirror<T>::C const& from) {
  Mirror<T>::members(to, from, [](auto& a, auto const& b) { copy(a, b); });
}

template <Mirrored T>
void copy(typename Mirror<T>::C& to, T const& from) {
  Mirror<T>::members(from, to, [](auto const& a, auto& b) { copy(b, a); });
}

template <typename T, typename U, size_t N>
void copy(T (&to)[N], U const (&from)[N]) {
  for (size_t i = 0; i < N; ++i) {
    copy(to[i], from[i]);
  }
}

}  // namespace tyrx::gold
