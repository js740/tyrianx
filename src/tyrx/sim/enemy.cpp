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

// The enemy's random acceleration on one axis (xaccel, yaccel): if accel is
// set, its speed changes by one toward the player's position, at random and up
// to accel - 89 either way.
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

// The enemy's fixed acceleration on one axis (excc, eycc): every ccw_max
// ticks, its speed changes by ccadd, and the acceleration reverses when the
// speed reaches rev.
void accelerate(int8_t& speed, int8_t& cc, int8_t& ccw, uint8_t ccw_max, int8_t& rev, int16_t& ccadd) {
  if (!cc || --ccw > 0) {
    return;
  }

  auto const reverse = [&] {
    cc = -cc;
    rev = -rev;
    ccadd = -ccadd;
  };

  if (speed == rev) {
    reverse();
    return;
  }

  speed += ccadd;
  ccw = ccw_max;
  if (speed == rev) {
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
  int end;             // the enemies are the 25 before this slot
  uint16_t map_x_ofs;  // how far the layer has scrolled sideways
  uint16_t back_move;  // how far it moved down in the tick
};

// The enemy's turret j (1 to 3), at (x, y), fires its weapon's shots.
// Returns false if there is no free slot for one, which ends the enemy's
// update.
bool fire_shots(State& state, JE_SingleEnemyType& e, int j, uint8_t weapon_id, int16_t x, int16_t y,
                Layer const& layer) {
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

    if (e.aniactive == 2) {
      e.aniactive = 1;
    }

    if (++e.eshotmultipos[j - 1] > weapon.max) {
      e.eshotmultipos[j - 1] = 1;
    }

    int const pos = e.eshotmultipos[j - 1] - 1;

    if (j == 1) {
      state.scratch.temp2 = 4;
    }

    EnemyShotType& shot = shots.slots[b];
    shot.sx = x + weapon.bx[pos] + layer.map_x_ofs;
    shot.sy = y + weapon.by[pos];
    shot.sdmg = weapon.attack[pos];
    shot.tx = weapon.tx;
    shot.ty = weapon.ty;
    shot.duration = weapon.del[pos];
    shot.animate = 0;
    shot.animax = weapon.weapani;

    shot.sgr = weapon.sg[pos];
    switch (j) {
      case 1:
        shot.syc = weapon.acceleration;
        shot.sxc = weapon.accelerationx;

        shot.sxm = weapon.sx[pos];
        shot.sym = weapon.sy[pos];
        break;
      case 3:
        shot.sxc = -weapon.acceleration;
        shot.syc = weapon.accelerationx;

        shot.sxm = -weapon.sy[pos];
        shot.sym = -weapon.sx[pos];
        break;
      case 2:
        shot.sxc = weapon.acceleration;
        shot.syc = -weapon.acceleration;

        shot.sxm = weapon.sy[pos];
        shot.sym = -weapon.sx[pos];
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
    shot.sxm = roundf((float)aim_x / max_mag_aim * aim);
    shot.sym = roundf((float)aim_y / max_mag_aim * aim);
  }
  return true;
}

// The enemy's turret j (1 to 3), at (x, y), fires if it is due.  player_x is
// player 1's position 25 pixels to the left.  Returns false if that ends the
// enemy's update.
bool fire(State& state, JE_SingleEnemyType& e, int j, int16_t x, int16_t y, int player_x,
          Layer const& layer) {
  auto& players = state.players.slots;
  int8_t const difficulty = state.game.difficulty_level;

  if (!e.freq[j - 1]) {
    return true;
  }

  uint8_t const weapon_id = e.tur[j - 1];

  if (--e.eshotwait[j - 1] != 0 || !weapon_id) {
    return true;
  }

  e.eshotwait[j - 1] = e.freq[j - 1];
  if (difficulty > DIFFICULTY_NORMAL) {
    e.eshotwait[j - 1] = (e.eshotwait[j - 1] / 2) + 1;
    if (difficulty > DIFFICULTY_MANIACAL) {
      e.eshotwait[j - 1] = (e.eshotwait[j - 1] / 2) + 1;
    }
  }

  if (state.game.galaga_mode &&
      (e.eyc == 0 || (mt_rand_r(&state.random.mt) % 400) >= state.level.galaga_shot_freq))
  {
    return false;
  }

  switch (weapon_id) {
    case 252:  // Savara Boss DualMissile
      if (e.ey > 20) {
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
void launch(State& state, JE_SingleEnemyType& e, int16_t x, int16_t y, Layer const& layer) {
  auto& players = state.players.slots;
  MtState& mt = state.random.mt;

  if (!e.launchfreq || --e.launchwait != 0) {
    return;
  }

  // Enemy Launch Routine
  e.launchwait = e.launchfreq;

  // Type  1 : Must be inline with player
  if (e.launchspecial != 0 && abs(e.ey - players[0].y) > 5) {
    return;
  }

  if (e.aniactive == 2) {
    e.aniactive = 1;
  }

  if (e.launchtype == 0) {
    return;
  }

  int const b = new_enemy(state, layer.end == 50 ? 75 : layer.end - 25, e.launchtype, 0);

  if (b <= 0) {
    return;
  }

  // Launch Enemy Placement
  JE_SingleEnemyType& launched = state.enemies.slots[b - 1];

  launched.ex = x;
  launched.ey = y + enemyDat[launched.enemytype].startyc;
  if (launched.size == 0) {
    launched.ey -= 7;
  }

  if (launched.launchtype > 0 && launched.launchfreq == 0) {
    if (launched.launchtype > 90) {
      launched.ex += mt_rand_r(&mt) % ((launched.launchtype - 90) * 4) - (launched.launchtype - 90) * 2;
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
      launched.exc = roundf((float)aim_x / max_mag_aim * launched.launchtype);
      launched.eyc = roundf((float)aim_y / max_mag_aim * launched.launchtype);
    }
  }

  unsigned long const channel = random_channel(mt);
  state.sound.queue[channel] = randomEnemyLaunchSounds[(mt_rand_r(&mt) % 3)];

  if (e.launchspecial == 1 && e.linknum < 100) {
    launched.linknum = e.linknum;
  }
}

// One enemy's update: it is drawn (added to visible) if on screen, then
// moves, fires and launches enemies.  player_x is player 1's position 25
// pixels to the left.
void update_enemy(State& state, int i, int player_x, Layer const& layer, std::vector<VisibleEnemy>& visible) {
  auto& players = state.players.slots;
  JE_SingleEnemyType& e = state.enemies.slots[i];
  MtState& mt = state.random.mt;

  e.mapoffset = layer.map_x_ofs;

  home_in(mt, e.xaccel, player_x, e.ex, e.exc);
  home_in(mt, e.yaccel, players[0].y, e.ey, e.eyc);

  auto const gone = [&] {
    // enemy[i].egr[10] &= 0x00ff; <MXD> madness?
    state.enemies.status[i] = 1;
  };

  if (e.ex + layer.map_x_ofs > -29 && e.ex + layer.map_x_ofs < 300) {
    if (e.aniactive == 1) {
      e.enemycycle++;

      if (e.enemycycle == e.animax) {
        e.aniactive = e.aniwhenfire;
      }
      else if (e.enemycycle > e.ani) {
        e.enemycycle = e.animin;
      }
    }

    if (e.egr[e.enemycycle - 1] == 999) {
      return gone();
    }

    visible.push_back({e.ex, e.ey, e.egr[e.enemycycle - 1], e.filter, e.size, e.sprite2s});

    e.filter = 0;
  }

  accelerate(e.exc, e.excc, e.exccw, e.exccwmax, e.exrev, e.exccadd);
  accelerate(e.eyc, e.eycc, e.eyccw, e.eyccwmax, e.eyrev, e.eyccadd);

  e.ey += e.fixedmovey;

  e.ex += e.exc;
  if (e.ex < -80 || e.ex > 340) {
    return gone();
  }

  e.ey += e.eyc;
  if (e.ey < -112 || e.ey > 190) {
    return gone();
  }

  // X bounce
  if (e.ex <= e.xminbounce || e.ex >= e.xmaxbounce) {
    e.exc = -e.exc;
  }

  // Y bounce
  if (e.ey <= e.yminbounce || e.ey >= e.ymaxbounce) {
    e.eyc = -e.eyc;
  }

  // Evalue != 0 - score item at boundary
  if (e.scoreitem) {
    if (e.ex < -5) {
      e.ex++;
    }
    if (e.ex > 245) {
      e.ex--;
    }
  }

  e.ey += layer.back_move;

  if (e.ex <= -24 || e.ex >= 296) {
    return;
  }

  int16_t const x = e.ex;
  int16_t const y = e.ey;

  // Enemy Shots
  if (e.edamaged == 1) {
    return;
  }

  state.enemies.on_screen++;

  if (e.iced) {
    e.iced--;
    if (e.enemyground != 0) {
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

// A hit that leaves the enemy (slot b, with armorleft armor before the hit)
// some armor: the damage, and the enemies that the damage changes, linked to
// it by link.
void damage_enemy(State& state, int b, int armorleft, int16_t damage, uint8_t filter, uint8_t link,
                  bool zinglon, int shot_x, int shot_y) {
  auto& slots = state.enemies.slots;
  auto& status = state.enemies.status;
  JE_SingleEnemyType& hit = slots[b];

  if (!zinglon) {
    if (hit.armorleft != 255) {
      hit.armorleft -= damage;
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

  if (!((armorleft - damage <= hit.edlevel) && ((!hit.edamaged) ^ (hit.edani < 0)))) {
    return;
  }

  for (int i = 0; i < 100; i++) {
    if (status[i] == 1) {
      continue;
    }

    JE_SingleEnemyType& e = slots[i];
    int const linknum = e.linknum;
    bool const linked = link != 255 && ((e.edlevel > 0 && linknum == link) ||
                                        (state.level.enemy_continual_damage && link - 100 == linknum) ||
                                        (linknum > 40 && linknum / 20 == link / 20 && linknum <= link));
    if (i != b && !linked) {
      continue;
    }

    e.enemycycle = 1;

    e.edamaged = !e.edamaged;

    if (e.edani != 0) {
      e.ani = abs(e.edani);
      e.aniactive = 1;
      e.animax = 0;
      e.animin = e.edgr;
      e.enemycycle = e.animin - 1;
    }
    else if (e.edgr > 0) {
      e.egr[1 - 1] = e.edgr;
      e.ani = 1;
      e.aniactive = 0;
      e.animax = 0;
      e.animin = 1;
    }
    else {
      status[i] = 1;
      state.enemies.killed++;
    }

    e.aniwhenfire = 0;

    if (e.armorleft > (unsigned char)e.edlevel) {
      e.armorleft = e.edlevel;
    }

    int16_t const x = e.ex + e.mapoffset;
    int16_t const y = e.ey;

    if (enemyDat[e.enemytype].esize != 1) {
      setup_explosion(state.explosions.slots, x, y - 6, 0, 1, false, false);
    }
    else {
      setup_explosion_large(state.explosions, e.enemyground, e.explonum / 2, x, y);
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

    JE_SingleEnemyType& e = slots[i];
    uint8_t const linknum = e.linknum;
    bool const linked =
        link == 254 || (link != 255 && (link == linknum || link - 100 == linknum ||
                                        (linknum > 40 && linknum / 20 == link / 20 && linknum <= link)));
    if (i != b && !linked) {
      continue;
    }

    int enemy_screen_x = e.ex + e.mapoffset;

    if (e.special) {
      assert((unsigned int)e.flagnum - 1 < std::size(state.cheats.global_flags));
      state.cheats.global_flags[e.flagnum - 1] = e.setto;
    }

    if ((e.enemydie > 0) && !((game.super_arcade_mode != SA_NONE) && (enemyDat[e.enemydie].value == 30000))) {
      uint16_t const die = e.enemydie;
      int first = i - (i % 25);
      if (enemyDat[die].value > 30000) {
        first = 0;
      }
      int const made = new_enemy(state, first, die, 0);
      if (made != 0) {
        JE_SingleEnemyType& m = slots[made - 1];
        if ((game.super_arcade_mode != SA_NONE) && (m.evalue > 30000)) {
          game.super_arcade_power_up++;
          if (game.super_arcade_power_up > 5) {
            game.super_arcade_power_up = 1;
          }
          m.egr[1 - 1] = 5 + game.super_arcade_power_up * 2;
          m.evalue = 30000 + game.super_arcade_power_up;
        }

        if (m.evalue != 0) {
          m.scoreitem = true;
        }
        else {
          m.scoreitem = false;
        }

        m.ex = e.ex;
        m.ey = e.ey;
      }
    }

    if ((e.evalue > 0) && (e.evalue < 10000)) {
      if (e.evalue == 1) {
        state.progress.cube_max++;
      }
      else {
        // in galaga mode player 2 is sidekick, so give cash to player 1
        state.players.slots[game.galaga_mode ? 0 : player_num - 1].cash += e.evalue;
      }
    }

    if ((e.edlevel == -1) && (link == linknum)) {
      e.edlevel = 0;
      status[i] = 2;
      e.egr[1 - 1] = e.edgr;
      e.ani = 1;
      e.aniactive = 0;
      e.animax = 0;
      e.animin = 1;
      e.edamaged = true;
      e.enemycycle = 1;
    }
    else {
      status[i] = 1;
      state.enemies.killed++;
    }

    if (enemyDat[e.enemytype].esize == 1) {
      setup_explosion_large(state.explosions, e.enemyground, e.explonum, enemy_screen_x, e.ey);
      state.sound.queue[6] = S_EXPLOSION_9;
    }
    else {
      setup_explosion(state.explosions.slots, enemy_screen_x, e.ey, 0, 1, false, false);
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

uint8_t make_enemy(State& state, JE_SingleEnemyType& enemy, uint16_t e_dat_i, int16_t unique_shape_table_i) {
  int8_t const difficulty = state.game.difficulty_level;
  MtState& mt = state.random.mt;
  uint8_t avail;

  if (state.game.super_arcade_mode != SA_NONE && e_dat_i == 534) {
    e_dat_i = 533;
  }

  auto const& dat = enemyDat[e_dat_i];

  uint8_t const shape_table_i = unique_shape_table_i > 0 ? unique_shape_table_i : dat.shapebank;

  Sprite2_array *sprite2s = nullptr;
  if (shape_table_i == 21) {
    sprite2s = &spriteSheet11;  // Coins&Gems
  }
  else if (shape_table_i == 26) {
    sprite2s = &spriteSheet10;  // Two-Player Stuff
  }
  else {
    for (size_t i = 0; i < std::size(enemySpriteSheetIds); ++i) {
      if (shape_table_i == enemySpriteSheetIds[i]) {
        sprite2s = &enemySpriteSheets[i];
      }
    }
  }

  if (sprite2s != nullptr) {
    enemy.sprite2s = sprite2s;
  }
  else {
    // Use shape table value from previous enemy that occupied the enemy slot. (Ex. APPROACH.)
    logWarn("Ignoring sprite from unloaded shape table %d.", shape_table_i);
  }

  if (enemy.sprite2s == nullptr) {
    logWarn("Enemy sprite is missing.");
  }

  enemy.enemydatofs = &enemyDat[e_dat_i];

  enemy.mapoffset = 0;

  for (unsigned i = 0; i < 3; ++i) {
    enemy.eshotmultipos[i] = 0;
  }

  enemy.enemyground = (dat.explosiontype & 1) == 0;
  enemy.explonum = dat.explosiontype >> 1;

  enemy.launchfreq = dat.elaunchfreq;
  enemy.launchwait = dat.elaunchfreq;
  enemy.launchtype = dat.elaunchtype % 1000;
  enemy.launchspecial = dat.elaunchtype / 1000;

  enemy.xaccel = dat.xaccel;
  enemy.yaccel = dat.yaccel;

  enemy.xminbounce = -10000;
  enemy.xmaxbounce = 10000;
  enemy.yminbounce = -10000;
  enemy.ymaxbounce = 10000;
  // Far enough away to be impossible to reach

  for (unsigned i = 0; i < 3; ++i) {
    enemy.tur[i] = dat.tur[i];
  }

  enemy.ani = dat.ani;
  enemy.animin = 1;

  switch (dat.animate) {
    case 0:
      enemy.enemycycle = 1;
      enemy.aniactive = 0;
      enemy.animax = 0;
      enemy.aniwhenfire = 0;
      break;
    case 1:
      enemy.enemycycle = 0;
      enemy.aniactive = 1;
      enemy.animax = 0;
      enemy.aniwhenfire = 0;
      break;
    case 2:
      enemy.enemycycle = 1;
      enemy.aniactive = 2;
      enemy.animax = enemy.ani;
      enemy.aniwhenfire = 2;
      break;
  }

  if (dat.startxc != 0) {
    enemy.ex = dat.startx + (mt_rand_r(&mt) % (dat.startxc * 2)) - dat.startxc + 1;
  }
  else {
    enemy.ex = dat.startx + 1;
  }

  if (dat.startyc != 0) {
    enemy.ey = dat.starty + (mt_rand_r(&mt) % (dat.startyc * 2)) - dat.startyc + 1;
  }
  else {
    enemy.ey = dat.starty + 1;
  }

  enemy.exc = dat.xmove;
  enemy.eyc = dat.ymove;
  enemy.excc = dat.xcaccel;
  enemy.eycc = dat.ycaccel;
  enemy.exccw = abs(enemy.excc);
  enemy.exccwmax = enemy.exccw;
  enemy.eyccw = abs(enemy.eycc);
  enemy.eyccwmax = enemy.eyccw;
  enemy.exccadd = (enemy.excc > 0) ? 1 : -1;
  enemy.eyccadd = (enemy.eycc > 0) ? 1 : -1;
  enemy.special = false;
  enemy.iced = 0;

  if (dat.xrev == 0) {
    enemy.exrev = 100;
  }
  else if (dat.xrev == -99) {
    enemy.exrev = 0;
  }
  else {
    enemy.exrev = dat.xrev;
  }

  if (dat.yrev == 0) {
    enemy.eyrev = 100;
  }
  else if (dat.yrev == -99) {
    enemy.eyrev = 0;
  }
  else {
    enemy.eyrev = dat.yrev;
  }

  enemy.exca = (enemy.xaccel > 0) ? 1 : -1;
  enemy.eyca = (enemy.yaccel > 0) ? 1 : -1;

  enemy.enemytype = e_dat_i;

  for (unsigned i = 0; i < 3; ++i) {
    if (enemy.tur[i] == 252) {
      enemy.eshotwait[i] = 1;
    }
    else if (enemy.tur[i] > 0) {
      enemy.eshotwait[i] = 20;
    }
    else {
      enemy.eshotwait[i] = 255;
    }
  }
  for (unsigned i = 0; i < 20; ++i) {
    enemy.egr[i] = dat.egraphic[i];
  }
  enemy.size = dat.esize;
  enemy.linknum = 0;
  enemy.edamaged = dat.dani < 0;
  enemy.enemydie = dat.eenemydie;

  enemy.freq[1 - 1] = dat.freq[1 - 1];
  enemy.freq[2 - 1] = dat.freq[2 - 1];
  enemy.freq[3 - 1] = dat.freq[3 - 1];

  enemy.edani = dat.dani;
  enemy.edgr = dat.dgr;
  enemy.edlevel = dat.dlevel;

  enemy.fixedmovey = 0;

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
    enemy.evalue = value;
  }
  else {
    enemy.evalue = dat.value;
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

    enemy.armorleft = armor;

    avail = 0;
    enemy.scoreitem = false;
  }
  else {
    avail = 2;
    enemy.armorleft = 255;
    if (enemy.evalue != 0) {
      enemy.scoreitem = true;
    }
  }

  if (!enemy.scoreitem) {
    state.enemies.total++;  // Destruction ratio
  }

  // indicates what to set ENEMYAVAIL to
  return avail;
}

int hit_enemy(State& state, int b, int16_t& damage, uint8_t& filter, bool zinglon, int shot_x, int shot_y,
              uint8_t player_num) {
  auto& slots = state.enemies.slots;
  Cheats& cheats = state.cheats;
  JE_SingleEnemyType& hit = slots[b];

  cheats.infinite_shot = false;

  if (damage == 99) {
    damage = 0;
    cheats.do_iced = 40;
    hit.iced = 40;
  }
  else {
    cheats.do_iced = 0;
    if (damage >= 250) {
      damage = damage - 250;
      cheats.infinite_shot = true;
    }
  }

  int const armorleft = hit.armorleft;

  uint8_t link = hit.linknum;
  if (link == 0) {
    link = 255;
  }

  if (hit.armorleft < 255) {
    for (boss_bar_t& bar : state.display.boss_bars) {
      if (link == bar.link_num) {
        bar.color = 6;
      }
    }

    if (hit.enemyground) {
      hit.filter = filter;
    }

    for (size_t e = 0; e < std::size(slots); e++) {
      if (!(slots[e].linknum == link && state.enemies.status[e] != 1 && slots[e].enemyground != 0)) {
        continue;
      }

      if (cheats.do_iced) {
        slots[e].iced = cheats.do_iced;
      }
      slots[e].filter = filter;
    }
  }

  if (armorleft > damage) {
    damage_enemy(state, b, armorleft, damage, filter, link, zinglon, shot_x, shot_y);
  }
  else {
    destroy_enemy(state, b, link, player_num);
    // The C code counts the enemies in temp2, which holds the shot's blast
    // filter, so the shot's later hits in the tick have the filter 100.
    filter = 100;
  }

  if (cheats.infinite_shot) {
    damage += 250;
  }
  return armorleft;
}

}  // namespace tyrx::sim
