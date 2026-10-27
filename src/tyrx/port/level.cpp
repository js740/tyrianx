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

// One pass of the level loop (from the level_loop label in JE_main, tyrian2.c),
// in C++: the tick's simulation, in the C code's order, which keeps a record
// for the drawing; then the drawing, from the record; then the frame.  See
// level.hpp.

#include "level.hpp"

#include "../gold/api.hpp"
#include "draw.hpp"
#include "port.hpp"

#include <bitset>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace tyrx::port {

namespace {

// The end of JE_starShowVGA, after the tick's drawing: waits for the frame
// deadline, presents the frame and polls events.  (Its drawing, the copy of
// the playing area to the screen, ends the tick's drawing.)
void star_show_vga_present() {
  if (!playerEndLevel && !skipStarShowVGA) {
    if (smoothScroll != 0) {
      delayUntilElapsed();
      setFrameCount(frameCountMax);
    }
    present();
  }
  poll_events();
  quitRequested = false;
  skipStarShowVGA = false;
}

// The tick's simulation, which is complete before anything is drawn.  `frame`
// is set if the tick ends with a frame (JE_starShowVGA): it doesn't when the
// in-game menu or help was opened, or the level ended early.
LevelPass simulate(bool& frame);

}  // namespace

LevelLocals& level_locals() {
  static LevelLocals locals;
  return locals;
}

LevelPass level_pass() {
  bool frame = false;
  LevelPass const next = simulate(frame);
  if (next != LevelPass::unsupported) {
    drawing_begins();
    draw_tick(draw_record());
    if (frame) {
      star_show_vga_present();
    }
  }
  return next;
}

namespace {

// The start of the tick: the smoothie screen code, background wrapping, and
// music.
void tick_start() {
  // tempScreenSeg = game_screen; // side-effect of game_screen

  if (isNetworkGame) {
    smoothies[9 - 1] = false;
    smoothies[6 - 1] = false;
  }
  else {
    starShowVGASpecialCode = smoothies[9 - 1] + (smoothies[6 - 1] << 1);
  }

  // Background Wrapping
  if (mapYPos <= BKwrap1) {
    mapYPos = BKwrap1to;
  }
  if (mapY2Pos <= BKwrap2) {
    mapY2Pos = BKwrap2to;
  }
  if (mapY3Pos <= BKwrap3) {
    mapY3Pos = BKwrap3to;
  }

  allPlayersGone =
      all_players_dead() &&
      ((*player[0].lives == 1 && player[0].exploding_ticks == 0) || (!onePlayerAction && !twoPlayerMode)) &&
      ((*player[1].lives == 1 && player[1].exploding_ticks == 0) || !twoPlayerMode);

  // -----MUSIC FADE------
  if (musicFade) {
    if (tempVolume > 10) {
      tempVolume--;
      set_volume(tempVolume, fxVolume);
    }
    else {
      musicFade = false;
    }
  }

  if (!allPlayersGone && levelEnd > 0 && endLevel) {
    play_song(9);
    musicFade = false;
  }
  else if (!playing && firstGameOver) {
    play_song(levelSong - 1);
  }
}

// The HUD at the top of the tick: message bar, shield generator, weapon power
// and power bar.
void hud_tick() {
  unsigned *const old_weapon_bar = level_locals().old_weapon_bar;

  if (endLevel) {
    return;
  }

  // draw HUD
  VGAScreen = VGAScreenSeg;  // side-effect of game_screen

  // -----------------------Message Bar------------------------
  if (textErase > 0 && --textErase == 0) {
    message_bar_erased_tick();  // in-game message area
  }

  // ------------------------Shield Gen-------------------------
  if (galagaMode) {
    for (uint i = 0; i < COUNTOF(player); ++i) {
      player[i].shield = 0;
    }

    // spawned dragonwing died :(
    if (*player[1].lives == 0 || player[1].armor == 0) {
      twoPlayerMode = false;
    }

    if (player[0].cash >= (unsigned)galagaLife) {
      soundQueue[6] = S_EXPLOSION_11;
      soundQueue[7] = S_SOUL_OF_ZINGLON;

      if (*player[0].lives < 11) {
        ++(*player[0].lives);
      }
      else {
        player[0].cash += 1000;
      }

      if (galagaLife == 10000) {
        galagaLife = 20000;
      }
      else {
        galagaLife += 25000;
      }
    }
  }
  // not galagaMode
  else {
    if (twoPlayerMode) {
      if (--shieldWait == 0) {
        shieldWait = 15;

        for (uint i = 0; i < COUNTOF(player); ++i) {
          if (player[i].shield < player[i].shield_max && player[i].is_alive) {
            ++player[i].shield;
          }
        }

        draw_shield_tick();
      }
    }
    else if (player[0].is_alive && player[0].shield < player[0].shield_max && power > shieldT) {
      if (--shieldWait == 0) {
        shieldWait = 15;

        power -= shieldT;

        ++player[0].shield;
        if (player[1].shield < player[0].shield_max) {
          ++player[1].shield;
        }

        draw_shield_tick();
      }
    }
  }

  // ---------------------Weapon Display-------------------------
  for (uint i = 0; i < 2; ++i) {
    uint item_power = player[twoPlayerMode ? i : 0].items.weapon[i].power;

    if (old_weapon_bar[i] == item_power) {
      continue;
    }

    old_weapon_bar[i] = item_power;

    int x = twoPlayerMode ? 286 : 289, y = (i == 0) ? (twoPlayerMode ? 6 : 17) : (twoPlayerMode ? 100 : 38);

    weapon_power_tick(x, y, item_power);
  }

  // ------------------------Power Bar-------------------------
  if (twoPlayerMode || onePlayerAction) {
    power = 900;
  }
  else {
    power += powerAdd;
    if (power > 900) {
      power = 900;
    }

    temp = power / 10;

    if (temp != lastPower) {
      power_bar_tick(temp, lastPower);

      lastPower = temp;
    }
  }

  oldMapX3Ofs = mapX3Ofs;

  enemyOnScreen = 0;
}

// Smoothies, background 1, the starfield and background 2 under the enemies.
void backgrounds_under_tick() {
  DrawRecord& r = draw_record();

  // SMOOTHIES!
  JE_checkSmoothies();
  if (anySmoothies) {
    VGAScreen = VGAScreen2;  // this makes things complicated, but we do it anyway :(
  }

  // --- BACKGROUNDS ---
  // --- BACKGROUND 1 ---

  if (forceEvents && !backMove) {
    curLoc++;
  }

  if (map1YDelayMax > 1 && backMove < 2) {
    backMove = (map1YDelay == 1) ? 1 : 0;
  }

  // Draw background
  if (astralDuration == 0) {
    background_1_tick(VGAScreen, false);
  }
  else {
    background_1_tick(VGAScreen, true);  // JE_clr256
  }

  // Set Movement of background 1
  if (--map1YDelay == 0) {
    map1YDelay = map1YDelayMax;

    curLoc += backMove;

    backPos += backMove;

    if (backPos > 27) {
      backPos -= 28;
      mapY--;
      mapYPos -= 14;  // Map Width
    }
  }

  if (starActive || astralDuration > 0) {
    starfield_tick(VGAScreen, starfield_speed);
  }

  if (processorType > 1 && smoothies[5 - 1]) {
    r.iced_blur_1 = Copy{game_screen, VGAScreen};
    VGAScreen = game_screen;
  }

  // -----------------------BACKGROUNDS------------------------
  // -----------------------BACKGROUND 2------------------------
  if (background2over == 3) {
    background_2_tick(VGAScreen, false, r.background_2_under);
    background2 = true;
  }

  if (background2over == 0) {
    if (!(smoothies[2 - 1] && processorType < 4) && !(smoothies[1 - 1] && processorType == 3)) {
      if (wild && !background2notTransparent) {
        background_2_tick(VGAScreen, true, r.background_2_under);
      }
      else {
        background_2_tick(VGAScreen, false, r.background_2_under);
      }
    }
  }

  if (smoothies[0] && processorType > 2 && smoothie_data[0] == 0) {
    r.lava_1 = Copy{game_screen, VGAScreen};
    VGAScreen = game_screen;
  }
  if (smoothies[2 - 1] && processorType > 2) {
    r.water = Copy{game_screen, VGAScreen};
    VGAScreen = game_screen;
  }
}

// The ground, sky and top enemies, with the background 2 and 3 layers and
// smoothie filters between them, and new random enemies.
void enemies_under_tick() {
  DrawRecord& r = draw_record();
  int lastEnemyOnScreen;

  // -----------------------Ground Enemy------------------------
  lastEnemyOnScreen = enemyOnScreen;

  tempMapXOfs = mapXOfs;
  tempBackMove = backMove;
  draw_enemy_tick(50, r.ground_enemies);
  draw_enemy_tick(100, r.ground_enemies_2);

  if ((enemyOnScreen == 0 || enemyOnScreen == lastEnemyOnScreen) && stopBackgroundNum == 1) {
    stopBackgroundNum = 9;
  }

  if (smoothies[0] && processorType > 2 && smoothie_data[0] > 0) {
    r.lava_2 = Copy{game_screen, VGAScreen};
    VGAScreen = game_screen;
  }

  if (superWild) {
    neat += 3;
    r.darken_1 = Darkened{VGAScreen, neat};
  }

  // -----------------------BACKGROUNDS------------------------
  // -----------------------BACKGROUND 2------------------------
  if (!(smoothies[2 - 1] && processorType < 4) && !(smoothies[1 - 1] && processorType == 3)) {
    if (background2over == 1) {
      if (wild && !background2notTransparent) {
        background_2_tick(VGAScreen, true, r.background_2_over_1);
      }
      else {
        background_2_tick(VGAScreen, false, r.background_2_over_1);
      }
    }
  }

  if (superWild) {
    neat++;
    r.darken_2 = Darkened{VGAScreen, neat};
  }

  if (background3over == 2) {
    background_3_tick(VGAScreen, r.background_3_over_2);
  }

  // New Enemy
  if (enemiesActive && mt_rand() % 100 > levelEnemyFrequency) {
    tempW = levelEnemy[mt_rand() % levelEnemyMax];
    if (tempW == 2) {
      soundQueue[3] = S_WEAPON_7;
    }
    b = new_enemy(0, tempW, 0);
  }

  if (processorType > 1 && smoothies[3 - 1]) {
    r.iced_blur_2 = Copy{game_screen, VGAScreen};
    VGAScreen = game_screen;
  }
  if (processorType > 1 && smoothies[4 - 1]) {
    r.blur = Copy{game_screen, VGAScreen};
    VGAScreen = game_screen;
  }

  // Draw Sky Enemy
  if (!skyEnemyOverAll) {
    lastEnemyOnScreen = enemyOnScreen;

    tempMapXOfs = mapX2Ofs;
    tempBackMove = 0;
    draw_enemy_tick(25, r.sky_enemies_under);

    if (enemyOnScreen == lastEnemyOnScreen && stopBackgroundNum == 2) {
      stopBackgroundNum = 9;
    }
  }

  if (background3over == 0) {
    background_3_tick(VGAScreen, r.background_3_over_0);
  }

  // Draw Top Enemy
  if (!topEnemyOver) {
    tempMapXOfs = (background3x1 == 0) ? oldMapX3Ofs : mapXOfs;
    tempBackMove = backMove3;
    draw_enemy_tick(75, r.top_enemies_under);
  }
}

// The player shots, and what they hit.
void player_shots_tick() {
  DrawRecord& r = draw_record();

  // Player Shot Images
  r.player_shots = PlayerShotsDrawn{VGAScreen, {}};
  for (int z = 0; z < MAX_PWEAPON; z++) {
    if (shotAvail[z] == 0) {
      continue;
    }

    bool is_special = false;
    int tempShotX = 0, tempShotY = 0;
    JE_byte chain;
    JE_byte playerNum;
    JE_word tempX2, tempY2;
    JE_integer damage;

    if (!player_shot_move(z, &is_special, &tempShotX, &tempShotY, &damage, &temp2, &chain, &playerNum,
                          &tempX2, &tempY2, r.player_shots->drawn))
    {
      goto draw_player_shot_loop_end;
    }

    for (b = 0; b < 100; b++) {
      if (enemyAvail[b] != 0) {
        continue;
      }

      bool collided;

      if (z == MAX_PWEAPON - 1) {
        temp = 25 - abs(zinglonDuration - 25);
        collided = abs(enemy[b].ex + enemy[b].mapoffset - (player[0].x + 7)) < temp;
        temp2 = 9;
        chain = 0;
        damage = 10;
      }
      else if (is_special) {
        collided = ((enemy[b].enemycycle == 0) &&
                    (abs(enemy[b].ex + enemy[b].mapoffset - tempShotX - tempX2) < (25 + tempX2)) &&
                    (abs(enemy[b].ey - tempShotY - 12 - tempY2) < (29 + tempY2))) ||
                   ((enemy[b].enemycycle > 0) &&
                    (abs(enemy[b].ex + enemy[b].mapoffset - tempShotX - tempX2) < (13 + tempX2)) &&
                    (abs(enemy[b].ey - tempShotY - 6 - tempY2) < (15 + tempY2)));
      }
      else {
        collided = ((enemy[b].enemycycle == 0) && (abs(enemy[b].ex + enemy[b].mapoffset - tempShotX) < 25) &&
                    (abs(enemy[b].ey - tempShotY - 12) < 29)) ||
                   ((enemy[b].enemycycle > 0) && (abs(enemy[b].ex + enemy[b].mapoffset - tempShotX) < 13) &&
                    (abs(enemy[b].ey - tempShotY - 6) < 15));
      }

      if (!collided) {
        continue;
      }

      if (chain > 0) {
        shotMultiPos[SHOT_MISC] = 0;
        b = port::player_shot_create(0, SHOT_MISC, tempShotX, tempShotY, player[0].mouseX, player[0].mouseY,
                                     chain, playerNum);
        shotAvail[z] = 0;
        goto draw_player_shot_loop_end;
      }

      infiniteShot = false;

      if (damage == 99) {
        damage = 0;
        doIced = 40;
        enemy[b].iced = 40;
      }
      else {
        doIced = 0;
        if (damage >= 250) {
          damage = damage - 250;
          infiniteShot = true;
        }
      }

      int armorleft = enemy[b].armorleft;

      temp = enemy[b].linknum;
      if (temp == 0) {
        temp = 255;
      }

      if (enemy[b].armorleft < 255) {
        for (unsigned int i = 0; i < COUNTOF(boss_bar); i++) {
          if (temp == boss_bar[i].link_num) {
            boss_bar[i].color = 6;
          }
        }

        if (enemy[b].enemyground) {
          enemy[b].filter = temp2;
        }

        for (unsigned int e = 0; e < COUNTOF(enemy); e++) {
          if (!(enemy[e].linknum == temp && enemyAvail[e] != 1 && enemy[e].enemyground != 0)) {
            continue;
          }

          if (doIced) {
            enemy[e].iced = doIced;
          }
          enemy[e].filter = temp2;
        }
      }

      if (armorleft > damage) {
        if (z != MAX_PWEAPON - 1) {
          if (enemy[b].armorleft != 255) {
            enemy[b].armorleft -= damage;
            setup_explosion(tempShotX, tempShotY, 0, 0, false, false);
          }
          else {
            do_sp(tempShotX + 6, tempShotY + 6, damage / 2 + 3, damage / 4 + 2, temp2);
          }
        }

        soundQueue[5] = S_ENEMY_HIT;

        if ((armorleft - damage <= enemy[b].edlevel) && ((!enemy[b].edamaged) ^ (enemy[b].edani < 0))) {
          for (temp3 = 0; temp3 < 100; temp3++) {
            if (enemyAvail[temp3] == 1) {
              continue;
            }

            int linknum = enemy[temp3].linknum;
            if (!((temp3 == b) || ((temp != 255) &&
                                   (((enemy[temp3].edlevel > 0) && (linknum == temp)) ||
                                    ((enemyContinualDamage && (temp - 100 == linknum)) ||
                                     ((linknum > 40) && (linknum / 20 == temp / 20) && (linknum <= temp)))))))
            {
              continue;
            }

            enemy[temp3].enemycycle = 1;

            enemy[temp3].edamaged = !enemy[temp3].edamaged;

            if (enemy[temp3].edani != 0) {
              enemy[temp3].ani = abs(enemy[temp3].edani);
              enemy[temp3].aniactive = 1;
              enemy[temp3].animax = 0;
              enemy[temp3].animin = enemy[temp3].edgr;
              enemy[temp3].enemycycle = enemy[temp3].animin - 1;
            }
            else if (enemy[temp3].edgr > 0) {
              enemy[temp3].egr[1 - 1] = enemy[temp3].edgr;
              enemy[temp3].ani = 1;
              enemy[temp3].aniactive = 0;
              enemy[temp3].animax = 0;
              enemy[temp3].animin = 1;
            }
            else {
              enemyAvail[temp3] = 1;
              enemyKilled++;
            }

            enemy[temp3].aniwhenfire = 0;

            if (enemy[temp3].armorleft > (unsigned char)enemy[temp3].edlevel) {
              enemy[temp3].armorleft = enemy[temp3].edlevel;
            }

            JE_integer tempX = enemy[temp3].ex + enemy[temp3].mapoffset;
            JE_integer tempY = enemy[temp3].ey;

            if (enemyDat[enemy[temp3].enemytype].esize != 1) {
              setup_explosion(tempX, tempY - 6, 0, 1, false, false);
            }
            else {
              setup_explosion_large(enemy[temp3].enemyground, enemy[temp3].explonum / 2, tempX, tempY);
            }
          }
        }
      }
      else {
        if ((temp == 254) && (superEnemy254Jump > 0)) {
          event_jump(superEnemy254Jump);
        }

        for (temp2 = 0; temp2 < 100; temp2++) {
          if (enemyAvail[temp2] == 1) {
            continue;
          }

          temp3 = enemy[temp2].linknum;
          if (!((temp2 == b) || (temp == 254) ||
                ((temp != 255) && ((temp == temp3) || (temp - 100 == temp3) ||
                                   ((temp3 > 40) && (temp3 / 20 == temp / 20) && (temp3 <= temp))))))
          {
            continue;
          }

          int enemy_screen_x = enemy[temp2].ex + enemy[temp2].mapoffset;

          if (enemy[temp2].special) {
            assert((unsigned int)enemy[temp2].flagnum - 1 < COUNTOF(globalFlags));
            globalFlags[enemy[temp2].flagnum - 1] = enemy[temp2].setto;
          }

          if ((enemy[temp2].enemydie > 0) &&
              !((superArcadeMode != SA_NONE) && (enemyDat[enemy[temp2].enemydie].value == 30000)))
          {
            int temp_b = b;
            tempW = enemy[temp2].enemydie;
            int enemy_offset = temp2 - (temp2 % 25);
            if (enemyDat[tempW].value > 30000) {
              enemy_offset = 0;
            }
            b = new_enemy(enemy_offset, tempW, 0);
            if (b != 0) {
              if ((superArcadeMode != SA_NONE) && (enemy[b - 1].evalue > 30000)) {
                superArcadePowerUp++;
                if (superArcadePowerUp > 5) {
                  superArcadePowerUp = 1;
                }
                enemy[b - 1].egr[1 - 1] = 5 + superArcadePowerUp * 2;
                enemy[b - 1].evalue = 30000 + superArcadePowerUp;
              }

              if (enemy[b - 1].evalue != 0) {
                enemy[b - 1].scoreitem = true;
              }
              else {
                enemy[b - 1].scoreitem = false;
              }

              enemy[b - 1].ex = enemy[temp2].ex;
              enemy[b - 1].ey = enemy[temp2].ey;
            }
            b = temp_b;
          }

          if ((enemy[temp2].evalue > 0) && (enemy[temp2].evalue < 10000)) {
            if (enemy[temp2].evalue == 1) {
              cubeMax++;
            }
            else {
              // in galaga mode player 2 is sidekick, so give cash to player 1
              player[galagaMode ? 0 : playerNum - 1].cash += enemy[temp2].evalue;
            }
          }

          if ((enemy[temp2].edlevel == -1) && (temp == temp3)) {
            enemy[temp2].edlevel = 0;
            enemyAvail[temp2] = 2;
            enemy[temp2].egr[1 - 1] = enemy[temp2].edgr;
            enemy[temp2].ani = 1;
            enemy[temp2].aniactive = 0;
            enemy[temp2].animax = 0;
            enemy[temp2].animin = 1;
            enemy[temp2].edamaged = true;
            enemy[temp2].enemycycle = 1;
          }
          else {
            enemyAvail[temp2] = 1;
            enemyKilled++;
          }

          if (enemyDat[enemy[temp2].enemytype].esize == 1) {
            setup_explosion_large(enemy[temp2].enemyground, enemy[temp2].explonum, enemy_screen_x,
                                  enemy[temp2].ey);
            soundQueue[6] = S_EXPLOSION_9;
          }
          else {
            setup_explosion(enemy_screen_x, enemy[temp2].ey, 0, 1, false, false);
            soundQueue[6] = S_EXPLOSION_8;
          }
        }
      }

      if (infiniteShot) {
        damage += 250;
      }
      else if (z != MAX_PWEAPON - 1) {
        if (damage <= armorleft) {
          shotAvail[z] = 0;
          goto draw_player_shot_loop_end;
        }
        else {
          playerShotData[z].shotDmg -= armorleft;
        }
      }
    }

  draw_player_shot_loop_end:;
  }
}

// The players: collisions, then movement and firing.
void players_tick() {
  // Player movement indicators for shots that track your ship
  for (uint i = 0; i < COUNTOF(player); ++i) {
    player[i].last_x_shot_move = player[i].x;
    player[i].last_y_shot_move = player[i].y;
  }

  // =================================
  // =======Collisions Detection======
  // =================================

  for (uint i = 0; i < (twoPlayerMode ? 2 : 1); ++i) {
    if (player[i].is_alive && !endLevel) {
      player_collide(&player[i], i + 1);
    }
  }

  if (firstGameOver) {
    main_game_player_functions();  // --------PLAYER DRAW+MOVEMENT---------
  }
}

// The enemy shots, and background 3 and the enemies over them.
void enemy_shots_tick() {
  DrawRecord& r = draw_record();
  int lastEnemyOnScreen;

  // MAIN DRAWING IS STOPPED STARTING HERE
  if (!endLevel) {
    // Draw Enemy Shots
    r.enemy_shots = EnemyShotsDrawn{VGAScreen, {}};
    for (int z = 0; z < ENEMY_SHOT_MAX; z++) {
      if (enemyShotAvail[z] != 0) {
        continue;
      }

      enemyShot[z].sxm += enemyShot[z].sxc;
      enemyShot[z].sx += enemyShot[z].sxm;

      if (enemyShot[z].tx != 0) {
        if (enemyShot[z].sx > player[0].x) {
          if (enemyShot[z].sxm > -enemyShot[z].tx) {
            enemyShot[z].sxm--;
          }
        }
        else {
          if (enemyShot[z].sxm < enemyShot[z].tx) {
            enemyShot[z].sxm++;
          }
        }
      }

      enemyShot[z].sym += enemyShot[z].syc;
      enemyShot[z].sy += enemyShot[z].sym;

      if (enemyShot[z].ty != 0) {
        if (enemyShot[z].sy > player[0].y) {
          if (enemyShot[z].sym > -enemyShot[z].ty) {
            enemyShot[z].sym--;
          }
        }
        else {
          if (enemyShot[z].sym < enemyShot[z].ty) {
            enemyShot[z].sym++;
          }
        }
      }

      if (enemyShot[z].duration-- == 0 || enemyShot[z].sy > 190 || enemyShot[z].sy <= -14 ||
          enemyShot[z].sx > 275 || enemyShot[z].sx <= 0)
      {
        enemyShotAvail[z] = true;
      }
      // check if shot collided with player
      else {
        for (uint i = 0; i < (twoPlayerMode ? 2 : 1); ++i) {
          if (!(player[i].is_alive && enemyShot[z].sx > player[i].x - (signed)player[i].shot_hit_area_x &&
                enemyShot[z].sx < player[i].x + (signed)player[i].shot_hit_area_x &&
                enemyShot[z].sy > player[i].y - (signed)player[i].shot_hit_area_y &&
                enemyShot[z].sy < player[i].y + (signed)player[i].shot_hit_area_y))
          {
            continue;
          }

          JE_integer tempX = enemyShot[z].sx;
          JE_integer tempY = enemyShot[z].sy;
          temp = enemyShot[z].sdmg;

          enemyShotAvail[z] = true;

          setup_explosion(tempX, tempY, 0, 0, false, false);

          if (player[i].invulnerable_ticks == 0 && (temp = player_damage(temp, &player[i])) > 0) {
            player[i].x_velocity += (enemyShot[z].sxm * temp) / 2;
            player[i].y_velocity += (enemyShot[z].sym * temp) / 2;
          }

          break;
        }

        if (enemyShotAvail[z] == false) {
          if (enemyShot[z].animax != 0 && ++enemyShot[z].animate >= enemyShot[z].animax) {
            enemyShot[z].animate = 0;
          }

          r.enemy_shots->drawn[z] = true;
        }
      }
    }
  }

  if (background3over == 1) {
    background_3_tick(VGAScreen, r.background_3_over_1);
  }

  // Draw Top Enemy
  if (topEnemyOver) {
    tempMapXOfs = (background3x1 == 0) ? oldMapX3Ofs : oldMapXOfs;
    tempBackMove = backMove3;
    draw_enemy_tick(75, r.top_enemies_over);
  }

  if (!skyEnemyOverAll) {
    return;
  }

  // Draw Sky Enemy
  lastEnemyOnScreen = enemyOnScreen;

  tempMapXOfs = mapX2Ofs;
  tempBackMove = 0;
  draw_enemy_tick(25, r.sky_enemies_over);

  if (enemyOnScreen != lastEnemyOnScreen) {
    return;
  }

  if (stopBackgroundNum == 2) {
    stopBackgroundNum = 9;
  }
}

// Sequenced explosions, and the explosions.
void explosions_tick() {
  DrawRecord& r = draw_record();

  // -------------------------- Sequenced Explosions -------------------------
  enemyStillExploding = false;
  for (int i = 0; i < MAX_REPEATING_EXPLOSIONS; i++) {
    if (rep_explosions[i].ttl == 0) {
      continue;
    }

    enemyStillExploding = true;

    if (rep_explosions[i].delay > 0) {
      rep_explosions[i].delay--;
      continue;
    }

    rep_explosions[i].y += backMove2 + 1;
    JE_integer tempX = rep_explosions[i].x + (mt_rand() % 24) - 12;
    JE_integer tempY = rep_explosions[i].y + (mt_rand() % 27) - 24;

    if (rep_explosions[i].big) {
      setup_explosion_large(false, 2, tempX, tempY);

      if (rep_explosions[i].ttl == 1 || mt_rand() % 5 == 1) {
        soundQueue[7] = S_EXPLOSION_11;
      }
      else {
        soundQueue[6] = S_EXPLOSION_9;
      }

      rep_explosions[i].delay = 4 + (mt_rand() % 3);
    }
    else {
      setup_explosion(tempX, tempY, 0, 1, false, false);

      soundQueue[5] = S_EXPLOSION_4;

      rep_explosions[i].delay = 3;
    }

    rep_explosions[i].ttl--;
  }

  // ---------------------------- Draw Explosions ----------------------------
  r.explosions = ExplosionsDrawn{VGAScreen, sim::update_explosions(state().explosions)};

  if (!portConfigChange) {
    portConfigDone = true;
  }
}

// Background 2 over everything, the low armor warning, and random explosions.
void warning_tick() {
  DrawRecord& r = draw_record();

  // -----------------------BACKGROUNDS------------------------
  // -----------------------BACKGROUND 2------------------------
  if (!(smoothies[2 - 1] && processorType < 4) && !(smoothies[1 - 1] && processorType == 3)) {
    if (background2over == 2) {
      if (wild && !background2notTransparent) {
        background_2_tick(VGAScreen, true, r.background_2_over_2);
      }
      else {
        background_2_tick(VGAScreen, false, r.background_2_over_2);
      }
    }
  }

  // -------------------------Warning---------------------------
  if ((player[0].is_alive && player[0].armor < 6) ||
      (twoPlayerMode && !galagaMode && player[1].is_alive && player[1].armor < 6))
  {
    int armor_amount = (player[0].is_alive && player[0].armor < 6) ? player[0].armor : player[1].armor;

    if (armorShipDelay > 0) {
      armorShipDelay--;
    }
    else {
      tempW = 560;
      b = new_enemy(50, tempW, 0);
      if (b > 0) {
        enemy[b - 1].enemydie = 560 + (mt_rand() % 3) + 1;
        enemy[b - 1].eyc -= backMove3;
        enemy[b - 1].armorleft = 4;
      }
      armorShipDelay = 500;
    }

    if ((player[0].is_alive && player[0].armor < 6 && (!isNetworkGame || thisPlayerNum == 1)) ||
        (twoPlayerMode && player[1].is_alive && player[1].armor < 6 &&
         (!isNetworkGame || thisPlayerNum == 2)))
    {
      tempW = armor_amount * 4 + 8;
      if (warningSoundDelay > tempW) {
        warningSoundDelay = tempW;
      }

      if (warningSoundDelay > 1) {
        warningSoundDelay--;
      }
      else {
        soundQueue[7] = S_WARNING;
        warningSoundDelay = tempW;
      }

      warningCol += warningColChange;
      if (warningCol > 113 + (14 - (armor_amount * 2))) {
        warningColChange = -warningColChange;
        warningCol = 113 + (14 - (armor_amount * 2));
      }
      else if (warningCol < 113) {
        warningColChange = -warningColChange;
      }
      r.warning = VGAScreen;
    }
  }

  // ------- Random Explosions --------
  if (randomExplosions && mt_rand() % 10 == 1) {
    setup_explosion_large(false, 20, mt_rand() % 280, mt_rand() % 180);
  }
}

// The sound routine, and the return event.
void sound_tick() {
  // =================================
  // =======The Sound Routine=========
  // =================================
  if (firstGameOver) {
    temp = 0;
    for (temp2 = 0; temp2 < COUNTOF(soundQueue); temp2++) {
      if (soundQueue[temp2] == S_NONE) {
        continue;
      }

      temp = soundQueue[temp2];
      if (temp2 == 3) {
        temp3 = fxPlayVol;
      }
      else if (temp == 15) {
        temp3 = fxPlayVol / 4;
      }
      // Lightning
      else {
        temp3 = fxPlayVol / 2;
      }

      multiSamplePlay(soundSamples[temp - 1], soundSampleCount[temp - 1], temp2, temp3);

      soundQueue[temp2] = S_NONE;
    }
  }

  if (returnActive && enemyOnScreen == 0) {
    event_jump(65535);
    returnActive = false;
  }
}

// The debug display, the bonus message, the speed mode and the level timer.
void texts_tick() {
  DrawRecord& r = draw_record();
  char buffer[256];

  // -------      DEbug      ---------
  debugTime = SDL_GetTicks();

  if (debug) {
    for (size_t i = 0; i < 9; i++) {
      tempStr[i] = '0' + smoothies[i];
    }
    tempStr[9] = '\0';
    r.debug = DebugDrawn{VGAScreen, {}};
    sprintf(buffer, "SM = %s", tempStr);
    r.debug->text[0] = buffer;

    sprintf(buffer, "Memory left = %d", -1);
    r.debug->text[1] = buffer;
    sprintf(buffer, "Enemies onscreen = %d", enemyOnScreen);
    r.debug->text[2] = buffer;

    debugHist += debugTime - lastDebugTime;
    debugHistCount++;
    sprintf(tempStr, "%2.3f", debugHistCount / (debugHist * 1e-3f));
    sprintf(buffer, "X:%d Y:%-5d  %s FPS  %d %d %d %d", (mapX - 1) * 12 + player[0].x, curLoc, tempStr,
            player[0].x_velocity, player[0].y_velocity, player[0].x, player[0].y);
    r.debug->text[3] = buffer;
    lastDebugTime = debugTime;
  }

  if (displayTime > 0) {
    displayTime--;
    r.message = MessageDrawn{VGAScreen, flash};
    flash += flashChange;
    if (flash > 4 || flash == 0) {
      flashChange = -flashChange;
    }
  }

  // Pentium Speed Mode?
  if (pentiumMode) {
    frameCountMax = (frameCountMax == 2) ? 3 : 2;
  }

  // --------  Level Timer    ---------
  if (!levelTimer || levelTimerCountdown <= 0) {
    return;
  }

  levelTimerCountdown--;
  if (levelTimerCountdown == 0) {
    event_jump(levelTimerJumpTo);
  }

  if (levelTimerCountdown > 200) {
    if (levelTimerCountdown % 100 == 0) {
      soundQueue[7] = S_WARNING;
    }

    if (levelTimerCountdown % 10 == 0) {
      soundQueue[6] = S_CLICK;
    }
  }
  else if (levelTimerCountdown % 20 == 0) {
    soundQueue[7] = S_WARNING;
  }

  r.level_timer = VGAScreen;
}

// Game over, and the input for stopping demos, pausing, the menu and cheats.
// Returns true if the in-game menu or help opened, which ends the tick
// without a frame (the C code jumps back to level_loop).
bool game_over_and_input_tick() {
  DrawRecord& r = draw_record();

  // GAME OVER
  if (!constantPlay && !constantDie && allPlayersGone) {
    if (player[0].exploding_ticks > 0 || player[1].exploding_ticks > 0) {
      if (galagaMode) {
        player[1].exploding_ticks = 0;
      }

      musicFade = true;
    }
    else {
      if (playDemo || normalBonusLevelCurrent || bonusLevelCurrent) {
        reallyEndLevel = true;
      }
      else {
        r.game_over = VGAScreen;  // game over
      }

      if (firstGameOver) {
        if (!playDemo) {
          play_song(SONG_GAMEOVER);
          set_volume(tyrMusicVolume, fxVolume);
        }
        firstGameOver = false;
      }

      if (!playDemo) {
        push_joysticks_as_keyboard();

        if (hasInput(INPUT_NO_MOTION)) {
          reallyEndLevel = true;
        }
      }

      if (isNetworkGame) {
        reallyEndLevel = true;
      }
    }
  }

  // input stops demo
  if (playDemo) {
    push_joysticks_as_keyboard();

    if (hasInput(INPUT_NO_MOTION)) {
      reallyEndLevel = true;

      stoppedDemo = true;
    }
  }
  // input handling for pausing, menu, cheats
  else {
    // Ensure gameplay input does not affect pause or menu.
    mouseClearInput();

    if (keyboardHasInput()) {
      // Pause, menu, and cheats are triggered on keysactive, so this is fine.
      keyboardClearInput();

      skipStarShowVGA = false;
      if (keysactive[SDL_SCANCODE_F1]) {
        in_game_help_ahead();  // main_keyboard_input opens help
      }
      main_keyboard_input();
      if (skipStarShowVGA) {
        return true;
      }
    }

#ifdef NDEBUG
    if (pause_pressed || !windowHasFocus)
#else
    if (pause_pressed)
#endif
    {
      pause_pressed = false;

      if (isNetworkGame) {
        pauseRequest = true;
      }
      else {
        JE_pauseGame();
      }
    }

    if (ingamemenu_pressed) {
      ingamemenu_pressed = false;

      if (isNetworkGame) {
        inGameMenuRequest = true;
      }
      else {
        yourInGameMenuRequest = true;
        JE_doInGameSetup();
        skipStarShowVGA = true;
      }
    }
  }

  return false;
}

LevelPass simulate(bool& frame) {
  DrawRecord& r = draw_record();
  r = DrawRecord{};

  if (isNetworkGame) {
    return LevelPass::unsupported;
  }

  tick_start();

  hud_tick();

  // use game_screen for all the generic drawing functions
  VGAScreen = game_screen;

  // ---------------------------EVENTS-------------------------
  while (eventRec[eventLoc - 1].eventtime <= curLoc && eventLoc <= maxEvent) {
    event_system();
  }

  if (isNetworkGame && reallyEndLevel) {
    return LevelPass::start_level;
  }

  backgrounds_under_tick();

  enemies_under_tick();

  player_shots_tick();

  players_tick();

  enemy_shots_tick();

  explosions_tick();

  warning_tick();

  sound_tick();

  texts_tick();

  if (game_over_and_input_tick()) {
    return LevelPass::level_loop;
  }

  // Network games aren't supported (level_pass returns early).

  // Test
  superpixels_tick();

  // Filtration
  if (filterActive) {
    filter_screen_tick();
  }

  boss_bar_tick();

  r.in_game_displays = VGAScreen;

  // The C code makes these changes after JE_starShowVGA; they read no input
  // and draw nothing, so they can come before the frame.
  // Start backgrounds if no enemies on screen
  // End level if number of enemies left to kill equals 0.
  if (stopBackgroundNum == 9 && backMove == 0 && !enemyStillExploding) {
    backMove = 1;
    backMove2 = 2;
    backMove3 = 3;
    state().explosions.move = 2;
    stopBackgroundNum = 0;
    stopBackgrounds = false;
    if (waitToEndLevel) {
      endLevel = true;
      levelEnd = 40;
    }
    if (allPlayersGone) {
      reallyEndLevel = true;
    }
  }

  if (!endLevel && enemyOnScreen == 0) {
    if (readyToEndLevel && !enemyStillExploding) {
      if (levelTimerCountdown > 0) {
        levelTimer = false;
      }
      readyToEndLevel = false;
      endLevel = true;
      levelEnd = 40;
      if (allPlayersGone) {
        reallyEndLevel = true;
      }
    }
    if (stopBackgrounds) {
      stopBackgrounds = false;
      backMove = 1;
      backMove2 = 2;
      backMove3 = 3;
      state().explosions.move = 2;
    }
  }

  VGAScreen = VGAScreenSeg;  // side-effect of game_screen

  r.show = !playerEndLevel && !skipStarShowVGA;  // JE_starShowVGA copies the playing area to the screen
  frame = true;

  // Other Network Functions
  JE_handleChat();

  if (reallyEndLevel) {
    return LevelPass::start_level;
  }

  return LevelPass::level_loop;
}

}  // namespace

}  // namespace tyrx::port
