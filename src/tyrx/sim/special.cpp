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

// The special moves: ports of JE_SFCodes (mainint.c), which follows the
// players' input through the ships' special-move codes, and of
// JE_doSpecialShot and JE_specialComplete (varz.c), which set the specials
// off and run those that last a while.

#include "sim.hpp"

#include <cstdlib>

namespace tyrx::sim {

namespace {

// JE_specialComplete: sets off special special_type for player player_num.
// Some specials take their power from the C code's temp2 (Scratch).
void special_complete(State& state, HudLog& hud, JE_byte player_num, JE_byte special_type) {
  auto& players = state.players.slots;
  auto& shot_repeat = state.players.shot_repeat;
  auto& shot_multi_pos = state.players.shot_multi_pos;
  auto& sound_queue = state.sound.queue;
  Specials& specials = state.specials;
  JE_byte const temp2 = state.scratch.temp2;
  auto const& special_data = special[special_type];

  specials.next_wait = 0;
  switch (special_data.stype) {
    // Weapon
    case 1:
      if (player_num == 1) {
        create_player_shot(state, 0, SHOT_SPECIAL2, players[0].x, players[0].y, players[0].mouseX,
                           players[0].mouseY, special_data.wpn, player_num);
      }
      else {
        create_player_shot(state, 0, SHOT_SPECIAL2, players[1].x, players[1].y, players[0].mouseX,
                           players[0].mouseY, special_data.wpn, player_num);
      }

      shot_repeat[SHOT_SPECIAL] = shot_repeat[SHOT_SPECIAL2];
      break;
    // Repulsor
    case 2:
      for (int i = 0; i < ENEMY_SHOT_MAX; i++) {
        if (state.enemy_shots.available[i]) {
          continue;
        }

        EnemyShotType& shot = state.enemy_shots.slots[i];
        if (players[0].x > shot.sx) {
          shot.sxm--;
        }
        else if (players[0].x < shot.sx) {
          shot.sxm++;
        }

        if (players[0].y > shot.sy) {
          shot.sym--;
        }
        else if (players[0].y < shot.sy) {
          shot.sym++;
        }
      }
      break;
    // Zinglon Blast
    case 3:
      specials.zinglon_duration = 50;
      shot_repeat[SHOT_SPECIAL] = 100;
      sound_queue[7] = S_SOUL_OF_ZINGLON;
      break;
    // Attractor
    case 4:
      for (int i = 0; i < 100; i++) {
        JE_SingleEnemyType& e = state.enemies.slots[i];
        if (!(state.enemies.status[i] != 1 && e.scoreitem && e.evalue != 0)) {
          continue;
        }

        if (players[0].x > e.ex) {
          e.exc++;
        }
        else if (players[0].x < e.ex) {
          e.exc--;
        }

        if (players[0].y > e.ey) {
          e.eyc++;
        }
        else if (players[0].y < e.ey) {
          e.eyc--;
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
    case 16: {
      auto const power = players[0].items.weapon[FRONT_WEAPON].power;

      if (specials.flare_duration == 0) {
        specials.flare_start = true;
      }

      specials.weapon_wpn = special_data.wpn;
      specials.link_to_player = false;
      specials.spray = false;
      switch (special_data.stype) {
        case 5:
          specials.weapon_filter = 7;
          specials.weapon_freq = 2;
          specials.flare_duration = 50;
          break;
        case 6:
          specials.weapon_filter = 1;
          specials.weapon_freq = 7;
          specials.flare_duration = 200 + 25 * power;
          break;
        case 7:
          specials.weapon_filter = 3;
          specials.weapon_freq = 3;
          specials.flare_duration = 50 + 10 * power;
          specials.zinglon_duration = 50;
          shot_repeat[SHOT_SPECIAL] = 100;
          sound_queue[7] = S_SOUL_OF_ZINGLON;
          break;
        case 8:
          specials.weapon_filter = -99;
          specials.weapon_freq = 7;
          specials.flare_duration = 10 + power;
          break;
        case 9:
          specials.weapon_filter = -99;
          specials.weapon_freq = 8;
          specials.flare_duration = 8 + 2 * power;
          specials.link_to_player = true;
          specials.next_wait = special_data.pwr;
          break;
        case 10:
          specials.weapon_filter = -99;
          specials.weapon_freq = 8;
          specials.flare_duration = 14 + 4 * power;
          specials.link_to_player = true;
          break;
        case 11:
          specials.weapon_filter = -99;
          specials.weapon_freq = special_data.pwr;
          specials.flare_duration = 10 + 10 * power;
          specials.astral_duration = 20 + 10 * power;
          break;
        case 16:
          specials.weapon_filter = -99;
          specials.weapon_freq = 8;
          specials.flare_duration = temp2 * 16 + 8;
          specials.link_to_player = true;
          specials.spray = true;
          break;
      }
      break;
    }
    case 12:  // Invulnerability
      players[player_num - 1].invulnerable_ticks = temp2 * 10;

      if (state.game.super_arcade_mode > 0 && state.game.super_arcade_mode <= SA) {
        shot_repeat[SHOT_SPECIAL] = 250;
        create_player_shot(state, 0, SHOT_SPECIAL2, players[0].x, players[0].y, players[0].mouseX,
                           players[0].mouseY, 707, 1);
        players[0].invulnerable_ticks = 100;
      }
      break;
    case 13:  // Repair Player 1
      players[0].armor += temp2 / 4 + 1;

      sound_queue[3] = S_POWERUP;
      break;
    case 14:  // Repair Player 2
      players[1].armor += temp2 / 4 + 1;

      sound_queue[3] = S_POWERUP;
      break;

    case 17:  // Spawn left or right sidekick
      sound_queue[3] = S_POWERUP;

      if (players[0].items.sidekick[LEFT_SIDEKICK] == special_data.wpn) {
        players[0].items.sidekick[RIGHT_SIDEKICK] = special_data.wpn;
        shot_multi_pos[RIGHT_SIDEKICK] = 0;
      }
      else {
        players[0].items.sidekick[LEFT_SIDEKICK] = special_data.wpn;
        shot_multi_pos[LEFT_SIDEKICK] = 0;
      }

      hud_options(state, hud);
      break;

    case 18:  // Spawn right sidekick
      players[0].items.sidekick[RIGHT_SIDEKICK] = special_data.wpn;

      hud_options(state, hud);

      sound_queue[4] = S_POWERUP;

      shot_multi_pos[RIGHT_SIDEKICK] = 0;
      break;
  }
}

// A special move's cost: some of the player's shield or armor, all of the
// shield, or half of it.  The C code keeps it in temp2 (Scratch), which the
// special reads.  Returns whether the player can afford it.
bool pay_for_special(JE_byte& temp2, uint& armor, uint& shield) {
  if (temp2 == 0) {
    return true;
  }

  // costs some shield
  if (temp2 < 98) {
    if (shield < temp2) {
      return false;
    }
    shield -= temp2;
    return true;
  }

  // costs all shield
  if (temp2 == 98) {
    bool const can_afford = shield >= 4;
    temp2 = shield;
    shield = 0;
    return can_afford;
  }

  // costs half shield
  if (temp2 == 99) {
    temp2 = shield / 2;
    shield = temp2;
    return true;
  }

  // costs some armor
  temp2 -= 100;
  if (armor <= temp2) {
    return false;
  }
  armor -= temp2;
  return true;
}

// The flares and the specials that fire while they last.
void update_flare(State& state, JE_byte player_num) {
  auto& players = state.players.slots;
  auto& shot_repeat = state.players.shot_repeat;
  Specials& specials = state.specials;
  Display& display = state.display;
  MtState& mt = state.random.mt;

  if (specials.flare_duration > 1) {
    if (specials.weapon_filter != -99) {
      if (display.filter == -99 && display.brightness == -99) {
        display.filter_active = false;
      }
      if (!display.filter_active) {
        display.filter = specials.weapon_filter;
        if (display.filter == 7) {
          display.brightness = 0;
        }
        display.filter_active = true;
      }

      if (mt_rand_r(&mt) % 2 == 0) {
        specials.flare_col_chg = -1;
      }
      else {
        specials.flare_col_chg = 1;
      }

      if (display.filter == 7) {
        if (display.brightness < -6) {
          specials.flare_col_chg = 1;
        }
        if (display.brightness > 6) {
          specials.flare_col_chg = -1;
        }
        display.brightness += specials.flare_col_chg;
      }
    }

    if ((signed)(mt_rand_r(&mt) % 6) < specials.weapon_freq) {
      JE_integer b = MAX_PWEAPON;

      if (specials.link_to_player) {
        if (shot_repeat[SHOT_SPECIAL] == 0) {
          b = create_player_shot(state, 0, SHOT_SPECIAL, players[0].x, players[0].y, players[0].mouseX,
                                 players[0].mouseY, specials.weapon_wpn, player_num);
        }
      }
      else {
        b = create_player_shot(state, 0, SHOT_SPECIAL, mt_rand_r(&mt) % 280, mt_rand_r(&mt) % 180,
                               players[0].mouseX, players[0].mouseY, specials.weapon_wpn, player_num);
      }

      if (specials.spray && b != MAX_PWEAPON) {
        PlayerShotDataType& shot = state.player_shots.slots[b];
        shot.shotXM = (mt_rand_r(&mt) % 5) - 2;
        shot.shotYM = (mt_rand_r(&mt) % 5) - 2;
        if (shot.shotYM == 0) {
          shot.shotYM++;
        }
      }
    }

    specials.flare_duration--;
    if (specials.flare_duration == 1) {
      specials.wait = specials.next_wait;
    }
    return;
  }

  if (!specials.flare_start) {
    return;
  }

  specials.flare_start = false;
  shot_repeat[SHOT_SPECIAL] = specials.link_to_player ? 15 : 200;
  specials.flare_duration = 0;
  if (display.filter == specials.weapon_filter) {
    display.filter = -99;
    display.brightness = -99;
    display.filter_active = false;
  }
}

}  // namespace

void do_special_shot(State& state, HudLog& hud, JE_byte player_num, uint& armor, uint& shield,
                     VisiblePlayer& visible) {
  auto& players = state.players.slots;
  auto& shot_repeat = state.players.shot_repeat;
  auto& shot_multi_pos = state.players.shot_multi_pos;
  auto& button = state.input.button;
  Specials& specials = state.specials;

  if (players[0].items.special > 0) {
    bool const ready = shot_repeat[SHOT_SPECIAL] == 0 && specials.wait == 0 && specials.flare_duration < 2 &&
                       specials.zinglon_duration < 2;
    visible.special_icon = VisiblePlayer::SpecialIcon{hud.game_screen, ready};
  }

  if (shot_repeat[SHOT_SPECIAL] > 0) {
    --shot_repeat[SHOT_SPECIAL];
  }
  if (specials.wait > 0) {
    specials.wait--;
  }

  // A special move's code
  JE_byte const executed = specials.sf_executed[player_num - 1];
  if (executed > 0 && shot_repeat[SHOT_SPECIAL] == 0 && specials.flare_duration == 0) {
    JE_byte& temp2 = state.scratch.temp2;
    temp2 = special[executed].pwr;

    bool const can_afford = pay_for_special(temp2, armor, shield);

    shot_multi_pos[SHOT_SPECIAL] = 0;
    shot_multi_pos[SHOT_SPECIAL2] = 0;

    if (can_afford) {
      special_complete(state, hud, player_num, executed);
    }

    specials.sf_executed[player_num - 1] = 0;

    hud_wipe_shield_armor(state, hud);
    hud_shield(state, hud);
    hud_armor(state, hud);
    hud.game_screen = true;
  }

  // Main Begin: the fire button's special
  if (player_num == 1 && players[0].items.special > 0) {
    JE_byte const sa = state.game.super_arcade_mode;
    if (sa > 0 && (button[2 - 1] || button[3 - 1])) {
      state.input.fire_button_held = false;
    }
    if (!button[1 - 1] && !(sa != SA_NONE && (button[2 - 1] || button[3 - 1]))) {
      state.input.fire_button_held = false;
    }
    else if (shot_repeat[SHOT_SPECIAL] == 0 && !state.input.fire_button_held &&
             !(specials.flare_duration > 0) && specials.wait == 0)
    {
      state.input.fire_button_held = true;
      special_complete(state, hud, player_num, players[0].items.special);
    }
  }  // Main End

  if (specials.astral_duration > 0) {
    specials.astral_duration--;
  }

  state.player_shots.ttl[MAX_PWEAPON - 1] = 0;
  update_flare(state, player_num);

  if (specials.zinglon_duration <= 1) {
    return;
  }

  JE_byte const width = 25 - abs(specials.zinglon_duration - 25);

  visible.zinglon = VisiblePlayer::Zinglon{hud.game_screen, players[0].x, width};

  specials.zinglon_duration--;
  if (specials.zinglon_duration % 5 == 0) {
    state.player_shots.ttl[MAX_PWEAPON - 1] = 1;
  }
}

void sf_codes(State& state, JE_byte player_num, JE_integer px, JE_integer py, JE_integer mouse_x,
              JE_integer mouse_y) {
  auto const& button = state.input.button;
  Specials& specials = state.specials;
  JE_byte temp, temp2, temp3, temp4, temp5;

  uint ship = state.players.slots[player_num - 1].items.ship;

  // Get direction
  if (player_num == 2 && ship < 15) {
    ship = 0;
  }

  if (ship >= 15) {
    return;
  }

  temp2 = (mouse_y > py) +     // UP
          (mouse_y < py) +     // DOWN
          (px < mouse_x) +     // LEFT
          (px > mouse_x);      // RIGHT
  temp = (mouse_y > py) * 1 +  // UP
         (mouse_y < py) * 2 +  // DOWN
         (px < mouse_x) * 3 +  // LEFT
         (px > mouse_x) * 4;   // RIGHT

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

  auto& code = specials.sf_current_code[player_num - 1];

  temp3 = state.game.super_tyrian ? 21 : 3;
  for (temp2 = 0; temp2 < temp3; temp2++) {
    // Use SuperTyrian ShipCombos or not?
    temp5 = state.game.super_tyrian ? shipCombosB[temp2] : shipCombos[ship][temp2];

    // temp5 == selected combo in ship
    // combo doesn't exists
    if (temp5 == 0) {
      // mark twiddles as cancelled/finished
      code[temp2] = 0;
      continue;
    }

    // get next combo key
    temp4 = keyboardCombos[temp5 - 1][code[temp2]];

    // correct key
    if (temp4 == temp) {
      code[temp2]++;

      temp4 = keyboardCombos[temp5 - 1][code[temp2]];
      if (temp4 > 100 && temp4 <= 100 + SPECIAL_NUM) {
        code[temp2] = 0;
        specials.sf_executed[player_num - 1] = temp4 - 100;
      }
      continue;
    }

    if ((temp != 9) && (temp4 - 1) % 4 != (temp - 1) % 4 &&
        (code[temp2] == 0 || keyboardCombos[temp5 - 1][code[temp2] - 1] != temp))
    {
      code[temp2] = 0;
    }
  }
}

}  // namespace tyrx::sim
