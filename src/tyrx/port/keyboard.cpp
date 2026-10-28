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

// The game's key commands during a level (JE_mainKeyboardInput in
// mainint.c): edited ships, help, cheats, debug and screen settings.  What it
// draws (the HUD after a ship change, a cheat's message) goes in the HUD log
// of the draw record.

#include "level.hpp"
#include "port.hpp"

#include <cassert>
#include <cstring>

namespace tyrx::port {

namespace {

// Port of JE_setNewGameSpeed (config.c).
void set_new_game_speed() {
  auto& pentiumMode = state().game.pentium_mode;
  auto& fastPlay = state().game.fast_play;
  auto& frameCountMax = state().game.frame_count_max;

  pentiumMode = false;

  Uint16 speed;
  switch (fastPlay) {
    default:
      assert(false);
      [[fallthrough]];
    case 0:  // Normal
      speed = 0x4300;
      smoothScroll = true;
      frameCountMax = 2;
      break;
    case 1:  // Pentium Hyper
      speed = 0x3000;
      smoothScroll = true;
      frameCountMax = 2;
      break;
    case 2:
      speed = 0x2000;
      smoothScroll = false;
      frameCountMax = 2;
      break;
    case 3:  // Slug mode
      speed = 0x5300;
      smoothScroll = true;
      frameCountMax = 4;
      break;
    case 4:  // Slower
      speed = 0x4300;
      smoothScroll = true;
      frameCountMax = 3;
      break;
    case 5:  // Slow
      speed = 0x4300;
      smoothScroll = true;
      frameCountMax = 2;
      pentiumMode = true;
      break;
  }

  setFrameSpeed(speed);
  setFrameCount(frameCountMax);
}

}  // namespace

// Port of JE_mainKeyboardInput.
void main_keyboard_input() {
  auto& twoPlayerMode = state().game.two_player_mode;
  auto& superArcadeMode = state().game.super_arcade_mode;
  auto& superTyrian = state().game.super_tyrian;
  auto& fastPlay = state().game.fast_play;
  auto& youAreCheating = state().cheats.you_are_cheating;
  auto& constantPlay = state().cheats.constant_play;
  auto& superPause = state().cheats.super_pause;
  auto& levelFilter = state().display.filter;
  auto& smoothies = state().display.smoothies;
  auto& endLevel = state().ending.end_level;
  auto& levelEnd = state().ending.countdown;
  auto& levelTimer = state().level.timer;
  auto& levelTimerCountdown = state().level.timer_countdown;
  auto& shotMultiPos = state().players.shot_multi_pos;
  auto& shipGr = state().players.ship_gr;
  auto& player = state().players.slots;
  JE_gammaCheck();

  // { Network Request Commands }

  if (!isNetworkGame) {
    // { Edited Ships } for Player 1
    if (extraAvail && keysactive[SDL_SCANCODE_TAB] && !isNetworkGame && !superTyrian) {
      for (x = SDL_SCANCODE_1; x <= SDL_SCANCODE_0; x++) {
        if (!keysactive[x]) {
          continue;
        }

        int z = x - SDL_SCANCODE_1 + 1;
        player[0].items.ship = 90 + z;  // Ships
        z = (z - 1) * 15;
        player[0].items.weapon[FRONT_WEAPON].id = extraShips[z + 1];
        player[0].items.weapon[REAR_WEAPON].id = extraShips[z + 2];
        player[0].items.special = extraShips[z + 3];
        player[0].items.sidekick[LEFT_SIDEKICK] = extraShips[z + 4];
        player[0].items.sidekick[RIGHT_SIDEKICK] = extraShips[z + 5];
        player[0].items.generator = extraShips[z + 6];
        // Armor
        player[0].items.shield = extraShips[z + 8];
        shotMultiPos.fill(0);

        if (player[0].weapon_mode > sim::port_configs(state())) {
          player[0].weapon_mode = 1;
        }

        tempW = player[0].armor;
        sim::get_ship_info(state());
        if (player[0].armor > tempW && editShip1) {
          player[0].armor = tempW;
        }
        else {
          editShip1 = true;
        }

        Surface const temp_surface = draw_record().screen;
        draw_record().screen = Surface::vga_screen_seg;
        wipe_shield_armor_bars_tick();
        draw_armor_tick();
        draw_shield_tick();
        draw_record().screen = temp_surface;
        draw_options_tick();

        keysactive[x] = false;
      }
    }

    // for Player 2
    if (extraAvail && keysactive[SDL_SCANCODE_CAPSLOCK] && !isNetworkGame && !superTyrian) {
      for (x = SDL_SCANCODE_1; x <= SDL_SCANCODE_0; x++) {
        if (!keysactive[x]) {
          continue;
        }

        int z = x - SDL_SCANCODE_1 + 1;
        player[1].items.ship = 90 + z;
        z = (z - 1) * 15;
        player[1].items.weapon[FRONT_WEAPON].id = extraShips[z + 1];
        player[1].items.weapon[REAR_WEAPON].id = extraShips[z + 2];
        player[1].items.special = extraShips[z + 3];
        player[1].items.sidekick[LEFT_SIDEKICK] = extraShips[z + 4];
        player[1].items.sidekick[RIGHT_SIDEKICK] = extraShips[z + 5];
        player[1].items.generator = extraShips[z + 6];
        // Armor
        player[1].items.shield = extraShips[z + 8];
        shotMultiPos.fill(0);

        if (player[1].weapon_mode > sim::port_configs(state())) {
          player[1].weapon_mode = 1;
        }

        tempW = player[1].armor;
        sim::get_ship_info(state());
        if (player[1].armor > tempW && editShip2) {
          player[1].armor = tempW;
        }
        else {
          editShip2 = true;
        }

        Surface const temp_surface = draw_record().screen;
        draw_record().screen = Surface::vga_screen_seg;
        wipe_shield_armor_bars_tick();
        draw_armor_tick();
        draw_shield_tick();
        draw_record().screen = temp_surface;
        draw_options_tick();

        keysactive[x] = false;
      }
    }
  }

  // { In-Game Help }
  if (keysactive[SDL_SCANCODE_F1]) {
    if (isNetworkGame) {
      helpRequest = true;
    }
    else {
      c_loop(JE_inGameHelp);
      skipStarShowVGA = true;
    }
  }

  // {!Activate Nort Ship!}
  if (keysactive[SDL_SCANCODE_F2] && keysactive[SDL_SCANCODE_F4] && keysactive[SDL_SCANCODE_F6] &&
      keysactive[SDL_SCANCODE_F7] && keysactive[SDL_SCANCODE_F9] && keysactive[SDL_SCANCODE_BACKSLASH] &&
      keysactive[SDL_SCANCODE_SLASH])
  {
    if (isNetworkGame) {
      nortShipRequest = true;
    }
    else {
      player[0].items.ship = 12;                     // Nort Ship
      player[0].items.special = 13;                  // Astral Zone
      player[0].items.weapon[FRONT_WEAPON].id = 36;  // NortShip Super Pulse
      player[0].items.weapon[REAR_WEAPON].id = 37;   // NortShip Spreader
      shipGr = 1;
    }
  }

  // {Cheating}
  if (!isNetworkGame && !twoPlayerMode && !superTyrian && superArcadeMode == SA_NONE) {
    if (keysactive[SDL_SCANCODE_F2] && keysactive[SDL_SCANCODE_F3] && keysactive[SDL_SCANCODE_F6]) {
      youAreCheating = !youAreCheating;
      keysactive[SDL_SCANCODE_F2] = false;
    }

    if (keysactive[SDL_SCANCODE_F2] && keysactive[SDL_SCANCODE_F3] &&
        (keysactive[SDL_SCANCODE_F4] || keysactive[SDL_SCANCODE_F5]))
    {
      for (uint i = 0; i < COUNTOF(player); ++i) {
        player[i].armor = 0;
      }

      youAreCheating = !youAreCheating;
      draw_text_window_tick(miscText[63 - 1]);
    }

    if (constantPlay && keysactive[SDL_SCANCODE_C]) {
      youAreCheating = !youAreCheating;
      keysactive[SDL_SCANCODE_C] = false;
    }
  }

  if (superTyrian) {
    youAreCheating = false;
  }

  // {Personal Commands}

  // {DEBUG}
  if (keysactive[SDL_SCANCODE_F10] && keysactive[SDL_SCANCODE_BACKSPACE]) {
    keysactive[SDL_SCANCODE_F10] = false;
    debug = !debug;

    debugHist = 0;
    debugHistCount = 0;

    // YKS: clock ticks since midnight replaced by SDL_GetTicks
    lastDebugTime = SDL_GetTicks();
  }

  // {CHEAT-SKIP LEVEL}
  if (keysactive[SDL_SCANCODE_F2] && keysactive[SDL_SCANCODE_F6] &&
      (keysactive[SDL_SCANCODE_F7] || keysactive[SDL_SCANCODE_F8]) && !keysactive[SDL_SCANCODE_F9] &&
      !superTyrian && superArcadeMode == SA_NONE)
  {
    if (isNetworkGame) {
      skipLevelRequest = true;
    }
    else {
      levelTimer = true;
      levelTimerCountdown = 0;
      endLevel = true;
      levelEnd = 40;
    }
  }

  // pause game
  pause_pressed |= keysactive[SDL_SCANCODE_P];

  // in-game setup
  ingamemenu_pressed |= keysactive[SDL_SCANCODE_ESCAPE];

  if (!keysactive[SDL_SCANCODE_BACKSPACE]) {
    return;
  }

  // toggle screenshot pause
  if (keysactive[SDL_SCANCODE_NUMLOCKCLEAR]) {
    superPause = !superPause;
  }

  // {SMOOTHIES}
  if (keysactive[SDL_SCANCODE_F12] && keysactive[SDL_SCANCODE_SCROLLLOCK]) {
    for (temp = SDL_SCANCODE_2; temp <= SDL_SCANCODE_9; temp++) {
      if (keysactive[temp]) {
        smoothies[temp - SDL_SCANCODE_2] = !smoothies[temp - SDL_SCANCODE_2];
      }
    }
    if (keysactive[SDL_SCANCODE_0]) {
      smoothies[8] = !smoothies[8];
    }
  }
  else

    // {CYCLE THROUGH FILTER COLORS}
    if (keysactive[SDL_SCANCODE_MINUS]) {
      if (levelFilter == -99) {
        levelFilter = 0;
      }
      else {
        levelFilter++;
        if (levelFilter == 16) {
          levelFilter = -99;
        }
      }
    }
    else

      // {HYPER-SPEED}
      if (keysactive[SDL_SCANCODE_1]) {
        fastPlay++;
        if (fastPlay > 2) {
          fastPlay = 0;
        }
        keysactive[SDL_SCANCODE_1] = false;
        set_new_game_speed();
      }

  // {IN-GAME RANDOM MUSIC SELECTION}
  if (keysactive[SDL_SCANCODE_SCROLLLOCK]) {
    play_song(mt_rand() % MUSIC_NUM);
  }
}

}  // namespace tyrx::port
