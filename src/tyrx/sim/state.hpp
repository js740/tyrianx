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
  Enemies enemies;
  PlayerShots player_shots;
  EnemyShots enemy_shots;
};

}  // namespace tyrx::sim
