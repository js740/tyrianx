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
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <set>
#include <span>
#include <vector>

namespace tyrx::gold {

namespace {

// Each member of each of State's subsystems, and the C global that holds its
// state in the C game.
template <typename State, typename F>
void for_each_binding(State& s, F f) {
  f(s.random.mt, ::mt_global);
  f(s.explosions.slots, ::explosions);
  f(s.explosions.repeating, ::rep_explosions);
  f(s.explosions.follow_x, ::explosionFollowAmountX);
  f(s.explosions.follow_y, ::explosionFollowAmountY);
  f(s.explosions.move, ::explodeMove);
  f(s.superpixels.slots, ::superpixels);
  f(s.superpixels.last, ::last_superpixel);
  f(s.starfield.stars, ::starfield_stars);
  f(s.starfield.speed, ::starfield_speed);
  f(s.map.back_pos, ::backPos);
  f(s.map.back_pos2, ::backPos2);
  f(s.map.back_pos3, ::backPos3);
  f(s.map.back_move, ::backMove);
  f(s.map.back_move2, ::backMove2);
  f(s.map.back_move3, ::backMove3);
  f(s.map.map_x, ::mapX);
  f(s.map.map_y, ::mapY);
  f(s.map.map_x2, ::mapX2);
  f(s.map.map_x3, ::mapX3);
  f(s.map.map_y2, ::mapY2);
  f(s.map.map_y3, ::mapY3);
  f(s.map.map_y_pos, ::mapYPos);
  f(s.map.map_y2_pos, ::mapY2Pos);
  f(s.map.map_y3_pos, ::mapY3Pos);
  f(s.map.map_x_pos, ::mapXPos);
  f(s.map.old_map_x_ofs, ::oldMapXOfs);
  f(s.map.map_x_ofs, ::mapXOfs);
  f(s.map.map_x2_ofs, ::mapX2Ofs);
  f(s.map.map_x2_pos, ::mapX2Pos);
  f(s.map.map_x3_pos, ::mapX3Pos);
  f(s.map.old_map_x3_ofs, ::oldMapX3Ofs);
  f(s.map.map_x3_ofs, ::mapX3Ofs);
  f(s.map.temp_map_x_ofs, ::tempMapXOfs);
  f(s.map.map_x_bp_pos, ::mapXbpPos);
  f(s.map.map_x2_bp_pos, ::mapX2bpPos);
  f(s.map.map_x3_bp_pos, ::mapX3bpPos);
  f(s.map.map1_y_delay, ::map1YDelay);
  f(s.map.map1_y_delay_max, ::map1YDelayMax);
  f(s.map.map2_y_delay, ::map2YDelay);
  f(s.map.map2_y_delay_max, ::map2YDelayMax);
  f(s.map.temp_back_move, ::tempBackMove);
  f(s.map.bk_wrap1, ::BKwrap1);
  f(s.map.bk_wrap2, ::BKwrap2);
  f(s.map.bk_wrap3, ::BKwrap3);
  f(s.map.bk_wrap1_to, ::BKwrap1to);
  f(s.map.bk_wrap2_to, ::BKwrap2to);
  f(s.map.bk_wrap3_to, ::BKwrap3to);
  f(s.map.stop_backgrounds, ::stopBackgrounds);
  f(s.map.stop_background_num, ::stopBackgroundNum);
  f(s.map.background2, ::background2);
  f(s.map.background2_not_transparent, ::background2notTransparent);
  f(s.map.background3x1, ::background3x1);
  f(s.map.background3x1b, ::background3x1b);
  f(s.map.background2_over, ::background2over);
  f(s.map.background3_over, ::background3over);
  f(s.map.sky_enemy_over_all, ::skyEnemyOverAll);
  f(s.map.top_enemy_over, ::topEnemyOver);
  f(s.level.events, ::eventRec);
  f(s.level.max_event, ::maxEvent);
  f(s.level.event_loc, ::eventLoc);
  f(s.level.cur_loc, ::curLoc);
  f(s.level.return_loc, ::returnLoc);
  f(s.level.return_active, ::returnActive);
  f(s.level.force_events, ::forceEvents);
  f(s.level.jump_section, ::jumpSection);
  f(s.level.enemies_active, ::enemiesActive);
  f(s.level.enemy_types, ::levelEnemy);
  f(s.level.enemy_types_max, ::levelEnemyMax);
  f(s.level.enemy_frequency, ::levelEnemyFrequency);
  f(s.level.enemy_continual_damage, ::enemyContinualDamage);
  f(s.level.super_enemy_254_jump, ::superEnemy254Jump);
  f(s.level.random_explosions, ::randomExplosions);
  f(s.level.small_enemy_adjust, ::smallEnemyAdjust);
  f(s.level.galaga_shot_freq, ::galagaShotFreq);
  f(s.level.galaga_life, ::galagaLife);
  f(s.level.timer, ::levelTimer);
  f(s.level.timer_countdown, ::levelTimerCountdown);
  f(s.level.timer_jump_to, ::levelTimerJumpTo);
  f(s.level.map_origin, ::mapOrigin);
  f(s.level.map_pnum, ::mapPNum);
  f(s.level.map_planet, ::mapPlanet);
  f(s.level.map_section, ::mapSection);
  f(s.level.new_pl, ::newPL);
  f(s.ending.end_level, ::endLevel);
  f(s.ending.really_end_level, ::reallyEndLevel);
  f(s.ending.wait_to_end_level, ::waitToEndLevel);
  f(s.ending.player_end_level, ::playerEndLevel);
  f(s.ending.ready_to_end_level, ::readyToEndLevel);
  f(s.ending.countdown, ::levelEnd);
  f(s.ending.fx_wait, ::levelEndFxWait);
  f(s.ending.warp, ::levelEndWarp);
  f(s.ending.first_game_over, ::firstGameOver);
  f(s.ending.all_players_gone, ::allPlayersGone);
  f(s.ending.enemy_still_exploding, ::enemyStillExploding);
  f(s.ending.stopped_demo, ::stoppedDemo);
  f(s.ending.bonus_level, ::bonusLevel);
  f(s.ending.bonus_level_current, ::bonusLevelCurrent);
  f(s.ending.normal_bonus_level_current, ::normalBonusLevelCurrent);
  f(s.ending.main_level, ::mainLevel);
  f(s.ending.next_level, ::nextLevel);
  f(s.players.slots, ::player);
  f(s.players.shot_repeat, ::shotRepeat);
  f(s.players.shot_multi_pos, ::shotMultiPos);
  f(s.players.power, ::power);
  f(s.players.last_power, ::lastPower);
  f(s.players.power_add, ::powerAdd);
  f(s.players.shield_wait, ::shieldWait);
  f(s.players.shield_cost, ::shieldT);
  f(s.players.charge_wait, ::chargeWait);
  f(s.players.charge_level, ::chargeLevel);
  f(s.players.charge_max, ::chargeMax);
  f(s.players.charge_gr, ::chargeGr);
  f(s.players.charge_gr_wait, ::chargeGrWait);
  f(s.players.damage_rate, ::damageRate);
  f(s.players.armor_ship_delay, ::armorShipDelay);
  f(s.players.two_player_linked, ::twoPlayerLinked);
  f(s.players.link_gun_direction, ::linkGunDirec);
  f(s.players.option_satellite_rotate, ::optionSatelliteRotate);
  f(s.players.option_attachment_move, ::optionAttachmentMove);
  f(s.players.option_attachment_linked, ::optionAttachmentLinked);
  f(s.players.option_attachment_return, ::optionAttachmentReturn);
  f(s.players.ship_gr, ::shipGr);
  f(s.players.ship_gr2, ::shipGr2);
  f(s.players.ship_gr_ptr, ::shipGrPtr);
  f(s.players.ship_gr2_ptr, ::shipGr2ptr);
  f(s.specials.wait, ::specialWait);
  f(s.specials.next_wait, ::nextSpecialWait);
  f(s.specials.sf_current_code, ::SFCurrentCode);
  f(s.specials.sf_executed, ::SFExecuted);
  f(s.specials.spray, ::spraySpecial);
  f(s.specials.weapon_wpn, ::specialWeaponWpn);
  f(s.specials.weapon_filter, ::specialWeaponFilter);
  f(s.specials.weapon_freq, ::specialWeaponFreq);
  f(s.specials.link_to_player, ::linkToPlayer);
  f(s.specials.flare_duration, ::flareDuration);
  f(s.specials.flare_start, ::flareStart);
  f(s.specials.flare_col_chg, ::flareColChg);
  f(s.specials.zinglon_duration, ::zinglonDuration);
  f(s.specials.astral_duration, ::astralDuration);
  f(s.enemies.slots, ::enemy);
  f(s.enemies.status, ::enemyAvail);
  f(s.enemies.on_screen, ::enemyOnScreen);
  f(s.enemies.total, ::totalEnemy);
  f(s.enemies.killed, ::enemyKilled);
  f(s.player_shots.slots, ::playerShotData);
  f(s.player_shots.ttl, ::shotAvail);
  f(s.enemy_shots.slots, ::enemyShot);
  f(s.enemy_shots.available, ::enemyShotAvail);
}

// Copies between a member and its global, which must have the same type, or
// be arrays of the same size whose elements copy so.
template <typename T>
void copy(T& to, T const& from) {
  to = from;
}

template <typename T, typename U, std::size_t N>
void copy(std::array<T, N>& to, U const (&from)[N]) {
  for (std::size_t i = 0; i < N; ++i) {
    copy(to[i], from[i]);
  }
}

template <typename T, typename U, std::size_t N>
void copy(T (&to)[N], std::array<U, N> const& from) {
  for (std::size_t i = 0; i < N; ++i) {
    copy(to[i], from[i]);
  }
}

// Where a member or global is, and where its counterpart is.
struct Relocation {
  std::uintptr_t from;
  std::size_t size;
  std::uintptr_t to;
};

// Points the pointers in v that point into one of the relocations' from
// objects into its to object instead.  Copied state can point into itself (a
// player's lives point into its items), and must point into the copy.
template <typename T>
void relocate_pointers(T& v, std::span<Relocation const> relocations) {
  using U = std::remove_cv_t<T>;
  if constexpr (detail::is_bytewise<std::remove_all_extents_t<U>>) {
    return;
  }
  else if constexpr (std::is_pointer_v<U>) {
    std::uintptr_t const p = reinterpret_cast<std::uintptr_t>(v);
    for (Relocation const& r : relocations) {
      if (p - r.from < r.size) {
        v = reinterpret_cast<U>(r.to + (p - r.from));
        return;
      }
    }
  }
  else if constexpr (std::is_array_v<U>) {
    for (auto& e : v) {
      relocate_pointers(e, relocations);
    }
  }
  else if constexpr (std::is_class_v<U>) {
    template for (constexpr auto m : detail::data_members<U>()) { relocate_pointers(v.[:m:], relocations); }
  }
}

// The relocations from the C globals to the State's members, or back.
std::vector<Relocation> relocations(sim::State const& state, bool to_state) {
  std::vector<Relocation> r;
  for_each_binding(state, [&](auto const& cxx, auto const& c) {
    static_assert(sizeof cxx == sizeof c);
    auto const cxx_at = reinterpret_cast<std::uintptr_t>(&cxx), c_at = reinterpret_cast<std::uintptr_t>(&c);
    r.push_back(to_state ? Relocation{c_at, sizeof c, cxx_at} : Relocation{cxx_at, sizeof cxx, c_at});
  });
  return r;
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

void read_sim_state(sim::State& state) {
  for_each_binding(state, [](auto& cxx, auto const& c) { copy(cxx, c); });
  relocate_pointers(state, relocations(state, true));
}

void write_sim_state(sim::State const& state) {
  for_each_binding(state, [](auto const& cxx, auto& c) { copy(c, cxx); });
  std::vector<Relocation> const r = relocations(state, false);
  for_each_binding(state, [&](auto const&, auto& c) { relocate_pointers(c, r); });
}

void scramble_sim_state_globals() {
  sim::State state;
  for_each_binding(state, [](auto&, auto& c) { scramble(c); });
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
