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

// The players (JE_mainGamePlayerFunctions, JE_playerMovement,
// JE_playerCollide in mainint.c, and JE_doSpecialShot, JE_specialComplete and
// JE_playerDamage in varz.c): input, movement, firing, specials, sidekicks,
// pickups and damage, and the game logic they use (JE_SFCodes, JE_getShipInfo
// and the power-ups of player.c).  The draw record gets the values that each
// part is drawn with where the simulation goes on to change them (positions,
// animation frames and charge), and the HUD log the gauges and messages they
// draw.  Network games aren't supported, so the network code is left out.

#include "draw.hpp"
#include "port.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace tyrx::port {

namespace {

// Port of JE_specialComplete.
void special_complete(JE_byte playerNum, JE_byte specialType) {
  auto& superArcadeMode = state().game.super_arcade_mode;
  auto& soundQueue = state().sound.queue;
  auto& nextSpecialWait = state().specials.next_wait;
  auto& spraySpecial = state().specials.spray;
  auto& specialWeaponWpn = state().specials.weapon_wpn;
  auto& specialWeaponFilter = state().specials.weapon_filter;
  auto& specialWeaponFreq = state().specials.weapon_freq;
  auto& linkToPlayer = state().specials.link_to_player;
  auto& flareDuration = state().specials.flare_duration;
  auto& flareStart = state().specials.flare_start;
  auto& zinglonDuration = state().specials.zinglon_duration;
  auto& astralDuration = state().specials.astral_duration;
  auto& shotRepeat = state().players.shot_repeat;
  auto& shotMultiPos = state().players.shot_multi_pos;
  auto& player = state().players.slots;
  auto& enemy = state().enemies.slots;
  auto& enemyAvail = state().enemies.status;
  auto& enemyShot = state().enemy_shots.slots;
  auto& enemyShotAvail = state().enemy_shots.available;
  nextSpecialWait = 0;
  switch (special[specialType].stype) {
    // Weapon
    case 1:
      if (playerNum == 1) {
        b = port::player_shot_create(0, SHOT_SPECIAL2, player[0].x, player[0].y, player[0].mouseX,
                                     player[0].mouseY, special[specialType].wpn, playerNum);
      }
      else {
        b = port::player_shot_create(0, SHOT_SPECIAL2, player[1].x, player[1].y, player[0].mouseX,
                                     player[0].mouseY, special[specialType].wpn, playerNum);
      }

      shotRepeat[SHOT_SPECIAL] = shotRepeat[SHOT_SPECIAL2];
      break;
    // Repulsor
    case 2:
      for (temp = 0; temp < ENEMY_SHOT_MAX; temp++) {
        if (enemyShotAvail[temp]) {
          continue;
        }

        if (player[0].x > enemyShot[temp].sx) {
          enemyShot[temp].sxm--;
        }
        else if (player[0].x < enemyShot[temp].sx) {
          enemyShot[temp].sxm++;
        }

        if (player[0].y > enemyShot[temp].sy) {
          enemyShot[temp].sym--;
        }
        else if (player[0].y < enemyShot[temp].sy) {
          enemyShot[temp].sym++;
        }
      }
      break;
    // Zinglon Blast
    case 3:
      zinglonDuration = 50;
      shotRepeat[SHOT_SPECIAL] = 100;
      soundQueue[7] = S_SOUL_OF_ZINGLON;
      break;
    // Attractor
    case 4:
      for (temp = 0; temp < 100; temp++) {
        if (!(enemyAvail[temp] != 1 && enemy[temp].scoreitem && enemy[temp].evalue != 0)) {
          continue;
        }

        if (player[0].x > enemy[temp].ex) {
          enemy[temp].exc++;
        }
        else if (player[0].x < enemy[temp].ex) {
          enemy[temp].exc--;
        }

        if (player[0].y > enemy[temp].ey) {
          enemy[temp].eyc++;
        }
        else if (player[0].y < enemy[temp].ey) {
          enemy[temp].eyc--;
        }
      }
      break;
    // Flare
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 16:
      if (flareDuration == 0) {
        flareStart = true;
      }

      specialWeaponWpn = special[specialType].wpn;
      linkToPlayer = false;
      spraySpecial = false;
      switch (special[specialType].stype) {
        case 5:
          specialWeaponFilter = 7;
          specialWeaponFreq = 2;
          flareDuration = 50;
          break;
        case 6:
          specialWeaponFilter = 1;
          specialWeaponFreq = 7;
          flareDuration = 200 + 25 * player[0].items.weapon[FRONT_WEAPON].power;
          break;
        case 7:
          specialWeaponFilter = 3;
          specialWeaponFreq = 3;
          flareDuration = 50 + 10 * player[0].items.weapon[FRONT_WEAPON].power;
          zinglonDuration = 50;
          shotRepeat[SHOT_SPECIAL] = 100;
          soundQueue[7] = S_SOUL_OF_ZINGLON;
          break;
        case 8:
          specialWeaponFilter = -99;
          specialWeaponFreq = 7;
          flareDuration = 10 + player[0].items.weapon[FRONT_WEAPON].power;
          break;
        case 9:
          specialWeaponFilter = -99;
          specialWeaponFreq = 8;
          flareDuration = 8 + 2 * player[0].items.weapon[FRONT_WEAPON].power;
          linkToPlayer = true;
          nextSpecialWait = special[specialType].pwr;
          break;
        case 10:
          specialWeaponFilter = -99;
          specialWeaponFreq = 8;
          flareDuration = 14 + 4 * player[0].items.weapon[FRONT_WEAPON].power;
          linkToPlayer = true;
          break;
        case 11:
          specialWeaponFilter = -99;
          specialWeaponFreq = special[specialType].pwr;
          flareDuration = 10 + 10 * player[0].items.weapon[FRONT_WEAPON].power;
          astralDuration = 20 + 10 * player[0].items.weapon[FRONT_WEAPON].power;
          break;
        case 16:
          specialWeaponFilter = -99;
          specialWeaponFreq = 8;
          flareDuration = temp2 * 16 + 8;
          linkToPlayer = true;
          spraySpecial = true;
          break;
      }
      break;
    case 12:  // Invulnerability
      player[playerNum - 1].invulnerable_ticks = temp2 * 10;

      if (superArcadeMode > 0 && superArcadeMode <= SA) {
        shotRepeat[SHOT_SPECIAL] = 250;
        b = port::player_shot_create(0, SHOT_SPECIAL2, player[0].x, player[0].y, player[0].mouseX,
                                     player[0].mouseY, 707, 1);
        player[0].invulnerable_ticks = 100;
      }
      break;
    case 13:  // Repair Player 1
      player[0].armor += temp2 / 4 + 1;

      soundQueue[3] = S_POWERUP;
      break;
    case 14:  // Repair Player 2
      player[1].armor += temp2 / 4 + 1;

      soundQueue[3] = S_POWERUP;
      break;

    case 17:  // Spawn left or right sidekick
      soundQueue[3] = S_POWERUP;

      if (player[0].items.sidekick[LEFT_SIDEKICK] == special[specialType].wpn) {
        player[0].items.sidekick[RIGHT_SIDEKICK] = special[specialType].wpn;
        shotMultiPos[RIGHT_SIDEKICK] = 0;
      }
      else {
        player[0].items.sidekick[LEFT_SIDEKICK] = special[specialType].wpn;
        shotMultiPos[LEFT_SIDEKICK] = 0;
      }

      draw_options_tick();
      break;

    case 18:  // Spawn right sidekick
      player[0].items.sidekick[RIGHT_SIDEKICK] = special[specialType].wpn;

      draw_options_tick();

      soundQueue[4] = S_POWERUP;

      shotMultiPos[RIGHT_SIDEKICK] = 0;
      break;
  }
}

// Port of JE_doSpecialShot.
void do_special_shot(JE_byte playerNum, uint *armor, uint *shield, PlayerDrawn& drawn) {
  auto& superArcadeMode = state().game.super_arcade_mode;
  auto& button = state().input.button;
  auto& fireButtonHeld = state().input.fire_button_held;
  auto& levelFilter = state().display.filter;
  auto& levelBrightness = state().display.brightness;
  auto& filterActive = state().display.filter_active;
  auto& specialWait = state().specials.wait;
  auto& nextSpecialWait = state().specials.next_wait;
  auto& SFExecuted = state().specials.sf_executed;
  auto& spraySpecial = state().specials.spray;
  auto& specialWeaponWpn = state().specials.weapon_wpn;
  auto& specialWeaponFilter = state().specials.weapon_filter;
  auto& specialWeaponFreq = state().specials.weapon_freq;
  auto& linkToPlayer = state().specials.link_to_player;
  auto& flareDuration = state().specials.flare_duration;
  auto& flareStart = state().specials.flare_start;
  auto& flareColChg = state().specials.flare_col_chg;
  auto& zinglonDuration = state().specials.zinglon_duration;
  auto& astralDuration = state().specials.astral_duration;
  auto& shotRepeat = state().players.shot_repeat;
  auto& shotMultiPos = state().players.shot_multi_pos;
  auto& player = state().players.slots;
  auto& playerShotData = state().player_shots.slots;
  auto& shotAvail = state().player_shots.ttl;
  if (player[0].items.special > 0) {
    bool const ready =
        shotRepeat[SHOT_SPECIAL] == 0 && specialWait == 0 && flareDuration < 2 && zinglonDuration < 2;
    drawn.special_icon = PlayerDrawn::SpecialIcon{VGAScreen, ready};
  }

  if (shotRepeat[SHOT_SPECIAL] > 0) {
    --shotRepeat[SHOT_SPECIAL];
  }
  if (specialWait > 0) {
    specialWait--;
  }
  temp = SFExecuted[playerNum - 1];
  if (temp > 0 && shotRepeat[SHOT_SPECIAL] == 0 && flareDuration == 0) {
    temp2 = special[temp].pwr;

    bool can_afford = true;

    if (temp2 > 0) {
      // costs some shield
      if (temp2 < 98) {
        if (*shield >= temp2) {
          *shield -= temp2;
        }
        else {
          can_afford = false;
        }
      }
      // costs all shield
      else if (temp2 == 98) {
        if (*shield < 4) {
          can_afford = false;
        }
        temp2 = *shield;
        *shield = 0;
      }
      // costs half shield
      else if (temp2 == 99) {
        temp2 = *shield / 2;
        *shield = temp2;
      }
      // costs some armor
      else {
        temp2 -= 100;
        if (*armor > temp2) {
          *armor -= temp2;
        }
        else {
          can_afford = false;
        }
      }
    }

    shotMultiPos[SHOT_SPECIAL] = 0;
    shotMultiPos[SHOT_SPECIAL2] = 0;

    if (can_afford) {
      special_complete(playerNum, temp);
    }

    SFExecuted[playerNum - 1] = 0;

    wipe_shield_armor_bars_tick();
    VGAScreen = VGAScreenSeg;  // side-effect of game_screen
    draw_shield_tick();
    draw_armor_tick();
    VGAScreen = game_screen;  // side-effect of game_screen
  }

  // Main Begin
  if (playerNum == 1 && player[0].items.special > 0) {
    if (superArcadeMode > 0 && (button[2 - 1] || button[3 - 1])) {
      fireButtonHeld = false;
    }
    if (!button[1 - 1] && !(superArcadeMode != SA_NONE && (button[2 - 1] || button[3 - 1]))) {
      fireButtonHeld = false;
    }
    else if (shotRepeat[SHOT_SPECIAL] == 0 && !fireButtonHeld && !(flareDuration > 0) && specialWait == 0) {
      fireButtonHeld = true;
      special_complete(playerNum, player[0].items.special);
    }

  }  // Main End

  if (astralDuration > 0) {
    astralDuration--;
  }

  shotAvail[MAX_PWEAPON - 1] = 0;
  if (flareDuration > 1) {
    if (specialWeaponFilter != -99) {
      if (levelFilter == -99 && levelBrightness == -99) {
        filterActive = false;
      }
      if (!filterActive) {
        levelFilter = specialWeaponFilter;
        if (levelFilter == 7) {
          levelBrightness = 0;
        }
        filterActive = true;
      }

      if (mt_rand() % 2 == 0) {
        flareColChg = -1;
      }
      else {
        flareColChg = 1;
      }

      if (levelFilter == 7) {
        if (levelBrightness < -6) {
          flareColChg = 1;
        }
        if (levelBrightness > 6) {
          flareColChg = -1;
        }
        levelBrightness += flareColChg;
      }
    }

    if ((signed)(mt_rand() % 6) < specialWeaponFreq) {
      b = MAX_PWEAPON;

      if (linkToPlayer) {
        if (shotRepeat[SHOT_SPECIAL] == 0) {
          b = port::player_shot_create(0, SHOT_SPECIAL, player[0].x, player[0].y, player[0].mouseX,
                                       player[0].mouseY, specialWeaponWpn, playerNum);
        }
      }
      else {
        b = port::player_shot_create(0, SHOT_SPECIAL, mt_rand() % 280, mt_rand() % 180, player[0].mouseX,
                                     player[0].mouseY, specialWeaponWpn, playerNum);
      }

      if (spraySpecial && b != MAX_PWEAPON) {
        playerShotData[b].shotXM = (mt_rand() % 5) - 2;
        playerShotData[b].shotYM = (mt_rand() % 5) - 2;
        if (playerShotData[b].shotYM == 0) {
          playerShotData[b].shotYM++;
        }
      }
    }

    flareDuration--;
    if (flareDuration == 1) {
      specialWait = nextSpecialWait;
    }
  }
  else if (flareStart) {
    flareStart = false;
    shotRepeat[SHOT_SPECIAL] = linkToPlayer ? 15 : 200;
    flareDuration = 0;
    if (levelFilter == specialWeaponFilter) {
      levelFilter = -99;
      levelBrightness = -99;
      filterActive = false;
    }
  }

  if (zinglonDuration <= 1) {
    return;
  }

  temp = 25 - abs(zinglonDuration - 25);

  drawn.zinglon = PlayerDrawn::Zinglon{VGAScreen, player[0].x, temp};

  zinglonDuration--;
  if (zinglonDuration % 5 == 0) {
    shotAvail[MAX_PWEAPON - 1] = 1;
  }
}

// The ship as drawn: in the fade-out at the end of a level, its trail.
void draw_ship_trail(SDL_Surface *screen, JE_word shipGr_, Sprite2_array *shipGrPtr_, int x, int trail_y) {
  if (shipGr_ == 0) {
    draw::blit_sprite2x2(screen, x - 17, trail_y - 7, *shipGrPtr_, 13);
    draw::blit_sprite2x2(screen, x + 7, trail_y - 7, *shipGrPtr_, 51);
  }
  else if (shipGr_ == 1) {
    draw::blit_sprite2x2(screen, x - 17, trail_y - 7, *shipGrPtr_, 220);
    draw::blit_sprite2x2(screen, x + 7, trail_y - 7, *shipGrPtr_, 222);
  }
  else {
    draw::blit_sprite2x2(screen, x - 5, trail_y - 7, *shipGrPtr_, shipGr_);
  }
}

void draw_ship(JE_word shipGr_, Sprite2_array *shipGrPtr_, ShipDrawn const& d) {
  SDL_Surface *const screen = d.screen;
  int const x = d.x, y = d.y;
  int const ship_sprite = d.ship_sprite;
  JE_word const mapX2Ofs = d.mapX2Ofs;

  if (shipGr_ == 0) {
    if (d.background2) {
      draw::blit_sprite2x2_darken(screen, x - 17 - mapX2Ofs + 30, y - 7 + shadowYDist, *shipGrPtr_,
                                  ship_sprite + 13);
      draw::blit_sprite2x2_darken(screen, x + 7 - mapX2Ofs + 30, y - 7 + shadowYDist, *shipGrPtr_,
                                  ship_sprite + 51);
      if (d.superWild) {
        draw::blit_sprite2x2_darken(screen, x - 16 - mapX2Ofs + 30, y - 7 + shadowYDist, *shipGrPtr_,
                                    ship_sprite + 13);
        draw::blit_sprite2x2_darken(screen, x + 6 - mapX2Ofs + 30, y - 7 + shadowYDist, *shipGrPtr_,
                                    ship_sprite + 51);
      }
    }
  }
  else if (shipGr_ == 1) {
    if (d.background2) {
      draw::blit_sprite2x2_darken(screen, x - 17 - mapX2Ofs + 30, y - 7 + shadowYDist, *shipGrPtr_, 220);
      draw::blit_sprite2x2_darken(screen, x + 7 - mapX2Ofs + 30, y - 7 + shadowYDist, *shipGrPtr_, 222);
    }
  }
  else {
    if (d.background2) {
      draw::blit_sprite2x2_darken(screen, x - 5 - mapX2Ofs + 30, y - 7 + shadowYDist, *shipGrPtr_,
                                  ship_sprite);
      if (d.superWild) {
        draw::blit_sprite2x2_darken(screen, x - 4 - mapX2Ofs + 30, y - 7 + shadowYDist, *shipGrPtr_,
                                    ship_sprite);
      }
    }
  }

  if (d.invulnerable) {
    if (shipGr_ == 0) {
      draw::blit_sprite2x2_blend(screen, x - 17, y - 7, *shipGrPtr_, ship_sprite + 13);
      draw::blit_sprite2x2_blend(screen, x + 7, y - 7, *shipGrPtr_, ship_sprite + 51);
    }
    else if (shipGr_ == 1) {
      draw::blit_sprite2x2_blend(screen, x - 17, y - 7, *shipGrPtr_, 220);
      draw::blit_sprite2x2_blend(screen, x + 7, y - 7, *shipGrPtr_, 222);
    }
    else {
      draw::blit_sprite2x2_blend(screen, x - 5, y - 7, *shipGrPtr_, ship_sprite);
    }
  }
  else {
    if (shipGr_ == 0) {
      draw::blit_sprite2x2(screen, x - 17, y - 7, *shipGrPtr_, ship_sprite + 13);
      draw::blit_sprite2x2(screen, x + 7, y - 7, *shipGrPtr_, ship_sprite + 51);
    }
    else if (shipGr_ == 1) {
      draw::blit_sprite2x2(screen, x - 17, y - 7, *shipGrPtr_, 220);
      draw::blit_sprite2x2(screen, x + 7, y - 7, *shipGrPtr_, 222);

      switch (ship_sprite) {
        case 5:
          draw::blit_sprite2(screen, x - 17, y + 7, *shipGrPtr_, 40);
          break;
        case 3:
          draw::blit_sprite2(screen, x - 17, y + 7, *shipGrPtr_, 39);
          break;
        case -1:
          draw::blit_sprite2(screen, x + 19, y + 7, *shipGrPtr_, 58);
          break;
        case -3:
          draw::blit_sprite2(screen, x + 19, y + 7, *shipGrPtr_, 59);
          break;
      }
    }
    else {
      draw::blit_sprite2x2(screen, x - 5, y - 7, *shipGrPtr_, ship_sprite);
    }
  }
}

}  // namespace

// The player's parts, in the order JE_playerMovement draws them.
void player_draw(PlayerDrawn const& d) {
  if (d.trail) {
    for (int trail_y : d.trail->trail_y) {
      draw_ship_trail(d.trail->screen, d.shipGr_, d.shipGrPtr_, d.trail->x, trail_y);
    }
  }
  if (d.insert_coin != nullptr) {
    draw::JE_dString(d.insert_coin, 115, 10, miscText[7], SMALL_FONT_SHAPES);  // insert coin
  }
  if (d.ship) {
    draw_ship(d.shipGr_, d.shipGrPtr_, *d.ship);
  }
  if (d.special_icon) {
    draw::blit_sprite2(d.special_icon->screen, 47, 4, spriteSheet9, d.special_icon->ready ? 94 : 93);
  }
  if (d.zinglon) {
    SDL_Surface *const screen = d.zinglon->screen;
    int const x = d.zinglon->x, temp = d.zinglon->temp;
    draw::JE_barBright(screen, x + 7 - temp, 0, x + 7 + temp, 184);
    draw::JE_barBright(screen, x + 7 - temp - 2, 0, x + 7 + temp + 2, 184);
  }
  if (d.charge) {
    draw::blit_sprite2(d.charge->screen, d.charge->x, d.charge->y, spriteSheet10, d.charge->sprite);
  }
  for (std::optional<PlayerDrawn::Sidekick> const& sidekick : d.sidekicks) {
    if (!sidekick) {
      continue;
    }

    if (sidekick->style == 1 || sidekick->style == 2) {
      draw::blit_sprite2x2(sidekick->screen, sidekick->x - 6, sidekick->y, spriteSheet10, sidekick->sprite);
    }
    else {
      draw::blit_sprite2(sidekick->screen, sidekick->x, sidekick->y, spriteSheet9, sidekick->sprite);
    }
  }
}

namespace {

// playDemoKeys (demo.c), which reads the demo's input, whose state is static
// there, and moves player 1 and sets the buttons in the C globals: it gets the
// State's player 1's position there, and the State gets the result.
bool play_demo_keys() {
  auto& player = state().players.slots;
  auto& button = state().input.button;
  ::player[0].x = player[0].x;
  ::player[0].y = player[0].y;
  std::ranges::copy(button, ::button);
  bool const more = playDemoKeys();
  player[0].x = ::player[0].x;
  player[0].y = ::player[0].y;
  std::ranges::copy(::button, button.begin());
  return more;
}

// Port of JE_playerMovement.
void player_movement(Player *this_player, JE_byte inputDevice, JE_byte playerNum_, JE_word shipGr_,
                     Sprite2_array *shipGrPtr_, JE_word *mouseX_, JE_word *mouseY_) {
  auto& twoPlayerMode = state().game.two_player_mode;
  auto& onePlayerAction = state().game.one_player_action;
  auto& galagaMode = state().game.galaga_mode;
  auto& superArcadeMode = state().game.super_arcade_mode;
  auto& difficultyLevel = state().game.difficulty_level;
  auto& playDemo = state().game.play_demo;
  auto& recordDemo = state().game.record_demo;
  auto& superWild = state().cheats.super_wild;
  auto& constantPlay = state().cheats.constant_play;
  auto& constantDie = state().cheats.constant_die;
  auto& constantLastX = state().cheats.constant_last_x;
  auto& episodeNum = state().progress.episode_num;
  auto& button = state().input.button;
  auto& soundQueue = state().sound.queue;
  auto& smoothies = state().display.smoothies;
  auto& portConfigChange = state().display.port_config_change;
  auto& portConfigDone = state().display.port_config_done;
  auto& mapX2Ofs = state().map.map_x2_ofs;
  auto& background2 = state().map.background2;
  auto& endLevel = state().ending.end_level;
  auto& reallyEndLevel = state().ending.really_end_level;
  auto& levelEnd = state().ending.countdown;
  auto& levelEndFxWait = state().ending.fx_wait;
  auto& levelEndWarp = state().ending.warp;
  auto& allPlayersGone = state().ending.all_players_gone;
  auto& shotRepeat = state().players.shot_repeat;
  auto& shotMultiPos = state().players.shot_multi_pos;
  auto& chargeWait = state().players.charge_wait;
  auto& chargeLevel = state().players.charge_level;
  auto& chargeMax = state().players.charge_max;
  auto& chargeGr = state().players.charge_gr;
  auto& chargeGrWait = state().players.charge_gr_wait;
  auto& twoPlayerLinked = state().players.two_player_linked;
  auto& linkGunDirec = state().players.link_gun_direction;
  auto& optionSatelliteRotate = state().players.option_satellite_rotate;
  auto& optionAttachmentMove = state().players.option_attachment_move;
  auto& optionAttachmentLinked = state().players.option_attachment_linked;
  auto& optionAttachmentReturn = state().players.option_attachment_return;
  auto& shipGr2 = state().players.ship_gr2;
  auto& player = state().players.slots;
  JE_integer mouseXC, mouseYC;
  JE_integer accelXC, accelYC;

  PlayerDrawn& drawn = draw_record().players.emplace_back();
  drawn.shipGr_ = shipGr_;
  drawn.shipGrPtr_ = shipGrPtr_;

  if (playerNum_ == 2 || !twoPlayerMode) {
    tempW = weaponPort[this_player->items.weapon[REAR_WEAPON].id].opnum;

    if (this_player->weapon_mode > tempW) {
      this_player->weapon_mode = 1;
    }
  }

redo:

  if (isNetworkGame) {
    inputDevice = 0;
  }

  mouseXC = 0;
  mouseYC = 0;
  accelXC = 0;
  accelYC = 0;

  bool link_gun_analog = false;
  float link_gun_angle = 0;

  // Draw Player
  if (!this_player->is_alive) {
    if (this_player->exploding_ticks > 0) {
      --this_player->exploding_ticks;

      if (levelEndFxWait > 0) {
        levelEndFxWait--;
      }
      else {
        levelEndFxWait = (mt_rand() % 6) + 3;
        if ((mt_rand() % 3) == 1) {
          soundQueue[6] = S_EXPLOSION_9;
        }
        else {
          soundQueue[5] = S_EXPLOSION_11;
        }
      }

      int explosion_x = this_player->x + (mt_rand() % 32) - 16;
      int explosion_y = this_player->y + (mt_rand() % 32) - 16;
      setup_explosion_large(false, 0, explosion_x, explosion_y + 7);
      setup_explosion_large(false, 0, this_player->x, this_player->y + 7);

      if (levelEnd > 0) {
        levelEnd--;
      }
    }
    else {
      // if arcade mode
      if (twoPlayerMode || onePlayerAction) {
        // respawn if any extra lives
        if (*this_player->lives > 1) {
          --(*this_player->lives);

          reallyEndLevel = false;
          shotMultiPos[playerNum_ - 1] = 0;
          port::calc_purple_balls_needed(this_player);
          twoPlayerLinked = false;
          if (galagaMode) {
            twoPlayerMode = false;
          }
          this_player->y = 160;
          this_player->invulnerable_ticks = 100;
          this_player->is_alive = true;
          endLevel = false;

          if (galagaMode || episodeNum == 4) {
            this_player->armor = this_player->initial_armor;
          }
          else {
            this_player->armor = this_player->initial_armor / 2;
          }

          if (galagaMode) {
            this_player->shield = 0;
          }
          else {
            this_player->shield = this_player->shield_max / 2;
          }

          VGAScreen = VGAScreenSeg;  // side-effect of game_screen
          draw_armor_tick();
          draw_shield_tick();
          VGAScreen = game_screen;  // side-effect of game_screen
          goto redo;
        }
        else {
          if (galagaMode) {
            twoPlayerMode = false;
          }
          if (allPlayersGone && isNetworkGame) {
            reallyEndLevel = true;
          }
        }
      }
    }
  }
  else if (constantDie) {
    // finished exploding?  start dying again
    if (this_player->exploding_ticks == 0) {
      this_player->shield = 0;

      if (this_player->armor > 0) {
        --this_player->armor;
      }
      else {
        this_player->is_alive = false;
        this_player->exploding_ticks = 60;
        levelEnd = 40;
      }

      wipe_shield_armor_bars_tick();
      VGAScreen = VGAScreenSeg;  // side-effect of game_screen
      draw_armor_tick();
      VGAScreen = game_screen;  // side-effect of game_screen

      // as if instant death weren't enough, player also gets infinite lives in order to enjoy an infinite
      // number of deaths -_-
      if (*player[0].lives < 11) {
        ++(*player[0].lives);
      }
    }
  }

  if (!this_player->is_alive) {
    state().explosions.follow_x = state().explosions.follow_y = 0;
    return;
  }

  if (!endLevel) {
    *mouseX_ = this_player->x;
    *mouseY_ = this_player->y;
    button[1 - 1] = false;
    button[2 - 1] = false;
    button[3 - 1] = false;
    button[4 - 1] = false;

    // --- Movement Routine Beginning ---

    if (!isNetworkGame || playerNum_ == thisPlayerNum) {
      if (endLevel) {
        this_player->y -= 2;
      }
      else {
        if (playDemo || recordDemo) {
          inputDevice = 1;  // keyboard is required device for demo recording
        }

        // demo playback input
        if (playDemo && !play_demo_keys()) {
          endLevel = true;
          levelEnd = 40;
        }

        // joystick input
        if ((inputDevice == 0 || inputDevice >= 3) && joysticks > 0) {
          int j = inputDevice == 0 ? 0 : inputDevice - 3;
          int j_max = inputDevice == 0 ? joysticks : inputDevice - 3 + 1;
          for (; j < j_max; j++) {
            poll_joystick(j);

            if (joystick[j].analog) {
              mouseXC += joystick_axis_reduce(j, joystick[j].x);
              mouseYC += joystick_axis_reduce(j, joystick[j].y);

              link_gun_analog = joystick_analog_angle(j, &link_gun_angle);
            }
            else {
              this_player->x += (joystick[j].direction[3] ? -CURRENT_KEY_SPEED : 0) +
                                (joystick[j].direction[1] ? CURRENT_KEY_SPEED : 0);
              this_player->y += (joystick[j].direction[0] ? -CURRENT_KEY_SPEED : 0) +
                                (joystick[j].direction[2] ? CURRENT_KEY_SPEED : 0);
            }

            button[0] |= joystick[j].action[0];
            button[1] |= joystick[j].action[2];
            button[2] |= joystick[j].action[3];
            button[3] |= joystick[j].action_pressed[1];

            ingamemenu_pressed |= joystick[j].action_pressed[4];
            pause_pressed |= joystick[j].action_pressed[5];
          }
        }

        // mouse input
        if ((inputDevice == 0 || inputDevice == 2) && has_mouse) {
          button[0] |= (mouseButtonsDown & SDL_BUTTON_LMASK) != 0;
          button[1] |= (mouseButtonsDown & SDL_BUTTON_RMASK) != 0;
          button[2] |=
              (mouseButtonsDown & (mouse_has_three_buttons ? SDL_BUTTON_MMASK : SDL_BUTTON_RMASK)) != 0;

          Sint32 mouseXR;
          Sint32 mouseYR;
          mouseGetRelativePosition(&mouseXR, &mouseYR);
          mouseXC += mouseXR;
          mouseYC += mouseYR;
        }

        // keyboard input
        if ((inputDevice == 0 || inputDevice == 1) && !playDemo) {
          if (keysactive[keySettings[KEY_SETTING_UP]]) {
            this_player->y -= CURRENT_KEY_SPEED;
          }
          if (keysactive[keySettings[KEY_SETTING_DOWN]]) {
            this_player->y += CURRENT_KEY_SPEED;
          }

          if (keysactive[keySettings[KEY_SETTING_LEFT]]) {
            this_player->x -= CURRENT_KEY_SPEED;
          }
          if (keysactive[keySettings[KEY_SETTING_RIGHT]]) {
            this_player->x += CURRENT_KEY_SPEED;
          }

          button[0] |= keysactive[keySettings[KEY_SETTING_FIRE]];
          button[3] |= keysactive[keySettings[KEY_SETTING_CHANGE_FIRE]];
          button[1] |= keysactive[keySettings[KEY_SETTING_LEFT_SIDEKICK]];
          button[2] |= keysactive[keySettings[KEY_SETTING_RIGHT_SIDEKICK]];

          if (constantPlay) {
            for (unsigned int i = 0; i < 4; i++) {
              button[i] = true;
            }

            ++this_player->y;
            this_player->x += constantLastX;
          }

          if (recordDemo) {
            recordDemoKeys();
          }
        }

        if (smoothies[9 - 1]) {
          *mouseY_ = this_player->y - (*mouseY_ - this_player->y);
          mouseYC = -mouseYC;
        }

        accelXC += this_player->x - *mouseX_;
        accelYC += this_player->y - *mouseY_;

        if (mouseXC > 30) {
          mouseXC = 30;
        }
        else if (mouseXC < -30) {
          mouseXC = -30;
        }
        if (mouseYC > 30) {
          mouseYC = 30;
        }
        else if (mouseYC < -30) {
          mouseYC = -30;
        }

        if (mouseXC > 0) {
          this_player->x += (mouseXC + 3) / 4;
        }
        else if (mouseXC < 0) {
          this_player->x += (mouseXC - 3) / 4;
        }
        if (mouseYC > 0) {
          this_player->y += (mouseYC + 3) / 4;
        }
        else if (mouseYC < 0) {
          this_player->y += (mouseYC - 3) / 4;
        }

        if (mouseXC > 3) {
          accelXC++;
        }
        else if (mouseXC < -2) {
          accelXC--;
        }
        if (mouseYC > 2) {
          accelYC++;
        }
        else if (mouseYC < -2) {
          accelYC--;
        }

      }  // endLevel
    }  // isNetworkGame

    // --- Movement Routine Ending ---

    moveOk = true;

    // Street-Fighter codes
    sf_codes(playerNum_, this_player->x, this_player->y, *mouseX_, *mouseY_);

    if (moveOk) {
      // END OF MOVEMENT ROUTINES

      // Linking Routines

      if (twoPlayerMode && !twoPlayerLinked && this_player->x == *mouseX_ && this_player->y == *mouseY_ &&
          abs(player[0].x - player[1].x) < 8 && abs(player[0].y - player[1].y) < 8 && player[0].is_alive &&
          player[1].is_alive && !galagaMode)
      {
        twoPlayerLinked = true;
      }

      if (playerNum_ == 1 && (button[3 - 1] || button[2 - 1]) && !galagaMode) {
        twoPlayerLinked = false;
      }

      if (twoPlayerMode && twoPlayerLinked && playerNum_ == 2 &&
          (this_player->x != *mouseX_ || this_player->y != *mouseY_))
      {
        if (button[0]) {
          if (link_gun_analog) {
            linkGunDirec = link_gun_angle;
          }
          else {
            JE_real tempR;

            if (abs(this_player->x - *mouseX_) > abs(this_player->y - *mouseY_)) {
              tempR = (this_player->x - *mouseX_ > 0) ? M_PI_2 : (M_PI + M_PI_2);
            }
            else {
              tempR = (this_player->y - *mouseY_ > 0) ? 0 : M_PI;
            }

            if (fabsf(linkGunDirec - tempR) < 0.3f) {
              linkGunDirec = tempR;
            }
            else if (linkGunDirec < tempR && linkGunDirec - tempR > -3.24f) {
              linkGunDirec += 0.2f;
            }
            else if (linkGunDirec - tempR < M_PI) {
              linkGunDirec -= 0.2f;
            }
            else {
              linkGunDirec += 0.2f;
            }
          }

          if (linkGunDirec >= (2 * M_PI)) {
            linkGunDirec -= (2 * M_PI);
          }
          else if (linkGunDirec < 0) {
            linkGunDirec += (2 * M_PI);
          }
        }
        else if (!galagaMode) {
          twoPlayerLinked = false;
        }
      }
    }
  }

  if (levelEnd > 0 && all_players_dead()) {
    reallyEndLevel = true;
  }

  // End Level Fade-Out
  if (this_player->is_alive && endLevel) {
    if (levelEnd == 0) {
      reallyEndLevel = true;
    }
    else {
      this_player->y -= levelEndWarp;
      if (this_player->y < -200) {
        reallyEndLevel = true;
      }

      int trail_spacing = 1;
      int trail_y = this_player->y;
      int num_trails = abs(41 - levelEnd);
      if (num_trails > 20) {
        num_trails = 20;
      }

      for (int i = 0; i < num_trails; i++) {
        trail_y += trail_spacing;
        trail_spacing++;
      }

      for (int i = 1; i < num_trails; i++) {
        trail_y -= trail_spacing;
        trail_spacing--;

        if (trail_y <= 0 || trail_y >= 170) {
          continue;
        }

        if (!drawn.trail) {
          drawn.trail = PlayerDrawn::Trail{VGAScreen, this_player->x, {}};
        }
        drawn.trail->trail_y.push_back(trail_y);
      }
    }
  }

  if (playDemo) {
    drawn.insert_coin = VGAScreen;
  }

  if (this_player->is_alive && !endLevel) {
    if (!twoPlayerLinked || playerNum_ < 2) {
      // if not dragonwing
      if (!twoPlayerMode || shipGr2 != 0) {
        if (this_player->sidekick[LEFT_SIDEKICK].style == 0) {
          this_player->sidekick[LEFT_SIDEKICK].x = *mouseX_ - 14;
          this_player->sidekick[LEFT_SIDEKICK].y = *mouseY_;
        }

        if (this_player->sidekick[RIGHT_SIDEKICK].style == 0) {
          this_player->sidekick[RIGHT_SIDEKICK].x = *mouseX_ + 16;
          this_player->sidekick[RIGHT_SIDEKICK].y = *mouseY_;
        }
      }

      if (this_player->x_friction_ticks > 0) {
        --this_player->x_friction_ticks;
      }
      else {
        this_player->x_friction_ticks = 1;

        if (this_player->x_velocity < 0) {
          ++this_player->x_velocity;
        }
        else if (this_player->x_velocity > 0) {
          --this_player->x_velocity;
        }
      }

      if (this_player->y_friction_ticks > 0) {
        --this_player->y_friction_ticks;
      }
      else {
        this_player->y_friction_ticks = 2;

        if (this_player->y_velocity < 0) {
          ++this_player->y_velocity;
        }
        else if (this_player->y_velocity > 0) {
          --this_player->y_velocity;
        }
      }

      this_player->x_velocity += accelXC;
      this_player->y_velocity += accelYC;

      this_player->x_velocity = MIN(MAX(-4, this_player->x_velocity), 4);
      this_player->y_velocity = MIN(MAX(-4, this_player->y_velocity), 4);

      this_player->x += this_player->x_velocity;
      this_player->y += this_player->y_velocity;

      // if player moved, add new ship x, y history entry
      if (this_player->x - *mouseX_ != 0 || this_player->y - *mouseY_ != 0) {
        for (uint i = 1; i < COUNTOF(this_player->old_x); ++i) {
          this_player->old_x[i - 1] = this_player->old_x[i];
          this_player->old_y[i - 1] = this_player->old_y[i];
        }
        this_player->old_x[COUNTOF(this_player->old_x) - 1] = this_player->x;
        this_player->old_y[COUNTOF(this_player->old_x) - 1] = this_player->y;
      }
    }
    // twoPlayerLinked
    else {
      if (shipGr_ == 0) {
        this_player->x = player[0].x - 1;
      }
      else {
        this_player->x = player[0].x;
      }
      this_player->y = player[0].y + 8;

      this_player->x_velocity = player[0].x_velocity;
      this_player->y_velocity = 4;

      // turret direction marker/shield
      shotMultiPos[SHOT_MISC] = 0;
      b = port::player_shot_create(0, SHOT_MISC, this_player->x + 1 + roundf(sinf(linkGunDirec + 0.2f) * 26),
                                   this_player->y + roundf(cosf(linkGunDirec + 0.2f) * 26), *mouseX_,
                                   *mouseY_, 148, playerNum_);
      shotMultiPos[SHOT_MISC] = 0;
      b = port::player_shot_create(0, SHOT_MISC, this_player->x + 1 + roundf(sinf(linkGunDirec - 0.2f) * 26),
                                   this_player->y + roundf(cosf(linkGunDirec - 0.2f) * 26), *mouseX_,
                                   *mouseY_, 148, playerNum_);
      shotMultiPos[SHOT_MISC] = 0;
      b = port::player_shot_create(0, SHOT_MISC, this_player->x + 1 + roundf(sinf(linkGunDirec) * 26),
                                   this_player->y + roundf(cosf(linkGunDirec) * 26), *mouseX_, *mouseY_, 147,
                                   playerNum_);

      if (shotRepeat[SHOT_REAR] > 0) {
        --shotRepeat[SHOT_REAR];
      }
      else if (button[1 - 1]) {
        shotMultiPos[SHOT_REAR] = 0;
        b = port::player_shot_create(0, SHOT_REAR, this_player->x + 1 + roundf(sinf(linkGunDirec) * 20),
                                     this_player->y + roundf(cosf(linkGunDirec) * 20), *mouseX_, *mouseY_,
                                     linkGunWeapons[this_player->items.weapon[REAR_WEAPON].id - 1],
                                     playerNum_);
        port::player_shot_set_direction(b, this_player->items.weapon[REAR_WEAPON].id, linkGunDirec);
      }
    }
  }

  if (!endLevel) {
    if (this_player->x > 256) {
      this_player->x = 256;
      constantLastX = -constantLastX;
    }
    if (this_player->x < 40) {
      this_player->x = 40;
      constantLastX = -constantLastX;
    }

    if (isNetworkGame && playerNum_ == 1) {
      if (this_player->y > 154) {
        this_player->y = 154;
      }
    }
    else {
      if (this_player->y > 160) {
        this_player->y = 160;
      }
    }

    if (this_player->y < 10) {
      this_player->y = 10;
    }

    // Determines the ship banking sprite to display, depending on horizontal velocity and acceleration
    int ship_banking = this_player->x_velocity / 2 + (this_player->x - *mouseX_) / 6;
    ship_banking = MAX(-2, MIN(ship_banking, 2));

    int ship_sprite = ship_banking * 2 + shipGr_;

    sim::Explosions& e = state().explosions;
    e.follow_x = this_player->x - this_player->last_x_explosion_follow;
    e.follow_y = this_player->y - this_player->last_y_explosion_follow;

    if (e.follow_y < 0) {
      e.follow_y = 0;
    }

    this_player->last_x_explosion_follow = this_player->x;
    this_player->last_y_explosion_follow = this_player->y;

    drawn.ship =
        ShipDrawn{VGAScreen, this_player->x,    this_player->y,  ship_sprite,
                  mapX2Ofs,  (bool)background2, (bool)superWild, this_player->invulnerable_ticks > 0};

    if (this_player->invulnerable_ticks > 0) {
      --this_player->invulnerable_ticks;
    }
    else if (shipGr_ == 1) {
      int ship_banking = 0;
      switch (ship_sprite) {
        case 5:
          tempW = this_player->x - 7;
          ship_banking = -2;
          break;
        case 3:
          tempW = this_player->x - 7;
          ship_banking = -1;
          break;
        case 1:
          ship_banking = 0;
          break;
        case -1:
          tempW = this_player->x + 9;
          ship_banking = 1;
          break;
        case -3:
          tempW = this_player->x + 9;
          ship_banking = 2;
          break;
      }
      // NortSparks
      if (ship_banking != 0) {
        if (shotRepeat[SHOT_NORTSPARKS] > 0) {
          --shotRepeat[SHOT_NORTSPARKS];
        }
        else {
          b = port::player_shot_create(0, SHOT_NORTSPARKS, tempW + (mt_rand() % 8) - 4,
                                       this_player->y + (mt_rand() % 8) - 4, *mouseX_, *mouseY_, 671, 1);
          shotRepeat[SHOT_NORTSPARKS] = abs(ship_banking) - 1;
        }
      }
    }

    // Options Location
    // if dragonwing
    if (playerNum_ == 2 && shipGr_ == 0) {
      if (this_player->sidekick[LEFT_SIDEKICK].style == 0) {
        this_player->sidekick[LEFT_SIDEKICK].x = this_player->x - 14 + ship_banking * 2;
        this_player->sidekick[LEFT_SIDEKICK].y = this_player->y;
      }

      if (this_player->sidekick[RIGHT_SIDEKICK].style == 0) {
        this_player->sidekick[RIGHT_SIDEKICK].x = this_player->x + 17 + ship_banking * 2;
        this_player->sidekick[RIGHT_SIDEKICK].y = this_player->y;
      }
    }
  }  // !endLevel

  if (moveOk) {
    if (this_player->is_alive) {
      if (!endLevel) {
        this_player->delta_x_shot_move = this_player->x - this_player->last_x_shot_move;
        this_player->delta_y_shot_move = this_player->y - this_player->last_y_shot_move;

        // PLAYER SHOT Change
        if (button[4 - 1]) {
          portConfigChange = true;
          if (portConfigDone) {
            shotMultiPos[SHOT_REAR] = 0;

            if (superArcadeMode != SA_NONE && superArcadeMode <= SA_NORTSHIPZ) {
              shotMultiPos[SHOT_SPECIAL] = 0;
              shotMultiPos[SHOT_SPECIAL2] = 0;
              if (player[0].items.special == SASpecialWeapon[superArcadeMode - 1]) {
                player[0].items.special = SASpecialWeaponB[superArcadeMode - 1];
                this_player->weapon_mode = 2;
              }
              else {
                player[0].items.special = SASpecialWeapon[superArcadeMode - 1];
                this_player->weapon_mode = 1;
              }
            }
            else if (++this_player->weapon_mode > port_configs()) {
              this_player->weapon_mode = 1;
            }

            draw_port_config_buttons_tick();
            portConfigDone = false;
          }
        }

        // PLAYER SHOT Creation

        // SpecialShot
        if (!galagaMode) {
          do_special_shot(playerNum_, &this_player->armor, &this_player->shield, drawn);
        }

        // Normal Main Weapons
        if (!(twoPlayerLinked && playerNum_ == 2)) {
          int min, max;

          if (!twoPlayerMode) {
            min = 1, max = 2;
          }
          else {
            min = max = playerNum_;
          }

          for (temp = min - 1; temp < max; temp++) {
            uint const item = this_player->items.weapon[temp].id;

            if (item <= 0) {
              continue;
            }

            if (shotRepeat[temp] > 0) {
              --shotRepeat[temp];
            }
            else if (button[1 - 1]) {
              uint const item_power = galagaMode ? 0 : this_player->items.weapon[temp].power - 1,
                         item_mode = (temp == REAR_WEAPON) ? this_player->weapon_mode - 1 : 0;

              b = port::player_shot_create(item, temp, this_player->x, this_player->y, *mouseX_, *mouseY_,
                                           weaponPort[item].op[item_mode][item_power], playerNum_);
            }
          }
        }

        // Super Charge Weapons
        if (playerNum_ == 2) {
          if (!twoPlayerLinked) {
            drawn.charge = PlayerDrawn::Charge{VGAScreen, this_player->x + (shipGr_ == 0) + 1,
                                               this_player->y - 13, 77 + chargeLevel + chargeGr * 19};
          }

          if (chargeGrWait > 0) {
            chargeGrWait--;
          }
          else {
            chargeGr++;
            if (chargeGr == 4) {
              chargeGr = 0;
            }
            chargeGrWait = 3;
          }

          if (chargeLevel > 0) {
            charge_level_tick(193);
          }

          if (chargeWait > 0) {
            chargeWait--;
          }
          else {
            if (chargeLevel < chargeMax) {
              chargeLevel++;
            }

            chargeWait = 28 - this_player->items.weapon[REAR_WEAPON].power * 2;
            if (difficultyLevel > DIFFICULTY_HARD) {
              chargeWait -= 5;
            }
          }

          if (chargeLevel > 0) {
            charge_level_tick(204);
          }

          if (shotRepeat[SHOT_P2_CHARGE] > 0) {
            --shotRepeat[SHOT_P2_CHARGE];
          }
          else if (button[1 - 1] && (!twoPlayerLinked || chargeLevel > 0)) {
            shotMultiPos[SHOT_P2_CHARGE] = 0;
            b = port::player_shot_create(
                16, SHOT_P2_CHARGE, this_player->x, this_player->y, *mouseX_, *mouseY_,
                chargeGunWeapons[player[1].items.weapon[REAR_WEAPON].id - 1] + chargeLevel, playerNum_);

            if (chargeLevel > 0) {
              charge_level_tick(193);
            }

            chargeLevel = 0;
            chargeWait = 30 - this_player->items.weapon[REAR_WEAPON].power * 2;
          }
        }

        // SUPER BOMB
        temp = playerNum_;
        if (temp == 0) {
          temp = 1;  // Get whether player 1 or 2
        }

        if (player[temp - 1].superbombs > 0) {
          if (shotRepeat[SHOT_P1_SUPERBOMB + temp - 1] > 0) {
            --shotRepeat[SHOT_P1_SUPERBOMB + temp - 1];
          }
          else if (button[3 - 1] || button[2 - 1]) {
            --player[temp - 1].superbombs;
            shotMultiPos[SHOT_P1_SUPERBOMB + temp - 1] = 0;
            b = port::player_shot_create(16, SHOT_P1_SUPERBOMB + temp - 1, this_player->x, this_player->y,
                                         *mouseX_, *mouseY_, 535, playerNum_);
          }
        }

        // sidekicks

        if (this_player->sidekick[LEFT_SIDEKICK].style == 4 &&
            this_player->sidekick[RIGHT_SIDEKICK].style == 4)
        {
          optionSatelliteRotate += 0.2f;
        }
        else if (this_player->sidekick[LEFT_SIDEKICK].style == 4 ||
                 this_player->sidekick[RIGHT_SIDEKICK].style == 4)
        {
          optionSatelliteRotate += 0.15f;
        }

        switch (this_player->sidekick[LEFT_SIDEKICK].style) {
          case 1:  // trailing
          case 3:
            this_player->sidekick[LEFT_SIDEKICK].x = this_player->old_x[COUNTOF(this_player->old_x) / 2 - 1];
            this_player->sidekick[LEFT_SIDEKICK].y = this_player->old_y[COUNTOF(this_player->old_x) / 2 - 1];
            break;
          case 2:  // front-mounted
            this_player->sidekick[LEFT_SIDEKICK].x = this_player->x;
            this_player->sidekick[LEFT_SIDEKICK].y = MAX(10, this_player->y - 20);
            break;
          case 4:  // orbiting
            this_player->sidekick[LEFT_SIDEKICK].x =
                this_player->x + roundf(sinf(optionSatelliteRotate) * 20);
            this_player->sidekick[LEFT_SIDEKICK].y =
                this_player->y + roundf(cosf(optionSatelliteRotate) * 20);
            break;
        }

        switch (this_player->sidekick[RIGHT_SIDEKICK].style) {
          case 4:  // orbiting
            this_player->sidekick[RIGHT_SIDEKICK].x =
                this_player->x - roundf(sinf(optionSatelliteRotate) * 20);
            this_player->sidekick[RIGHT_SIDEKICK].y =
                this_player->y - roundf(cosf(optionSatelliteRotate) * 20);
            break;
          case 1:  // trailing
          case 3:
            this_player->sidekick[RIGHT_SIDEKICK].x = this_player->old_x[0];
            this_player->sidekick[RIGHT_SIDEKICK].y = this_player->old_y[0];
            break;
          case 2:  // front-mounted
            if (!optionAttachmentLinked) {
              this_player->sidekick[RIGHT_SIDEKICK].y += optionAttachmentMove / 2;
              if (optionAttachmentMove >= -2) {
                if (optionAttachmentReturn) {
                  temp = 2;
                }
                else {
                  temp = 0;
                }

                if (this_player->sidekick[RIGHT_SIDEKICK].y > (this_player->y - 20) + 5) {
                  temp = 2;
                  optionAttachmentMove -= 1 + optionAttachmentReturn;
                }
                else if (this_player->sidekick[RIGHT_SIDEKICK].y > (this_player->y - 20) - 0) {
                  temp = 3;
                  if (optionAttachmentMove > 0) {
                    optionAttachmentMove--;
                  }
                  else {
                    optionAttachmentMove++;
                  }
                }
                else if (this_player->sidekick[RIGHT_SIDEKICK].y > (this_player->y - 20) - 5) {
                  temp = 2;
                  optionAttachmentMove++;
                }
                else if (optionAttachmentMove < 2 + optionAttachmentReturn * 4) {
                  optionAttachmentMove += 1 + optionAttachmentReturn;
                }

                if (optionAttachmentReturn) {
                  temp = temp * 2;
                }
                if (abs(this_player->sidekick[RIGHT_SIDEKICK].x - this_player->x) < temp) {
                  temp = 1;
                }

                if (this_player->sidekick[RIGHT_SIDEKICK].x > this_player->x) {
                  this_player->sidekick[RIGHT_SIDEKICK].x -= temp;
                }
                else if (this_player->sidekick[RIGHT_SIDEKICK].x < this_player->x) {
                  this_player->sidekick[RIGHT_SIDEKICK].x += temp;
                }

                if (abs(this_player->sidekick[RIGHT_SIDEKICK].y - (this_player->y - 20)) +
                        abs(this_player->sidekick[RIGHT_SIDEKICK].x - this_player->x) <
                    8)
                {
                  optionAttachmentLinked = true;
                  soundQueue[2] = S_CLINK;
                }

                if (button[3 - 1]) {
                  optionAttachmentReturn = true;
                }
              }
              // sidekick needs to catch up to player
              else {
                optionAttachmentMove += 1 + optionAttachmentReturn;
                setup_explosion(this_player->sidekick[RIGHT_SIDEKICK].x + 1,
                                this_player->sidekick[RIGHT_SIDEKICK].y + 10, 0, 0, false, false);
              }
            }
            else {
              this_player->sidekick[RIGHT_SIDEKICK].x = this_player->x;
              this_player->sidekick[RIGHT_SIDEKICK].y = this_player->y - 20;
              if (button[3 - 1]) {
                optionAttachmentLinked = false;
                optionAttachmentReturn = false;
                optionAttachmentMove = -20;
                soundQueue[3] = S_WEAPON_26;
              }
            }

            if (this_player->sidekick[RIGHT_SIDEKICK].y < 10) {
              this_player->sidekick[RIGHT_SIDEKICK].y = 10;
            }
            break;
        }

        // if player has sidekicks
        if (playerNum_ == 2 || !twoPlayerMode) {
          for (uint i = 0; i < COUNTOF(this_player->items.sidekick); ++i) {
            uint shot_i = (i == 0) ? SHOT_LEFT_SIDEKICK : SHOT_RIGHT_SIDEKICK;

            JE_OptionType *this_option = &options[this_player->items.sidekick[i]];

            if (this_option->wport <= 0) {
              continue;
            }

            // fire/refill sidekick
            if (shotRepeat[shot_i] > 0) {
              --shotRepeat[shot_i];
            }
            else {
              int const ammo_max = this_player->sidekick[i].ammo_max;

              // sidekick has limited ammo
              if (ammo_max > 0) {
                if (this_player->sidekick[i].ammo_refill_ticks > 0) {
                  --this_player->sidekick[i].ammo_refill_ticks;
                }
                // refill one ammo
                else {
                  this_player->sidekick[i].ammo_refill_ticks = this_player->sidekick[i].ammo_refill_ticks_max;

                  if (this_player->sidekick[i].ammo < ammo_max) {
                    ++this_player->sidekick[i].ammo;
                  }

                  // draw sidekick refill ammo gauge
                  int const y = hud_sidekick_y[twoPlayerMode ? 1 : 0][i] + 13;
                  sidekick_ammo_tick(y, ammo_max, this_player->sidekick[i].ammo, false);
                }

                if (button[1 + i] && this_player->sidekick[i].ammo > 0) {
                  b = port::player_shot_create(
                      this_option->wport, shot_i, this_player->sidekick[i].x, this_player->sidekick[i].y,
                      *mouseX_, *mouseY_, this_option->wpnum + this_player->sidekick[i].charge, playerNum_);

                  --this_player->sidekick[i].ammo;
                  if (this_player->sidekick[i].charge > 0) {
                    shotMultiPos[shot_i] = 0;
                    this_player->sidekick[i].charge = 0;
                  }
                  this_player->sidekick[i].charge_ticks = 20;
                  this_player->sidekick[i].animation_enabled = true;

                  // draw sidekick discharge ammo gauge
                  int const y = hud_sidekick_y[twoPlayerMode ? 1 : 0][i] + 13;
                  sidekick_ammo_tick(y, ammo_max, this_player->sidekick[i].ammo, true);
                }
              }
              // has infinite ammo
              else {
                if (button[0] || button[1 + i]) {
                  b = port::player_shot_create(
                      this_option->wport, shot_i, this_player->sidekick[i].x, this_player->sidekick[i].y,
                      *mouseX_, *mouseY_, this_option->wpnum + this_player->sidekick[i].charge, playerNum_);

                  if (this_player->sidekick[i].charge > 0) {
                    shotMultiPos[shot_i] = 0;
                    this_player->sidekick[i].charge = 0;
                  }
                  this_player->sidekick[i].charge_ticks = 20;
                  this_player->sidekick[i].animation_enabled = true;
                }
              }
            }
          }
        }  // end of if player has sidekicks
      }  // !endLevel
    }  // this_player->is_alive
  }  // moveOK

  if (!((playerNum_ == 2 || !twoPlayerMode) && !endLevel)) {
    return;
  }

  // draw sidekicks
  for (uint i = 0; i < COUNTOF(this_player->sidekick); ++i) {
    JE_OptionType *this_option = &options[this_player->items.sidekick[i]];

    if (this_option->option > 0) {
      if (this_player->sidekick[i].animation_enabled) {
        if (++this_player->sidekick[i].animation_frame >= this_option->ani) {
          this_player->sidekick[i].animation_frame = 0;
          this_player->sidekick[i].animation_enabled = (this_option->option == 1);
        }
      }

      int const x = this_player->sidekick[i].x, y = this_player->sidekick[i].y;
      uint const sprite =
          this_option->gr[this_player->sidekick[i].animation_frame] + this_player->sidekick[i].charge;

      drawn.sidekicks[i] = PlayerDrawn::Sidekick{VGAScreen, x, y, sprite, this_player->sidekick[i].style};
    }

    if (--this_player->sidekick[i].charge_ticks != 0) {
      continue;
    }

    if (this_player->sidekick[i].charge < this_option->pwr) {
      ++this_player->sidekick[i].charge;
    }
    this_player->sidekick[i].charge_ticks = 20;
  }
}

}  // namespace

// Port of JE_mainGamePlayerFunctions.
void main_game_player_functions() {
  auto& twoPlayerMode = state().game.two_player_mode;
  auto& galagaMode = state().game.galaga_mode;
  auto& portConfigChange = state().display.port_config_change;
  auto& mapXPos = state().map.map_x_pos;
  auto& oldMapXOfs = state().map.old_map_x_ofs;
  auto& mapXOfs = state().map.map_x_ofs;
  auto& mapX2Ofs = state().map.map_x2_ofs;
  auto& mapX2Pos = state().map.map_x2_pos;
  auto& mapX3Pos = state().map.map_x3_pos;
  auto& mapX3Ofs = state().map.map_x3_ofs;
  auto& mapXbpPos = state().map.map_x_bp_pos;
  auto& mapX2bpPos = state().map.map_x2_bp_pos;
  auto& mapX3bpPos = state().map.map_x3_bp_pos;
  auto& background3x1 = state().map.background3x1;
  auto& endLevel = state().ending.end_level;
  auto& levelEnd = state().ending.countdown;
  auto& levelEndWarp = state().ending.warp;
  auto& SFExecuted = state().specials.sf_executed;
  auto& shipGr = state().players.ship_gr;
  auto& shipGr2 = state().players.ship_gr2;
  auto& shipGrPtr = state().players.ship_gr_ptr;
  auto& shipGr2ptr = state().players.ship_gr2_ptr;
  auto& player = state().players.slots;
  // PLAYER MOVEMENT/MOUSE ROUTINES

  if (endLevel && levelEnd > 0) {
    levelEnd--;
    levelEndWarp++;
  }

  // Reset Street-Fighter commands
  SFExecuted.fill(0);

  portConfigChange = false;

  if (twoPlayerMode) {
    player_movement(&player[0], !galagaMode ? inputDevice[0] : 0, 1, shipGr, shipGrPtr, &player[0].mouseX,
                    &player[0].mouseY);
    player_movement(&player[1], !galagaMode ? inputDevice[1] : 0, 2, shipGr2, shipGr2ptr, &player[1].mouseX,
                    &player[1].mouseY);
  }
  else {
    player_movement(&player[0], 0, 1, shipGr, shipGrPtr, &player[0].mouseX, &player[0].mouseY);
  }

  // == Parallax Map Scrolling ==
  JE_word tempX;
  if (twoPlayerMode) {
    tempX = (player[0].x + player[1].x) / 2;
  }
  else {
    tempX = player[0].x;
  }

  tempW = floorf((float)(260 - (tempX - 36)) / (260 - 36) * (24 * 3) - 1);
  mapX3Ofs = tempW;
  mapX3Pos = mapX3Ofs % 24;
  mapX3bpPos = 1 - (mapX3Ofs / 24);

  mapX2Ofs = (tempW * 2) / 3;
  mapX2Pos = mapX2Ofs % 24;
  mapX2bpPos = 1 - (mapX2Ofs / 24);

  oldMapXOfs = mapXOfs;
  mapXOfs = mapX2Ofs / 2;
  mapXPos = mapXOfs % 24;
  mapXbpPos = 1 - (mapXOfs / 24);

  if (background3x1) {
    mapX3Ofs = mapXOfs;
    mapX3Pos = mapXPos;
    mapX3bpPos = mapXbpPos - 1;
  }
}

// Port of JE_playerDamage.
JE_byte player_damage(JE_byte temp, Player *this_player) {
  auto& twoPlayerMode = state().game.two_player_mode;
  auto& youAreCheating = state().cheats.you_are_cheating;
  auto& soundQueue = state().sound.queue;
  auto& tempVolume = state().sound.temp_volume;
  auto& levelEnd = state().ending.countdown;
  auto& levelTimer = state().level.timer;
  int playerDamage = 0;
  soundQueue[7] = S_SHIELD_HIT;

  // Player Damage Routines
  if (this_player->shield < temp) {
    playerDamage = temp;
    temp -= this_player->shield;
    this_player->shield = 0;

    if (temp > 0) {
      // Through Shields - Now Armor

      if (this_player->armor < temp) {
        temp -= this_player->armor;
        this_player->armor = 0;

        if (this_player->is_alive && !youAreCheating) {
          levelTimer = false;
          this_player->is_alive = false;
          this_player->exploding_ticks = 60;
          levelEnd = 40;
          tempVolume = tyrMusicVolume;
          soundQueue[1] = S_EXPLOSION_22;
        }
      }
      else {
        this_player->armor -= temp;
        soundQueue[7] = S_HULL_HIT;
      }
    }
  }
  else {
    this_player->shield -= temp;

    setup_explosion(this_player->x - 17, this_player->y - 12, 0, 14, false, !twoPlayerMode);
    setup_explosion(this_player->x - 5, this_player->y - 12, 0, 15, false, !twoPlayerMode);
    setup_explosion(this_player->x + 7, this_player->y - 12, 0, 16, false, !twoPlayerMode);
    setup_explosion(this_player->x + 19, this_player->y - 12, 0, 17, false, !twoPlayerMode);

    setup_explosion(this_player->x - 17, this_player->y + 2, 0, 18, false, !twoPlayerMode);
    setup_explosion(this_player->x + 19, this_player->y + 2, 0, 19, false, !twoPlayerMode);

    setup_explosion(this_player->x - 17, this_player->y + 16, 0, 20, false, !twoPlayerMode);
    setup_explosion(this_player->x - 5, this_player->y + 16, 0, 21, false, !twoPlayerMode);
    setup_explosion(this_player->x + 7, this_player->y + 16, 0, 22, false, !twoPlayerMode);
  }

  wipe_shield_armor_bars_tick();
  VGAScreen = VGAScreenSeg;  // side-effect of game_screen
  draw_shield_tick();
  draw_armor_tick();
  VGAScreen = game_screen;  // side-effect of game_screen

  return playerDamage;
}

// Port of JE_playerCollide.
void player_collide(Player *this_player, JE_byte playerNum_) {
  auto& twoPlayerMode = state().game.two_player_mode;
  auto& onePlayerAction = state().game.one_player_action;
  auto& galagaMode = state().game.galaga_mode;
  auto& superArcadeMode = state().game.super_arcade_mode;
  auto& cubeMax = state().progress.cube_max;
  auto& soundQueue = state().sound.queue;
  auto& displayTime = state().display.display_time;
  auto& bonusLevel = state().ending.bonus_level;
  auto& nextLevel = state().ending.next_level;
  auto& shotRepeat = state().players.shot_repeat;
  auto& shotMultiPos = state().players.shot_multi_pos;
  auto& damageRate = state().players.damage_rate;
  auto& twoPlayerLinked = state().players.two_player_linked;
  auto& player = state().players.slots;
  auto& enemy = state().enemies.slots;
  auto& enemyAvail = state().enemies.status;
  auto& shotAvail = state().player_shots.ttl;
  char tempStr[256];

  for (int z = 0; z < 100; z++) {
    if (enemyAvail[z] == 1) {
      continue;
    }

    int enemy_screen_x = enemy[z].ex + enemy[z].mapoffset;

    if (abs(this_player->x - enemy_screen_x) >= 12 || abs(this_player->y - enemy[z].ey) >= 14) {
      continue;
    }

    // Collide
    int evalue = enemy[z].evalue;
    if (evalue > 29999) {
      // spawn dragonwing in galaga mode, otherwise just a purple ball
      if (evalue == 30000) {
        this_player->cash += 100;

        if (!galagaMode) {
          port::handle_got_purple_ball(this_player);
        }
        else {
          // spawn the dragonwing?
          if (twoPlayerMode) {
            this_player->cash += 2400;
          }
          twoPlayerMode = true;
          twoPlayerLinked = true;
          player[1].items.weapon[REAR_WEAPON].power = 1;
          player[1].armor = 10;
          player[1].is_alive = true;
        }
        enemyAvail[z] = 1;
        soundQueue[7] = S_POWERUP;
      }
      else if (superArcadeMode != SA_NONE && evalue > 30000) {
        shotMultiPos[SHOT_FRONT] = 0;
        shotRepeat[SHOT_FRONT] = 10;

        tempW = SAWeapon[superArcadeMode - 1][evalue - 30000 - 1];

        // if picked up already-owned weapon, power weapon up
        if (tempW == player[0].items.weapon[FRONT_WEAPON].id) {
          this_player->cash += 1000;
          port::power_up_weapon(this_player, FRONT_WEAPON);
        }
        // else weapon also gives purple ball
        else {
          port::handle_got_purple_ball(this_player);
        }

        player[0].items.weapon[FRONT_WEAPON].id = tempW;
        this_player->cash += 200;
        soundQueue[7] = S_POWERUP;
        enemyAvail[z] = 1;
      }
      else if (evalue > 32100) {
        if (playerNum_ == 1) {
          this_player->cash += 250;
          player[0].items.special = evalue - 32100;
          shotMultiPos[SHOT_SPECIAL] = 0;
          shotRepeat[SHOT_SPECIAL] = 10;
          shotMultiPos[SHOT_SPECIAL2] = 0;
          shotRepeat[SHOT_SPECIAL2] = 0;

          if (isNetworkGame) {
            snprintf(tempStr, sizeof tempStr, "%s %s %s", JE_getName(1), miscTextB[4 - 1],
                     special[evalue - 32100].name);
          }
          else if (twoPlayerMode) {
            sprintf(tempStr, "%s %s", miscText[43 - 1], special[evalue - 32100].name);
          }
          else {
            sprintf(tempStr, "%s %s", miscText[64 - 1], special[evalue - 32100].name);
          }
          draw_text_window_tick(tempStr);
          soundQueue[7] = S_POWERUP;
          enemyAvail[z] = 1;
        }
      }
      else if (evalue > 32000) {
        if (playerNum_ == 2) {
          enemyAvail[z] = 1;
          if (isNetworkGame) {
            snprintf(tempStr, sizeof tempStr, "%s %s %s", JE_getName(2), miscTextB[4 - 1],
                     options[evalue - 32000].name);
          }
          else {
            sprintf(tempStr, "%s %s", miscText[44 - 1], options[evalue - 32000].name);
          }
          draw_text_window_tick(tempStr);

          // if picked up a different sidekick than player already has, then reset sidekicks to least
          // powerful, else power them up
          if (evalue - 32000u != player[1].items.sidekick_series) {
            player[1].items.sidekick_series = evalue - 32000;
            player[1].items.sidekick_level = 101;
          }
          else if (player[1].items.sidekick_level < 103) {
            ++player[1].items.sidekick_level;
          }

          uint temp = player[1].items.sidekick_level - 100 - 1;
          for (uint i = 0; i < COUNTOF(player[1].items.sidekick); ++i) {
            player[1].items.sidekick[i] = optionSelect[player[1].items.sidekick_series][temp][i];
          }

          shotMultiPos[SHOT_LEFT_SIDEKICK] = 0;
          shotMultiPos[SHOT_RIGHT_SIDEKICK] = 0;
          draw_options_tick();
          soundQueue[7] = S_POWERUP;
        }
        else if (onePlayerAction) {
          enemyAvail[z] = 1;
          sprintf(tempStr, "%s %s", miscText[64 - 1], options[evalue - 32000].name);
          draw_text_window_tick(tempStr);

          for (uint i = 0; i < COUNTOF(player[0].items.sidekick); ++i) {
            player[0].items.sidekick[i] = evalue - 32000;
          }
          shotMultiPos[SHOT_LEFT_SIDEKICK] = 0;
          shotMultiPos[SHOT_RIGHT_SIDEKICK] = 0;

          draw_options_tick();
          soundQueue[7] = S_POWERUP;
        }
        if (enemyAvail[z] == 1) {
          this_player->cash += 250;
        }
      }
      else if (evalue > 31000) {
        this_player->cash += 250;
        if (playerNum_ == 2) {
          if (isNetworkGame) {
            snprintf(tempStr, sizeof tempStr, "%s %s %s", JE_getName(2), miscTextB[4 - 1],
                     weaponPort[evalue - 31000].name);
          }
          else {
            sprintf(tempStr, "%s %s", miscText[44 - 1], weaponPort[evalue - 31000].name);
          }
          draw_text_window_tick(tempStr);
          player[1].items.weapon[REAR_WEAPON].id = evalue - 31000;
          shotMultiPos[SHOT_REAR] = 0;
          enemyAvail[z] = 1;
          soundQueue[7] = S_POWERUP;
        }
        else if (onePlayerAction) {
          sprintf(tempStr, "%s %s", miscText[64 - 1], weaponPort[evalue - 31000].name);
          draw_text_window_tick(tempStr);
          player[0].items.weapon[REAR_WEAPON].id = evalue - 31000;
          shotMultiPos[SHOT_REAR] = 0;
          enemyAvail[z] = 1;
          soundQueue[7] = S_POWERUP;

          // does this ever happen?
          if (player[0].items.weapon[REAR_WEAPON].power == 0) {
            player[0].items.weapon[REAR_WEAPON].power = 1;
          }
        }
      }
      else if (evalue > 30000) {
        if (playerNum_ == 1 && twoPlayerMode) {
          if (isNetworkGame) {
            snprintf(tempStr, sizeof tempStr, "%s %s %s", JE_getName(1), miscTextB[4 - 1],
                     weaponPort[evalue - 30000].name);
          }
          else {
            sprintf(tempStr, "%s %s", miscText[43 - 1], weaponPort[evalue - 30000].name);
          }
          draw_text_window_tick(tempStr);
          player[0].items.weapon[FRONT_WEAPON].id = evalue - 30000;
          shotMultiPos[SHOT_FRONT] = 0;
          enemyAvail[z] = 1;
          soundQueue[7] = S_POWERUP;
        }
        else if (onePlayerAction) {
          sprintf(tempStr, "%s %s", miscText[64 - 1], weaponPort[evalue - 30000].name);
          draw_text_window_tick(tempStr);
          player[0].items.weapon[FRONT_WEAPON].id = evalue - 30000;
          shotMultiPos[SHOT_FRONT] = 0;
          enemyAvail[z] = 1;
          soundQueue[7] = S_POWERUP;
        }

        if (enemyAvail[z] == 1) {
          player[0].items.special = specialArcadeWeapon[evalue - 30000 - 1];
          if (player[0].items.special > 0) {
            shotMultiPos[SHOT_SPECIAL] = 0;
            shotRepeat[SHOT_SPECIAL] = 0;
            shotMultiPos[SHOT_SPECIAL2] = 0;
            shotRepeat[SHOT_SPECIAL2] = 0;
          }
          this_player->cash += 250;
        }
      }
    }
    else if (evalue > 20000) {
      if (twoPlayerLinked) {
        // share the armor evenly between linked players
        for (uint i = 0; i < COUNTOF(player); ++i) {
          player[i].armor += (evalue - 20000) / COUNTOF(player);
          if (player[i].armor > 28) {
            player[i].armor = 28;
          }
        }
      }
      else {
        this_player->armor += evalue - 20000;
        if (this_player->armor > 28) {
          this_player->armor = 28;
        }
      }
      enemyAvail[z] = 1;
      VGAScreen = VGAScreenSeg;  // side-effect of game_screen
      draw_armor_tick();
      VGAScreen = game_screen;  // side-effect of game_screen
      soundQueue[7] = S_POWERUP;
    }
    else if (evalue > 10000 && enemyAvail[z] == 2) {
      if (!bonusLevel) {
        play_song(30);  // Zanac
        bonusLevel = true;
        nextLevel = evalue - 10000;
        enemyAvail[z] = 1;
        displayTime = 150;
      }
    }
    else if (enemy[z].scoreitem) {
      enemyAvail[z] = 1;
      soundQueue[7] = S_ITEM;
      if (evalue == 1) {
        cubeMax++;
        soundQueue[3] = V_DATA_CUBE;
      }
      // got front weapon powerup
      else if (evalue == -1) {
        if (isNetworkGame) {
          snprintf(tempStr, sizeof tempStr, "%s %s %s", JE_getName(1), miscTextB[4 - 1], miscText[45 - 1]);
        }
        else if (twoPlayerMode) {
          sprintf(tempStr, "%s %s", miscText[43 - 1], miscText[45 - 1]);
        }
        else {
          strcpy(tempStr, miscText[45 - 1]);
        }
        draw_text_window_tick(tempStr);

        port::power_up_weapon(&player[0], FRONT_WEAPON);
        soundQueue[7] = S_POWERUP;
      }
      // got rear weapon powerup
      else if (evalue == -2) {
        if (isNetworkGame) {
          snprintf(tempStr, sizeof tempStr, "%s %s %s", JE_getName(2), miscTextB[4 - 1], miscText[46 - 1]);
        }
        else if (twoPlayerMode) {
          sprintf(tempStr, "%s %s", miscText[44 - 1], miscText[46 - 1]);
        }
        else {
          strcpy(tempStr, miscText[46 - 1]);
        }
        draw_text_window_tick(tempStr);

        port::power_up_weapon(twoPlayerMode ? &player[1] : &player[0], REAR_WEAPON);
        soundQueue[7] = S_POWERUP;
      }
      else if (evalue == -3) {
        // picked up orbiting asteroid killer
        shotMultiPos[SHOT_MISC] = 0;
        b = port::player_shot_create(0, SHOT_MISC, this_player->x, this_player->y, player[0].mouseX,
                                     player[0].mouseY, 104, playerNum_);
        shotAvail[z] = 0;
      }
      else if (evalue == -4) {
        if (player[playerNum_ - 1].superbombs < 10) {
          ++player[playerNum_ - 1].superbombs;
        }
      }
      else if (evalue == -5) {
        player[0].items.weapon[FRONT_WEAPON].id = 25;  // HOT DOG!
        player[0].items.weapon[REAR_WEAPON].id = 26;
        player[1].items.weapon[REAR_WEAPON].id = 26;

        player[0].last_items = player[0].items;

        for (uint i = 0; i < COUNTOF(player); ++i) {
          player[i].weapon_mode = 1;
        }

        shotMultiPos.fill(0);
      }
      else if (twoPlayerLinked) {
        // players get equal share of pick-up cash when linked
        for (uint i = 0; i < COUNTOF(player); ++i) {
          player[i].cash += evalue / COUNTOF(player);
        }
      }
      else {
        this_player->cash += evalue;
      }
      setup_explosion(enemy_screen_x, enemy[z].ey, 0, enemyDat[enemy[z].enemytype].explosiontype, true,
                      false);
    }
    // explosiontype & 1 == 0: not ground enemy
    else if (this_player->invulnerable_ticks == 0 && enemyAvail[z] == 0 &&
             (enemyDat[enemy[z].enemytype].explosiontype & 1) == 0)
    {
      int armorleft = enemy[z].armorleft;
      if (armorleft > damageRate) {
        armorleft = damageRate;
      }

      player_damage(armorleft, this_player);

      // player ship gets push-back from collision
      if (enemy[z].armorleft > 0) {
        this_player->x_velocity += (enemy[z].exc * enemy[z].armorleft) / 2;
        this_player->y_velocity += (enemy[z].eyc * enemy[z].armorleft) / 2;
      }

      int armorleft2 = enemy[z].armorleft;
      if (armorleft2 == 255) {
        armorleft2 = 30000;
      }

      temp = enemy[z].linknum;
      if (temp == 0) {
        temp = 255;
      }

      b = z;

      if (armorleft2 > armorleft) {
        // damage enemy
        if (enemy[z].armorleft != 255) {
          enemy[z].armorleft -= armorleft;
        }
        soundQueue[5] = S_ENEMY_HIT;
      }
      else {
        // kill enemy
        for (temp2 = 0; temp2 < 100; temp2++) {
          if (enemyAvail[temp2] == 1) {
            continue;
          }

          temp3 = enemy[temp2].linknum;
          if (!(temp2 == b || (temp != 255 && (temp == temp3 || temp - 100 == temp3 ||
                                               (temp3 > 40 && temp3 / 20 == temp / 20 && temp3 <= temp)))))
          {
            continue;
          }

          int enemy_screen_x = enemy[temp2].ex + enemy[temp2].mapoffset;

          enemy[temp2].linknum = 0;

          enemyAvail[temp2] = 1;

          if (enemyDat[enemy[temp2].enemytype].esize == 1) {
            setup_explosion_large(enemy[temp2].enemyground, enemy[temp2].explonum, enemy_screen_x,
                                  enemy[temp2].ey);
            soundQueue[6] = S_EXPLOSION_9;
          }
          else {
            setup_explosion(enemy_screen_x, enemy[temp2].ey, 0, 1, false, false);
            soundQueue[5] = S_EXPLOSION_4;
          }
        }
        enemyAvail[z] = 1;
      }
    }
  }
}

// Port of JE_SFCodes (mainint.c).
void sf_codes(JE_byte playerNum_, JE_integer PX_, JE_integer PY_, JE_integer mouseX_, JE_integer mouseY_) {
  auto& superTyrian = state().game.super_tyrian;
  auto& button = state().input.button;
  auto& SFCurrentCode = state().specials.sf_current_code;
  auto& SFExecuted = state().specials.sf_executed;
  auto& player = state().players.slots;
  JE_byte temp, temp2, temp3, temp4, temp5;

  uint ship = player[playerNum_ - 1].items.ship;

  // Get direction
  if (playerNum_ == 2 && ship < 15) {
    ship = 0;
  }

  if (ship >= 15) {
    return;
  }

  temp2 = (mouseY_ > PY_) +     // UP
          (mouseY_ < PY_) +     // DOWN
          (PX_ < mouseX_) +     // LEFT
          (PX_ > mouseX_);      // RIGHT
  temp = (mouseY_ > PY_) * 1 +  // UP
         (mouseY_ < PY_) * 2 +  // DOWN
         (PX_ < mouseX_) * 3 +  // LEFT
         (PX_ > mouseX_) * 4;   // RIGHT

  // no direction being pressed
  if (temp == 0) {
    // if fire button is released
    if (!button[0]) {
      temp = 9;
      temp2 = 1;
    }
    else {
      temp2 = 0;
      temp = 99;
    }
  }

  if (temp2 != 1) {
    return;
  }

  // if exactly one direction pressed or fire button is released
  temp += button[0] * 4;

  temp3 = superTyrian ? 21 : 3;
  for (temp2 = 0; temp2 < temp3; temp2++) {
    // Use SuperTyrian ShipCombos or not?
    temp5 = superTyrian ? shipCombosB[temp2] : shipCombos[ship][temp2];

    // temp5 == selected combo in ship
    // combo doesn't exists
    if (temp5 == 0) {
      // mark twiddles as cancelled/finished
      SFCurrentCode[playerNum_ - 1][temp2] = 0;
    }
    else {
      // get next combo key
      temp4 = keyboardCombos[temp5 - 1][SFCurrentCode[playerNum_ - 1][temp2]];

      // correct key
      if (temp4 == temp) {
        SFCurrentCode[playerNum_ - 1][temp2]++;

        temp4 = keyboardCombos[temp5 - 1][SFCurrentCode[playerNum_ - 1][temp2]];
        if (temp4 > 100 && temp4 <= 100 + SPECIAL_NUM) {
          SFCurrentCode[playerNum_ - 1][temp2] = 0;
          SFExecuted[playerNum_ - 1] = temp4 - 100;
        }
      }
      else {
        if ((temp != 9) && (temp4 - 1) % 4 != (temp - 1) % 4 &&
            (SFCurrentCode[playerNum_ - 1][temp2] == 0 ||
             keyboardCombos[temp5 - 1][SFCurrentCode[playerNum_ - 1][temp2] - 1] != temp))
        {
          SFCurrentCode[playerNum_ - 1][temp2] = 0;
        }
      }
    }
  }
}

// Port of JE_SGr (varz.c).
JE_word s_gr(JE_word ship, Sprite2_array **ptr) {
  JE_word const GR[15] = {233, 157, 195, 271, 81, 0, 119, 5, 43, 81, 119, 157, 195, 233, 271};  // [1..15]

  JE_word tempW = extraShips[(ship - 1) * 15];
  if (tempW > 7) {
    *ptr = &extraShapes;
  }

  return GR[tempW - 1];
}

// Port of JE_getShipInfo (varz.c).
void get_ship_info(void) {
  auto& powerAdd = state().players.power_add;
  auto& shipGr = state().players.ship_gr;
  auto& shipGr2 = state().players.ship_gr2;
  auto& shipGrPtr = state().players.ship_gr_ptr;
  auto& shipGr2ptr = state().players.ship_gr2_ptr;
  auto& player = state().players.slots;
  JE_boolean extraShip, extraShip2;

  shipGrPtr = &spriteSheet9;
  shipGr2ptr = &spriteSheet9;

  powerAdd = powerSys[player[0].items.generator].power;

  extraShip = player[0].items.ship > 90;
  if (extraShip) {
    JE_byte base = (player[0].items.ship - 91) * 15;
    shipGr = s_gr(player[0].items.ship - 90, &shipGrPtr);
    player[0].armor = extraShips[base + 7];
  }
  else {
    shipGr = ships[player[0].items.ship].shipgraphic;
    player[0].armor = ships[player[0].items.ship].dmg;
  }

  extraShip2 = player[1].items.ship > 90;
  if (extraShip2) {
    JE_byte base2 = (player[1].items.ship - 91) * 15;
    shipGr2 = s_gr(player[1].items.ship - 90, &shipGr2ptr);
    player[1].armor = extraShips[base2 + 7];  // bug?
  }
  else {
    shipGr2 = 0;
    player[1].armor = 10;
  }

  for (uint i = 0; i < COUNTOF(player); ++i) {
    player[i].initial_armor = player[i].armor;

    uint temp = ((i == 0 && extraShip) || (i == 1 && extraShip2)) ? 2 : ships[player[i].items.ship].ani;

    if (temp == 0) {
      player[i].shot_hit_area_x = 12;
      player[i].shot_hit_area_y = 10;
    }
    else {
      player[i].shot_hit_area_x = 11;
      player[i].shot_hit_area_y = 14;
    }
  }
}

// Port of JE_portConfigs (varz.c).
JE_word port_configs(void) {
  auto& twoPlayerMode = state().game.two_player_mode;
  auto& player = state().players.slots;
  uint const player_index = twoPlayerMode ? 1 : 0;
  return tempW = weaponPort[player[player_index].items.weapon[REAR_WEAPON].id].opnum;
}

// Port of calc_purple_balls_needed (player.c).
void calc_purple_balls_needed(Player *this_player) {
  static uint const purple_balls_required[12] = {1, 1, 2, 4, 8, 12, 16, 20, 25, 30, 40, 50};

  this_player->purple_balls_needed = purple_balls_required[*this_player->lives];
}

// Port of power_up_weapon (player.c).
bool power_up_weapon(Player *this_player, uint port) {
  auto& shotMultiPos = state().players.shot_multi_pos;
  bool const can_power_up = this_player->items.weapon[port].id != 0 &&   // not None
                            this_player->items.weapon[port].power < 11;  // not at max power
  if (can_power_up) {
    ++this_player->items.weapon[port].power;
    shotMultiPos[port] = 0;  // TODO: should be part of Player structure

    port::calc_purple_balls_needed(this_player);
  }
  // cash consolation prize
  else {
    this_player->cash += 1000;
  }

  return can_power_up;
}

// Port of handle_got_purple_ball (player.c).
void handle_got_purple_ball(Player *this_player) {
  if (this_player->purple_balls_needed > 1) {
    --this_player->purple_balls_needed;
  }
  else {
    port::power_up_weapon(this_player, this_player->is_dragonwing ? REAR_WEAPON : FRONT_WEAPON);
  }
}

}  // namespace tyrx::port
