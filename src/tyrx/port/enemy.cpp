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

// The enemies (JE_drawEnemy in tyrian2.c, which "actually does a whole lot more
// than just drawing").  Each enemy is drawn, then moves, fires and launches
// enemies.

#include "draw.hpp"
#include "port.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace tyrx::port {

namespace {

// Port of blit_enemy.
void blit_enemy(SDL_Surface *surface, unsigned int i, signed int x_offset, signed int y_offset,
                signed int sprite_offset) {
  if (enemy[i].sprite2s == nullptr) {
    return;
  }

  int const x = enemy[i].ex + x_offset + tempMapXOfs, y = enemy[i].ey + y_offset;
  unsigned int const index = enemy[i].egr[enemy[i].enemycycle - 1] + sprite_offset;

  if (enemy[i].filter != 0) {
    draw::blit_sprite2_filter(surface, x, y, *enemy[i].sprite2s, index, enemy[i].filter);
  }
  else {
    draw::blit_sprite2(surface, x, y, *enemy[i].sprite2s, index);
  }
}

// One enemy's part of JE_drawEnemy; returning is `goto draw_enemy_end`.
void draw_enemy(int enemyOffset, int i) {
  enemy[i].mapoffset = tempMapXOfs;

  if (enemy[i].xaccel && enemy[i].xaccel - 89u > mt_rand() % 11) {
    if (player[0].x > enemy[i].ex) {
      if (enemy[i].exc < enemy[i].xaccel - 89) {
        enemy[i].exc++;
      }
    }
    else {
      if (enemy[i].exc >= 0 || -enemy[i].exc < enemy[i].xaccel - 89) {
        enemy[i].exc--;
      }
    }
  }

  if (enemy[i].yaccel && enemy[i].yaccel - 89u > mt_rand() % 11) {
    if (player[0].y > enemy[i].ey) {
      if (enemy[i].eyc < enemy[i].yaccel - 89) {
        enemy[i].eyc++;
      }
    }
    else {
      if (enemy[i].eyc >= 0 || -enemy[i].eyc < enemy[i].yaccel - 89) {
        enemy[i].eyc--;
      }
    }
  }

  auto const enemy_gone = [&] {
    // enemy[i].egr[10] &= 0x00ff; <MXD> madness?
    enemyAvail[i] = 1;
  };

  if (enemy[i].ex + tempMapXOfs > -29 && enemy[i].ex + tempMapXOfs < 300) {
    if (enemy[i].aniactive == 1) {
      enemy[i].enemycycle++;

      if (enemy[i].enemycycle == enemy[i].animax) {
        enemy[i].aniactive = enemy[i].aniwhenfire;
      }
      else if (enemy[i].enemycycle > enemy[i].ani) {
        enemy[i].enemycycle = enemy[i].animin;
      }
    }

    if (enemy[i].egr[enemy[i].enemycycle - 1] == 999) {
      return enemy_gone();
    }

    // 2x2 enemy
    if (enemy[i].size == 1) {
      if (enemy[i].ey > -13) {
        blit_enemy(VGAScreen, i, -6, -7, 0);
        blit_enemy(VGAScreen, i, 6, -7, 1);
      }
      if (enemy[i].ey > -26 && enemy[i].ey < 182) {
        blit_enemy(VGAScreen, i, -6, 7, 19);
        blit_enemy(VGAScreen, i, 6, 7, 20);
      }
    }
    else {
      if (enemy[i].ey > -13) {
        blit_enemy(VGAScreen, i, 0, 0, 0);
      }
    }

    enemy[i].filter = 0;
  }

  if (enemy[i].excc && --enemy[i].exccw <= 0) {
    if (enemy[i].exc == enemy[i].exrev) {
      enemy[i].excc = -enemy[i].excc;
      enemy[i].exrev = -enemy[i].exrev;
      enemy[i].exccadd = -enemy[i].exccadd;
    }
    else {
      enemy[i].exc += enemy[i].exccadd;
      enemy[i].exccw = enemy[i].exccwmax;
      if (enemy[i].exc == enemy[i].exrev) {
        enemy[i].excc = -enemy[i].excc;
        enemy[i].exrev = -enemy[i].exrev;
        enemy[i].exccadd = -enemy[i].exccadd;
      }
    }
  }

  if (enemy[i].eycc && --enemy[i].eyccw <= 0) {
    if (enemy[i].eyc == enemy[i].eyrev) {
      enemy[i].eycc = -enemy[i].eycc;
      enemy[i].eyrev = -enemy[i].eyrev;
      enemy[i].eyccadd = -enemy[i].eyccadd;
    }
    else {
      enemy[i].eyc += enemy[i].eyccadd;
      enemy[i].eyccw = enemy[i].eyccwmax;
      if (enemy[i].eyc == enemy[i].eyrev) {
        enemy[i].eycc = -enemy[i].eycc;
        enemy[i].eyrev = -enemy[i].eyrev;
        enemy[i].eyccadd = -enemy[i].eyccadd;
      }
    }
  }

  enemy[i].ey += enemy[i].fixedmovey;

  enemy[i].ex += enemy[i].exc;
  if (enemy[i].ex < -80 || enemy[i].ex > 340) {
    return enemy_gone();
  }

  enemy[i].ey += enemy[i].eyc;
  if (enemy[i].ey < -112 || enemy[i].ey > 190) {
    return enemy_gone();
  }

  // X bounce
  if (enemy[i].ex <= enemy[i].xminbounce || enemy[i].ex >= enemy[i].xmaxbounce) {
    enemy[i].exc = -enemy[i].exc;
  }

  // Y bounce
  if (enemy[i].ey <= enemy[i].yminbounce || enemy[i].ey >= enemy[i].ymaxbounce) {
    enemy[i].eyc = -enemy[i].eyc;
  }

  // Evalue != 0 - score item at boundary
  if (enemy[i].scoreitem) {
    if (enemy[i].ex < -5) {
      enemy[i].ex++;
    }
    if (enemy[i].ex > 245) {
      enemy[i].ex--;
    }
  }

  enemy[i].ey += tempBackMove;

  if (enemy[i].ex <= -24 || enemy[i].ex >= 296) {
    return;
  }

  JE_integer tempX = enemy[i].ex;
  JE_integer tempY = enemy[i].ey;

  temp = enemy[i].enemytype;

  // Enemy Shots
  if (enemy[i].edamaged == 1) {
    return;
  }

  enemyOnScreen++;

  if (enemy[i].iced) {
    enemy[i].iced--;
    if (enemy[i].enemyground != 0) {
      enemy[i].filter = 0x09;
    }
    return;
  }

  for (int j = 3; j > 0; j--) {
    if (!enemy[i].freq[j - 1]) {
      continue;
    }

    temp3 = enemy[i].tur[j - 1];

    if (--enemy[i].eshotwait[j - 1] != 0 || !temp3) {
      continue;
    }

    enemy[i].eshotwait[j - 1] = enemy[i].freq[j - 1];
    if (difficultyLevel > DIFFICULTY_NORMAL) {
      enemy[i].eshotwait[j - 1] = (enemy[i].eshotwait[j - 1] / 2) + 1;
      if (difficultyLevel > DIFFICULTY_MANIACAL) {
        enemy[i].eshotwait[j - 1] = (enemy[i].eshotwait[j - 1] / 2) + 1;
      }
    }

    if (galagaMode && (enemy[i].eyc == 0 || (mt_rand() % 400) >= galagaShotFreq)) {
      return;
    }

    switch (temp3) {
      case 252:  // Savara Boss DualMissile
        if (enemy[i].ey > 20) {
          JE_setupExplosion(tempX - 8 + tempMapXOfs, tempY - 20 - backMove * 8, -2, 6, false, false);
          JE_setupExplosion(tempX + 4 + tempMapXOfs, tempY - 20 - backMove * 8, -2, 6, false, false);
        }
        break;
      case 251: {  // Suck-O-Magnet
        JE_integer const attraction = 4 - (abs(player[0].x - tempX) + abs(player[0].y - tempY)) / 100;
        if (attraction > 0) {
          player[0].x_velocity += (player[0].x > tempX) ? -attraction : attraction;
        }
        break;
      }
      case 253:  // Left ShortRange Magnet
        if (abs(player[0].x + 25 - 14 - tempX) < 24 && abs(player[0].y - tempY) < 28) {
          player[0].x_velocity += 2;
        }
        if (twoPlayerMode && (abs(player[1].x - 14 - tempX) < 24 && abs(player[1].y - tempY) < 28)) {
          player[1].x_velocity += 2;
        }
        break;
      case 254:  // Left ShortRange Magnet
        if (abs(player[0].x + 25 - 14 - tempX) < 24 && abs(player[0].y - tempY) < 28) {
          player[0].x_velocity -= 2;
        }
        if (twoPlayerMode && (abs(player[1].x - 14 - tempX) < 24 && abs(player[1].y - tempY) < 28)) {
          player[1].x_velocity -= 2;
        }
        break;
      case 255:  // Magneto RePulse!!
        // DIF
        if (difficultyLevel != DIFFICULTY_EASY) {
          if (j == 3) {
            enemy[i].filter = 0x70;
          }
          else {
            JE_integer const repulsion = 4 - (abs(player[0].x - tempX) + abs(player[0].y - tempY)) / 20;
            if (repulsion > 0) {
              player[0].x_velocity += (player[0].x > tempX) ? repulsion : -repulsion;
            }
          }
        }
        break;
      default:
        // Rot
        for (int tempCount = weapons[temp3].multi; tempCount > 0; tempCount--) {
          for (b = 0; b < ENEMY_SHOT_MAX; b++) {
            if (enemyShotAvail[b] == 1) {
              break;
            }
          }
          if (b == ENEMY_SHOT_MAX) {
            return;
          }

          enemyShotAvail[b] = !enemyShotAvail[b];

          if (weapons[temp3].sound > 0) {
            do {
              temp = mt_rand() % 8;
            } while (temp == 3);
            soundQueue[temp] = weapons[temp3].sound;
          }

          if (enemy[i].aniactive == 2) {
            enemy[i].aniactive = 1;
          }

          if (++enemy[i].eshotmultipos[j - 1] > weapons[temp3].max) {
            enemy[i].eshotmultipos[j - 1] = 1;
          }

          int tempPos = enemy[i].eshotmultipos[j - 1] - 1;

          if (j == 1) {
            temp2 = 4;
          }

          enemyShot[b].sx = tempX + weapons[temp3].bx[tempPos] + tempMapXOfs;
          enemyShot[b].sy = tempY + weapons[temp3].by[tempPos];
          enemyShot[b].sdmg = weapons[temp3].attack[tempPos];
          enemyShot[b].tx = weapons[temp3].tx;
          enemyShot[b].ty = weapons[temp3].ty;
          enemyShot[b].duration = weapons[temp3].del[tempPos];
          enemyShot[b].animate = 0;
          enemyShot[b].animax = weapons[temp3].weapani;

          enemyShot[b].sgr = weapons[temp3].sg[tempPos];
          switch (j) {
            case 1:
              enemyShot[b].syc = weapons[temp3].acceleration;
              enemyShot[b].sxc = weapons[temp3].accelerationx;

              enemyShot[b].sxm = weapons[temp3].sx[tempPos];
              enemyShot[b].sym = weapons[temp3].sy[tempPos];
              break;
            case 3:
              enemyShot[b].sxc = -weapons[temp3].acceleration;
              enemyShot[b].syc = weapons[temp3].accelerationx;

              enemyShot[b].sxm = -weapons[temp3].sy[tempPos];
              enemyShot[b].sym = -weapons[temp3].sx[tempPos];
              break;
            case 2:
              enemyShot[b].sxc = weapons[temp3].acceleration;
              enemyShot[b].syc = -weapons[temp3].acceleration;

              enemyShot[b].sxm = weapons[temp3].sy[tempPos];
              enemyShot[b].sym = -weapons[temp3].sx[tempPos];
              break;
          }

          if (weapons[temp3].aim <= 0) {
            continue;
          }

          JE_byte aim = weapons[temp3].aim;

          // DIF
          if (difficultyLevel > DIFFICULTY_NORMAL) {
            aim += difficultyLevel - 2;
          }

          JE_word targetX = player[0].x;
          JE_word targetY = player[0].y;

          if (twoPlayerMode) {
            // fire at live player(s)
            if (player[0].is_alive && !player[1].is_alive) {
              temp = 0;
            }
            else if (player[1].is_alive && !player[0].is_alive) {
              temp = 1;
            }
            else {
              temp = mt_rand() % 2;
            }

            if (temp == 1) {
              targetX = player[1].x - 25;
              targetY = player[1].y;
            }
          }

          JE_integer aimX = (targetX + 25) - tempX - tempMapXOfs - 4;
          if (aimX == 0) {
            aimX = 1;
          }
          JE_integer aimY = targetY - tempY;
          if (aimY == 0) {
            aimY = 1;
          }
          JE_integer const maxMagAim = MAX(abs(aimX), abs(aimY));
          enemyShot[b].sxm = roundf((float)aimX / maxMagAim * aim);
          enemyShot[b].sym = roundf((float)aimY / maxMagAim * aim);
        }
        break;
    }
  }

  if (!enemy[i].launchfreq) {
    return;
  }

  if (--enemy[i].launchwait != 0) {
    return;
  }

  // Enemy Launch Routine
  enemy[i].launchwait = enemy[i].launchfreq;

  if (enemy[i].launchspecial != 0) {
    // Type  1 : Must be inline with player
    if (abs(enemy[i].ey - player[0].y) > 5) {
      return;
    }
  }

  if (enemy[i].aniactive == 2) {
    enemy[i].aniactive = 1;
  }

  if (enemy[i].launchtype == 0) {
    return;
  }

  tempW = enemy[i].launchtype;
  b = JE_newEnemy(enemyOffset == 50 ? 75 : enemyOffset - 25, tempW, 0);

  if (b <= 0) {
    return;
  }

  // Launch Enemy Placement
  struct JE_SingleEnemyType *e = &enemy[b - 1];

  e->ex = tempX;
  e->ey = tempY + enemyDat[e->enemytype].startyc;
  if (e->size == 0) {
    e->ey -= 7;
  }

  if (e->launchtype > 0 && e->launchfreq == 0) {
    if (e->launchtype > 90) {
      e->ex += mt_rand() % ((e->launchtype - 90) * 4) - (e->launchtype - 90) * 2;
    }
    else {
      JE_integer aimX = (player[0].x + 25) - tempX - tempMapXOfs - 4;
      if (aimX == 0) {
        aimX = 1;
      }
      JE_integer aimY = player[0].y - tempY;
      if (aimY == 0) {
        aimY = 1;
      }
      JE_integer const maxMagAim = MAX(abs(aimX), abs(aimY));
      e->exc = roundf((float)aimX / maxMagAim * e->launchtype);
      e->eyc = roundf((float)aimY / maxMagAim * e->launchtype);
    }
  }

  do {
    temp = mt_rand() % 8;
  } while (temp == 3);
  soundQueue[temp] = randomEnemyLaunchSounds[(mt_rand() % 3)];

  if (enemy[i].launchspecial == 1 && enemy[i].linknum < 100) {
    e->linknum = enemy[i].linknum;
  }
}

}  // namespace

// Port of JE_drawEnemy.
void draw_enemy(int enemyOffset) {
  player[0].x -= 25;

  for (int i = enemyOffset - 25; i < enemyOffset; i++) {
    if (enemyAvail[i] != 1) {
      draw_enemy(enemyOffset, i);
    }
  }

  player[0].x += 25;
}

}  // namespace tyrx::port
