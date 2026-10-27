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

// The C++ game's state: the state that the C globals hold in the C game, moved
// into structures, a subsystem at a time.  The C++ code works on a State that
// the code running the tick provides (port::state()).
//
// While the C game runs, it is the reference: gold/sim_state.hpp copies the C
// globals that a State holds into it and back.

#pragma once

#include "../gold/api.hpp"

#include <array>

namespace tyrx::sim {

struct Explosions {
  std::array<Explosion, MAX_EXPLOSIONS> slots;
  std::array<rep_explosion_type, MAX_REPEATING_EXPLOSIONS> repeating;  // which set off explosions over ticks
  JE_word move;                   // how far explosions that aren't fixed move down each tick
  JE_integer follow_x, follow_y;  // how far the player moved, for explosions that follow the player
};

struct Superpixels {
  std::array<superpixel_type, MAX_SUPERPIXELS> slots;
  unsigned int last;  // the slot of the last one created
};

struct Starfield {
  std::array<StarfieldStar, MAX_STARS> stars;  // their positions are offsets into the screen
  int speed;                                   // how much faster than their own speed all stars move
};

struct Random {
  MtState mt;  // the random number generator that the game logic draws from
};

struct Players {
  std::array<Player, 2> slots;

  std::array<JE_byte, 11> shot_repeat;     // ticks until each kind of shot can fire again
  std::array<JE_byte, 11> shot_multi_pos;  // the pattern each kind of shot fires next
  uint power, last_power, power_add;       // the generator's power, as last drawn, and gained per tick
  JE_byte shield_wait, shield_cost;        // ticks until the shield recharges, and the power it takes
  JE_byte charge_wait, charge_level, charge_max, charge_gr, charge_gr_wait;  // the charge weapon
  JE_byte damage_rate;       // the most armor that a collision takes
  JE_word armor_ship_delay;  // ticks until the low-armor ship shows again

  // Two-player linked mode, and the sidekicks that attach to the ship.
  JE_boolean two_player_linked;
  JE_real link_gun_direction;
  JE_real option_satellite_rotate;
  JE_integer option_attachment_move;
  JE_boolean option_attachment_linked, option_attachment_return;

  // The ships' sprites.
  JE_word ship_gr, ship_gr2;
  Sprite2_array *ship_gr_ptr, *ship_gr2_ptr;
};

struct Specials {
  JE_byte wait, next_wait;
  std::array<std::array<JE_byte, 21>, 2> sf_current_code;  // the keys of each player's special moves so far
  std::array<JE_byte, 2> sf_executed;
  JE_boolean spray;

  // The special weapon.
  JE_word weapon_wpn;
  JE_shortint weapon_filter, weapon_freq;
  JE_boolean link_to_player;

  // The specials that last a while.
  JE_word flare_duration;
  JE_boolean flare_start;
  JE_shortint flare_col_chg;
  JE_byte zinglon_duration, astral_duration;
};

struct Enemies {
  std::array<JE_SingleEnemyType, 100> slots;
  std::array<JE_byte, 100> status;  // 1 for a free slot, 0 for an enemy, 2 for one without armor
  JE_word on_screen;                // how many were updated in the tick
  JE_word total, killed;            // for the destruction ratio
};

struct PlayerShots {
  std::array<PlayerShotDataType, MAX_PWEAPON + 1> slots;
  std::array<JE_byte, MAX_PWEAPON> ttl;  // ticks each shot has left, 0 for a free slot
};

struct EnemyShots {
  std::array<EnemyShotType, ENEMY_SHOT_MAX> slots;
  std::array<JE_boolean, ENEMY_SHOT_MAX> available;  // whether each slot is free
};

struct State {
  Random random;
  Explosions explosions;
  Superpixels superpixels;
  Starfield starfield;
  Players players;
  Specials specials;
  Enemies enemies;
  PlayerShots player_shots;
  EnemyShots enemy_shots;
};

}  // namespace tyrx::sim
