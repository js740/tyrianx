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
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <type_traits>

namespace tyrx::gold {

// Mirror<T>::C is the C struct that the C++ struct T stands for, and
// Mirror<T>::members(cxx, c, f) calls f(cxx member, c member) for each pair
// that copies as it is, and f(cxx member, c member, Convert{}) for each that
// doesn't: Convert::to_cxx(cxx, c) and Convert::to_c(c, cxx) convert it,
// given the whole structs.
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

template <>
struct Mirror<sim::Enemy> {
  using C = JE_SingleEnemyType;
  static constexpr void members(auto& cxx, auto& c, auto f) {
    f(cxx.type, c.enemytype);
    f(cxx.dat, c.enemydatofs);
    f(cxx.sprite_sheet, c.sprite2s);
    f(cxx.size, c.size);
    f(cxx.ground, c.enemyground);
    f(cxx.explosion, c.explonum);
    f(cxx.value, c.evalue);
    f(cxx.score_item, c.scoreitem);
    f(cxx.dies_into, c.enemydie);
    f(cxx.x, c.ex);
    f(cxx.y, c.ey);
    f(cxx.map_offset, c.mapoffset);
    f(cxx.fixed_move_y, c.fixedmovey);
    f(cxx.x_speed, c.exc);
    f(cxx.y_speed, c.eyc);
    f(cxx.x_homing, c.xaccel);
    f(cxx.y_homing, c.yaccel);
    f(cxx.x_homing_dir, c.exca);
    f(cxx.y_homing_dir, c.eyca);
    f(cxx.x_accel, c.excc);
    f(cxx.y_accel, c.eycc);
    f(cxx.x_accel_wait, c.exccw);
    f(cxx.y_accel_wait, c.eyccw);
    f(cxx.x_accel_wait_max, c.exccwmax);
    f(cxx.y_accel_wait_max, c.eyccwmax);
    f(cxx.x_accel_step, c.exccadd);
    f(cxx.y_accel_step, c.eyccadd);
    f(cxx.x_reverse_speed, c.exrev);
    f(cxx.y_reverse_speed, c.eyrev);
    f(cxx.x_min_bounce, c.xminbounce);
    f(cxx.x_max_bounce, c.xmaxbounce);
    f(cxx.y_min_bounce, c.yminbounce);
    f(cxx.y_max_bounce, c.ymaxbounce);
    f(cxx.sprites, c.egr);
    f(cxx.frame, c.enemycycle);
    f(cxx.first_frame, c.animin);
    f(cxx.last_frame, c.ani);
    f(cxx.animation, c.aniactive);
    f(cxx.stop_frame, c.animax);
    f(cxx.animation_at_stop, c.aniwhenfire);
    f(cxx.filter, c.filter);
    f(cxx.armor, c.armorleft);
    f(cxx.link_num, c.linknum);
    f(cxx.iced, c.iced);
    f(cxx.damaged, c.edamaged);
    f(cxx.damaged_armor, c.edlevel);
    f(cxx.damaged_sprite, c.edgr);
    f(cxx.damaged_frames, c.edani);
    f(cxx.sets_flag, c.special);
    f(cxx.flag_num, c.flagnum);
    f(cxx.flag_value, c.setto);
    f(cxx.turrets, c.tur);
    f(cxx.shot_freq, c.freq);
    f(cxx.shot_wait, c.eshotwait);
    f(cxx.shot_pos, c.eshotmultipos);
    f(cxx.launch_type, c.launchtype);
    f(cxx.launch_freq, c.launchfreq);
    f(cxx.launch_wait, c.launchwait);
    f(cxx.launch_special, c.launchspecial);
    f(cxx.c_fillbyte, c.fillbyte);
    f(cxx.c_fill1, c.fill1);
    f(cxx.c_fill, c.fill);
  }
};

template <>
struct Mirror<sim::PlayerItems::Weapon> {
  using C = std::remove_extent_t<decltype(::PlayerItems::weapon)>;
  static constexpr void members(auto& cxx, auto& c, auto f) {
    f(cxx.id, c.id);
    f(cxx.power, c.power);
  }
};

template <>
struct Mirror<sim::PlayerItems> {
  using C = ::PlayerItems;
  static constexpr void members(auto& cxx, auto& c, auto f) {
    f(cxx.ship, c.ship);
    f(cxx.generator, c.generator);
    f(cxx.shield, c.shield);
    f(cxx.weapon, c.weapon);
    f(cxx.sidekick, c.sidekick);
    f(cxx.special, c.special);
    f(cxx.sidekick_series, c.sidekick_series);
    f(cxx.sidekick_level, c.sidekick_level);
    f(cxx.super_arcade_mode, c.super_arcade_mode);
  }
};

template <>
struct Mirror<sim::Player::Sidekick> {
  using C = std::remove_extent_t<decltype(::Player::sidekick)>;
  static constexpr void members(auto& cxx, auto& c, auto f) {
    f(cxx.ammo_max, c.ammo_max);
    f(cxx.ammo_refill_ticks_max, c.ammo_refill_ticks_max);
    f(cxx.style, c.style);
    f(cxx.x, c.x);
    f(cxx.y, c.y);
    f(cxx.ammo, c.ammo);
    f(cxx.ammo_refill_ticks, c.ammo_refill_ticks);
    f(cxx.animation_enabled, c.animation_enabled);
    f(cxx.animation_frame, c.animation_frame);
    f(cxx.charge, c.charge);
    f(cxx.charge_ticks, c.charge_ticks);
  }
};

// A player's lives are the power of one of its item weapons, which the C
// struct points to and the C++ struct has the index of.
struct PlayerLives {
  static void to_cxx(sim::Player& to, ::Player const& from) {
    to.lives_weapon = sim::Player::no_lives;
    for (uint8_t i = 0; i < std::size(from.items.weapon); ++i) {
      if (from.lives == &from.items.weapon[i].power) {
        to.lives_weapon = i;
      }
    }
    if (from.lives != nullptr && to.lives_weapon == sim::Player::no_lives) {
      std::fprintf(stderr, "tyrx: a player's lives point outside its items\n");
      std::abort();
    }
  }
  static void to_c(::Player& to, sim::Player const& from) {
    to.lives =
        from.lives_weapon == sim::Player::no_lives ? nullptr : &to.items.weapon[from.lives_weapon].power;
  }
};

template <>
struct Mirror<sim::Player> {
  using C = ::Player;
  static constexpr void members(auto& cxx, auto& c, auto f) {
    f(cxx.cash, c.cash);
    f(cxx.items, c.items);
    f(cxx.last_items, c.last_items);
    f(cxx.is_dragonwing, c.is_dragonwing);
    f(cxx.lives_weapon, c.lives, PlayerLives{});
    f(cxx.shield_max, c.shield_max);
    f(cxx.initial_armor, c.initial_armor);
    f(cxx.shot_hit_area_x, c.shot_hit_area_x);
    f(cxx.shot_hit_area_y, c.shot_hit_area_y);
    f(cxx.is_alive, c.is_alive);
    f(cxx.invulnerable_ticks, c.invulnerable_ticks);
    f(cxx.exploding_ticks, c.exploding_ticks);
    f(cxx.shield, c.shield);
    f(cxx.armor, c.armor);
    f(cxx.weapon_mode, c.weapon_mode);
    f(cxx.superbombs, c.superbombs);
    f(cxx.purple_balls_needed, c.purple_balls_needed);
    f(cxx.mouse_x, c.mouseX);
    f(cxx.mouse_y, c.mouseY);
    f(cxx.x, c.x);
    f(cxx.y, c.y);
    f(cxx.old_x, c.old_x);
    f(cxx.old_y, c.old_y);
    f(cxx.x_velocity, c.x_velocity);
    f(cxx.y_velocity, c.y_velocity);
    f(cxx.x_friction_ticks, c.x_friction_ticks);
    f(cxx.y_friction_ticks, c.y_friction_ticks);
    f(cxx.delta_x_shot_move, c.delta_x_shot_move);
    f(cxx.delta_y_shot_move, c.delta_y_shot_move);
    f(cxx.last_x_shot_move, c.last_x_shot_move);
    f(cxx.last_y_shot_move, c.last_y_shot_move);
    f(cxx.last_x_explosion_follow, c.last_x_explosion_follow);
    f(cxx.last_y_explosion_follow, c.last_y_explosion_follow);
    f(cxx.sidekick, c.sidekick);
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
  Mirror<T>::members(cxx, c, [&](auto& a, auto& b, auto...) {
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
static_assert(detail::mirror_complete<sim::Enemy>());
static_assert(detail::mirror_complete<sim::PlayerItems::Weapon>());
static_assert(detail::mirror_complete<sim::PlayerItems>());
static_assert(detail::mirror_complete<sim::Player::Sidekick>());
static_assert(detail::mirror_complete<sim::Player>());

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
  Mirror<T>::members(to, from, [&](auto& a, auto const& b, auto... convert) {
    if constexpr (sizeof...(convert) == 0) {
      copy(a, b);
    }
    else {
      (decltype(convert)::to_cxx(to, from), ...);
    }
  });
}

template <Mirrored T>
void copy(typename Mirror<T>::C& to, T const& from) {
  Mirror<T>::members(from, to, [&](auto const& a, auto& b, auto... convert) {
    if constexpr (sizeof...(convert) == 0) {
      copy(b, a);
    }
    else {
      (decltype(convert)::to_c(to, from), ...);
    }
  });
}

template <typename T, typename U, size_t N>
void copy(T (&to)[N], U const (&from)[N]) {
  for (size_t i = 0; i < N; ++i) {
    copy(to[i], from[i]);
  }
}

}  // namespace tyrx::gold
