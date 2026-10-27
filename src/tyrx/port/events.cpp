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

// The level's events (JE_eventSystem, and JE_createNewEventEnemy, JE_eventJump
// and JE_searchFor, in tyrian2.c).  The one thing an event draws is a message
// in the message bar (draw_text_window_tick).

#include "port.hpp"

#include "../../logging.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

namespace tyrx::port {

// Port of JE_eventSystem.
void event_system() {
  switch (eventRec[eventLoc - 1].eventtype) {
    case 1:
      starfield_speed = eventRec[eventLoc - 1].eventdat;
      break;

    case 2:
      map1YDelay = 1;
      map1YDelayMax = 1;
      map2YDelay = 1;
      map2YDelayMax = 1;

      backMove = eventRec[eventLoc - 1].eventdat;
      backMove2 = eventRec[eventLoc - 1].eventdat2;

      if (backMove2 > 0) {
        state().explosions.move = backMove2;
      }
      else {
        state().explosions.move = backMove;
      }

      backMove3 = eventRec[eventLoc - 1].eventdat3;

      if (backMove > 0) {
        stopBackgroundNum = 0;
      }
      break;

    case 3:
      backMove = 1;
      map1YDelay = 3;
      map1YDelayMax = 3;
      backMove2 = 1;
      map2YDelay = 2;
      map2YDelayMax = 2;
      backMove3 = 1;
      break;

    case 4:
      stopBackgrounds = true;
      switch (eventRec[eventLoc - 1].eventdat) {
        case 0:
        case 1:
          stopBackgroundNum = 1;
          break;
        case 2:
          stopBackgroundNum = 2;
          break;
        case 3:
          stopBackgroundNum = 3;
          break;
      }
      break;

    case 5: {  // load enemy shape banks
      Uint8 const newEnemyShapeTables[] = {
          (Uint8)(eventRec[eventLoc - 1].eventdat > 0 ? eventRec[eventLoc - 1].eventdat : 0),
          (Uint8)(eventRec[eventLoc - 1].eventdat2 > 0 ? eventRec[eventLoc - 1].eventdat2 : 0),
          (Uint8)(eventRec[eventLoc - 1].eventdat3 > 0 ? eventRec[eventLoc - 1].eventdat3 : 0),
          (Uint8)(eventRec[eventLoc - 1].eventdat4 > 0 ? eventRec[eventLoc - 1].eventdat4 : 0),
      };

      for (unsigned int i = 0; i < COUNTOF(newEnemyShapeTables); ++i) {
        if (enemySpriteSheetIds[i] == newEnemyShapeTables[i]) {
          continue;
        }

        if (newEnemyShapeTables[i] > 0) {
          assert(newEnemyShapeTables[i] <= COUNTOF(shapeFile));
          JE_loadCompShapes(&enemySpriteSheets[i], shapeFile[newEnemyShapeTables[i] - 1]);
        }
        else {
          free_sprite2s(&enemySpriteSheets[i]);
        }

        enemySpriteSheetIds[i] = newEnemyShapeTables[i];
      }
    } break;

    case 6:  // Ground Enemy
      create_new_event_enemy(0, 25, 0);
      break;

    case 7:  // Top Enemy
      create_new_event_enemy(0, 50, 0);
      break;

    case 8:
      starActive = false;
      break;

    case 9:
      starActive = true;
      break;

    case 10:  // Ground Enemy 2
      create_new_event_enemy(0, 75, 0);
      break;

    case 11:
      if (allPlayersGone || eventRec[eventLoc - 1].eventdat == 1) {
        reallyEndLevel = true;
      }
      else if (!endLevel) {
        readyToEndLevel = false;
        endLevel = true;
        levelEnd = 40;
      }
      break;

    case 12: {  // Custom 4x4 Ground Enemy
      uint temp = 0;
      switch (eventRec[eventLoc - 1].eventdat6) {
        case 0:
        case 1:
          temp = 25;
          break;
        case 2:
          temp = 0;
          break;
        case 3:
          temp = 50;
          break;
        case 4:
          temp = 75;
          break;
      }
      eventRec[eventLoc - 1].eventdat6 = 0;  // We use EVENTDAT6 for the background
      create_new_event_enemy(0, temp, 0);
      create_new_event_enemy(1, temp, 0);
      if (b > 0) {
        enemy[b - 1].ex += 24;
      }
      create_new_event_enemy(2, temp, 0);
      if (b > 0) {
        enemy[b - 1].ey -= 28;
      }
      create_new_event_enemy(3, temp, 0);
      if (b > 0) {
        enemy[b - 1].ex += 24;
        enemy[b - 1].ey -= 28;
      }
      break;
    }
    case 13:
      enemiesActive = false;
      break;

    case 14:
      enemiesActive = true;
      break;

    case 15:  // Sky Enemy
      create_new_event_enemy(0, 0, 0);
      break;

    case 16: {  // Window Text
      Sint16 id = eventRec[eventLoc - 1].eventdat;
      if (id < 1 || id > 9) {
        logWarn("Event %05d %d has invalid window text ID %d.", eventRec[eventLoc - 1].eventtime,
                eventRec[eventLoc - 1].eventtype, id);
        break;
      }
      draw_text_window_tick(outputs[id - 1]);
      soundQueue[3] = windowTextSamples[id - 1];
      break;
    }

    case 17:  // Ground Bottom
      create_new_event_enemy(0, 25, 0);
      if (b > 0) {
        enemy[b - 1].ey = 190 + eventRec[eventLoc - 1].eventdat5;
      }
      break;

    case 18:  // Sky Enemy on Bottom
      create_new_event_enemy(0, 0, 0);
      if (b > 0) {
        enemy[b - 1].ey = 190 + eventRec[eventLoc - 1].eventdat5;
      }
      break;

    case 19: {  // Enemy Global Move
      int initial_i = 0, max_i = 0;
      bool all_enemies = false;

      if (eventRec[eventLoc - 1].eventdat3 > 79 && eventRec[eventLoc - 1].eventdat3 < 90) {
        initial_i = 0;
        max_i = 100;
        all_enemies = false;
        eventRec[eventLoc - 1].eventdat4 = newPL[eventRec[eventLoc - 1].eventdat3 - 80];
      }
      else {
        switch (eventRec[eventLoc - 1].eventdat3) {
          case 0:
            initial_i = 0;
            max_i = 100;
            all_enemies = false;
            break;
          case 2:
            initial_i = 0;
            max_i = 25;
            all_enemies = true;
            break;
          case 1:
            initial_i = 25;
            max_i = 50;
            all_enemies = true;
            break;
          case 3:
            initial_i = 50;
            max_i = 75;
            all_enemies = true;
            break;
          case 99:
            initial_i = 0;
            max_i = 100;
            all_enemies = true;
            break;
        }
      }

      for (int i = initial_i; i < max_i; i++) {
        if (!all_enemies && enemy[i].linknum != eventRec[eventLoc - 1].eventdat4) {
          continue;
        }

        if (eventRec[eventLoc - 1].eventdat != -99) {
          enemy[i].exc = eventRec[eventLoc - 1].eventdat;
        }

        if (eventRec[eventLoc - 1].eventdat2 != -99) {
          enemy[i].eyc = eventRec[eventLoc - 1].eventdat2;
        }

        if (eventRec[eventLoc - 1].eventdat6 != 0) {
          enemy[i].fixedmovey = eventRec[eventLoc - 1].eventdat6;
        }

        if (eventRec[eventLoc - 1].eventdat6 == -99) {
          enemy[i].fixedmovey = 0;
        }

        if (eventRec[eventLoc - 1].eventdat5 > 0) {
          enemy[i].enemycycle = eventRec[eventLoc - 1].eventdat5;
        }
      }
      break;
    }

    case 20:  // Enemy Global Accel
      if (eventRec[eventLoc - 1].eventdat3 > 79 && eventRec[eventLoc - 1].eventdat3 < 90) {
        eventRec[eventLoc - 1].eventdat4 = newPL[eventRec[eventLoc - 1].eventdat3 - 80];
      }

      for (temp = 0; temp < 100; temp++) {
        if (!(enemyAvail[temp] != 1 && (enemy[temp].linknum == eventRec[eventLoc - 1].eventdat4 ||
                                        eventRec[eventLoc - 1].eventdat4 == 0)))
        {
          continue;
        }

        if (eventRec[eventLoc - 1].eventdat != -99) {
          enemy[temp].excc = eventRec[eventLoc - 1].eventdat;
          enemy[temp].exccw = abs(eventRec[eventLoc - 1].eventdat);
          enemy[temp].exccwmax = abs(eventRec[eventLoc - 1].eventdat);
          if (eventRec[eventLoc - 1].eventdat > 0) {
            enemy[temp].exccadd = 1;
          }
          else {
            enemy[temp].exccadd = -1;
          }
        }

        if (eventRec[eventLoc - 1].eventdat2 != -99) {
          enemy[temp].eycc = eventRec[eventLoc - 1].eventdat2;
          enemy[temp].eyccw = abs(eventRec[eventLoc - 1].eventdat2);
          enemy[temp].eyccwmax = abs(eventRec[eventLoc - 1].eventdat2);
          if (eventRec[eventLoc - 1].eventdat2 > 0) {
            enemy[temp].eyccadd = 1;
          }
          else {
            enemy[temp].eyccadd = -1;
          }
        }

        if (eventRec[eventLoc - 1].eventdat5 > 0) {
          enemy[temp].enemycycle = eventRec[eventLoc - 1].eventdat5;
        }
        if (eventRec[eventLoc - 1].eventdat6 > 0) {
          enemy[temp].ani = eventRec[eventLoc - 1].eventdat6;
          enemy[temp].animin = eventRec[eventLoc - 1].eventdat5;
          enemy[temp].animax = 0;
          enemy[temp].aniactive = 1;
        }
      }
      break;

    case 21:
      background3over = 1;
      break;

    case 22:
      background3over = 0;
      break;

    case 23:  // Sky Enemy on Bottom
      create_new_event_enemy(0, 50, 0);
      if (b > 0) {
        enemy[b - 1].ey = 180 + eventRec[eventLoc - 1].eventdat5;
      }
      break;

    case 24:  // Enemy Global Animate
      for (temp = 0; temp < 100; temp++) {
        if (enemy[temp].linknum != eventRec[eventLoc - 1].eventdat4) {
          continue;
        }

        enemy[temp].aniactive = 1;
        enemy[temp].aniwhenfire = 0;
        if (eventRec[eventLoc - 1].eventdat2 > 0) {
          enemy[temp].enemycycle = eventRec[eventLoc - 1].eventdat2;
          enemy[temp].animin = enemy[temp].enemycycle;
        }
        else {
          enemy[temp].enemycycle = 0;
        }

        if (eventRec[eventLoc - 1].eventdat > 0) {
          enemy[temp].ani = eventRec[eventLoc - 1].eventdat;
        }

        if (eventRec[eventLoc - 1].eventdat3 == 1) {
          enemy[temp].animax = enemy[temp].ani;
        }
        else if (eventRec[eventLoc - 1].eventdat3 == 2) {
          enemy[temp].aniactive = 2;
          enemy[temp].animax = enemy[temp].ani;
          enemy[temp].aniwhenfire = 2;
        }
      }
      break;

    case 25:  // Enemy Global Damage change
      for (temp = 0; temp < 100; temp++) {
        if (eventRec[eventLoc - 1].eventdat4 != 0 && enemy[temp].linknum != eventRec[eventLoc - 1].eventdat4)
        {
          continue;
        }

        if (galagaMode) {
          enemy[temp].armorleft = roundf(eventRec[eventLoc - 1].eventdat * (difficultyLevel / 2));
        }
        else {
          enemy[temp].armorleft = eventRec[eventLoc - 1].eventdat;
        }
      }
      break;

    case 26:
      smallEnemyAdjust = eventRec[eventLoc - 1].eventdat;
      break;

    case 27:  // Enemy Global AccelRev
      if (eventRec[eventLoc - 1].eventdat3 > 79 && eventRec[eventLoc - 1].eventdat3 < 90) {
        eventRec[eventLoc - 1].eventdat4 = newPL[eventRec[eventLoc - 1].eventdat3 - 80];
      }

      for (temp = 0; temp < 100; temp++) {
        if (eventRec[eventLoc - 1].eventdat4 != 0 && enemy[temp].linknum != eventRec[eventLoc - 1].eventdat4)
        {
          continue;
        }

        if (eventRec[eventLoc - 1].eventdat != -99) {
          enemy[temp].exrev = eventRec[eventLoc - 1].eventdat;
        }
        if (eventRec[eventLoc - 1].eventdat2 != -99) {
          enemy[temp].eyrev = eventRec[eventLoc - 1].eventdat2;
        }
        if (eventRec[eventLoc - 1].eventdat3 != 0 && eventRec[eventLoc - 1].eventdat3 < 17) {
          enemy[temp].filter = eventRec[eventLoc - 1].eventdat3;
        }
      }
      break;

    case 28:
      topEnemyOver = false;
      break;

    case 29:
      topEnemyOver = true;
      break;

    case 30:
      map1YDelay = 1;
      map1YDelayMax = 1;
      map2YDelay = 1;
      map2YDelayMax = 1;

      backMove = eventRec[eventLoc - 1].eventdat;
      backMove2 = eventRec[eventLoc - 1].eventdat2;
      state().explosions.move = backMove2;
      backMove3 = eventRec[eventLoc - 1].eventdat3;
      break;

    case 31:  // Enemy Fire Override
      for (temp = 0; temp < 100; temp++) {
        if (eventRec[eventLoc - 1].eventdat4 != 99 && enemy[temp].linknum != eventRec[eventLoc - 1].eventdat4)
        {
          continue;
        }

        enemy[temp].freq[1 - 1] = eventRec[eventLoc - 1].eventdat;
        enemy[temp].freq[2 - 1] = eventRec[eventLoc - 1].eventdat2;
        enemy[temp].freq[3 - 1] = eventRec[eventLoc - 1].eventdat3;
        for (temp2 = 0; temp2 < 3; temp2++) {
          enemy[temp].eshotwait[temp2] = 1;
        }
        if (enemy[temp].launchtype > 0) {
          enemy[temp].launchfreq = eventRec[eventLoc - 1].eventdat5;
          enemy[temp].launchwait = 1;
        }
      }
      break;

    case 32:  // create enemy
      create_new_event_enemy(0, 50, 0);
      if (b > 0) {
        enemy[b - 1].ey = 190;
      }
      break;

    case 33:  // Enemy From other Enemies
      if (!((eventRec[eventLoc - 1].eventdat == 512 || eventRec[eventLoc - 1].eventdat == 513) &&
            (twoPlayerMode || onePlayerAction || superTyrian)))
      {
        if (superArcadeMode != SA_NONE) {
          if (eventRec[eventLoc - 1].eventdat == 534) {
            eventRec[eventLoc - 1].eventdat = 827;
          }
        }
        else if (!superTyrian) {
          Uint8 const lives = *player[0].lives;

          if (eventRec[eventLoc - 1].eventdat == 533 && (lives == 11 || (mt_rand() % 15) < lives)) {
            // enemy will drop random special weapon
            eventRec[eventLoc - 1].eventdat = 829 + (mt_rand() % 6);
          }
        }
        if (eventRec[eventLoc - 1].eventdat == 534 && superTyrian) {
          eventRec[eventLoc - 1].eventdat = 828 + superTyrianSpecials[mt_rand() % 4];
        }

        for (temp = 0; temp < 100; temp++) {
          if (enemy[temp].linknum == eventRec[eventLoc - 1].eventdat4) {
            enemy[temp].enemydie = eventRec[eventLoc - 1].eventdat;
          }
        }
      }
      break;

    case 34:  // Start Music Fade
      if (firstGameOver) {
        musicFade = true;
        tempVolume = tyrMusicVolume;
      }
      break;

    case 35:  // Play new song
      if (firstGameOver) {
        play_song(eventRec[eventLoc - 1].eventdat - 1);
        set_volume(tyrMusicVolume, fxVolume);
      }
      musicFade = false;
      break;

    case 36:
      readyToEndLevel = true;
      break;

    case 37:
      levelEnemyFrequency = eventRec[eventLoc - 1].eventdat;
      break;

    case 38: {
      curLoc = eventRec[eventLoc - 1].eventdat;
      int new_event_loc = 1;
      for (tempW = 0; tempW < maxEvent; tempW++) {
        if (eventRec[tempW].eventtime <= curLoc) {
          new_event_loc = tempW + 1 - 1;
        }
      }
      eventLoc = new_event_loc;
      break;
    }

    case 39:  // Enemy Global Linknum Change
      for (temp = 0; temp < 100; temp++) {
        if (enemy[temp].linknum == eventRec[eventLoc - 1].eventdat) {
          enemy[temp].linknum = eventRec[eventLoc - 1].eventdat2;
        }
      }
      break;

    case 40:  // Enemy Continual Damage
      enemyContinualDamage = true;
      break;

    case 41:
      if (eventRec[eventLoc - 1].eventdat == 0) {
        memset(enemyAvail, 1, sizeof(enemyAvail));
      }
      else {
        for (x = 0; x <= 24; x++) {
          enemyAvail[x] = 1;
        }
      }
      break;

    case 42:
      background3over = 2;
      break;

    case 43:
      background2over = eventRec[eventLoc - 1].eventdat;
      break;

    case 44:
      filterActive = (eventRec[eventLoc - 1].eventdat > 0);
      filterFade = (eventRec[eventLoc - 1].eventdat == 2);
      levelFilter = eventRec[eventLoc - 1].eventdat2;
      levelBrightness = eventRec[eventLoc - 1].eventdat3;
      levelFilterNew = eventRec[eventLoc - 1].eventdat4;
      levelBrightnessChg = eventRec[eventLoc - 1].eventdat5;
      filterFadeStart = (eventRec[eventLoc - 1].eventdat6 == 0);
      break;

    case 45:  // arcade-only enemy from other enemies
      if (!superTyrian) {
        Uint8 const lives = *player[0].lives;

        if (eventRec[eventLoc - 1].eventdat == 533 && (lives == 11 || (mt_rand() % 15) < lives)) {
          eventRec[eventLoc - 1].eventdat = 829 + (mt_rand() % 6);
        }
        if (twoPlayerMode || onePlayerAction) {
          for (temp = 0; temp < 100; temp++) {
            if (enemy[temp].linknum == eventRec[eventLoc - 1].eventdat4) {
              enemy[temp].enemydie = eventRec[eventLoc - 1].eventdat;
            }
          }
        }
      }
      break;

    case 46:  // change difficulty
      if (eventRec[eventLoc - 1].eventdat3 != 0) {
        damageRate = eventRec[eventLoc - 1].eventdat3;
      }

      if (eventRec[eventLoc - 1].eventdat2 == 0 || twoPlayerMode || onePlayerAction) {
        difficultyLevel += eventRec[eventLoc - 1].eventdat;
        if (difficultyLevel < DIFFICULTY_EASY) {
          difficultyLevel = DIFFICULTY_EASY;
        }
        if (difficultyLevel > DIFFICULTY_10) {
          difficultyLevel = DIFFICULTY_10;
        }
      }
      break;

    case 47:  // Enemy Global AccelRev
      for (temp = 0; temp < 100; temp++) {
        if (eventRec[eventLoc - 1].eventdat4 == 0 || enemy[temp].linknum == eventRec[eventLoc - 1].eventdat4)
        {
          enemy[temp].armorleft = eventRec[eventLoc - 1].eventdat;
        }
      }
      break;

    case 48:  // Background 2 Cannot be Transparent
      background2notTransparent = true;
      break;

    case 49:
    case 50:
    case 51:
    case 52:
      tempDat2 = eventRec[eventLoc - 1].eventdat;
      eventRec[eventLoc - 1].eventdat = 0;
      tempDat = eventRec[eventLoc - 1].eventdat3;
      eventRec[eventLoc - 1].eventdat3 = 0;
      tempDat3 = eventRec[eventLoc - 1].eventdat6;
      eventRec[eventLoc - 1].eventdat6 = 0;
      enemyDat[0].armor = tempDat3;
      enemyDat[0].egraphic[1 - 1] = tempDat2;
      switch (eventRec[eventLoc - 1].eventtype - 48) {
        case 1:
          temp = 25;
          break;
        case 2:
          temp = 0;
          break;
        case 3:
          temp = 50;
          break;
        case 4:
          temp = 75;
          break;
      }
      create_new_event_enemy(0, temp, tempDat);
      eventRec[eventLoc - 1].eventdat = tempDat2;
      eventRec[eventLoc - 1].eventdat3 = tempDat;
      eventRec[eventLoc - 1].eventdat6 = tempDat3;
      break;

    case 53:
      forceEvents = (eventRec[eventLoc - 1].eventdat != 99);
      break;

    case 54:
      event_jump(eventRec[eventLoc - 1].eventdat);
      break;

    case 55:  // Enemy Global AccelRev
      if (eventRec[eventLoc - 1].eventdat3 > 79 && eventRec[eventLoc - 1].eventdat3 < 90) {
        eventRec[eventLoc - 1].eventdat4 = newPL[eventRec[eventLoc - 1].eventdat3 - 80];
      }

      for (temp = 0; temp < 100; temp++) {
        if (eventRec[eventLoc - 1].eventdat4 != 0 && enemy[temp].linknum != eventRec[eventLoc - 1].eventdat4)
        {
          continue;
        }

        if (eventRec[eventLoc - 1].eventdat != -99) {
          enemy[temp].xaccel = eventRec[eventLoc - 1].eventdat;
        }
        if (eventRec[eventLoc - 1].eventdat2 != -99) {
          enemy[temp].yaccel = eventRec[eventLoc - 1].eventdat2;
        }
      }
      break;

    case 56:  // Ground2 Bottom
      create_new_event_enemy(0, 75, 0);
      if (b > 0) {
        enemy[b - 1].ey = 190;
      }
      break;

    case 57:
      superEnemy254Jump = eventRec[eventLoc - 1].eventdat;
      break;

    case 60:  // Assign Special Enemy
      for (temp = 0; temp < 100; temp++) {
        if (enemy[temp].linknum == eventRec[eventLoc - 1].eventdat4) {
          enemy[temp].special = true;
          enemy[temp].flagnum = eventRec[eventLoc - 1].eventdat;
          enemy[temp].setto = (eventRec[eventLoc - 1].eventdat2 == 1);
        }
      }
      break;

    case 61:  // if specific flag set to specific value, skip events
      if (globalFlags[eventRec[eventLoc - 1].eventdat - 1] == eventRec[eventLoc - 1].eventdat2) {
        eventLoc += eventRec[eventLoc - 1].eventdat3;
      }
      break;

    case 62:  // Play sound effect
      soundQueue[3] = eventRec[eventLoc - 1].eventdat;
      break;

    case 63:  // skip events if not in 2-player mode
      if (!twoPlayerMode && !onePlayerAction) {
        eventLoc += eventRec[eventLoc - 1].eventdat;
      }
      break;

    case 64:
      if (!(eventRec[eventLoc - 1].eventdat == 6 && twoPlayerMode && difficultyLevel > DIFFICULTY_NORMAL)) {
        smoothies[eventRec[eventLoc - 1].eventdat - 1] = eventRec[eventLoc - 1].eventdat2;
        temp = eventRec[eventLoc - 1].eventdat;
        if (temp == 5) {
          temp = 3;
        }
        smoothie_data[temp - 1] = eventRec[eventLoc - 1].eventdat3;
      }
      break;

    case 65:
      background3x1 = (eventRec[eventLoc - 1].eventdat == 0);
      break;

    case 66:  // If not on this difficulty level or higher then...
      if (initialDifficulty <= eventRec[eventLoc - 1].eventdat) {
        eventLoc += eventRec[eventLoc - 1].eventdat2;
      }
      break;

    case 67:
      levelTimer = (eventRec[eventLoc - 1].eventdat == 1);
      levelTimerCountdown = eventRec[eventLoc - 1].eventdat3 * 100;
      levelTimerJumpTo = eventRec[eventLoc - 1].eventdat2;
      break;

    case 68:
      randomExplosions = (eventRec[eventLoc - 1].eventdat == 1);
      break;

    case 69:
      for (uint i = 0; i < COUNTOF(player); ++i) {
        player[i].invulnerable_ticks = eventRec[eventLoc - 1].eventdat;
      }
      break;

    case 70:
      // 1-10
      if (eventRec[eventLoc - 1].eventdat2 == 0) {
        bool found = false;

        for (temp = 1; temp <= 19; temp++) {
          found = found || search_for(temp, NULL);
        }

        if (!found) {
          event_jump(eventRec[eventLoc - 1].eventdat);
        }
      }
      else if (!search_for(eventRec[eventLoc - 1].eventdat2, NULL) &&
               (eventRec[eventLoc - 1].eventdat3 == 0 ||
                !search_for(eventRec[eventLoc - 1].eventdat3, NULL)) &&
               (eventRec[eventLoc - 1].eventdat4 == 0 || !search_for(eventRec[eventLoc - 1].eventdat4, NULL)))
      {
        event_jump(eventRec[eventLoc - 1].eventdat);
      }
      break;

    case 71:
      if (((((intptr_t)mapYPos - (intptr_t)&megaData1.mainmap) / sizeof(JE_byte *)) * 2) <=
          (unsigned)eventRec[eventLoc - 1].eventdat2)
      {
        event_jump(eventRec[eventLoc - 1].eventdat);
      }
      break;

    case 72:
      background3x1b = (eventRec[eventLoc - 1].eventdat == 1);
      break;

    case 73:
      skyEnemyOverAll = (eventRec[eventLoc - 1].eventdat == 1);
      break;

    case 74:  // Enemy Global BounceParams
      for (temp = 0; temp < 100; temp++) {
        if (eventRec[eventLoc - 1].eventdat4 != 0 && enemy[temp].linknum != eventRec[eventLoc - 1].eventdat4)
        {
          continue;
        }

        if (eventRec[eventLoc - 1].eventdat5 != -99) {
          enemy[temp].xminbounce = eventRec[eventLoc - 1].eventdat5;
        }

        if (eventRec[eventLoc - 1].eventdat6 != -99) {
          enemy[temp].yminbounce = eventRec[eventLoc - 1].eventdat6;
        }

        if (eventRec[eventLoc - 1].eventdat != -99) {
          enemy[temp].xmaxbounce = eventRec[eventLoc - 1].eventdat;
        }

        if (eventRec[eventLoc - 1].eventdat2 != -99) {
          enemy[temp].ymaxbounce = eventRec[eventLoc - 1].eventdat2;
        }
      }
      break;

    case 75: {
      bool temp_no_clue = false;  // TODO: figure out what this is doing

      for (temp = 0; temp < 100; temp++) {
        if (enemyAvail[temp] == 0 && enemy[temp].eyc == 0 &&
            enemy[temp].linknum >= eventRec[eventLoc - 1].eventdat &&
            enemy[temp].linknum <= eventRec[eventLoc - 1].eventdat2)
        {
          temp_no_clue = true;
        }
      }

      if (temp_no_clue) {
        JE_byte enemy_i;
        do {
          temp = (mt_rand() % (eventRec[eventLoc - 1].eventdat2 + 1 - eventRec[eventLoc - 1].eventdat)) +
                 eventRec[eventLoc - 1].eventdat;
        } while (!(search_for(temp, &enemy_i) && enemy[enemy_i].eyc == 0));

        newPL[eventRec[eventLoc - 1].eventdat3 - 80] = temp;
      }
      else {
        newPL[eventRec[eventLoc - 1].eventdat3 - 80] = 255;
        // Skip
        if (eventRec[eventLoc - 1].eventdat4 > 0) {
          curLoc = eventRec[eventLoc - 1 + eventRec[eventLoc - 1].eventdat4].eventtime - 1;
          eventLoc += eventRec[eventLoc - 1].eventdat4 - 1;
        }
      }

      break;
    }

    case 76:
      returnActive = true;
      break;

    case 77:
      mapYPos = &megaData1.mainmap[0][0];
      mapYPos += eventRec[eventLoc - 1].eventdat / 2;
      if (eventRec[eventLoc - 1].eventdat2 > 0) {
        mapY2Pos = &megaData2.mainmap[0][0];
        mapY2Pos += eventRec[eventLoc - 1].eventdat2 / 2;
      }
      else {
        mapY2Pos = &megaData2.mainmap[0][0];
        mapY2Pos += eventRec[eventLoc - 1].eventdat / 2;
      }
      break;

    case 78:
      if (galagaShotFreq < 10) {
        galagaShotFreq++;
      }
      break;

    case 79:
      boss_bar[0].link_num = eventRec[eventLoc - 1].eventdat;
      boss_bar[1].link_num = eventRec[eventLoc - 1].eventdat2;
      break;

    case 80:  // skip events if in 2-player mode
      if (twoPlayerMode) {
        eventLoc += eventRec[eventLoc - 1].eventdat;
      }
      break;

    case 81:  // WRAP2
      BKwrap2 = &megaData2.mainmap[0][0];
      BKwrap2 += eventRec[eventLoc - 1].eventdat / 2;
      BKwrap2to = &megaData2.mainmap[0][0];
      BKwrap2to += eventRec[eventLoc - 1].eventdat2 / 2;
      break;

    case 82:  // Give SPECIAL WEAPON
      player[0].items.special = eventRec[eventLoc - 1].eventdat;
      shotMultiPos[SHOT_SPECIAL] = 0;
      shotRepeat[SHOT_SPECIAL] = 0;
      shotMultiPos[SHOT_SPECIAL2] = 0;
      shotRepeat[SHOT_SPECIAL2] = 0;
      break;

    default:
      logWarn("Event %05d %d has invalid type.", eventRec[eventLoc - 1].eventtime,
              eventRec[eventLoc - 1].eventtype);
      break;
  }

  eventLoc++;
}

// Port of JE_createNewEventEnemy (tyrian2.c).
void create_new_event_enemy(JE_byte enemyTypeOfs, JE_word enemyOffset, Sint16 uniqueShapeTableI) {
  int i;

  b = 0;

  for (i = enemyOffset; i < enemyOffset + 25; i++) {
    if (enemyAvail[i] == 1) {
      b = i + 1;
      break;
    }
  }

  if (b == 0) {
    return;
  }

  tempW = eventRec[eventLoc - 1].eventdat + enemyTypeOfs;

  enemyAvail[b - 1] = make_enemy(&enemy[b - 1], tempW, uniqueShapeTableI);

  if (eventRec[eventLoc - 1].eventdat2 != -99) {
    switch (enemyOffset) {
      case 0:
        enemy[b - 1].ex = eventRec[eventLoc - 1].eventdat2 - (mapX - 1) * 24;
        enemy[b - 1].ey -= backMove2;
        break;
      case 25:
      case 75:
        enemy[b - 1].ex = eventRec[eventLoc - 1].eventdat2 - (mapX - 1) * 24 - 12;
        enemy[b - 1].ey -= backMove;
        break;
      case 50:
        if (background3x1) {
          enemy[b - 1].ex = eventRec[eventLoc - 1].eventdat2 - (mapX - 1) * 24 - 12;
        }
        else {
          enemy[b - 1].ex = eventRec[eventLoc - 1].eventdat2 - mapX3 * 24 - 24 * 2 + 6;
        }
        enemy[b - 1].ey -= backMove3;

        if (background3x1b) {
          enemy[b - 1].ex -= 6;
        }
        break;
    }
    enemy[b - 1].ey = -28;
    if (background3x1b && enemyOffset == 50) {
      enemy[b - 1].ey += 4;
    }
  }

  if (smallEnemyAdjust && enemy[b - 1].size == 0) {
    enemy[b - 1].ex -= 10;
    enemy[b - 1].ey -= 7;
  }

  enemy[b - 1].ey += eventRec[eventLoc - 1].eventdat5;
  enemy[b - 1].eyc += eventRec[eventLoc - 1].eventdat3;
  enemy[b - 1].linknum = eventRec[eventLoc - 1].eventdat4;
  enemy[b - 1].fixedmovey = eventRec[eventLoc - 1].eventdat6;
}

// Port of JE_eventJump (tyrian2.c).
void event_jump(JE_word jump) {
  JE_word tempW;

  if (jump == 65535) {
    curLoc = returnLoc;
  }
  else {
    returnLoc = curLoc + 1;
    curLoc = jump;
  }
  tempW = 0;
  do {
    tempW++;
  } while (!(eventRec[tempW - 1].eventtime >= curLoc));
  eventLoc = tempW - 1;
}

// Port of JE_searchFor (tyrian2.c).
bool search_for(JE_byte PLType, JE_byte *out_index) {
  int found_id = -1;

  for (int i = 0; i < 100; i++) {
    if (enemyAvail[i] != 0 || enemy[i].linknum != PLType) {
      continue;
    }

    found_id = i;
    if (galagaMode) {
      enemy[i].evalue += enemy[i].evalue;
    }
  }

  if (found_id != -1) {
    if (out_index) {
      *out_index = found_id;
    }
    return true;
  }
  else {
    return false;
  }
}

}  // namespace tyrx::port
