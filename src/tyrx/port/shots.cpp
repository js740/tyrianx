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

// The player shots (player_shot_move_and_draw in shots.c).

#include "draw.hpp"
#include "port.hpp"

#include <cstdlib>

namespace tyrx::port {

// Port of player_shot_move_and_draw (shots.c).
bool player_shot_move_and_draw(int shot_id, bool *out_is_special, int *out_shotx, int *out_shoty,
                               JE_integer *out_shot_damage, JE_byte *out_blast_filter, JE_byte *out_chain,
                               JE_byte *out_playerNum, JE_word *out_special_radiusw,
                               JE_word *out_special_radiush) {
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
        JE_setupExplosion(shot->shotX - shot->shotXM, shot->shotY - shot->shotYM, 0, shot->shotTrail, false,
                          false);
      }
      else {
        JE_setupExplosion(shot->shotX, shot->shotY, 0, shot->shotTrail, false, false);
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
      draw::blit_sprite_blend(VGAScreen, *out_shotx + 1, *out_shoty, OPTION_SHAPES, sprite_frame - 60001);

      *out_special_radiusw = sprite(OPTION_SHAPES, sprite_frame - 60001)->width / 2;
      *out_special_radiush = sprite(OPTION_SHAPES, sprite_frame - 60001)->height / 2;
    }
    else {
      if (sprite_frame > 1000) {
        JE_doSP(*out_shotx + 1 + 6, *out_shoty + 6, 5, 3, (sprite_frame / 1000) << 4);
        sprite_frame = sprite_frame % 1000;
      }
      if (sprite_frame > 500) {
        if (background2 && *out_shoty + shadowYDist < 190 && tmp_shotXM < 100) {
          draw::blit_sprite2_darken(VGAScreen, *out_shotx + 1, *out_shoty + shadowYDist, spriteSheet12,
                                    sprite_frame - 500);
        }
        draw::blit_sprite2(VGAScreen, *out_shotx + 1, *out_shoty, spriteSheet12, sprite_frame - 500);
      }
      else {
        if (background2 && *out_shoty + shadowYDist < 190 && tmp_shotXM < 100) {
          draw::blit_sprite2_darken(VGAScreen, *out_shotx + 1, *out_shoty + shadowYDist, spriteSheet8,
                                    sprite_frame);
        }
        draw::blit_sprite2(VGAScreen, *out_shotx + 1, *out_shoty, spriteSheet8, sprite_frame);
      }
    }
  }

  return true;
}

}  // namespace tyrx::port
