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

// The enemies: ports of JE_drawEnemy (which "actually does a whole lot more
// than just drawing"), JE_newEnemy and JE_makeEnemy, and of player shots'
// hits on enemies in JE_main's level loop (tyrian2.c).  The enemies' and
// weapons' data, which the level's files load, are still in the C globals
// enemyDat and weapons.

#include "sim.hpp"

#include "../../logging.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iterator>

namespace tyrx::sim {

namespace {

// The enemy's homing in on player 1 on one axis (x_homing, y_homing): if
// accel is set, its speed changes by one toward the player's position, at
// random and up to accel - 89 either way.
void home_in(MtState& mt, uint8_t accel, int player_pos, int16_t pos, int8_t& speed) {
  if (!accel || accel - 89u <= mt_rand_r(&mt) % 11) {
    return;
  }

  if (player_pos > pos) {
    if (speed < accel - 89) {
      speed++;
    }
  }
  else if (speed >= 0 || -speed < accel - 89) {
    speed--;
  }
}

// The enemy's fixed acceleration on one axis (x_accel, y_accel): every
// wait_max ticks, its speed changes by step, and the acceleration reverses
// when the speed reaches reverse_speed.
void accelerate(int8_t& speed, int8_t& accel, int8_t& wait, uint8_t wait_max, int8_t& reverse_speed,
                int16_t& step) {
  if (!accel || --wait > 0) {
    return;
  }

  auto const reverse = [&] {
    accel = -accel;
    reverse_speed = -reverse_speed;
    step = -step;
  };

  if (speed == reverse_speed) {
    reverse();
    return;
  }

  speed += step;
  wait = wait_max;
  if (speed == reverse_speed) {
    reverse();
  }
}

// Picks a free channel in the sound queue, at random: any but the fourth.
unsigned long random_channel(MtState& mt) {
  unsigned long channel;
  do {
    channel = mt_rand_r(&mt) % 8;
  } while (channel == 3);
  return channel;
}

// What the enemies' update knows of the layer it updates.
struct Layer {
  int      end;        // the enemies are the 25 before this slot
  uint16_t map_x_ofs;  // how far the layer has scrolled sideways
  uint16_t back_move;  // how far it moved down in the tick
};

// The enemy's turret j (1 to 3), at (x, y), fires its weapon's shots.
// Returns false if there is no free slot for one, which ends the enemy's
// update.
bool fire_shots(State& state, Enemy& e, int j, uint8_t weapon_id, int16_t x, int16_t y, Layer const& layer) {
  auto& players = state.players.slots;
  auto& shots = state.enemy_shots;

  JE_WeaponType const& weapon = weapons[weapon_id];

  for (int n = weapon.multi; n > 0; n--) {
    auto const free = std::ranges::find(shots.available, 1);
    if (free == std::end(shots.available)) {
      return false;
    }
    int const b = free - std::begin(shots.available);
    shots.available[b] = false;

    if (weapon.sound > 0) {
      unsigned long const channel = random_channel(state.random.mt);
      state.sound.queue[channel] = weapon.sound;
    }

    if (e.animation == 2) {
      e.animation = 1;
    }

    if (++e.shot_pos[j - 1] > weapon.max) {
      e.shot_pos[j - 1] = 1;
    }

    int const pos = e.shot_pos[j - 1] - 1;

    if (j == 1) {
      state.scratch.temp2 = 4;
    }

    EnemyShot& shot = shots.slots[b];
    shot.x = x + weapon.bx[pos] + layer.map_x_ofs;
    shot.y = y + weapon.by[pos];
    shot.damage = weapon.attack[pos];
    shot.x_track = weapon.tx;
    shot.y_track = weapon.ty;
    shot.duration = weapon.del[pos];
    shot.frame = 0;
    shot.frames = weapon.weapani;

    shot.sprite = weapon.sg[pos];
    switch (j) {
      case 1:
        shot.y_accel = weapon.acceleration;
        shot.x_accel = weapon.accelerationx;

        shot.x_speed = weapon.sx[pos];
        shot.y_speed = weapon.sy[pos];
        break;
      case 3:
        shot.x_accel = -weapon.acceleration;
        shot.y_accel = weapon.accelerationx;

        shot.x_speed = -weapon.sy[pos];
        shot.y_speed = -weapon.sx[pos];
        break;
      case 2:
        shot.x_accel = weapon.acceleration;
        shot.y_accel = -weapon.acceleration;

        shot.x_speed = weapon.sy[pos];
        shot.y_speed = -weapon.sx[pos];
        break;
    }

    if (weapon.aim <= 0) {
      continue;
    }

    uint8_t aim = weapon.aim;

    // DIF
    if (state.game.difficulty_level > DIFFICULTY_NORMAL) {
      aim += state.game.difficulty_level - 2;
    }

    int target_x = players[0].x;
    int target_y = players[0].y;

    if (state.game.two_player_mode) {
      // fire at live player(s)
      int target;
      if (players[0].is_alive && !players[1].is_alive) {
        target = 0;
      }
      else if (players[1].is_alive && !players[0].is_alive) {
        target = 1;
      }
      else {
        target = mt_rand_r(&state.random.mt) % 2;
      }

      if (target == 1) {
        target_x = players[1].x;
        target_y = players[1].y;
      }
    }

    int16_t aim_x = target_x - x - layer.map_x_ofs - 4;
    if (aim_x == 0) {
      aim_x = 1;
    }
    int16_t aim_y = target_y - y;
    if (aim_y == 0) {
      aim_y = 1;
    }
    int16_t const max_mag_aim = std::max(abs(aim_x), abs(aim_y));
    shot.x_speed = roundf((float)aim_x / max_mag_aim * aim);
    shot.y_speed = roundf((float)aim_y / max_mag_aim * aim);
  }
  return true;
}

// The enemy's turret j (1 to 3), at (x, y), fires if it is due.  player_x is
// player 1's position 25 pixels to the left.  Returns false if that ends the
// enemy's update.
bool fire(State& state, Enemy& e, int j, int16_t x, int16_t y, int player_x, Layer const& layer) {
  auto&        players = state.players.slots;
  int8_t const difficulty = state.game.difficulty_level;

  if (!e.shot_freq[j - 1]) {
    return true;
  }

  uint8_t const weapon_id = e.turrets[j - 1];

  if (--e.shot_wait[j - 1] != 0 || !weapon_id) {
    return true;
  }

  e.shot_wait[j - 1] = e.shot_freq[j - 1];
  if (difficulty > DIFFICULTY_NORMAL) {
    e.shot_wait[j - 1] = (e.shot_wait[j - 1] / 2) + 1;
    if (difficulty > DIFFICULTY_MANIACAL) {
      e.shot_wait[j - 1] = (e.shot_wait[j - 1] / 2) + 1;
    }
  }

  if (state.game.galaga_mode &&
      (e.y_speed == 0 || (mt_rand_r(&state.random.mt) % 400) >= state.level.galaga_shot_freq))
  {
    return false;
  }

  switch (weapon_id) {
    case 252:  // Savara Boss DualMissile
      if (e.y > 20) {
        uint16_t const back_move = state.map.back_move;
        setup_explosion(state.explosions.slots, x - 8 + layer.map_x_ofs, y - 20 - back_move * 8, -2, 6, false,
                        false);
        setup_explosion(state.explosions.slots, x + 4 + layer.map_x_ofs, y - 20 - back_move * 8, -2, 6, false,
                        false);
      }
      break;
    case 251: {  // Suck-O-Magnet
      int16_t const attraction = 4 - (abs(player_x - x) + abs(players[0].y - y)) / 100;
      if (attraction > 0) {
        players[0].x_velocity += (player_x > x) ? -attraction : attraction;
      }
      break;
    }
    case 253:  // Left ShortRange Magnet
      if (abs(players[0].x - 14 - x) < 24 && abs(players[0].y - y) < 28) {
        players[0].x_velocity += 2;
      }
      if (state.game.two_player_mode && (abs(players[1].x - 14 - x) < 24 && abs(players[1].y - y) < 28)) {
        players[1].x_velocity += 2;
      }
      break;
    case 254:  // Left ShortRange Magnet
      if (abs(players[0].x - 14 - x) < 24 && abs(players[0].y - y) < 28) {
        players[0].x_velocity -= 2;
      }
      if (state.game.two_player_mode && (abs(players[1].x - 14 - x) < 24 && abs(players[1].y - y) < 28)) {
        players[1].x_velocity -= 2;
      }
      break;
    case 255:  // Magneto RePulse!!
      // DIF
      if (difficulty == DIFFICULTY_EASY) {
        break;
      }
      if (j == 3) {
        e.filter = 0x70;
      }
      else {
        int16_t const repulsion = 4 - (abs(player_x - x) + abs(players[0].y - y)) / 20;
        if (repulsion > 0) {
          players[0].x_velocity += (player_x > x) ? repulsion : -repulsion;
        }
      }
      break;
    default:
      // Rot
      return fire_shots(state, e, j, weapon_id, x, y, layer);
  }
  return true;
}

// The enemy, at (x, y), launches an enemy if it is due.
void launch(State& state, Enemy& e, int16_t x, int16_t y, Layer const& layer) {
  auto&    players = state.players.slots;
  MtState& mt = state.random.mt;

  if (!e.launch_freq || --e.launch_wait != 0) {
    return;
  }

  // Enemy Launch Routine
  e.launch_wait = e.launch_freq;

  // Type  1 : Must be inline with player
  if (e.launch_special != 0 && abs(e.y - players[0].y) > 5) {
    return;
  }

  if (e.animation == 2) {
    e.animation = 1;
  }

  if (e.launch_type == 0) {
    return;
  }

  int const b = new_enemy(state, layer.end == 50 ? 75 : layer.end - 25, e.launch_type, 0);

  if (b <= 0) {
    return;
  }

  // Launch Enemy Placement
  Enemy& launched = state.enemies.slots[b - 1];

  launched.x = x;
  launched.y = y + enemyDat[launched.type].startyc;
  if (launched.size == 0) {
    launched.y -= 7;
  }

  if (launched.launch_type > 0 && launched.launch_freq == 0) {
    if (launched.launch_type > 90) {
      launched.x += mt_rand_r(&mt) % ((launched.launch_type - 90) * 4) - (launched.launch_type - 90) * 2;
    }
    else {
      int16_t aim_x = players[0].x - x - layer.map_x_ofs - 4;
      if (aim_x == 0) {
        aim_x = 1;
      }
      int16_t aim_y = players[0].y - y;
      if (aim_y == 0) {
        aim_y = 1;
      }
      int16_t const max_mag_aim = std::max(abs(aim_x), abs(aim_y));
      launched.x_speed = roundf((float)aim_x / max_mag_aim * launched.launch_type);
      launched.y_speed = roundf((float)aim_y / max_mag_aim * launched.launch_type);
    }
  }

  unsigned long const channel = random_channel(mt);
  state.sound.queue[channel] = randomEnemyLaunchSounds[(mt_rand_r(&mt) % 3)];

  if (e.launch_special == 1 && e.link_num < 100) {
    launched.link_num = e.link_num;
  }
}

// One enemy's update: it is drawn (added to visible) if on screen, then
// moves, fires and launches enemies.  player_x is player 1's position 25
// pixels to the left.
void update_enemy(State& state, int i, int player_x, Layer const& layer, std::vector<VisibleEnemy>& visible) {
  auto& players = state.players.slots;

  Enemy&   e = state.enemies.slots[i];
  MtState& mt = state.random.mt;

  e.map_offset = layer.map_x_ofs;

  home_in(mt, e.x_homing, player_x, e.x, e.x_speed);
  home_in(mt, e.y_homing, players[0].y, e.y, e.y_speed);

  auto const gone = [&] {
    // enemy[i].egr[10] &= 0x00ff; <MXD> madness?
    state.enemies.status[i] = 1;
  };

  if (e.x + layer.map_x_ofs > -29 && e.x + layer.map_x_ofs < 300) {
    if (e.animation == 1) {
      e.frame++;

      if (e.frame == e.stop_frame) {
        e.animation = e.animation_at_stop;
      }
      else if (e.frame > e.last_frame) {
        e.frame = e.first_frame;
      }
    }

    if (e.sprites[e.frame - 1] == 999) {
      return gone();
    }

    visible.push_back({e.x, e.y, e.sprites[e.frame - 1], e.filter, e.size, e.sprite_sheet});

    e.filter = 0;
  }

  accelerate(e.x_speed, e.x_accel, e.x_accel_wait, e.x_accel_wait_max, e.x_reverse_speed, e.x_accel_step);
  accelerate(e.y_speed, e.y_accel, e.y_accel_wait, e.y_accel_wait_max, e.y_reverse_speed, e.y_accel_step);

  e.y += e.fixed_move_y;

  e.x += e.x_speed;
  if (e.x < -80 || e.x > 340) {
    return gone();
  }

  e.y += e.y_speed;
  if (e.y < -112 || e.y > 190) {
    return gone();
  }

  // X bounce
  if (e.x <= e.x_min_bounce || e.x >= e.x_max_bounce) {
    e.x_speed = -e.x_speed;
  }

  // Y bounce
  if (e.y <= e.y_min_bounce || e.y >= e.y_max_bounce) {
    e.y_speed = -e.y_speed;
  }

  // Evalue != 0 - score item at boundary
  if (e.score_item) {
    if (e.x < -5) {
      e.x++;
    }
    if (e.x > 245) {
      e.x--;
    }
  }

  e.y += layer.back_move;

  if (e.x <= -24 || e.x >= 296) {
    return;
  }

  int16_t const x = e.x;
  int16_t const y = e.y;

  // Enemy Shots
  if (e.damaged == 1) {
    return;
  }

  state.enemies.on_screen++;

  if (e.iced) {
    e.iced--;
    if (e.ground != 0) {
      e.filter = 0x09;
    }
    return;
  }

  for (int j = 3; j > 0; j--) {
    if (!fire(state, e, j, x, y, player_x, layer)) {
      return;
    }
  }

  launch(state, e, x, y, layer);
}

// A hit that leaves the enemy (slot b, which had armor_before armor) some
// armor: the damage, and the enemies that the damage changes, linked to
// it by link.
void damage_enemy(State& state, int b, int armor_before, int16_t damage, uint8_t filter, uint8_t link,
                  bool zinglon, int shot_x, int shot_y) {
  auto& slots = state.enemies.slots;
  auto& status = state.enemies.status;

  Enemy& hit = slots[b];

  if (!zinglon) {
    if (hit.armor != 255) {
      hit.armor -= damage;
      setup_explosion(state.explosions.slots, shot_x, shot_y, 0, 0, false, false);
    }
    else {
      uint16_t const num = damage / 2 + 3;
      do_sp(state.superpixels, state.random.mt, shot_x + 6, shot_y + 6, num, damage / 4 + 2, filter);
      // JE_doSP counts in the global temp, which holds the link here
      link = static_cast<uint8_t>(num);
    }
  }

  state.sound.queue[5] = S_ENEMY_HIT;

  if (!((armor_before - damage <= hit.damaged_armor) && ((!hit.damaged) ^ (hit.damaged_frames < 0)))) {
    return;
  }

  for (int i = 0; i < 100; i++) {
    if (status[i] == 1) {
      continue;
    }

    Enemy&     e = slots[i];
    int const  linknum = e.link_num;
    bool const linked = link != 255 && ((e.damaged_armor > 0 && linknum == link) ||
                                        (state.level.enemy_continual_damage && link - 100 == linknum) ||
                                        (linknum > 40 && linknum / 20 == link / 20 && linknum <= link));
    if (i != b && !linked) {
      continue;
    }

    e.frame = 1;

    e.damaged = !e.damaged;

    if (e.damaged_frames != 0) {
      e.last_frame = abs(e.damaged_frames);
      e.animation = 1;
      e.stop_frame = 0;
      e.first_frame = e.damaged_sprite;
      e.frame = e.first_frame - 1;
    }
    else if (e.damaged_sprite > 0) {
      e.sprites[1 - 1] = e.damaged_sprite;
      e.last_frame = 1;
      e.animation = 0;
      e.stop_frame = 0;
      e.first_frame = 1;
    }
    else {
      status[i] = 1;
      state.enemies.killed++;
    }

    e.animation_at_stop = 0;

    if (e.armor > (unsigned char)e.damaged_armor) {
      e.armor = e.damaged_armor;
    }

    int16_t const x = e.x + e.map_offset;
    int16_t const y = e.y;

    if (enemyDat[e.type].esize != 1) {
      setup_explosion(state.explosions.slots, x, y - 6, 0, 1, false, false);
    }
    else {
      setup_explosion_large(state.explosions, e.ground, e.explosion / 2, x, y);
    }
  }
}

// A hit that takes all the enemy's (slot b's) armor: the enemies destroyed
// with it, linked to it by link, and what they leave.
void destroy_enemy(State& state, int b, uint8_t link, uint8_t player_num) {
  auto& slots = state.enemies.slots;
  auto& status = state.enemies.status;
  Game& game = state.game;

  if ((link == 254) && (state.level.super_enemy_254_jump > 0)) {
    event_jump(state.level, state.level.super_enemy_254_jump);
  }

  for (int i = 0; i < 100; i++) {
    if (status[i] == 1) {
      continue;
    }

    Enemy&        e = slots[i];
    uint8_t const linknum = e.link_num;
    bool const    linked =
        link == 254 || (link != 255 && (link == linknum || link - 100 == linknum ||
                                        (linknum > 40 && linknum / 20 == link / 20 && linknum <= link)));
    if (i != b && !linked) {
      continue;
    }

    int enemy_screen_x = e.x + e.map_offset;

    if (e.sets_flag) {
      assert((unsigned int)e.flag_num - 1 < std::size(state.level.global_flags));
      state.level.global_flags[e.flag_num - 1] = e.flag_value;
    }

    if ((e.dies_into > 0) && !((game.super_arcade_mode != SA_NONE) && (enemyDat[e.dies_into].value == 30000)))
    {
      uint16_t const die = e.dies_into;
      int            first = i - (i % 25);
      if (enemyDat[die].value > 30000) {
        first = 0;
      }
      int const made = new_enemy(state, first, die, 0);
      if (made != 0) {
        Enemy& m = slots[made - 1];
        if ((game.super_arcade_mode != SA_NONE) && (m.value > 30000)) {
          game.super_arcade_power_up++;
          if (game.super_arcade_power_up > 5) {
            game.super_arcade_power_up = 1;
          }
          m.sprites[1 - 1] = 5 + game.super_arcade_power_up * 2;
          m.value = 30000 + game.super_arcade_power_up;
        }

        if (m.value != 0) {
          m.score_item = true;
        }
        else {
          m.score_item = false;
        }

        m.x = e.x;
        m.y = e.y;
      }
    }

    if ((e.value > 0) && (e.value < 10000)) {
      if (e.value == 1) {
        state.progress.cube_max++;
      }
      else {
        // in galaga mode player 2 is sidekick, so give cash to player 1
        state.players.slots[game.galaga_mode ? 0 : player_num - 1].cash += e.value;
      }
    }

    if ((e.damaged_armor == -1) && (link == linknum)) {
      e.damaged_armor = 0;
      status[i] = 2;
      e.sprites[1 - 1] = e.damaged_sprite;
      e.last_frame = 1;
      e.animation = 0;
      e.stop_frame = 0;
      e.first_frame = 1;
      e.damaged = true;
      e.frame = 1;
    }
    else {
      status[i] = 1;
      state.enemies.killed++;
    }

    if (enemyDat[e.type].esize == 1) {
      setup_explosion_large(state.explosions, e.ground, e.explosion, enemy_screen_x, e.y);
      state.sound.queue[6] = S_EXPLOSION_9;
    }
    else {
      setup_explosion(state.explosions.slots, enemy_screen_x, e.y, 0, 1, false, false);
      state.sound.queue[6] = S_EXPLOSION_8;
    }
  }
}

}  // namespace

std::vector<VisibleEnemy> update_enemies(State& state, int end, uint16_t map_x_ofs, uint16_t back_move) {
  state.map.temp_map_x_ofs = map_x_ofs;
  state.map.temp_back_move = back_move;

  Layer const layer{end, map_x_ofs, back_move};
  // JE_drawEnemy moves player 1 25 pixels to the left while it runs.
  int const player_x = state.players.slots[0].x - 25;

  std::vector<VisibleEnemy> visible;
  for (int i = end - 25; i < end; i++) {
    if (state.enemies.status[i] != 1) {
      update_enemy(state, i, player_x, layer, visible);
    }
  }
  return visible;
}

int new_enemy(State& state, int first, uint16_t e_dat_i, int16_t unique_shape_table_i) {
  for (int i = first; i < first + 25; ++i) {
    if (state.enemies.status[i] == 1) {
      state.enemies.status[i] = make_enemy(state, state.enemies.slots[i], e_dat_i, unique_shape_table_i);
      return i + 1;
    }
  }

  return 0;
}

uint8_t make_enemy(State& state, Enemy& enemy, uint16_t e_dat_i, int16_t unique_shape_table_i) {
  int8_t const difficulty = state.game.difficulty_level;
  MtState&     mt = state.random.mt;
  uint8_t      avail;

  if (state.game.super_arcade_mode != SA_NONE && e_dat_i == 534) {
    e_dat_i = 533;
  }

  auto const& dat = enemyDat[e_dat_i];

  uint8_t const shape_table_i = unique_shape_table_i > 0 ? unique_shape_table_i : dat.shapebank;

  Sprite2_array *sheet = nullptr;
  if (shape_table_i == 21) {
    sheet = &spriteSheet11;  // Coins&Gems
  }
  else if (shape_table_i == 26) {
    sheet = &spriteSheet10;  // Two-Player Stuff
  }
  else {
    for (size_t i = 0; i < std::size(enemySpriteSheetIds); ++i) {
      if (shape_table_i == enemySpriteSheetIds[i]) {
        sheet = &enemySpriteSheets[i];
      }
    }
  }

  if (sheet != nullptr) {
    enemy.sprite_sheet = sheet;
  }
  else {
    // Use shape table value from previous enemy that occupied the enemy slot. (Ex. APPROACH.)
    logWarn("Ignoring sprite from unloaded shape table %d.", shape_table_i);
  }

  if (enemy.sprite_sheet == nullptr) {
    logWarn("Enemy sprite is missing.");
  }

  enemy.dat = &enemyDat[e_dat_i];

  enemy.map_offset = 0;

  for (unsigned i = 0; i < 3; ++i) {
    enemy.shot_pos[i] = 0;
  }

  enemy.ground = (dat.explosiontype & 1) == 0;
  enemy.explosion = dat.explosiontype >> 1;

  enemy.launch_freq = dat.elaunchfreq;
  enemy.launch_wait = dat.elaunchfreq;
  enemy.launch_type = dat.elaunchtype % 1000;
  enemy.launch_special = dat.elaunchtype / 1000;

  enemy.x_homing = dat.xaccel;
  enemy.y_homing = dat.yaccel;

  enemy.x_min_bounce = -10000;
  enemy.x_max_bounce = 10000;
  enemy.y_min_bounce = -10000;
  enemy.y_max_bounce = 10000;
  // Far enough away to be impossible to reach

  for (unsigned i = 0; i < 3; ++i) {
    enemy.turrets[i] = dat.tur[i];
  }

  enemy.last_frame = dat.ani;
  enemy.first_frame = 1;

  switch (dat.animate) {
    case 0:
      enemy.frame = 1;
      enemy.animation = 0;
      enemy.stop_frame = 0;
      enemy.animation_at_stop = 0;
      break;
    case 1:
      enemy.frame = 0;
      enemy.animation = 1;
      enemy.stop_frame = 0;
      enemy.animation_at_stop = 0;
      break;
    case 2:
      enemy.frame = 1;
      enemy.animation = 2;
      enemy.stop_frame = enemy.last_frame;
      enemy.animation_at_stop = 2;
      break;
  }

  if (dat.startxc != 0) {
    enemy.x = dat.startx + (mt_rand_r(&mt) % (dat.startxc * 2)) - dat.startxc + 1;
  }
  else {
    enemy.x = dat.startx + 1;
  }

  if (dat.startyc != 0) {
    enemy.y = dat.starty + (mt_rand_r(&mt) % (dat.startyc * 2)) - dat.startyc + 1;
  }
  else {
    enemy.y = dat.starty + 1;
  }

  enemy.x_speed = dat.xmove;
  enemy.y_speed = dat.ymove;
  enemy.x_accel = dat.xcaccel;
  enemy.y_accel = dat.ycaccel;
  enemy.x_accel_wait = abs(enemy.x_accel);
  enemy.x_accel_wait_max = enemy.x_accel_wait;
  enemy.y_accel_wait = abs(enemy.y_accel);
  enemy.y_accel_wait_max = enemy.y_accel_wait;
  enemy.x_accel_step = (enemy.x_accel > 0) ? 1 : -1;
  enemy.y_accel_step = (enemy.y_accel > 0) ? 1 : -1;
  enemy.sets_flag = false;
  enemy.iced = 0;

  if (dat.xrev == 0) {
    enemy.x_reverse_speed = 100;
  }
  else if (dat.xrev == -99) {
    enemy.x_reverse_speed = 0;
  }
  else {
    enemy.x_reverse_speed = dat.xrev;
  }

  if (dat.yrev == 0) {
    enemy.y_reverse_speed = 100;
  }
  else if (dat.yrev == -99) {
    enemy.y_reverse_speed = 0;
  }
  else {
    enemy.y_reverse_speed = dat.yrev;
  }

  enemy.x_homing_dir = (enemy.x_homing > 0) ? 1 : -1;
  enemy.y_homing_dir = (enemy.y_homing > 0) ? 1 : -1;

  enemy.type = e_dat_i;

  for (unsigned i = 0; i < 3; ++i) {
    if (enemy.turrets[i] == 252) {
      enemy.shot_wait[i] = 1;
    }
    else if (enemy.turrets[i] > 0) {
      enemy.shot_wait[i] = 20;
    }
    else {
      enemy.shot_wait[i] = 255;
    }
  }
  for (unsigned i = 0; i < 20; ++i) {
    enemy.sprites[i] = dat.egraphic[i];
  }
  enemy.size = dat.esize;
  enemy.link_num = 0;
  enemy.damaged = dat.dani < 0;
  enemy.dies_into = dat.eenemydie;

  enemy.shot_freq[1 - 1] = dat.freq[1 - 1];
  enemy.shot_freq[2 - 1] = dat.freq[2 - 1];
  enemy.shot_freq[3 - 1] = dat.freq[3 - 1];

  enemy.damaged_frames = dat.dani;
  enemy.damaged_sprite = dat.dgr;
  enemy.damaged_armor = dat.dlevel;

  enemy.fixed_move_y = 0;

  enemy.filter = 0x00;

  int value = 0;
  if (dat.value > 1 && dat.value < 10000) {
    switch (difficulty) {
      case -1:
      case DIFFICULTY_WIMP:
        value = dat.value * 0.75f;
        break;
      case DIFFICULTY_EASY:
      case DIFFICULTY_NORMAL:
        value = dat.value;
        break;
      case DIFFICULTY_HARD:
        value = dat.value * 1.125f;
        break;
      case DIFFICULTY_IMPOSSIBLE:
        value = dat.value * 1.5f;
        break;
      case DIFFICULTY_INSANITY:
        value = dat.value * 2;
        break;
      case DIFFICULTY_SUICIDE:
        value = dat.value * 2.5f;
        break;
      case DIFFICULTY_MANIACAL:
      case DIFFICULTY_ZINGLON:
        value = dat.value * 4;
        break;
      case DIFFICULTY_NORTANEOUS:
      case DIFFICULTY_10:
        value = dat.value * 8;
        break;
    }
    if (value > 10000) {
      value = 10000;
    }
    enemy.value = value;
  }
  else {
    enemy.value = dat.value;
  }

  int armor = 1;
  if (dat.armor > 0) {
    if (dat.armor != 255) {
      switch (difficulty) {
        case -1:
        case DIFFICULTY_WIMP:
          armor = dat.armor * 0.5f + 1;
          break;
        case DIFFICULTY_EASY:
          armor = dat.armor * 0.75f + 1;
          break;
        case DIFFICULTY_NORMAL:
          armor = dat.armor;
          break;
        case DIFFICULTY_HARD:
          armor = dat.armor * 1.2f;
          break;
        case DIFFICULTY_IMPOSSIBLE:
          armor = dat.armor * 1.5f;
          break;
        case DIFFICULTY_INSANITY:
          armor = dat.armor * 1.8f;
          break;
        case DIFFICULTY_SUICIDE:
          armor = dat.armor * 2;
          break;
        case DIFFICULTY_MANIACAL:
          armor = dat.armor * 3;
          break;
        case DIFFICULTY_ZINGLON:
          armor = dat.armor * 4;
          break;
        case DIFFICULTY_NORTANEOUS:
        case DIFFICULTY_10:
          armor = dat.armor * 8;
          break;
      }

      if (armor > 254) {
        armor = 254;
      }
    }
    else {
      armor = 255;
    }

    enemy.armor = armor;

    avail = 0;
    enemy.score_item = false;
  }
  else {
    avail = 2;
    enemy.armor = 255;
    if (enemy.value != 0) {
      enemy.score_item = true;
    }
  }

  if (!enemy.score_item) {
    state.enemies.total++;  // Destruction ratio
  }

  // indicates what to set ENEMYAVAIL to
  return avail;
}

int hit_enemy(State& state, int b, int16_t& damage, uint8_t& filter, bool zinglon, int shot_x, int shot_y,
              uint8_t player_num) {
  auto&        slots = state.enemies.slots;
  PlayerShots& shots = state.player_shots;

  Enemy& hit = slots[b];

  shots.infinite_shot = false;

  if (damage == 99) {
    damage = 0;
    shots.do_iced = 40;
    hit.iced = 40;
  }
  else {
    shots.do_iced = 0;
    if (damage >= 250) {
      damage = damage - 250;
      shots.infinite_shot = true;
    }
  }

  int const armor_before = hit.armor;

  uint8_t link = hit.link_num;
  if (link == 0) {
    link = 255;
  }

  if (hit.armor < 255) {
    for (BossBar& bar : state.display.boss_bars) {
      if (link == bar.link_num) {
        bar.color = 6;
      }
    }

    if (hit.ground) {
      hit.filter = filter;
    }

    for (size_t e = 0; e < std::size(slots); e++) {
      if (!(slots[e].link_num == link && state.enemies.status[e] != 1 && slots[e].ground != 0)) {
        continue;
      }

      if (shots.do_iced) {
        slots[e].iced = shots.do_iced;
      }
      slots[e].filter = filter;
    }
  }

  if (armor_before > damage) {
    damage_enemy(state, b, armor_before, damage, filter, link, zinglon, shot_x, shot_y);
  }
  else {
    destroy_enemy(state, b, link, player_num);
    // The C code counts the enemies in temp2, which holds the shot's blast
    // filter, so the shot's later hits in the tick have the filter 100.
    filter = 100;
  }

  if (shots.infinite_shot) {
    damage += 250;
  }
  return armor_before;
}

}  // namespace tyrx::sim
