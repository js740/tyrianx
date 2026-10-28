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
// The game's key commands during a level: ports of JE_mainKeyboardInput
// (mainint.c), with edited ships, help, cheats, debug and screen settings, and
// of JE_setNewGameSpeed (config.c).  They read the keys from the C globals, as
// the C code does, and set some of the C code's settings (the debug display,
// the frame speed and the gamma).

#include "sim.hpp"

#include <cassert>

namespace tyrx::sim {

namespace {

// JE_setNewGameSpeed: the frame speed for the game speed (fastPlay).
void set_new_game_speed(State& state) {
  Game& game = state.game;

  game.pentium_mode = false;

  Uint16 speed;
  switch (game.fast_play) {
    default:
      assert(false);
      [[fallthrough]];
    case 0:  // Normal
      speed = 0x4300;
      smoothScroll = true;
      game.frame_count_max = 2;
      break;
    case 1:  // Pentium Hyper
      speed = 0x3000;
      smoothScroll = true;
      game.frame_count_max = 2;
      break;
    case 2:
      speed = 0x2000;
      smoothScroll = false;
      game.frame_count_max = 2;
      break;
    case 3:  // Slug mode
      speed = 0x5300;
      smoothScroll = true;
      game.frame_count_max = 4;
      break;
    case 4:  // Slower
      speed = 0x4300;
      smoothScroll = true;
      game.frame_count_max = 3;
      break;
    case 5:  // Slow
      speed = 0x4300;
      smoothScroll = true;
      game.frame_count_max = 2;
      game.pentium_mode = true;
      break;
  }

  setFrameSpeed(speed);
  setFrameCount(game.frame_count_max);
}

// { Edited Ships }: a number key with tab (player 1) or caps lock (player 2)
// gives the player that edited ship.  edited is whether the player has had
// one, which keeps the armor from rising.
void edit_ship(State& state, DrawRecord& record, int i, JE_boolean& edited) {
  Player& player = state.players.slots[i];

  for (int key = SDL_SCANCODE_1; key <= SDL_SCANCODE_0; key++) {
    if (!keysactive[key]) {
      continue;
    }

    int z = key - SDL_SCANCODE_1 + 1;
    player.items.ship = 90 + z;  // Ships
    z = (z - 1) * 15;
    player.items.weapon[FRONT_WEAPON].id = extraShips[z + 1];
    player.items.weapon[REAR_WEAPON].id = extraShips[z + 2];
    player.items.special = extraShips[z + 3];
    player.items.sidekick[LEFT_SIDEKICK] = extraShips[z + 4];
    player.items.sidekick[RIGHT_SIDEKICK] = extraShips[z + 5];
    player.items.generator = extraShips[z + 6];
    // Armor
    player.items.shield = extraShips[z + 8];
    state.players.shot_multi_pos.fill(0);

    if (player.weapon_mode > port_configs(state)) {
      player.weapon_mode = 1;
    }

    JE_word const armor = player.armor;
    get_ship_info(state);
    if (player.armor > armor && edited) {
      player.armor = armor;
    }
    else {
      edited = true;
    }

    // The C code draws these with VGAScreen at the HUD's surface, and puts it
    // back after.
    hud_wipe_shield_armor(state, record.hud);
    hud_armor(state, record.hud);
    hud_shield(state, record.hud);
    hud_options(state, record.hud);

    keysactive[key] = false;
  }
}

}  // namespace

void main_keyboard_input(State& state, DrawRecord& record, Host& host) {
  Game& game = state.game;
  Cheats& cheats = state.cheats;
  auto& players = state.players.slots;

  JE_gammaCheck();

  // { Network Request Commands }

  if (!isNetworkGame && extraAvail && !game.super_tyrian) {
    if (keysactive[SDL_SCANCODE_TAB]) {
      edit_ship(state, record, 0, editShip1);
    }
    if (keysactive[SDL_SCANCODE_CAPSLOCK]) {
      edit_ship(state, record, 1, editShip2);
    }
  }

  // { In-Game Help }
  if (keysactive[SDL_SCANCODE_F1]) {
    if (isNetworkGame) {
      helpRequest = true;
    }
    else {
      host.c_loop(JE_inGameHelp);
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
      players[0].items.ship = 12;                     // Nort Ship
      players[0].items.special = 13;                  // Astral Zone
      players[0].items.weapon[FRONT_WEAPON].id = 36;  // NortShip Super Pulse
      players[0].items.weapon[REAR_WEAPON].id = 37;   // NortShip Spreader
      state.players.ship_gr = 1;
    }
  }

  // {Cheating}
  if (!isNetworkGame && !game.two_player_mode && !game.super_tyrian && game.super_arcade_mode == SA_NONE) {
    if (keysactive[SDL_SCANCODE_F2] && keysactive[SDL_SCANCODE_F3] && keysactive[SDL_SCANCODE_F6]) {
      cheats.you_are_cheating = !cheats.you_are_cheating;
      keysactive[SDL_SCANCODE_F2] = false;
    }

    if (keysactive[SDL_SCANCODE_F2] && keysactive[SDL_SCANCODE_F3] &&
        (keysactive[SDL_SCANCODE_F4] || keysactive[SDL_SCANCODE_F5]))
    {
      for (Player& p : players) {
        p.armor = 0;
      }

      cheats.you_are_cheating = !cheats.you_are_cheating;
      hud_text_window(state, record.hud, miscText[63 - 1]);
    }

    if (cheats.constant_play && keysactive[SDL_SCANCODE_C]) {
      cheats.you_are_cheating = !cheats.you_are_cheating;
      keysactive[SDL_SCANCODE_C] = false;
    }
  }

  if (game.super_tyrian) {
    cheats.you_are_cheating = false;
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
      !game.super_tyrian && game.super_arcade_mode == SA_NONE)
  {
    if (isNetworkGame) {
      skipLevelRequest = true;
    }
    else {
      state.level.timer = true;
      state.level.timer_countdown = 0;
      state.ending.end_level = true;
      state.ending.countdown = 40;
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
    cheats.super_pause = !cheats.super_pause;
  }

  // {SMOOTHIES}
  if (keysactive[SDL_SCANCODE_F12] && keysactive[SDL_SCANCODE_SCROLLLOCK]) {
    auto& smoothies = state.display.smoothies;
    for (int key = SDL_SCANCODE_2; key <= SDL_SCANCODE_9; key++) {
      if (keysactive[key]) {
        smoothies[key - SDL_SCANCODE_2] = !smoothies[key - SDL_SCANCODE_2];
      }
    }
    if (keysactive[SDL_SCANCODE_0]) {
      smoothies[8] = !smoothies[8];
    }
  }
  // {CYCLE THROUGH FILTER COLORS}
  else if (keysactive[SDL_SCANCODE_MINUS]) {
    JE_shortint& filter = state.display.filter;
    if (filter == -99) {
      filter = 0;
    }
    else {
      filter++;
      if (filter == 16) {
        filter = -99;
      }
    }
  }
  // {HYPER-SPEED}
  else if (keysactive[SDL_SCANCODE_1]) {
    game.fast_play++;
    if (game.fast_play > 2) {
      game.fast_play = 0;
    }
    keysactive[SDL_SCANCODE_1] = false;
    set_new_game_speed(state);
  }

  // {IN-GAME RANDOM MUSIC SELECTION}
  if (keysactive[SDL_SCANCODE_SCROLLLOCK]) {
    play_song(mt_rand_r(&state.random.mt) % MUSIC_NUM);
  }
}

}  // namespace tyrx::sim
