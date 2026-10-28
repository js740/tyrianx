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
  auto& frameCountMax = state().game.frame_count_max;
  auto& playerEndLevel = state().ending.player_end_level;
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

void draw_so_far() {
  draw_tick(draw_record());
  draw_record() = DrawRecord{};
}

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
  auto& twoPlayerMode = state().game.two_player_mode;
  auto& onePlayerAction = state().game.one_player_action;
  auto& musicFade = state().sound.music_fade;
  auto& tempVolume = state().sound.temp_volume;
  auto& levelSong = state().sound.level_song;
  auto& smoothies = state().display.smoothies;
  auto& starShowVGASpecialCode = state().display.star_show_vga_special_code;
  auto& mapYPos = state().map.map_y_pos;
  auto& mapY2Pos = state().map.map_y2_pos;
  auto& mapY3Pos = state().map.map_y3_pos;
  auto& BKwrap1 = state().map.bk_wrap1;
  auto& BKwrap2 = state().map.bk_wrap2;
  auto& BKwrap3 = state().map.bk_wrap3;
  auto& BKwrap1to = state().map.bk_wrap1_to;
  auto& BKwrap2to = state().map.bk_wrap2_to;
  auto& BKwrap3to = state().map.bk_wrap3_to;
  auto& endLevel = state().ending.end_level;
  auto& levelEnd = state().ending.countdown;
  auto& firstGameOver = state().ending.first_game_over;
  auto& allPlayersGone = state().ending.all_players_gone;
  auto& player = state().players.slots;
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
  auto& twoPlayerMode = state().game.two_player_mode;
  auto& onePlayerAction = state().game.one_player_action;
  auto& galagaMode = state().game.galaga_mode;
  auto& soundQueue = state().sound.queue;
  auto& textErase = state().display.text_erase;
  auto& oldMapX3Ofs = state().map.old_map_x3_ofs;
  auto& mapX3Ofs = state().map.map_x3_ofs;
  auto& endLevel = state().ending.end_level;
  auto& galagaLife = state().level.galaga_life;
  auto& power = state().players.power;
  auto& lastPower = state().players.last_power;
  auto& powerAdd = state().players.power_add;
  auto& shieldWait = state().players.shield_wait;
  auto& shieldT = state().players.shield_cost;
  auto& player = state().players.slots;
  auto& enemyOnScreen = state().enemies.on_screen;
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
  auto& processorType = state().game.processor_type;
  auto& wild = state().cheats.wild;
  auto& smoothies = state().display.smoothies;
  auto& smoothie_data = state().display.smoothie_data;
  auto& anySmoothies = state().display.any_smoothies;
  auto& starActive = state().display.star_active;
  auto& backPos = state().map.back_pos;
  auto& backMove = state().map.back_move;
  auto& mapY = state().map.map_y;
  auto& mapYPos = state().map.map_y_pos;
  auto& map1YDelay = state().map.map1_y_delay;
  auto& map1YDelayMax = state().map.map1_y_delay_max;
  auto& background2 = state().map.background2;
  auto& background2notTransparent = state().map.background2_not_transparent;
  auto& background2over = state().map.background2_over;
  auto& curLoc = state().level.cur_loc;
  auto& forceEvents = state().level.force_events;
  auto& astralDuration = state().specials.astral_duration;
  DrawRecord& r = draw_record();

  // SMOOTHIES!
  check_smoothies();
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
    sim::update_starfield(state().starfield, VGAScreen->pitch);
    r.starfield = VGAScreen;
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
  auto& processorType = state().game.processor_type;
  auto& superWild = state().cheats.super_wild;
  auto& wild = state().cheats.wild;
  auto& soundQueue = state().sound.queue;
  auto& smoothies = state().display.smoothies;
  auto& smoothie_data = state().display.smoothie_data;
  auto& neat = state().display.neat;
  auto& backMove = state().map.back_move;
  auto& backMove3 = state().map.back_move3;
  auto& mapXOfs = state().map.map_x_ofs;
  auto& mapX2Ofs = state().map.map_x2_ofs;
  auto& oldMapX3Ofs = state().map.old_map_x3_ofs;
  auto& stopBackgroundNum = state().map.stop_background_num;
  auto& background2notTransparent = state().map.background2_not_transparent;
  auto& background3x1 = state().map.background3x1;
  auto& background2over = state().map.background2_over;
  auto& background3over = state().map.background3_over;
  auto& skyEnemyOverAll = state().map.sky_enemy_over_all;
  auto& topEnemyOver = state().map.top_enemy_over;
  auto& enemiesActive = state().level.enemies_active;
  auto& levelEnemy = state().level.enemy_types;
  auto& levelEnemyMax = state().level.enemy_types_max;
  auto& levelEnemyFrequency = state().level.enemy_frequency;
  auto& enemyOnScreen = state().enemies.on_screen;
  DrawRecord& r = draw_record();
  int lastEnemyOnScreen;

  // -----------------------Ground Enemy------------------------
  lastEnemyOnScreen = enemyOnScreen;

  draw_enemy_tick(50, mapXOfs, backMove, r.ground_enemies);
  draw_enemy_tick(100, mapXOfs, backMove, r.ground_enemies_2);

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
    b = sim::new_enemy(state(), 0, tempW, 0);
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

    draw_enemy_tick(25, mapX2Ofs, 0, r.sky_enemies_under);

    if (enemyOnScreen == lastEnemyOnScreen && stopBackgroundNum == 2) {
      stopBackgroundNum = 9;
    }
  }

  if (background3over == 0) {
    background_3_tick(VGAScreen, r.background_3_over_0);
  }

  // Draw Top Enemy
  if (!topEnemyOver) {
    draw_enemy_tick(75, (background3x1 == 0) ? oldMapX3Ofs : mapXOfs, backMove3, r.top_enemies_under);
  }
}

// The player shots, and what they hit.
void player_shots_tick() {
  DrawRecord& r = draw_record();

  // Player Shot Images
  r.player_shots = PlayerShotsDrawn{VGAScreen, sim::update_player_shots(state())};
}

// The players: collisions, then movement and firing.
void players_tick() {
  auto& twoPlayerMode = state().game.two_player_mode;
  auto& endLevel = state().ending.end_level;
  auto& firstGameOver = state().ending.first_game_over;
  auto& player = state().players.slots;
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
  auto& backMove3 = state().map.back_move3;
  auto& oldMapXOfs = state().map.old_map_x_ofs;
  auto& mapX2Ofs = state().map.map_x2_ofs;
  auto& oldMapX3Ofs = state().map.old_map_x3_ofs;
  auto& stopBackgroundNum = state().map.stop_background_num;
  auto& background3x1 = state().map.background3x1;
  auto& background3over = state().map.background3_over;
  auto& skyEnemyOverAll = state().map.sky_enemy_over_all;
  auto& topEnemyOver = state().map.top_enemy_over;
  auto& endLevel = state().ending.end_level;
  auto& enemyOnScreen = state().enemies.on_screen;
  DrawRecord& r = draw_record();
  int lastEnemyOnScreen;

  // MAIN DRAWING IS STOPPED STARTING HERE
  if (!endLevel) {
    // Draw Enemy Shots
    sim::EnemyShotsUpdate const update = sim::update_enemy_shots(state());
    r.enemy_shots = EnemyShotsDrawn{VGAScreen, update.visible};
    for (sim::PlayerDamage const& d : update.damage) {
      player_damage_hud(d);
      VGAScreen = game_screen;  // side-effect of game_screen
    }
  }

  if (background3over == 1) {
    background_3_tick(VGAScreen, r.background_3_over_1);
  }

  // Draw Top Enemy
  if (topEnemyOver) {
    draw_enemy_tick(75, (background3x1 == 0) ? oldMapX3Ofs : oldMapXOfs, backMove3, r.top_enemies_over);
  }

  if (!skyEnemyOverAll) {
    return;
  }

  // Draw Sky Enemy
  lastEnemyOnScreen = enemyOnScreen;

  draw_enemy_tick(25, mapX2Ofs, 0, r.sky_enemies_over);

  if (enemyOnScreen != lastEnemyOnScreen) {
    return;
  }

  if (stopBackgroundNum == 2) {
    stopBackgroundNum = 9;
  }
}

// Sequenced explosions, and the explosions.
void explosions_tick() {
  auto& soundQueue = state().sound.queue;
  auto& portConfigChange = state().display.port_config_change;
  auto& portConfigDone = state().display.port_config_done;
  auto& backMove2 = state().map.back_move2;
  auto& enemyStillExploding = state().ending.enemy_still_exploding;
  DrawRecord& r = draw_record();

  // -------------------------- Sequenced Explosions -------------------------
  enemyStillExploding =
      sim::update_repeating_explosions(state().explosions, state().random.mt, backMove2, soundQueue);

  // ---------------------------- Draw Explosions ----------------------------
  r.explosions = ExplosionsDrawn{VGAScreen, sim::update_explosions(state().explosions)};

  if (!portConfigChange) {
    portConfigDone = true;
  }
}

// Background 2 over everything, the low armor warning, and random explosions.
void warning_tick() {
  auto& twoPlayerMode = state().game.two_player_mode;
  auto& galagaMode = state().game.galaga_mode;
  auto& processorType = state().game.processor_type;
  auto& wild = state().cheats.wild;
  auto& soundQueue = state().sound.queue;
  auto& smoothies = state().display.smoothies;
  auto& warningCol = state().display.warning_col;
  auto& warningColChange = state().display.warning_col_change;
  auto& warningSoundDelay = state().display.warning_sound_delay;
  auto& backMove3 = state().map.back_move3;
  auto& background2notTransparent = state().map.background2_not_transparent;
  auto& background2over = state().map.background2_over;
  auto& randomExplosions = state().level.random_explosions;
  auto& armorShipDelay = state().players.armor_ship_delay;
  auto& player = state().players.slots;
  auto& enemy = state().enemies.slots;
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
      b = sim::new_enemy(state(), 50, tempW, 0);
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
  auto& soundQueue = state().sound.queue;
  auto& firstGameOver = state().ending.first_game_over;
  auto& returnActive = state().level.return_active;
  auto& enemyOnScreen = state().enemies.on_screen;
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
  auto& pentiumMode = state().game.pentium_mode;
  auto& frameCountMax = state().game.frame_count_max;
  auto& soundQueue = state().sound.queue;
  auto& smoothies = state().display.smoothies;
  auto& flash = state().display.flash;
  auto& flashChange = state().display.flash_change;
  auto& displayTime = state().display.display_time;
  auto& mapX = state().map.map_x;
  auto& curLoc = state().level.cur_loc;
  auto& levelTimer = state().level.timer;
  auto& levelTimerCountdown = state().level.timer_countdown;
  auto& levelTimerJumpTo = state().level.timer_jump_to;
  auto& player = state().players.slots;
  auto& enemyOnScreen = state().enemies.on_screen;
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
  auto& galagaMode = state().game.galaga_mode;
  auto& playDemo = state().game.play_demo;
  auto& constantPlay = state().cheats.constant_play;
  auto& constantDie = state().cheats.constant_die;
  auto& musicFade = state().sound.music_fade;
  auto& reallyEndLevel = state().ending.really_end_level;
  auto& firstGameOver = state().ending.first_game_over;
  auto& allPlayersGone = state().ending.all_players_gone;
  auto& stoppedDemo = state().ending.stopped_demo;
  auto& bonusLevelCurrent = state().ending.bonus_level_current;
  auto& normalBonusLevelCurrent = state().ending.normal_bonus_level_current;
  auto& player = state().players.slots;
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
        c_loop(JE_pauseGame);
      }
    }

    if (ingamemenu_pressed) {
      ingamemenu_pressed = false;

      if (isNetworkGame) {
        inGameMenuRequest = true;
      }
      else {
        yourInGameMenuRequest = true;
        c_loop(JE_doInGameSetup);
        skipStarShowVGA = true;
      }
    }
  }

  return false;
}

LevelPass simulate(bool& frame) {
  auto& filterActive = state().display.filter_active;
  auto& backMove = state().map.back_move;
  auto& backMove2 = state().map.back_move2;
  auto& backMove3 = state().map.back_move3;
  auto& stopBackgrounds = state().map.stop_backgrounds;
  auto& stopBackgroundNum = state().map.stop_background_num;
  auto& endLevel = state().ending.end_level;
  auto& reallyEndLevel = state().ending.really_end_level;
  auto& waitToEndLevel = state().ending.wait_to_end_level;
  auto& playerEndLevel = state().ending.player_end_level;
  auto& readyToEndLevel = state().ending.ready_to_end_level;
  auto& levelEnd = state().ending.countdown;
  auto& allPlayersGone = state().ending.all_players_gone;
  auto& enemyStillExploding = state().ending.enemy_still_exploding;
  auto& eventRec = state().level.events;
  auto& maxEvent = state().level.max_event;
  auto& eventLoc = state().level.event_loc;
  auto& curLoc = state().level.cur_loc;
  auto& levelTimer = state().level.timer;
  auto& levelTimerCountdown = state().level.timer_countdown;
  auto& enemyOnScreen = state().enemies.on_screen;
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
  r.superpixels = SuperpixelsDrawn{VGAScreen, sim::update_superpixels(state().superpixels)};

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
