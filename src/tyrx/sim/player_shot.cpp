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
constexpr JE_word link_multi_gr[17] = {77,  221, 183, 301, 1,   282, 164, 202, 58,  // [0..16]
                                       201, 163, 281, 39,  300, 182, 220, 77};
constexpr JE_word link_sonic_gr[17] = {85,  242, 131, 303, 47,  284, 150, 223, 66,  // [0..16]
                                       224, 149, 283, 9,   302, 130, 243, 85};
constexpr JE_word link_mult2_gr[17] = {78,  299, 295, 297, 2,   278, 276, 280, 59,  // [0..16]
                                       279, 275, 277, 40,  296, 294, 298, 78};

// What a shot's move leaves for its hits on enemies.
struct MovedShot {
  bool is_special = false;
  int x = 0, y = 0;
  JE_integer damage = 0;
  JE_byte filter = 0, chain = 0;
  JE_word radius_w = 0, radius_h = 0;  // a special shot's
};

// player_shot_move_and_draw: moves the shot in slot shot_id, and adds it to
// visible if it is drawn.  Returns false if it ended.  It sets player_num to
// the shot's player, except for the Zinglon's shot (the last slot), whose
// move does nothing.
bool move_shot(State& state, int shot_id, MovedShot& moved, JE_byte& player_num,
               std::vector<VisiblePlayerShot>& visible) {
  auto& players = state.players.slots;
  auto& enemies = state.enemies;
  auto& ttl = state.player_shots.ttl;
  PlayerShotDataType& shot = state.player_shots.slots[shot_id];

  ttl[shot_id]--;
  if (shot_id == MAX_PWEAPON - 1) {
    return true;
  }

  auto const player = [&]() -> Player const& { return players[shot.playerNumber - 1]; };

  shot.shotXM += shot.shotXC;
  shot.shotX += shot.shotXM;
  JE_integer const tmp_shot_xm = shot.shotXM;

  if (shot.shotXM > 100) {
    if (shot.shotXM == 101) {
      shot.shotX -= 101;
      shot.shotX += player().delta_x_shot_move;
      shot.shotY += player().delta_y_shot_move;
    }
    else {
      shot.shotX -= 120;
      shot.shotX += player().delta_x_shot_move;
    }
  }

  shot.shotYM += shot.shotYC;
  shot.shotY += shot.shotYM;

  if (shot.shotYM > 100) {
    shot.shotY -= 120;
    shot.shotY += player().delta_y_shot_move;
  }

  if (shot.shotComplicated != 0) {
    shot.shotDevX += shot.shotDirX;
    shot.shotX += shot.shotDevX;

    if (abs(shot.shotDevX) == shot.shotCirSizeX) {
      shot.shotDirX = -shot.shotDirX;
    }

    shot.shotDevY += shot.shotDirY;
    shot.shotY += shot.shotDevY;

    if (abs(shot.shotDevY) == shot.shotCirSizeY) {
      shot.shotDirY = -shot.shotDirY;
    }

    // Double Speed Circle Shots - add a second copy of above loop
  }

  moved.x = shot.shotX;
  moved.y = shot.shotY;

  if (shot.shotX < -34 || shot.shotX > 290 || shot.shotY < -15 || shot.shotY > 190) {
    ttl[shot_id] = 0;
    return false;
  }

  if (shot.shotTrail != 255) {
    if (shot.shotTrail == 98) {
      setup_explosion(state.explosions.slots, shot.shotX - shot.shotXM, shot.shotY - shot.shotYM, 0,
                      shot.shotTrail, false, false);
    }
    else {
      setup_explosion(state.explosions.slots, shot.shotX, shot.shotY, 0, shot.shotTrail, false, false);
    }
  }

  if (shot.aimAtEnemy != 0 && --shot.aimDelay == 0) {
    shot.aimDelay = shot.aimDelayMax;

    if (enemies.status[shot.aimAtEnemy - 1] != 1) {
      JE_SingleEnemyType const& target = enemies.slots[shot.aimAtEnemy - 1];
      if (shot.shotX < target.ex) {
        shot.shotXM++;
      }
      else {
        shot.shotXM--;
      }

      if (shot.shotY < target.ey) {
        shot.shotYM++;
      }
      else {
        shot.shotYM--;
      }
    }
    else {
      if (shot.shotXM > 0) {
        shot.shotXM++;
      }
      else {
        shot.shotXM--;
      }
    }
  }

  JE_word sprite_frame = shot.shotGr + shot.shotAni;
  if (++shot.shotAni == shot.shotAniMax) {
    shot.shotAni = 0;
  }

  moved.damage = shot.shotDmg;
  moved.filter = shot.shotBlastFilter;
  moved.chain = shot.chainReaction;
  player_num = shot.playerNumber;

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
void hit_enemies(State& state, int z, MovedShot& shot, JE_byte player_num) {
  auto& players = state.players.slots;
  auto& enemies = state.enemies;
  bool const zinglon = z == MAX_PWEAPON - 1;

  for (int b = 0; b < 100; b++) {
    if (enemies.status[b] != 0) {
      continue;
    }

    JE_SingleEnemyType const& e = enemies.slots[b];
    bool collided;

    if (zinglon) {
      JE_byte const reach = 25 - abs(state.specials.zinglon_duration - 25);
      collided = abs(e.ex + e.mapoffset - (players[0].x + 7)) < reach;
      shot.filter = 9;
      shot.chain = 0;
      shot.damage = 10;
    }
    else if (shot.is_special) {
      JE_word const w = shot.radius_w, h = shot.radius_h;
      collided = ((e.enemycycle == 0) && (abs(e.ex + e.mapoffset - shot.x - w) < (25 + w)) &&
                  (abs(e.ey - shot.y - 12 - h) < (29 + h))) ||
                 ((e.enemycycle > 0) && (abs(e.ex + e.mapoffset - shot.x - w) < (13 + w)) &&
                  (abs(e.ey - shot.y - 6 - h) < (15 + h)));
    }
    else {
      collided =
          ((e.enemycycle == 0) && (abs(e.ex + e.mapoffset - shot.x) < 25) &&
           (abs(e.ey - shot.y - 12) < 29)) ||
          ((e.enemycycle > 0) && (abs(e.ex + e.mapoffset - shot.x) < 13) && (abs(e.ey - shot.y - 6) < 15));
    }

    if (!collided) {
      continue;
    }

    if (shot.chain > 0) {
      state.players.shot_multi_pos[SHOT_MISC] = 0;
      create_player_shot(state, 0, SHOT_MISC, shot.x, shot.y, players[0].mouseX, players[0].mouseY,
                         shot.chain, player_num);
      state.player_shots.ttl[z] = 0;
      return;
    }

    int const armorleft = hit_enemy(state, b, shot.damage, shot.filter, zinglon, shot.x, shot.y, player_num);

    if (state.cheats.infinite_shot || zinglon) {
      continue;
    }

    if (shot.damage <= armorleft) {
      state.player_shots.ttl[z] = 0;
      return;
    }

    state.player_shots.slots[z].shotDmg -= armorleft;
  }
}

}  // namespace

std::vector<VisiblePlayerShot> update_player_shots(State& state) {
  std::vector<VisiblePlayerShot> visible;

  // The C code doesn't set the player number for the Zinglon's shot, whose
  // move does nothing, so it is undefined there.  Here it is the last moved
  // shot's, or player 1's.
  JE_byte player_num = 1;

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
JE_integer create_player_shot(State& state, JE_word port_num, uint bay_i, JE_word px, JE_word py,
                              JE_word mouse_x, JE_word mouse_y, JE_word wp_num, JE_byte player_num) {
  auto& players = state.players;
  auto& enemies = state.enemies;
  auto& ttl = state.player_shots.ttl;
  static JE_byte const sound_channel[11] = {0, 2, 4, 4, 2, 2, 5, 5, 1, 4, 1};  // [1..11]

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

    JE_byte& multi_pos = players.shot_multi_pos[bay_i];
    if (multi_pos == weapon.max || multi_pos > 8) {
      multi_pos = 1;
    }
    else {
      multi_pos++;
    }
    int const pos = multi_pos - 1;

    PlayerShotDataType& shot = state.player_shots.slots[shot_id];
    shot.chainReaction = 0;

    shot.playerNumber = player_num;

    shot.shotAni = 0;

    shot.shotComplicated = weapon.circlesize != 0;

    if (weapon.circlesize == 0) {
      shot.shotDevX = 0;
      shot.shotDirX = 0;
      shot.shotDevY = 0;
      shot.shotDirY = 0;
      shot.shotCirSizeX = 0;
      shot.shotCirSizeY = 0;
    }
    else {
      JE_byte circsize = weapon.circlesize;

      if (circsize > 19) {
        JE_byte circsize_mod20 = circsize % 20;
        shot.shotCirSizeX = circsize_mod20;
        shot.shotDevX = circsize_mod20 >> 1;

        circsize = circsize / 20;
        shot.shotCirSizeY = circsize;
        shot.shotDevY = circsize >> 1;
      }
      else {
        shot.shotCirSizeX = circsize;
        shot.shotCirSizeY = circsize;
        shot.shotDevX = circsize >> 1;
        shot.shotDevY = circsize >> 1;
      }
      shot.shotDirX = 1;
      shot.shotDirY = -1;
    }

    shot.shotTrail = weapon.trail;

    if (weapon.attack[pos] > 99 && weapon.attack[pos] < 250) {
      shot.chainReaction = weapon.attack[pos] - 100;
      shot.shotDmg = 1;
    }
    else {
      shot.shotDmg = weapon.attack[pos];
    }

    shot.shotBlastFilter = weapon.shipblastfilter;

    JE_integer tmp_by = weapon.by[pos];

    // Note: Only front selection used for player shots...

    shot.shotX = px + weapon.bx[pos];

    shot.shotY = py + tmp_by;
    shot.shotYC = -weapon.acceleration;
    shot.shotXC = weapon.accelerationx;

    shot.shotXM = weapon.sx[pos];

    // Not sure what this field does exactly.
    JE_byte del = weapon.del[pos];

    if (del == 121) {
      shot.shotTrail = 0;
      del = 255;
    }

    shot.shotGr = weapon.sg[pos];
    if (shot.shotGr == 0) {
      ttl[shot_id] = 0;
    }
    else {
      ttl[shot_id] = del;
    }

    if (del > 100 && del < 120) {
      shot.shotAniMax = (del - 100 + 1);
    }
    else {
      shot.shotAniMax = weapon.weapani + 1;
    }

    if (del == 99 || del == 98) {
      tmp_by = px - mouse_x;
      if (tmp_by < -5) {
        tmp_by = -5;
      }
      else if (tmp_by > 5) {
        tmp_by = 5;
      }
      shot.shotXM += tmp_by;
    }

    auto const player = [&]() -> Player const& { return players.slots[shot.playerNumber - 1]; };

    if (del == 99 || del == 100) {
      tmp_by = py - mouse_y - weapon.sy[pos];
      if (tmp_by < -4) {
        tmp_by = -4;
      }
      else if (tmp_by > 4) {
        tmp_by = 4;
      }
      shot.shotYM = tmp_by;
    }
    else if (weapon.sy[pos] == 98) {
      shot.shotYM = 0;
      shot.shotYC = -1;
    }
    else if (weapon.sy[pos] > 100) {
      shot.shotYM = weapon.sy[pos];
      shot.shotY -= player().delta_y_shot_move;
    }
    else {
      shot.shotYM = -weapon.sy[pos];
    }

    if (weapon.sx[pos] > 100) {
      shot.shotXM = weapon.sx[pos];
      shot.shotX -= player().delta_x_shot_move;
      if (shot.shotXM == 101) {
        shot.shotY -= player().delta_y_shot_move;
      }
    }

    // Guided Shot
    if (weapon.aim > 5) {
      uint best_dist = 65000;
      JE_byte closest_enemy = 0;
      // Find Closest Enemy (the C code counts in the JE_word globals x and y)
      for (JE_word i = 0; i < 100; i++) {
        if (enemies.status[i] == 1 || enemies.slots[i].scoreitem) {
          continue;
        }

        JE_word const dist = abs(enemies.slots[i].ex - shot.shotX) + abs(enemies.slots[i].ey - shot.shotY);
        if (dist < best_dist) {
          best_dist = dist;
          closest_enemy = i + 1;
        }
      }
      shot.aimAtEnemy = closest_enemy;
      shot.aimDelay = 5;
      shot.aimDelayMax = weapon.aim - 5;
    }
    else {
      shot.aimAtEnemy = 0;
    }

    players.shot_repeat[bay_i] = weapon.shotrepeat;
  }

  return shot_id;
}

void set_player_shot_direction(PlayerShotDataType& shot, uint weapon_id, JE_real direction) {
  shot.shotXM = -roundf(sinf(direction) * shot.shotYM);
  shot.shotYM = -roundf(cosf(direction) * shot.shotYM);

  // Some weapons have sprites for each direction, use those.
  int rounded_dir;

  switch (weapon_id) {
    case 27:
    case 32:
    case 10:
      rounded_dir = roundf(direction * (16 / (2 * M_PI)));  // 16 directions
      shot.shotGr = link_multi_gr[rounded_dir];
      break;
    case 28:
    case 33:
    case 11:
      rounded_dir = roundf(direction * (16 / (2 * M_PI)));  // 16 directions
      shot.shotGr = link_sonic_gr[rounded_dir];
      break;
    case 30:
    case 35:
    case 14:
      if (direction > M_PI_2 && direction < M_PI + M_PI_2) {
        shot.shotYC = 1;
      }
      break;
    case 38:
    case 22:
      rounded_dir = roundf(direction * (16 / (2 * M_PI)));  // 16 directions
      shot.shotGr = link_mult2_gr[rounded_dir];
      break;
  }
}

}  // namespace tyrx::sim
