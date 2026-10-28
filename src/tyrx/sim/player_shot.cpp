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

// The player shots: ports of player_shot_move_and_draw, player_shot_create
// and player_shot_set_direction (shots.c), and of their update in JE_main's
// level loop (tyrian2.c), where they hit enemies.

#include "sim.hpp"

#include <cmath>
#include <cstdlib>

namespace tyrx::sim {

namespace {

// shots.c
constexpr uint16_t link_multi_gr[17] = {77,  221, 183, 301, 1,   282, 164, 202, 58,  // [0..16]
                                        201, 163, 281, 39,  300, 182, 220, 77};
constexpr uint16_t link_sonic_gr[17] = {85,  242, 131, 303, 47,  284, 150, 223, 66,  // [0..16]
                                        224, 149, 283, 9,   302, 130, 243, 85};
constexpr uint16_t link_mult2_gr[17] = {78,  299, 295, 297, 2,   278, 276, 280, 59,  // [0..16]
                                        279, 275, 277, 40,  296, 294, 298, 78};

// What a shot's move leaves for its hits on enemies.  Its blast filter is in
// the C code's temp2 (Scratch), which the hits use and can change.
struct MovedShot {
  bool     is_special = false;
  int      x = 0, y = 0;
  int16_t  damage = 0;
  uint8_t  chain = 0;
  uint16_t radius_w = 0, radius_h = 0;  // a special shot's
};

// player_shot_move_and_draw: moves the shot in slot shot_id, and adds it to
// visible if it is drawn.  Returns false if it ended.  It sets player_num to
// the shot's player, except for the Zinglon's shot (the last slot), whose
// move does nothing.
bool move_shot(State& state, int shot_id, MovedShot& moved, uint8_t& player_num,
               std::vector<VisiblePlayerShot>& visible) {
  auto& players = state.players.slots;
  auto& enemies = state.enemies;
  auto& ttl = state.player_shots.ttl;

  PlayerShot& shot = state.player_shots.slots[shot_id];

  ttl[shot_id]--;
  if (shot_id == MAX_PWEAPON - 1) {
    return true;
  }

  auto const player = [&]() -> Player const& { return players[shot.player_num - 1]; };

  shot.x_speed += shot.x_accel;
  shot.x += shot.x_speed;
  int16_t const tmp_shot_xm = shot.x_speed;

  if (shot.x_speed > 100) {
    if (shot.x_speed == 101) {
      shot.x -= 101;
      shot.x += player().delta_x_shot_move;
      shot.y += player().delta_y_shot_move;
    }
    else {
      shot.x -= 120;
      shot.x += player().delta_x_shot_move;
    }
  }

  shot.y_speed += shot.y_accel;
  shot.y += shot.y_speed;

  if (shot.y_speed > 100) {
    shot.y -= 120;
    shot.y += player().delta_y_shot_move;
  }

  if (shot.circling != 0) {
    shot.x_circle += shot.x_circle_dir;
    shot.x += shot.x_circle;

    if (abs(shot.x_circle) == shot.x_circle_size) {
      shot.x_circle_dir = -shot.x_circle_dir;
    }

    shot.y_circle += shot.y_circle_dir;
    shot.y += shot.y_circle;

    if (abs(shot.y_circle) == shot.y_circle_size) {
      shot.y_circle_dir = -shot.y_circle_dir;
    }

    // Double Speed Circle Shots - add a second copy of above loop
  }

  moved.x = shot.x;
  moved.y = shot.y;

  if (shot.x < -34 || shot.x > 290 || shot.y < -15 || shot.y > 190) {
    ttl[shot_id] = 0;
    return false;
  }

  if (shot.trail != 255) {
    if (shot.trail == 98) {
      setup_explosion(state.explosions.slots, shot.x - shot.x_speed, shot.y - shot.y_speed, 0, shot.trail,
                      false, false);
    }
    else {
      setup_explosion(state.explosions.slots, shot.x, shot.y, 0, shot.trail, false, false);
    }
  }

  if (shot.aim_at_enemy != 0 && --shot.aim_delay == 0) {
    shot.aim_delay = shot.aim_delay_max;

    if (enemies.status[shot.aim_at_enemy - 1] != 1) {
      Enemy const& target = enemies.slots[shot.aim_at_enemy - 1];
      if (shot.x < target.x) {
        shot.x_speed++;
      }
      else {
        shot.x_speed--;
      }

      if (shot.y < target.y) {
        shot.y_speed++;
      }
      else {
        shot.y_speed--;
      }
    }
    else {
      if (shot.x_speed > 0) {
        shot.x_speed++;
      }
      else {
        shot.x_speed--;
      }
    }
  }

  uint16_t sprite_frame = shot.sprite + shot.frame;
  if (++shot.frame == shot.frames) {
    shot.frame = 0;
  }

  moved.damage = shot.damage;
  state.scratch.temp2 = shot.blast_filter;
  moved.chain = shot.chain_reaction;
  player_num = shot.player_num;

  moved.is_special = sprite_frame > 60000;

  if (moved.is_special) {
    visible.push_back({moved.x, moved.y, sprite_frame, false});

    moved.radius_w = sprite(OPTION_SHAPES, sprite_frame - 60001)->width / 2;
    moved.radius_h = sprite(OPTION_SHAPES, sprite_frame - 60001)->height / 2;
    return true;
  }

  if (sprite_frame > 1000) {
    do_sp(state.superpixels, state.random.mt, moved.x + 1 + 6, moved.y + 6, 5, 3, (sprite_frame / 1000) << 4);
    sprite_frame = sprite_frame % 1000;
  }
  visible.push_back({moved.x, moved.y, sprite_frame,
                     state.map.background2 && moved.y + shadowYDist < 190 && tmp_shot_xm < 100});
  return true;
}

// The shot in slot z hits the enemies it collides with, until it ends.
void hit_enemies(State& state, int z, MovedShot& shot, uint8_t player_num) {
  auto&      players = state.players.slots;
  auto&      enemies = state.enemies;
  bool const zinglon = z == MAX_PWEAPON - 1;

  for (int b = 0; b < 100; b++) {
    if (enemies.status[b] != 0) {
      continue;
    }

    Enemy const& e = enemies.slots[b];

    bool collided;

    if (zinglon) {
      uint8_t const reach = 25 - abs(state.specials.zinglon_duration - 25);
      collided = abs(e.x + e.map_offset - (players[0].x + 7)) < reach;
      state.scratch.temp2 = 9;
      shot.chain = 0;
      shot.damage = 10;
    }
    else if (shot.is_special) {
      uint16_t const w = shot.radius_w, h = shot.radius_h;
      collided = ((e.frame == 0) && (abs(e.x + e.map_offset - shot.x - w) < (25 + w)) &&
                  (abs(e.y - shot.y - 12 - h) < (29 + h))) ||
                 ((e.frame > 0) && (abs(e.x + e.map_offset - shot.x - w) < (13 + w)) &&
                  (abs(e.y - shot.y - 6 - h) < (15 + h)));
    }
    else {
      collided =
          ((e.frame == 0) && (abs(e.x + e.map_offset - shot.x) < 25) && (abs(e.y - shot.y - 12) < 29)) ||
          ((e.frame > 0) && (abs(e.x + e.map_offset - shot.x) < 13) && (abs(e.y - shot.y - 6) < 15));
    }

    if (!collided) {
      continue;
    }

    if (shot.chain > 0) {
      state.players.shot_multi_pos[SHOT_MISC] = 0;
      create_player_shot(state, 0, SHOT_MISC, shot.x, shot.y, players[0].mouse_x, players[0].mouse_y,
                         shot.chain, player_num);
      state.player_shots.ttl[z] = 0;
      return;
    }

    int const armorleft =
        hit_enemy(state, b, shot.damage, state.scratch.temp2, zinglon, shot.x, shot.y, player_num);

    if (state.player_shots.infinite_shot || zinglon) {
      continue;
    }

    if (shot.damage <= armorleft) {
      state.player_shots.ttl[z] = 0;
      return;
    }

    state.player_shots.slots[z].damage -= armorleft;
  }
}

}  // namespace

std::vector<VisiblePlayerShot> update_player_shots(State& state) {
  std::vector<VisiblePlayerShot> visible;

  // The C code doesn't set the player number for the Zinglon's shot, whose
  // move does nothing, so it is undefined there.  Here it is the last moved
  // shot's, or player 1's.
  uint8_t player_num = 1;

  for (int z = 0; z < MAX_PWEAPON; z++) {
    if (state.player_shots.ttl[z] == 0) {
      continue;
    }

    MovedShot shot;
    if (!move_shot(state, z, shot, player_num, visible)) {
      continue;
    }

    hit_enemies(state, z, shot, player_num);
  }
  return visible;
}

// FKA Varz.initPlayerShot
int16_t create_player_shot(State& state, uint16_t port_num, unsigned bay_i, uint16_t px, uint16_t py,
                           uint16_t mouse_x, uint16_t mouse_y, uint16_t wp_num, uint8_t player_num) {
  auto& players = state.players;
  auto& enemies = state.enemies;
  auto& ttl = state.player_shots.ttl;

  static uint8_t const sound_channel[11] = {0, 2, 4, 4, 2, 2, 5, 5, 1, 4, 1};  // [1..11]

  // Bounds check
  if (port_num > PORT_NUM || wp_num <= 0 || wp_num > WEAP_NUM) {
    return MAX_PWEAPON;
  }

  JE_WeaponType const& weapon = weapons[wp_num];

  if (players.power < weaponPort[port_num].poweruse) {
    return MAX_PWEAPON;
  }
  players.power -= weaponPort[port_num].poweruse;

  if (weapon.sound > 0) {
    state.sound.queue[sound_channel[bay_i]] = weapon.sound;
  }

  int shot_id = MAX_PWEAPON;
  // Rot
  for (int multi_i = 1; multi_i <= weapon.multi; multi_i++) {
    for (shot_id = 0; shot_id < MAX_PWEAPON; shot_id++) {
      if (ttl[shot_id] == 0) {
        break;
      }
    }
    if (shot_id == MAX_PWEAPON) {
      return MAX_PWEAPON;
    }

    uint8_t& multi_pos = players.shot_multi_pos[bay_i];
    if (multi_pos == weapon.max || multi_pos > 8) {
      multi_pos = 1;
    }
    else {
      multi_pos++;
    }
    int const pos = multi_pos - 1;

    PlayerShot& shot = state.player_shots.slots[shot_id];
    shot.chain_reaction = 0;

    shot.player_num = player_num;

    shot.frame = 0;

    shot.circling = weapon.circlesize != 0;

    if (weapon.circlesize == 0) {
      shot.x_circle = 0;
      shot.x_circle_dir = 0;
      shot.y_circle = 0;
      shot.y_circle_dir = 0;
      shot.x_circle_size = 0;
      shot.y_circle_size = 0;
    }
    else {
      uint8_t circsize = weapon.circlesize;

      if (circsize > 19) {
        uint8_t circsize_mod20 = circsize % 20;
        shot.x_circle_size = circsize_mod20;
        shot.x_circle = circsize_mod20 >> 1;

        circsize = circsize / 20;
        shot.y_circle_size = circsize;
        shot.y_circle = circsize >> 1;
      }
      else {
        shot.x_circle_size = circsize;
        shot.y_circle_size = circsize;
        shot.x_circle = circsize >> 1;
        shot.y_circle = circsize >> 1;
      }
      shot.x_circle_dir = 1;
      shot.y_circle_dir = -1;
    }

    shot.trail = weapon.trail;

    if (weapon.attack[pos] > 99 && weapon.attack[pos] < 250) {
      shot.chain_reaction = weapon.attack[pos] - 100;
      shot.damage = 1;
    }
    else {
      shot.damage = weapon.attack[pos];
    }

    shot.blast_filter = weapon.shipblastfilter;

    int16_t tmp_by = weapon.by[pos];

    // Note: Only front selection used for player shots...

    shot.x = px + weapon.bx[pos];

    shot.y = py + tmp_by;
    shot.y_accel = -weapon.acceleration;
    shot.x_accel = weapon.accelerationx;

    shot.x_speed = weapon.sx[pos];

    // Not sure what this field does exactly.
    uint8_t del = weapon.del[pos];

    if (del == 121) {
      shot.trail = 0;
      del = 255;
    }

    shot.sprite = weapon.sg[pos];
    if (shot.sprite == 0) {
      ttl[shot_id] = 0;
    }
    else {
      ttl[shot_id] = del;
    }

    if (del > 100 && del < 120) {
      shot.frames = (del - 100 + 1);
    }
    else {
      shot.frames = weapon.weapani + 1;
    }

    if (del == 99 || del == 98) {
      tmp_by = px - mouse_x;
      if (tmp_by < -5) {
        tmp_by = -5;
      }
      else if (tmp_by > 5) {
        tmp_by = 5;
      }
      shot.x_speed += tmp_by;
    }

    auto const player = [&]() -> Player const& { return players.slots[shot.player_num - 1]; };

    if (del == 99 || del == 100) {
      tmp_by = py - mouse_y - weapon.sy[pos];
      if (tmp_by < -4) {
        tmp_by = -4;
      }
      else if (tmp_by > 4) {
        tmp_by = 4;
      }
      shot.y_speed = tmp_by;
    }
    else if (weapon.sy[pos] == 98) {
      shot.y_speed = 0;
      shot.y_accel = -1;
    }
    else if (weapon.sy[pos] > 100) {
      shot.y_speed = weapon.sy[pos];
      shot.y -= player().delta_y_shot_move;
    }
    else {
      shot.y_speed = -weapon.sy[pos];
    }

    if (weapon.sx[pos] > 100) {
      shot.x_speed = weapon.sx[pos];
      shot.x -= player().delta_x_shot_move;
      if (shot.x_speed == 101) {
        shot.y -= player().delta_y_shot_move;
      }
    }

    // Guided Shot
    if (weapon.aim > 5) {
      unsigned best_dist = 65000;
      uint8_t  closest_enemy = 0;
      // Find Closest Enemy (the C code counts in the uint16_t globals x and y)
      for (uint16_t i = 0; i < 100; i++) {
        if (enemies.status[i] == 1 || enemies.slots[i].score_item) {
          continue;
        }

        uint16_t const dist = abs(enemies.slots[i].x - shot.x) + abs(enemies.slots[i].y - shot.y);
        if (dist < best_dist) {
          best_dist = dist;
          closest_enemy = i + 1;
        }
      }
      shot.aim_at_enemy = closest_enemy;
      shot.aim_delay = 5;
      shot.aim_delay_max = weapon.aim - 5;
    }
    else {
      shot.aim_at_enemy = 0;
    }

    players.shot_repeat[bay_i] = weapon.shotrepeat;
  }

  return shot_id;
}

void set_player_shot_direction(PlayerShot& shot, unsigned weapon_id, float direction) {
  shot.x_speed = -roundf(sinf(direction) * shot.y_speed);
  shot.y_speed = -roundf(cosf(direction) * shot.y_speed);

  // Some weapons have sprites for each direction, use those.
  int rounded_dir;

  switch (weapon_id) {
    case 27:
    case 32:
    case 10:
      rounded_dir = roundf(direction * (16 / (2 * M_PI)));  // 16 directions
      shot.sprite = link_multi_gr[rounded_dir];
      break;
    case 28:
    case 33:
    case 11:
      rounded_dir = roundf(direction * (16 / (2 * M_PI)));  // 16 directions
      shot.sprite = link_sonic_gr[rounded_dir];
      break;
    case 30:
    case 35:
    case 14:
      if (direction > M_PI_2 && direction < M_PI + M_PI_2) {
        shot.y_accel = 1;
      }
      break;
    case 38:
    case 22:
      rounded_dir = roundf(direction * (16 / (2 * M_PI)));  // 16 directions
      shot.sprite = link_mult2_gr[rounded_dir];
      break;
  }
}

}  // namespace tyrx::sim
