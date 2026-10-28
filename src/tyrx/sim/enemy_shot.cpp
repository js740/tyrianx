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

// The enemy shots: a port of their update in JE_main's level loop
// (tyrian2.c).

#include "sim.hpp"

namespace tyrx::sim {

namespace {

// A shot that tracks player 1 on one axis (tx, ty): its speed changes by one
// toward the player's position, up to limit either way.
void track(uint8_t limit, int16_t pos, int player_pos, int16_t& speed) {
  if (limit == 0) {
    return;
  }

  if (pos > player_pos) {
    if (speed > -limit) {
      speed--;
    }
  }
  else if (speed < limit) {
    speed++;
  }
}

}  // namespace

std::bitset<ENEMY_SHOT_MAX> update_enemy_shots(State& state, DrawRecord& record) {
  auto& players = state.players.slots;
  auto& shots = state.enemy_shots;

  std::bitset<ENEMY_SHOT_MAX> visible;

  for (int z = 0; z < ENEMY_SHOT_MAX; z++) {
    if (shots.available[z] != 0) {
      continue;
    }

    EnemyShotType& shot = shots.slots[z];

    shot.sxm += shot.sxc;
    shot.sx += shot.sxm;
    track(shot.tx, shot.sx, players[0].x, shot.sxm);

    shot.sym += shot.syc;
    shot.sy += shot.sym;
    track(shot.ty, shot.sy, players[0].y, shot.sym);

    if (shot.duration-- == 0 || shot.sy > 190 || shot.sy <= -14 || shot.sx > 275 || shot.sx <= 0) {
      shots.available[z] = true;
      continue;
    }

    // check if shot collided with player
    for (unsigned i = 0; i < (state.game.two_player_mode ? 2 : 1); ++i) {
      Player& player = players[i];
      if (!(player.is_alive && shot.sx > player.x - (signed)player.shot_hit_area_x &&
            shot.sx < player.x + (signed)player.shot_hit_area_x &&
            shot.sy > player.y - (signed)player.shot_hit_area_y &&
            shot.sy < player.y + (signed)player.shot_hit_area_y))
      {
        continue;
      }

      shots.available[z] = true;

      setup_explosion(state.explosions.slots, shot.sx, shot.sy, 0, 0, false, false);

      if (player.invulnerable_ticks == 0) {
        uint8_t const through = damage_player(state, record, player, shot.sdmg);
        if (through > 0) {
          player.x_velocity += (shot.sxm * through) / 2;
          player.y_velocity += (shot.sym * through) / 2;
        }
      }

      break;
    }

    if (shots.available[z]) {
      continue;
    }

    if (shot.animax != 0 && ++shot.animate >= shot.animax) {
      shot.animate = 0;
    }

    visible[z] = true;
  }
  return visible;
}

}  // namespace tyrx::sim
