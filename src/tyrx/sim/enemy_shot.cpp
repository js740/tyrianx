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

    EnemyShot& shot = shots.slots[z];

    shot.x_speed += shot.x_accel;
    shot.x += shot.x_speed;
    track(shot.x_track, shot.x, players[0].x, shot.x_speed);

    shot.y_speed += shot.y_accel;
    shot.y += shot.y_speed;
    track(shot.y_track, shot.y, players[0].y, shot.y_speed);

    if (shot.duration-- == 0 || shot.y > 190 || shot.y <= -14 || shot.x > 275 || shot.x <= 0) {
      shots.available[z] = true;
      continue;
    }

    // check if shot collided with player
    for (unsigned i = 0; i < (state.game.two_player_mode ? 2 : 1); ++i) {
      Player& player = players[i];
      if (!(player.is_alive && shot.x > player.x - (signed)player.shot_hit_area_x &&
            shot.x < player.x + (signed)player.shot_hit_area_x &&
            shot.y > player.y - (signed)player.shot_hit_area_y &&
            shot.y < player.y + (signed)player.shot_hit_area_y))
      {
        continue;
      }

      shots.available[z] = true;

      setup_explosion(state.explosions.slots, shot.x, shot.y, 0, 0, false, false);

      if (player.invulnerable_ticks == 0) {
        uint8_t const through = damage_player(state, record, player, shot.damage);
        if (through > 0) {
          player.x_velocity += (shot.x_speed * through) / 2;
          player.y_velocity += (shot.y_speed * through) / 2;
        }
      }

      break;
    }

    if (shots.available[z]) {
      continue;
    }

    if (shot.frames != 0 && ++shot.frame >= shot.frames) {
      shot.frame = 0;
    }

    visible[z] = true;
  }
  return visible;
}

}  // namespace tyrx::sim
