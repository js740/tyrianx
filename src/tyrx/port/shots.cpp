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

// The player shots (player_shot_move_and_draw, player_shot_create and
// player_shot_set_direction in shots.c).  A shot that hits can be freed, and
// its slot taken by a chain-reaction shot later in the same loop, so the draw
// record gets each drawn shot's position and frame, and whether it has a
// shadow.

#include "draw.hpp"
#include "port.hpp"

#include <cstdlib>

namespace tyrx::port {

namespace {

// shots.c
JE_word const linkMultiGr[17] = {77,  221, 183, 301, 1,   282, 164, 202, 58,  // [0..16]
                                 201, 163, 281, 39,  300, 182, 220, 77};
JE_word const linkSonicGr[17] = {85,  242, 131, 303, 47,  284, 150, 223, 66,  // [0..16]
                                 224, 149, 283, 9,   302, 130, 243, 85};
JE_word const linkMult2Gr[17] = {78,  299, 295, 297, 2,   278, 276, 280, 59,  // [0..16]
                                 279, 275, 277, 40,  296, 294, 298, 78};

}  // namespace

// Port of player_shot_move_and_draw (shots.c), which adds the shot to `drawn`
// instead of drawing it.
bool player_shot_move(int shot_id, bool *out_is_special, int *out_shotx, int *out_shoty,
                      JE_integer *out_shot_damage, JE_byte *out_blast_filter, JE_byte *out_chain,
                      JE_byte *out_playerNum, JE_word *out_special_radiusw, JE_word *out_special_radiush,
                      std::vector<PlayerShotDrawn>& drawn) {
  auto& enemy = state().enemies.slots;
  auto& enemyAvail = state().enemies.status;
  auto& playerShotData = state().player_shots.slots;
  auto& shotAvail = state().player_shots.ttl;
  PlayerShotDataType *shot = &playerShotData[shot_id];

  shotAvail[shot_id]--;
  if (shot_id != MAX_PWEAPON - 1) {
    shot->shotXM += shot->shotXC;
    shot->shotX += shot->shotXM;
    JE_integer tmp_shotXM = shot->shotXM;

    if (shot->shotXM > 100) {
      if (shot->shotXM == 101) {
        shot->shotX -= 101;
        shot->shotX += player[shot->playerNumber - 1].delta_x_shot_move;
        shot->shotY += player[shot->playerNumber - 1].delta_y_shot_move;
      }
      else {
        shot->shotX -= 120;
        shot->shotX += player[shot->playerNumber - 1].delta_x_shot_move;
      }
    }

    shot->shotYM += shot->shotYC;
    shot->shotY += shot->shotYM;

    if (shot->shotYM > 100) {
      shot->shotY -= 120;
      shot->shotY += player[shot->playerNumber - 1].delta_y_shot_move;
    }

    if (shot->shotComplicated != 0) {
      shot->shotDevX += shot->shotDirX;
      shot->shotX += shot->shotDevX;

      if (abs(shot->shotDevX) == shot->shotCirSizeX) {
        shot->shotDirX = -shot->shotDirX;
      }

      shot->shotDevY += shot->shotDirY;
      shot->shotY += shot->shotDevY;

      if (abs(shot->shotDevY) == shot->shotCirSizeY) {
        shot->shotDirY = -shot->shotDirY;
      }

      // Double Speed Circle Shots - add a second copy of above loop
    }

    *out_shotx = shot->shotX;
    *out_shoty = shot->shotY;

    if (shot->shotX < -34 || shot->shotX > 290 || shot->shotY < -15 || shot->shotY > 190) {
      shotAvail[shot_id] = 0;
      return false;
    }

    if (shot->shotTrail != 255) {
      if (shot->shotTrail == 98) {
        setup_explosion(shot->shotX - shot->shotXM, shot->shotY - shot->shotYM, 0, shot->shotTrail, false,
                        false);
      }
      else {
        setup_explosion(shot->shotX, shot->shotY, 0, shot->shotTrail, false, false);
      }
    }

    if (shot->aimAtEnemy != 0 && --shot->aimDelay == 0) {
      shot->aimDelay = shot->aimDelayMax;

      if (enemyAvail[shot->aimAtEnemy - 1] != 1) {
        if (shot->shotX < enemy[shot->aimAtEnemy - 1].ex) {
          shot->shotXM++;
        }
        else {
          shot->shotXM--;
        }

        if (shot->shotY < enemy[shot->aimAtEnemy - 1].ey) {
          shot->shotYM++;
        }
        else {
          shot->shotYM--;
        }
      }
      else {
        if (shot->shotXM > 0) {
          shot->shotXM++;
        }
        else {
          shot->shotXM--;
        }
      }
    }

    JE_word sprite_frame = shot->shotGr + shot->shotAni;
    if (++shot->shotAni == shot->shotAniMax) {
      shot->shotAni = 0;
    }

    *out_shot_damage = shot->shotDmg;
    *out_blast_filter = shot->shotBlastFilter;
    *out_chain = shot->chainReaction;
    *out_playerNum = shot->playerNumber;

    *out_is_special = sprite_frame > 60000;

    if (*out_is_special) {
      drawn.push_back({*out_shotx, *out_shoty, sprite_frame, false});

      *out_special_radiusw = sprite(OPTION_SHAPES, sprite_frame - 60001)->width / 2;
      *out_special_radiush = sprite(OPTION_SHAPES, sprite_frame - 60001)->height / 2;
    }
    else {
      if (sprite_frame > 1000) {
        do_sp(*out_shotx + 1 + 6, *out_shoty + 6, 5, 3, (sprite_frame / 1000) << 4);
        sprite_frame = sprite_frame % 1000;
      }
      drawn.push_back({*out_shotx, *out_shoty, sprite_frame,
                       background2 && *out_shoty + shadowYDist < 190 && tmp_shotXM < 100});
    }
  }

  return true;
}

void player_shots_draw(PlayerShotsDrawn const& d) {
  SDL_Surface *const surface = d.screen;
  for (PlayerShotDrawn const& shot : d.drawn) {
    JE_word const sprite_frame = shot.sprite_frame;
    int const x = shot.x + 1, y = shot.y;
    if (sprite_frame > 60000) {
      draw::blit_sprite_blend(surface, x, y, OPTION_SHAPES, sprite_frame - 60001);
    }
    else if (sprite_frame > 500) {
      if (shot.shadow) {
        draw::blit_sprite2_darken(surface, x, y + shadowYDist, spriteSheet12, sprite_frame - 500);
      }
      draw::blit_sprite2(surface, x, y, spriteSheet12, sprite_frame - 500);
    }
    else {
      if (shot.shadow) {
        draw::blit_sprite2_darken(surface, x, y + shadowYDist, spriteSheet8, sprite_frame);
      }
      draw::blit_sprite2(surface, x, y, spriteSheet8, sprite_frame);
    }
  }
}

// Port of player_shot_create (shots.c).
// FKA Varz.initPlayerShot
JE_integer player_shot_create(JE_word portNum, uint bay_i, JE_word PX, JE_word PY, JE_word mouseX,
                              JE_word mouseY, JE_word wpNum, JE_byte playerNum) {
  auto& enemy = state().enemies.slots;
  auto& enemyAvail = state().enemies.status;
  auto& playerShotData = state().player_shots.slots;
  auto& shotAvail = state().player_shots.ttl;
  static JE_byte const soundChannel[11] = {0, 2, 4, 4, 2, 2, 5, 5, 1, 4, 1};  // [1..11]

  // Bounds check
  if (portNum > PORT_NUM || wpNum <= 0 || wpNum > WEAP_NUM) {
    return MAX_PWEAPON;
  }

  JE_WeaponType const *weapon = &weapons[wpNum];

  if (power < weaponPort[portNum].poweruse) {
    return MAX_PWEAPON;
  }
  power -= weaponPort[portNum].poweruse;

  if (weapon->sound > 0) {
    soundQueue[soundChannel[bay_i]] = weapon->sound;
  }

  int shot_id = MAX_PWEAPON;
  // Rot
  for (int multi_i = 1; multi_i <= weapon->multi; multi_i++) {
    for (shot_id = 0; shot_id < MAX_PWEAPON; shot_id++) {
      if (shotAvail[shot_id] == 0) {
        break;
      }
    }
    if (shot_id == MAX_PWEAPON) {
      return MAX_PWEAPON;
    }

    if (shotMultiPos[bay_i] == weapon->max || shotMultiPos[bay_i] > 8) {
      shotMultiPos[bay_i] = 1;
    }
    else {
      shotMultiPos[bay_i]++;
    }

    PlayerShotDataType *shot = &playerShotData[shot_id];
    shot->chainReaction = 0;

    shot->playerNumber = playerNum;

    shot->shotAni = 0;

    shot->shotComplicated = weapon->circlesize != 0;

    if (weapon->circlesize == 0) {
      shot->shotDevX = 0;
      shot->shotDirX = 0;
      shot->shotDevY = 0;
      shot->shotDirY = 0;
      shot->shotCirSizeX = 0;
      shot->shotCirSizeY = 0;
    }
    else {
      JE_byte circsize = weapon->circlesize;

      if (circsize > 19) {
        JE_byte circsize_mod20 = circsize % 20;
        shot->shotCirSizeX = circsize_mod20;
        shot->shotDevX = circsize_mod20 >> 1;

        circsize = circsize / 20;
        shot->shotCirSizeY = circsize;
        shot->shotDevY = circsize >> 1;
      }
      else {
        shot->shotCirSizeX = circsize;
        shot->shotCirSizeY = circsize;
        shot->shotDevX = circsize >> 1;
        shot->shotDevY = circsize >> 1;
      }
      shot->shotDirX = 1;
      shot->shotDirY = -1;
    }

    shot->shotTrail = weapon->trail;

    if (weapon->attack[shotMultiPos[bay_i] - 1] > 99 && weapon->attack[shotMultiPos[bay_i] - 1] < 250) {
      shot->chainReaction = weapon->attack[shotMultiPos[bay_i] - 1] - 100;
      shot->shotDmg = 1;
    }
    else {
      shot->shotDmg = weapon->attack[shotMultiPos[bay_i] - 1];
    }

    shot->shotBlastFilter = weapon->shipblastfilter;

    JE_integer tmp_by = weapon->by[shotMultiPos[bay_i] - 1];

    // Note: Only front selection used for player shots...

    shot->shotX = PX + weapon->bx[shotMultiPos[bay_i] - 1];

    shot->shotY = PY + tmp_by;
    shot->shotYC = -weapon->acceleration;
    shot->shotXC = weapon->accelerationx;

    shot->shotXM = weapon->sx[shotMultiPos[bay_i] - 1];

    // Not sure what this field does exactly.
    JE_byte del = weapon->del[shotMultiPos[bay_i] - 1];

    if (del == 121) {
      shot->shotTrail = 0;
      del = 255;
    }

    shot->shotGr = weapon->sg[shotMultiPos[bay_i] - 1];
    if (shot->shotGr == 0) {
      shotAvail[shot_id] = 0;
    }
    else {
      shotAvail[shot_id] = del;
    }

    if (del > 100 && del < 120) {
      shot->shotAniMax = (del - 100 + 1);
    }
    else {
      shot->shotAniMax = weapon->weapani + 1;
    }

    if (del == 99 || del == 98) {
      tmp_by = PX - mouseX;
      if (tmp_by < -5) {
        tmp_by = -5;
      }
      else if (tmp_by > 5) {
        tmp_by = 5;
      }
      shot->shotXM += tmp_by;
    }

    if (del == 99 || del == 100) {
      tmp_by = PY - mouseY - weapon->sy[shotMultiPos[bay_i] - 1];
      if (tmp_by < -4) {
        tmp_by = -4;
      }
      else if (tmp_by > 4) {
        tmp_by = 4;
      }
      shot->shotYM = tmp_by;
    }
    else if (weapon->sy[shotMultiPos[bay_i] - 1] == 98) {
      shot->shotYM = 0;
      shot->shotYC = -1;
    }
    else if (weapon->sy[shotMultiPos[bay_i] - 1] > 100) {
      shot->shotYM = weapon->sy[shotMultiPos[bay_i] - 1];
      shot->shotY -= player[shot->playerNumber - 1].delta_y_shot_move;
    }
    else {
      shot->shotYM = -weapon->sy[shotMultiPos[bay_i] - 1];
    }

    if (weapon->sx[shotMultiPos[bay_i] - 1] > 100) {
      shot->shotXM = weapon->sx[shotMultiPos[bay_i] - 1];
      shot->shotX -= player[shot->playerNumber - 1].delta_x_shot_move;
      if (shot->shotXM == 101) {
        shot->shotY -= player[shot->playerNumber - 1].delta_y_shot_move;
      }
    }

    // Guided Shot
    if (weapon->aim > 5) {
      uint best_dist = 65000;
      JE_byte closest_enemy = 0;
      // Find Closest Enemy
      for (x = 0; x < 100; x++) {
        if (enemyAvail[x] == 1 || enemy[x].scoreitem) {
          continue;
        }

        y = abs(enemy[x].ex - shot->shotX) + abs(enemy[x].ey - shot->shotY);
        if (y < best_dist) {
          best_dist = y;
          closest_enemy = x + 1;
        }
      }
      shot->aimAtEnemy = closest_enemy;
      shot->aimDelay = 5;
      shot->aimDelayMax = weapon->aim - 5;
    }
    else {
      shot->aimAtEnemy = 0;
    }

    shotRepeat[bay_i] = weapon->shotrepeat;
  }

  return shot_id;
}

// Port of player_shot_set_direction (shots.c).
void player_shot_set_direction(JE_integer shot_id, uint weapon_id, JE_real direction) {
  auto& playerShotData = state().player_shots.slots;
  PlayerShotDataType *shot = &playerShotData[shot_id];

  shot->shotXM = -roundf(sinf(direction) * shot->shotYM);
  shot->shotYM = -roundf(cosf(direction) * shot->shotYM);

  // Some weapons have sprites for each direction, use those.
  int rounded_dir;

  switch (weapon_id) {
    case 27:
    case 32:
    case 10:
      rounded_dir = roundf(direction * (16 / (2 * M_PI)));  // 16 directions
      shot->shotGr = linkMultiGr[rounded_dir];
      break;
    case 28:
    case 33:
    case 11:
      rounded_dir = roundf(direction * (16 / (2 * M_PI)));  // 16 directions
      shot->shotGr = linkSonicGr[rounded_dir];
      break;
    case 30:
    case 35:
    case 14:
      if (direction > M_PI_2 && direction < M_PI + M_PI_2) {
        shot->shotYC = 1;
      }
      break;
    case 38:
    case 22:
      rounded_dir = roundf(direction * (16 / (2 * M_PI)));  // 16 directions
      shot->shotGr = linkMult2Gr[rounded_dir];
      break;
  }
}

}  // namespace tyrx::port
