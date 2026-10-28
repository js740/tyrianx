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
// The level's events: ports of JE_eventSystem, JE_createNewEventEnemy,
// JE_eventJump and JE_searchFor (tyrian2.c), and of the events' part of
// JE_main's level loop.
//
// Besides the State, the events change the C code's level data and
// resources, as the C code does: events 49 to 52 set the armor and graphic of
// enemy type 0 (enemyDat[0]) for the enemy they create, event 5 loads the
// enemies' sprite sheets, and event 35 starts a song.

#include "sim.hpp"

#include <algorithm>

#include "../../logging.h"

#include <cassert>
#include <cmath>
#include <cstdlib>

namespace tyrx::sim {

namespace {

// JE_createNewEventEnemy: creates the current event's enemy, of type
// eventdat + enemy_type_ofs, in the first free slot of the 25 from first, at
// the event's position.  Returns the slot + 1, or 0 if none was free (the C
// code returns it in the global b).
int create_new_event_enemy(State& state, uint8_t enemy_type_ofs, uint16_t first,
                           int16_t unique_shape_table_i) {
  Level&     level = state.level;
  Map const& map = state.map;

  JE_EventRecType const& ev = level.events[level.event_loc - 1];

  int b = 0;
  for (int i = first; i < first + 25; i++) {
    if (state.enemies.status[i] == 1) {
      b = i + 1;
      break;
    }
  }

  if (b == 0) {
    return 0;
  }

  JE_SingleEnemyType& e = state.enemies.slots[b - 1];
  state.enemies.status[b - 1] = make_enemy(state, e, ev.eventdat + enemy_type_ofs, unique_shape_table_i);

  if (ev.eventdat2 != -99) {
    switch (first) {
      case 0:
        e.ex = ev.eventdat2 - (map.map_x - 1) * 24;
        e.ey -= map.back_move2;
        break;
      case 25:
      case 75:
        e.ex = ev.eventdat2 - (map.map_x - 1) * 24 - 12;
        e.ey -= map.back_move;
        break;
      case 50:
        if (map.background3x1) {
          e.ex = ev.eventdat2 - (map.map_x - 1) * 24 - 12;
        }
        else {
          e.ex = ev.eventdat2 - map.map_x3 * 24 - 24 * 2 + 6;
        }
        e.ey -= map.back_move3;

        if (map.background3x1b) {
          e.ex -= 6;
        }
        break;
    }
    e.ey = -28;
    if (map.background3x1b && first == 50) {
      e.ey += 4;
    }
  }

  if (level.small_enemy_adjust && e.size == 0) {
    e.ex -= 10;
    e.ey -= 7;
  }

  e.ey += ev.eventdat5;
  e.eyc += ev.eventdat3;
  e.linknum = ev.eventdat4;
  e.fixedmovey = ev.eventdat6;
  return b;
}

// JE_searchFor: whether an enemy with armor has link number link, in which
// case it puts the last one's slot in out_index.  In Galaga mode, it doubles
// their value.
bool search_for(State& state, uint8_t link, uint8_t *out_index) {
  int found_id = -1;

  for (int i = 0; i < 100; i++) {
    JE_SingleEnemyType& e = state.enemies.slots[i];
    if (state.enemies.status[i] != 0 || e.linknum != link) {
      continue;
    }

    found_id = i;
    if (state.game.galaga_mode) {
      e.evalue += e.evalue;
    }
  }

  if (found_id == -1) {
    return false;
  }

  if (out_index) {
    *out_index = found_id;
  }
  return true;
}

// The first and end slots of the enemies that event 19 moves, and whether it
// moves all of them, or only those with its link number.
struct Group {
  int  first = 0, end = 0;
  bool all = false;
};

// The enemies that the event's eventdat4 (a link number, or 0 for all)
// selects; eventdat3 80 to 89 takes the link number from newPL.
void take_link_from_new_pl(Level& level, JE_EventRecType& ev) {
  if (ev.eventdat3 > 79 && ev.eventdat3 < 90) {
    ev.eventdat4 = level.new_pl[ev.eventdat3 - 80];
  }
}

// JE_eventSystem: runs the current event.
void event_system(State& state, HudLog& hud) {
  auto&            game = state.game;
  auto&            display = state.display;
  auto&            map = state.map;
  auto&            ending = state.ending;
  auto&            level = state.level;
  auto&            players = state.players;
  auto&            sound = state.sound;
  auto&            enemy = state.enemies.slots;
  auto&            enemy_avail = state.enemies.status;
  MtState&         mt = state.random.mt;
  JE_EventRecType& ev = level.events[level.event_loc - 1];

  switch (ev.eventtype) {
    case 1:
      state.starfield.speed = ev.eventdat;
      break;

    case 2:
      map.map1_y_delay = 1;
      map.map1_y_delay_max = 1;
      map.map2_y_delay = 1;
      map.map2_y_delay_max = 1;

      map.back_move = ev.eventdat;
      map.back_move2 = ev.eventdat2;

      if (map.back_move2 > 0) {
        state.explosions.move = map.back_move2;
      }
      else {
        state.explosions.move = map.back_move;
      }

      map.back_move3 = ev.eventdat3;

      if (map.back_move > 0) {
        map.stop_background_num = 0;
      }
      break;

    case 3:
      map.back_move = 1;
      map.map1_y_delay = 3;
      map.map1_y_delay_max = 3;
      map.back_move2 = 1;
      map.map2_y_delay = 2;
      map.map2_y_delay_max = 2;
      map.back_move3 = 1;
      break;

    case 4:
      map.stop_backgrounds = true;
      switch (ev.eventdat) {
        case 0:
        case 1:
          map.stop_background_num = 1;
          break;
        case 2:
          map.stop_background_num = 2;
          break;
        case 3:
          map.stop_background_num = 3;
          break;
      }
      break;

    case 5: {  // load enemy shape banks
      uint8_t const new_tables[] = {
          (uint8_t)(ev.eventdat > 0 ? ev.eventdat : 0),
          (uint8_t)(ev.eventdat2 > 0 ? ev.eventdat2 : 0),
          (uint8_t)(ev.eventdat3 > 0 ? ev.eventdat3 : 0),
          (uint8_t)(ev.eventdat4 > 0 ? ev.eventdat4 : 0),
      };

      for (unsigned int i = 0; i < std::size(new_tables); ++i) {
        if (enemySpriteSheetIds[i] == new_tables[i]) {
          continue;
        }

        if (new_tables[i] > 0) {
          assert(new_tables[i] <= std::size(shapeFile));
          JE_loadCompShapes(&enemySpriteSheets[i], shapeFile[new_tables[i] - 1]);
        }
        else {
          free_sprite2s(&enemySpriteSheets[i]);
        }

        enemySpriteSheetIds[i] = new_tables[i];
      }
      break;
    }

    case 6:  // Ground Enemy
      create_new_event_enemy(state, 0, 25, 0);
      break;

    case 7:  // Top Enemy
      create_new_event_enemy(state, 0, 50, 0);
      break;

    case 8:
      display.star_active = false;
      break;

    case 9:
      display.star_active = true;
      break;

    case 10:  // Ground Enemy 2
      create_new_event_enemy(state, 0, 75, 0);
      break;

    case 11:
      if (ending.all_players_gone || ev.eventdat == 1) {
        ending.really_end_level = true;
      }
      else if (!ending.end_level) {
        ending.ready_to_end_level = false;
        ending.end_level = true;
        ending.countdown = 40;
      }
      break;

    case 12: {  // Custom 4x4 Ground Enemy
      unsigned first = 0;
      switch (ev.eventdat6) {
        case 0:
        case 1:
          first = 25;
          break;
        case 2:
          first = 0;
          break;
        case 3:
          first = 50;
          break;
        case 4:
          first = 75;
          break;
      }
      ev.eventdat6 = 0;  // We use EVENTDAT6 for the background
      create_new_event_enemy(state, 0, first, 0);
      if (int const b = create_new_event_enemy(state, 1, first, 0); b > 0) {
        enemy[b - 1].ex += 24;
      }
      if (int const b = create_new_event_enemy(state, 2, first, 0); b > 0) {
        enemy[b - 1].ey -= 28;
      }
      if (int const b = create_new_event_enemy(state, 3, first, 0); b > 0) {
        enemy[b - 1].ex += 24;
        enemy[b - 1].ey -= 28;
      }
      break;
    }
    case 13:
      level.enemies_active = false;
      break;

    case 14:
      level.enemies_active = true;
      break;

    case 15:  // Sky Enemy
      create_new_event_enemy(state, 0, 0, 0);
      break;

    case 16: {  // Window Text
      int16_t id = ev.eventdat;
      if (id < 1 || id > 9) {
        logWarn("Event %05d %d has invalid window text ID %d.", ev.eventtime, ev.eventtype, id);
        break;
      }
      hud_text_window(state, hud, outputs[id - 1]);
      sound.queue[3] = windowTextSamples[id - 1];
      break;
    }

    case 17:  // Ground Bottom
      if (int const b = create_new_event_enemy(state, 0, 25, 0); b > 0) {
        enemy[b - 1].ey = 190 + ev.eventdat5;
      }
      break;

    case 18:  // Sky Enemy on Bottom
      if (int const b = create_new_event_enemy(state, 0, 0, 0); b > 0) {
        enemy[b - 1].ey = 190 + ev.eventdat5;
      }
      break;

    case 19: {  // Enemy Global Move
      Group group;

      if (ev.eventdat3 > 79 && ev.eventdat3 < 90) {
        group = {0, 100, false};
        ev.eventdat4 = level.new_pl[ev.eventdat3 - 80];
      }
      else {
        switch (ev.eventdat3) {
          case 0:
            group = {0, 100, false};
            break;
          case 2:
            group = {0, 25, true};
            break;
          case 1:
            group = {25, 50, true};
            break;
          case 3:
            group = {50, 75, true};
            break;
          case 99:
            group = {0, 100, true};
            break;
        }
      }

      for (int i = group.first; i < group.end; i++) {
        if (!group.all && enemy[i].linknum != ev.eventdat4) {
          continue;
        }

        if (ev.eventdat != -99) {
          enemy[i].exc = ev.eventdat;
        }

        if (ev.eventdat2 != -99) {
          enemy[i].eyc = ev.eventdat2;
        }

        if (ev.eventdat6 != 0) {
          enemy[i].fixedmovey = ev.eventdat6;
        }

        if (ev.eventdat6 == -99) {
          enemy[i].fixedmovey = 0;
        }

        if (ev.eventdat5 > 0) {
          enemy[i].enemycycle = ev.eventdat5;
        }
      }
      break;
    }

    case 20:  // Enemy Global Accel
      take_link_from_new_pl(level, ev);

      for (int i = 0; i < 100; i++) {
        if (!(enemy_avail[i] != 1 && (enemy[i].linknum == ev.eventdat4 || ev.eventdat4 == 0))) {
          continue;
        }

        if (ev.eventdat != -99) {
          enemy[i].excc = ev.eventdat;
          enemy[i].exccw = abs(ev.eventdat);
          enemy[i].exccwmax = abs(ev.eventdat);
          if (ev.eventdat > 0) {
            enemy[i].exccadd = 1;
          }
          else {
            enemy[i].exccadd = -1;
          }
        }

        if (ev.eventdat2 != -99) {
          enemy[i].eycc = ev.eventdat2;
          enemy[i].eyccw = abs(ev.eventdat2);
          enemy[i].eyccwmax = abs(ev.eventdat2);
          if (ev.eventdat2 > 0) {
            enemy[i].eyccadd = 1;
          }
          else {
            enemy[i].eyccadd = -1;
          }
        }

        if (ev.eventdat5 > 0) {
          enemy[i].enemycycle = ev.eventdat5;
        }
        if (ev.eventdat6 > 0) {
          enemy[i].ani = ev.eventdat6;
          enemy[i].animin = ev.eventdat5;
          enemy[i].animax = 0;
          enemy[i].aniactive = 1;
        }
      }
      break;

    case 21:
      map.background3_over = 1;
      break;

    case 22:
      map.background3_over = 0;
      break;

    case 23:  // Sky Enemy on Bottom
      if (int const b = create_new_event_enemy(state, 0, 50, 0); b > 0) {
        enemy[b - 1].ey = 180 + ev.eventdat5;
      }
      break;

    case 24:  // Enemy Global Animate
      for (int i = 0; i < 100; i++) {
        if (enemy[i].linknum != ev.eventdat4) {
          continue;
        }

        enemy[i].aniactive = 1;
        enemy[i].aniwhenfire = 0;
        if (ev.eventdat2 > 0) {
          enemy[i].enemycycle = ev.eventdat2;
          enemy[i].animin = enemy[i].enemycycle;
        }
        else {
          enemy[i].enemycycle = 0;
        }

        if (ev.eventdat > 0) {
          enemy[i].ani = ev.eventdat;
        }

        if (ev.eventdat3 == 1) {
          enemy[i].animax = enemy[i].ani;
        }
        else if (ev.eventdat3 == 2) {
          enemy[i].aniactive = 2;
          enemy[i].animax = enemy[i].ani;
          enemy[i].aniwhenfire = 2;
        }
      }
      break;

    case 25:  // Enemy Global Damage change
      for (int i = 0; i < 100; i++) {
        if (ev.eventdat4 != 0 && enemy[i].linknum != ev.eventdat4) {
          continue;
        }

        if (game.galaga_mode) {
          enemy[i].armorleft = roundf(ev.eventdat * (game.difficulty_level / 2));
        }
        else {
          enemy[i].armorleft = ev.eventdat;
        }
      }
      break;

    case 26:
      level.small_enemy_adjust = ev.eventdat;
      break;

    case 27:  // Enemy Global AccelRev
      take_link_from_new_pl(level, ev);

      for (int i = 0; i < 100; i++) {
        if (ev.eventdat4 != 0 && enemy[i].linknum != ev.eventdat4) {
          continue;
        }

        if (ev.eventdat != -99) {
          enemy[i].exrev = ev.eventdat;
        }
        if (ev.eventdat2 != -99) {
          enemy[i].eyrev = ev.eventdat2;
        }
        if (ev.eventdat3 != 0 && ev.eventdat3 < 17) {
          enemy[i].filter = ev.eventdat3;
        }
      }
      break;

    case 28:
      map.top_enemy_over = false;
      break;

    case 29:
      map.top_enemy_over = true;
      break;

    case 30:
      map.map1_y_delay = 1;
      map.map1_y_delay_max = 1;
      map.map2_y_delay = 1;
      map.map2_y_delay_max = 1;

      map.back_move = ev.eventdat;
      map.back_move2 = ev.eventdat2;
      state.explosions.move = map.back_move2;
      map.back_move3 = ev.eventdat3;
      break;

    case 31:  // Enemy Fire Override
      for (int i = 0; i < 100; i++) {
        if (ev.eventdat4 != 99 && enemy[i].linknum != ev.eventdat4) {
          continue;
        }

        enemy[i].freq[1 - 1] = ev.eventdat;
        enemy[i].freq[2 - 1] = ev.eventdat2;
        enemy[i].freq[3 - 1] = ev.eventdat3;
        // the C code counts in temp2 (Scratch)
        uint8_t& temp2 = state.scratch.temp2;
        for (temp2 = 0; temp2 < 3; temp2++) {
          enemy[i].eshotwait[temp2] = 1;
        }
        if (enemy[i].launchtype > 0) {
          enemy[i].launchfreq = ev.eventdat5;
          enemy[i].launchwait = 1;
        }
      }
      break;

    case 32:  // create enemy
      if (int const b = create_new_event_enemy(state, 0, 50, 0); b > 0) {
        enemy[b - 1].ey = 190;
      }
      break;

    case 33:  // Enemy From other Enemies
      if ((ev.eventdat == 512 || ev.eventdat == 513) &&
          (game.two_player_mode || game.one_player_action || game.super_tyrian))
      {
        break;
      }

      if (game.super_arcade_mode != SA_NONE) {
        if (ev.eventdat == 534) {
          ev.eventdat = 827;
        }
      }
      else if (!game.super_tyrian) {
        uint8_t const lives = *players.slots[0].lives;

        if (ev.eventdat == 533 && (lives == 11 || (mt_rand_r(&mt) % 15) < lives)) {
          // enemy will drop random special weapon
          ev.eventdat = 829 + (mt_rand_r(&mt) % 6);
        }
      }
      if (ev.eventdat == 534 && game.super_tyrian) {
        ev.eventdat = 828 + superTyrianSpecials[mt_rand_r(&mt) % 4];
      }

      for (int i = 0; i < 100; i++) {
        if (enemy[i].linknum == ev.eventdat4) {
          enemy[i].enemydie = ev.eventdat;
        }
      }
      break;

    case 34:  // Start Music Fade
      if (ending.first_game_over) {
        sound.music_fade = true;
        sound.temp_volume = tyrMusicVolume;
      }
      break;

    case 35:  // Play new song
      if (ending.first_game_over) {
        play_song(ev.eventdat - 1);
        set_volume(tyrMusicVolume, fxVolume);
      }
      sound.music_fade = false;
      break;

    case 36:
      ending.ready_to_end_level = true;
      break;

    case 37:
      level.enemy_frequency = ev.eventdat;
      break;

    case 38: {
      level.cur_loc = ev.eventdat;
      int new_event_loc = 1;
      for (uint16_t i = 0; i < level.max_event; i++) {
        if (level.events[i].eventtime <= level.cur_loc) {
          new_event_loc = i + 1 - 1;
        }
      }
      level.event_loc = new_event_loc;
      break;
    }

    case 39:  // Enemy Global Linknum Change
      for (int i = 0; i < 100; i++) {
        if (enemy[i].linknum == ev.eventdat) {
          enemy[i].linknum = ev.eventdat2;
        }
      }
      break;

    case 40:  // Enemy Continual Damage
      level.enemy_continual_damage = true;
      break;

    case 41:
      if (ev.eventdat == 0) {
        std::ranges::fill(enemy_avail, 1);
      }
      else {
        for (int i = 0; i <= 24; i++) {
          enemy_avail[i] = 1;
        }
      }
      break;

    case 42:
      map.background3_over = 2;
      break;

    case 43:
      map.background2_over = ev.eventdat;
      break;

    case 44:
      display.filter_active = (ev.eventdat > 0);
      display.filter_fade = (ev.eventdat == 2);
      display.filter = ev.eventdat2;
      display.brightness = ev.eventdat3;
      display.filter_new = ev.eventdat4;
      display.brightness_chg = ev.eventdat5;
      display.filter_fade_start = (ev.eventdat6 == 0);
      break;

    case 45:  // arcade-only enemy from other enemies
      if (game.super_tyrian) {
        break;
      }

      if (uint8_t const lives = *players.slots[0].lives;
          ev.eventdat == 533 && (lives == 11 || (mt_rand_r(&mt) % 15) < lives))
      {
        ev.eventdat = 829 + (mt_rand_r(&mt) % 6);
      }
      if (game.two_player_mode || game.one_player_action) {
        for (int i = 0; i < 100; i++) {
          if (enemy[i].linknum == ev.eventdat4) {
            enemy[i].enemydie = ev.eventdat;
          }
        }
      }
      break;

    case 46:  // change difficulty
      if (ev.eventdat3 != 0) {
        players.damage_rate = ev.eventdat3;
      }

      if (ev.eventdat2 == 0 || game.two_player_mode || game.one_player_action) {
        game.difficulty_level += ev.eventdat;
        if (game.difficulty_level < DIFFICULTY_EASY) {
          game.difficulty_level = DIFFICULTY_EASY;
        }
        if (game.difficulty_level > DIFFICULTY_10) {
          game.difficulty_level = DIFFICULTY_10;
        }
      }
      break;

    case 47:  // Enemy Global AccelRev
      for (int i = 0; i < 100; i++) {
        if (ev.eventdat4 == 0 || enemy[i].linknum == ev.eventdat4) {
          enemy[i].armorleft = ev.eventdat;
        }
      }
      break;

    case 48:  // Background 2 Cannot be Transparent
      map.background2_not_transparent = true;
      break;

    case 49:
    case 50:
    case 51:
    case 52: {
      // An enemy of type 0, whose graphic and armor the event gives
      // (eventdat and eventdat6), from the shape table eventdat3.  The C code
      // keeps them in tempDat2, tempDat3 and tempDat while it clears them.
      int16_t const graphic = ev.eventdat;
      ev.eventdat = 0;
      int16_t const shape_table = ev.eventdat3;
      ev.eventdat3 = 0;
      int16_t const armor = ev.eventdat6;
      ev.eventdat6 = 0;
      enemyDat[0].armor = armor;
      enemyDat[0].egraphic[1 - 1] = graphic;
      uint8_t first = 0;
      switch (ev.eventtype - 48) {
        case 1:
          first = 25;
          break;
        case 2:
          first = 0;
          break;
        case 3:
          first = 50;
          break;
        case 4:
          first = 75;
          break;
      }
      create_new_event_enemy(state, 0, first, shape_table);
      ev.eventdat = graphic;
      ev.eventdat3 = shape_table;
      ev.eventdat6 = armor;
      break;
    }

    case 53:
      level.force_events = (ev.eventdat != 99);
      break;

    case 54:
      event_jump(level, ev.eventdat);
      break;

    case 55:  // Enemy Global AccelRev
      take_link_from_new_pl(level, ev);

      for (int i = 0; i < 100; i++) {
        if (ev.eventdat4 != 0 && enemy[i].linknum != ev.eventdat4) {
          continue;
        }

        if (ev.eventdat != -99) {
          enemy[i].xaccel = ev.eventdat;
        }
        if (ev.eventdat2 != -99) {
          enemy[i].yaccel = ev.eventdat2;
        }
      }
      break;

    case 56:  // Ground2 Bottom
      if (int const b = create_new_event_enemy(state, 0, 75, 0); b > 0) {
        enemy[b - 1].ey = 190;
      }
      break;

    case 57:
      level.super_enemy_254_jump = ev.eventdat;
      break;

    case 60:  // Assign Special Enemy
      for (int i = 0; i < 100; i++) {
        if (enemy[i].linknum == ev.eventdat4) {
          enemy[i].special = true;
          enemy[i].flagnum = ev.eventdat;
          enemy[i].setto = (ev.eventdat2 == 1);
        }
      }
      break;

    case 61:  // if specific flag set to specific value, skip events
      if (state.level.global_flags[ev.eventdat - 1] == ev.eventdat2) {
        level.event_loc += ev.eventdat3;
      }
      break;

    case 62:  // Play sound effect
      sound.queue[3] = ev.eventdat;
      break;

    case 63:  // skip events if not in 2-player mode
      if (!game.two_player_mode && !game.one_player_action) {
        level.event_loc += ev.eventdat;
      }
      break;

    case 64:
      if (!(ev.eventdat == 6 && game.two_player_mode && game.difficulty_level > DIFFICULTY_NORMAL)) {
        display.smoothies[ev.eventdat - 1] = ev.eventdat2;
        uint8_t smoothie = ev.eventdat;
        if (smoothie == 5) {
          smoothie = 3;
        }
        display.smoothie_data[smoothie - 1] = ev.eventdat3;
      }
      break;

    case 65:
      map.background3x1 = (ev.eventdat == 0);
      break;

    case 66:  // If not on this difficulty level or higher then...
      if (game.initial_difficulty <= ev.eventdat) {
        level.event_loc += ev.eventdat2;
      }
      break;

    case 67:
      level.timer = (ev.eventdat == 1);
      level.timer_countdown = ev.eventdat3 * 100;
      level.timer_jump_to = ev.eventdat2;
      break;

    case 68:
      level.random_explosions = (ev.eventdat == 1);
      break;

    case 69:
      for (Player& p : players.slots) {
        p.invulnerable_ticks = ev.eventdat;
      }
      break;

    case 70:
      // 1-10
      if (ev.eventdat2 == 0) {
        bool found = false;

        for (uint8_t link = 1; link <= 19; link++) {
          found = found || search_for(state, link, NULL);
        }

        if (!found) {
          event_jump(level, ev.eventdat);
        }
      }
      else if (!search_for(state, ev.eventdat2, NULL) &&
               (ev.eventdat3 == 0 || !search_for(state, ev.eventdat3, NULL)) &&
               (ev.eventdat4 == 0 || !search_for(state, ev.eventdat4, NULL)))
      {
        event_jump(level, ev.eventdat);
      }
      break;

    case 71:
      if (((((intptr_t)map.map_y_pos - (intptr_t)&megaData1.mainmap) / sizeof(uint8_t *)) * 2) <=
          (unsigned)ev.eventdat2)
      {
        event_jump(level, ev.eventdat);
      }
      break;

    case 72:
      map.background3x1b = (ev.eventdat == 1);
      break;

    case 73:
      map.sky_enemy_over_all = (ev.eventdat == 1);
      break;

    case 74:  // Enemy Global BounceParams
      for (int i = 0; i < 100; i++) {
        if (ev.eventdat4 != 0 && enemy[i].linknum != ev.eventdat4) {
          continue;
        }

        if (ev.eventdat5 != -99) {
          enemy[i].xminbounce = ev.eventdat5;
        }

        if (ev.eventdat6 != -99) {
          enemy[i].yminbounce = ev.eventdat6;
        }

        if (ev.eventdat != -99) {
          enemy[i].xmaxbounce = ev.eventdat;
        }

        if (ev.eventdat2 != -99) {
          enemy[i].ymaxbounce = ev.eventdat2;
        }
      }
      break;

    case 75: {
      // Picks one of the link numbers eventdat to eventdat2 of enemies with
      // armor that don't move down, at random, for newPL; or skips events if
      // there is none.
      bool any = false;

      for (int i = 0; i < 100; i++) {
        if (enemy_avail[i] == 0 && enemy[i].eyc == 0 && enemy[i].linknum >= ev.eventdat &&
            enemy[i].linknum <= ev.eventdat2)
        {
          any = true;
        }
      }

      if (any) {
        uint8_t link;
        uint8_t enemy_i;
        do {
          link = (mt_rand_r(&mt) % (ev.eventdat2 + 1 - ev.eventdat)) + ev.eventdat;
        } while (!(search_for(state, link, &enemy_i) && enemy[enemy_i].eyc == 0));

        level.new_pl[ev.eventdat3 - 80] = link;
      }
      else {
        level.new_pl[ev.eventdat3 - 80] = 255;
        // Skip
        if (ev.eventdat4 > 0) {
          level.cur_loc = level.events[level.event_loc - 1 + ev.eventdat4].eventtime - 1;
          level.event_loc += ev.eventdat4 - 1;
        }
      }

      break;
    }

    case 76:
      level.return_active = true;
      break;

    case 77:
      map.map_y_pos = &megaData1.mainmap[0][0];
      map.map_y_pos += ev.eventdat / 2;
      if (ev.eventdat2 > 0) {
        map.map_y2_pos = &megaData2.mainmap[0][0];
        map.map_y2_pos += ev.eventdat2 / 2;
      }
      else {
        map.map_y2_pos = &megaData2.mainmap[0][0];
        map.map_y2_pos += ev.eventdat / 2;
      }
      break;

    case 78:
      if (level.galaga_shot_freq < 10) {
        level.galaga_shot_freq++;
      }
      break;

    case 79:
      display.boss_bars[0].link_num = ev.eventdat;
      display.boss_bars[1].link_num = ev.eventdat2;
      break;

    case 80:  // skip events if in 2-player mode
      if (game.two_player_mode) {
        level.event_loc += ev.eventdat;
      }
      break;

    case 81:  // WRAP2
      map.bk_wrap2 = &megaData2.mainmap[0][0];
      map.bk_wrap2 += ev.eventdat / 2;
      map.bk_wrap2_to = &megaData2.mainmap[0][0];
      map.bk_wrap2_to += ev.eventdat2 / 2;
      break;

    case 82:  // Give SPECIAL WEAPON
      players.slots[0].items.special = ev.eventdat;
      players.shot_multi_pos[SHOT_SPECIAL] = 0;
      players.shot_repeat[SHOT_SPECIAL] = 0;
      players.shot_multi_pos[SHOT_SPECIAL2] = 0;
      players.shot_repeat[SHOT_SPECIAL2] = 0;
      break;

    default:
      logWarn("Event %05d %d has invalid type.", ev.eventtime, ev.eventtype);
      break;
  }

  level.event_loc++;
}

}  // namespace

void update_events(State& state, HudLog& hud) {
  Level& level = state.level;
  while (level.events[level.event_loc - 1].eventtime <= level.cur_loc && level.event_loc <= level.max_event) {
    event_system(state, hud);
  }
}

void event_jump(Level& level, uint16_t jump) {
  if (jump == 65535) {
    level.cur_loc = level.return_loc;
  }
  else {
    level.return_loc = level.cur_loc + 1;
    level.cur_loc = jump;
  }

  uint16_t i = 0;
  do {
    i++;
  } while (!(level.events[i - 1].eventtime >= level.cur_loc));
  level.event_loc = i - 1;
}

}  // namespace tyrx::sim
