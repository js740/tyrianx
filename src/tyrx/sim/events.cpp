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

  Event const& ev = level.events[level.event_loc - 1];

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

  Enemy& e = state.enemies.slots[b - 1];
  state.enemies.status[b - 1] = make_enemy(state, e, ev.dat + enemy_type_ofs, unique_shape_table_i);

  if (ev.dat2 != -99) {
    switch (first) {
      case 0:
        e.x = ev.dat2 - (map.map_x - 1) * 24;
        e.y -= map.back_move2;
        break;
      case 25:
      case 75:
        e.x = ev.dat2 - (map.map_x - 1) * 24 - 12;
        e.y -= map.back_move;
        break;
      case 50:
        if (map.background3x1) {
          e.x = ev.dat2 - (map.map_x - 1) * 24 - 12;
        }
        else {
          e.x = ev.dat2 - map.map_x3 * 24 - 24 * 2 + 6;
        }
        e.y -= map.back_move3;

        if (map.background3x1b) {
          e.x -= 6;
        }
        break;
    }
    e.y = -28;
    if (map.background3x1b && first == 50) {
      e.y += 4;
    }
  }

  if (level.small_enemy_adjust && e.size == 0) {
    e.x -= 10;
    e.y -= 7;
  }

  e.y += ev.dat5;
  e.y_speed += ev.dat3;
  e.link_num = ev.dat4;
  e.fixed_move_y = ev.dat6;
  return b;
}

// JE_searchFor: whether an enemy with armor has link number link, in which
// case it puts the last one's slot in out_index.  In Galaga mode, it doubles
// their value.
bool search_for(State& state, uint8_t link, uint8_t *out_index) {
  int found_id = -1;

  for (int i = 0; i < 100; i++) {
    Enemy& e = state.enemies.slots[i];
    if (state.enemies.status[i] != 0 || e.link_num != link) {
      continue;
    }

    found_id = i;
    if (state.game.galaga_mode) {
      e.value += e.value;
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
void take_link_from_new_pl(Level& level, Event& ev) {
  if (ev.dat3 > 79 && ev.dat3 < 90) {
    ev.dat4 = level.new_pl[ev.dat3 - 80];
  }
}

// JE_eventSystem: runs the current event.
void event_system(State& state, HudLog& hud) {
  auto&    game = state.game;
  auto&    display = state.display;
  auto&    map = state.map;
  auto&    ending = state.ending;
  auto&    level = state.level;
  auto&    players = state.players;
  auto&    sound = state.sound;
  auto&    enemy = state.enemies.slots;
  auto&    enemy_avail = state.enemies.status;
  MtState& mt = state.random.mt;
  Event&   ev = level.events[level.event_loc - 1];

  switch (ev.type) {
    case 1:
      state.starfield.speed = ev.dat;
      break;

    case 2:
      map.map1_y_delay = 1;
      map.map1_y_delay_max = 1;
      map.map2_y_delay = 1;
      map.map2_y_delay_max = 1;

      map.back_move = ev.dat;
      map.back_move2 = ev.dat2;

      if (map.back_move2 > 0) {
        state.explosions.move = map.back_move2;
      }
      else {
        state.explosions.move = map.back_move;
      }

      map.back_move3 = ev.dat3;

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
      switch (ev.dat) {
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
          (uint8_t)(ev.dat > 0 ? ev.dat : 0),
          (uint8_t)(ev.dat2 > 0 ? ev.dat2 : 0),
          (uint8_t)(ev.dat3 > 0 ? ev.dat3 : 0),
          (uint8_t)(ev.dat4 > 0 ? ev.dat4 : 0),
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
      if (ending.all_players_gone || ev.dat == 1) {
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
      switch (ev.dat6) {
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
      ev.dat6 = 0;  // We use EVENTDAT6 for the background
      create_new_event_enemy(state, 0, first, 0);
      if (int const b = create_new_event_enemy(state, 1, first, 0); b > 0) {
        enemy[b - 1].x += 24;
      }
      if (int const b = create_new_event_enemy(state, 2, first, 0); b > 0) {
        enemy[b - 1].y -= 28;
      }
      if (int const b = create_new_event_enemy(state, 3, first, 0); b > 0) {
        enemy[b - 1].x += 24;
        enemy[b - 1].y -= 28;
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
      int16_t id = ev.dat;
      if (id < 1 || id > 9) {
        logWarn("Event %05d %d has invalid window text ID %d.", ev.time, ev.type, id);
        break;
      }
      hud_text_window(state, hud, outputs[id - 1]);
      sound.queue[3] = windowTextSamples[id - 1];
      break;
    }

    case 17:  // Ground Bottom
      if (int const b = create_new_event_enemy(state, 0, 25, 0); b > 0) {
        enemy[b - 1].y = 190 + ev.dat5;
      }
      break;

    case 18:  // Sky Enemy on Bottom
      if (int const b = create_new_event_enemy(state, 0, 0, 0); b > 0) {
        enemy[b - 1].y = 190 + ev.dat5;
      }
      break;

    case 19: {  // Enemy Global Move
      Group group;

      if (ev.dat3 > 79 && ev.dat3 < 90) {
        group = {0, 100, false};
        ev.dat4 = level.new_pl[ev.dat3 - 80];
      }
      else {
        switch (ev.dat3) {
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
        if (!group.all && enemy[i].link_num != ev.dat4) {
          continue;
        }

        if (ev.dat != -99) {
          enemy[i].x_speed = ev.dat;
        }

        if (ev.dat2 != -99) {
          enemy[i].y_speed = ev.dat2;
        }

        if (ev.dat6 != 0) {
          enemy[i].fixed_move_y = ev.dat6;
        }

        if (ev.dat6 == -99) {
          enemy[i].fixed_move_y = 0;
        }

        if (ev.dat5 > 0) {
          enemy[i].frame = ev.dat5;
        }
      }
      break;
    }

    case 20:  // Enemy Global Accel
      take_link_from_new_pl(level, ev);

      for (int i = 0; i < 100; i++) {
        if (!(enemy_avail[i] != 1 && (enemy[i].link_num == ev.dat4 || ev.dat4 == 0))) {
          continue;
        }

        if (ev.dat != -99) {
          enemy[i].x_accel = ev.dat;
          enemy[i].x_accel_wait = abs(ev.dat);
          enemy[i].x_accel_wait_max = abs(ev.dat);
          if (ev.dat > 0) {
            enemy[i].x_accel_step = 1;
          }
          else {
            enemy[i].x_accel_step = -1;
          }
        }

        if (ev.dat2 != -99) {
          enemy[i].y_accel = ev.dat2;
          enemy[i].y_accel_wait = abs(ev.dat2);
          enemy[i].y_accel_wait_max = abs(ev.dat2);
          if (ev.dat2 > 0) {
            enemy[i].y_accel_step = 1;
          }
          else {
            enemy[i].y_accel_step = -1;
          }
        }

        if (ev.dat5 > 0) {
          enemy[i].frame = ev.dat5;
        }
        if (ev.dat6 > 0) {
          enemy[i].last_frame = ev.dat6;
          enemy[i].first_frame = ev.dat5;
          enemy[i].stop_frame = 0;
          enemy[i].animation = 1;
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
        enemy[b - 1].y = 180 + ev.dat5;
      }
      break;

    case 24:  // Enemy Global Animate
      for (int i = 0; i < 100; i++) {
        if (enemy[i].link_num != ev.dat4) {
          continue;
        }

        enemy[i].animation = 1;
        enemy[i].animation_at_stop = 0;
        if (ev.dat2 > 0) {
          enemy[i].frame = ev.dat2;
          enemy[i].first_frame = enemy[i].frame;
        }
        else {
          enemy[i].frame = 0;
        }

        if (ev.dat > 0) {
          enemy[i].last_frame = ev.dat;
        }

        if (ev.dat3 == 1) {
          enemy[i].stop_frame = enemy[i].last_frame;
        }
        else if (ev.dat3 == 2) {
          enemy[i].animation = 2;
          enemy[i].stop_frame = enemy[i].last_frame;
          enemy[i].animation_at_stop = 2;
        }
      }
      break;

    case 25:  // Enemy Global Damage change
      for (int i = 0; i < 100; i++) {
        if (ev.dat4 != 0 && enemy[i].link_num != ev.dat4) {
          continue;
        }

        if (game.galaga_mode) {
          enemy[i].armor = roundf(ev.dat * (game.difficulty_level / 2));
        }
        else {
          enemy[i].armor = ev.dat;
        }
      }
      break;

    case 26:
      level.small_enemy_adjust = ev.dat;
      break;

    case 27:  // Enemy Global AccelRev
      take_link_from_new_pl(level, ev);

      for (int i = 0; i < 100; i++) {
        if (ev.dat4 != 0 && enemy[i].link_num != ev.dat4) {
          continue;
        }

        if (ev.dat != -99) {
          enemy[i].x_reverse_speed = ev.dat;
        }
        if (ev.dat2 != -99) {
          enemy[i].y_reverse_speed = ev.dat2;
        }
        if (ev.dat3 != 0 && ev.dat3 < 17) {
          enemy[i].filter = ev.dat3;
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

      map.back_move = ev.dat;
      map.back_move2 = ev.dat2;
      state.explosions.move = map.back_move2;
      map.back_move3 = ev.dat3;
      break;

    case 31:  // Enemy Fire Override
      for (int i = 0; i < 100; i++) {
        if (ev.dat4 != 99 && enemy[i].link_num != ev.dat4) {
          continue;
        }

        enemy[i].shot_freq[1 - 1] = ev.dat;
        enemy[i].shot_freq[2 - 1] = ev.dat2;
        enemy[i].shot_freq[3 - 1] = ev.dat3;
        // the C code counts in temp2 (Scratch)
        uint8_t& temp2 = state.scratch.temp2;
        for (temp2 = 0; temp2 < 3; temp2++) {
          enemy[i].shot_wait[temp2] = 1;
        }
        if (enemy[i].launch_type > 0) {
          enemy[i].launch_freq = ev.dat5;
          enemy[i].launch_wait = 1;
        }
      }
      break;

    case 32:  // create enemy
      if (int const b = create_new_event_enemy(state, 0, 50, 0); b > 0) {
        enemy[b - 1].y = 190;
      }
      break;

    case 33:  // Enemy From other Enemies
      if ((ev.dat == 512 || ev.dat == 513) &&
          (game.two_player_mode || game.one_player_action || game.super_tyrian))
      {
        break;
      }

      if (game.super_arcade_mode != SA_NONE) {
        if (ev.dat == 534) {
          ev.dat = 827;
        }
      }
      else if (!game.super_tyrian) {
        uint8_t const lives = *players.slots[0].lives;

        if (ev.dat == 533 && (lives == 11 || (mt_rand_r(&mt) % 15) < lives)) {
          // enemy will drop random special weapon
          ev.dat = 829 + (mt_rand_r(&mt) % 6);
        }
      }
      if (ev.dat == 534 && game.super_tyrian) {
        ev.dat = 828 + superTyrianSpecials[mt_rand_r(&mt) % 4];
      }

      for (int i = 0; i < 100; i++) {
        if (enemy[i].link_num == ev.dat4) {
          enemy[i].dies_into = ev.dat;
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
        play_song(ev.dat - 1);
        set_volume(tyrMusicVolume, fxVolume);
      }
      sound.music_fade = false;
      break;

    case 36:
      ending.ready_to_end_level = true;
      break;

    case 37:
      level.enemy_frequency = ev.dat;
      break;

    case 38: {
      level.cur_loc = ev.dat;
      int new_event_loc = 1;
      for (uint16_t i = 0; i < level.max_event; i++) {
        if (level.events[i].time <= level.cur_loc) {
          new_event_loc = i + 1 - 1;
        }
      }
      level.event_loc = new_event_loc;
      break;
    }

    case 39:  // Enemy Global Linknum Change
      for (int i = 0; i < 100; i++) {
        if (enemy[i].link_num == ev.dat) {
          enemy[i].link_num = ev.dat2;
        }
      }
      break;

    case 40:  // Enemy Continual Damage
      level.enemy_continual_damage = true;
      break;

    case 41:
      if (ev.dat == 0) {
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
      map.background2_over = ev.dat;
      break;

    case 44:
      display.filter_active = (ev.dat > 0);
      display.filter_fade = (ev.dat == 2);
      display.filter = ev.dat2;
      display.brightness = ev.dat3;
      display.filter_new = ev.dat4;
      display.brightness_chg = ev.dat5;
      display.filter_fade_start = (ev.dat6 == 0);
      break;

    case 45:  // arcade-only enemy from other enemies
      if (game.super_tyrian) {
        break;
      }

      if (uint8_t const lives = *players.slots[0].lives;
          ev.dat == 533 && (lives == 11 || (mt_rand_r(&mt) % 15) < lives))
      {
        ev.dat = 829 + (mt_rand_r(&mt) % 6);
      }
      if (game.two_player_mode || game.one_player_action) {
        for (int i = 0; i < 100; i++) {
          if (enemy[i].link_num == ev.dat4) {
            enemy[i].dies_into = ev.dat;
          }
        }
      }
      break;

    case 46:  // change difficulty
      if (ev.dat3 != 0) {
        players.damage_rate = ev.dat3;
      }

      if (ev.dat2 == 0 || game.two_player_mode || game.one_player_action) {
        game.difficulty_level += ev.dat;
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
        if (ev.dat4 == 0 || enemy[i].link_num == ev.dat4) {
          enemy[i].armor = ev.dat;
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
      int16_t const graphic = ev.dat;
      ev.dat = 0;
      int16_t const shape_table = ev.dat3;
      ev.dat3 = 0;
      int16_t const armor = ev.dat6;
      ev.dat6 = 0;
      enemyDat[0].armor = armor;
      enemyDat[0].egraphic[1 - 1] = graphic;
      uint8_t first = 0;
      switch (ev.type - 48) {
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
      ev.dat = graphic;
      ev.dat3 = shape_table;
      ev.dat6 = armor;
      break;
    }

    case 53:
      level.force_events = (ev.dat != 99);
      break;

    case 54:
      event_jump(level, ev.dat);
      break;

    case 55:  // Enemy Global AccelRev
      take_link_from_new_pl(level, ev);

      for (int i = 0; i < 100; i++) {
        if (ev.dat4 != 0 && enemy[i].link_num != ev.dat4) {
          continue;
        }

        if (ev.dat != -99) {
          enemy[i].x_homing = ev.dat;
        }
        if (ev.dat2 != -99) {
          enemy[i].y_homing = ev.dat2;
        }
      }
      break;

    case 56:  // Ground2 Bottom
      if (int const b = create_new_event_enemy(state, 0, 75, 0); b > 0) {
        enemy[b - 1].y = 190;
      }
      break;

    case 57:
      level.super_enemy_254_jump = ev.dat;
      break;

    case 60:  // Assign Special Enemy
      for (int i = 0; i < 100; i++) {
        if (enemy[i].link_num == ev.dat4) {
          enemy[i].sets_flag = true;
          enemy[i].flag_num = ev.dat;
          enemy[i].flag_value = (ev.dat2 == 1);
        }
      }
      break;

    case 61:  // if specific flag set to specific value, skip events
      if (state.level.global_flags[ev.dat - 1] == ev.dat2) {
        level.event_loc += ev.dat3;
      }
      break;

    case 62:  // Play sound effect
      sound.queue[3] = ev.dat;
      break;

    case 63:  // skip events if not in 2-player mode
      if (!game.two_player_mode && !game.one_player_action) {
        level.event_loc += ev.dat;
      }
      break;

    case 64:
      if (!(ev.dat == 6 && game.two_player_mode && game.difficulty_level > DIFFICULTY_NORMAL)) {
        display.smoothies[ev.dat - 1] = ev.dat2;
        uint8_t smoothie = ev.dat;
        if (smoothie == 5) {
          smoothie = 3;
        }
        display.smoothie_data[smoothie - 1] = ev.dat3;
      }
      break;

    case 65:
      map.background3x1 = (ev.dat == 0);
      break;

    case 66:  // If not on this difficulty level or higher then...
      if (game.initial_difficulty <= ev.dat) {
        level.event_loc += ev.dat2;
      }
      break;

    case 67:
      level.timer = (ev.dat == 1);
      level.timer_countdown = ev.dat3 * 100;
      level.timer_jump_to = ev.dat2;
      break;

    case 68:
      level.random_explosions = (ev.dat == 1);
      break;

    case 69:
      for (Player& p : players.slots) {
        p.invulnerable_ticks = ev.dat;
      }
      break;

    case 70:
      // 1-10
      if (ev.dat2 == 0) {
        bool found = false;

        for (uint8_t link = 1; link <= 19; link++) {
          found = found || search_for(state, link, NULL);
        }

        if (!found) {
          event_jump(level, ev.dat);
        }
      }
      else if (!search_for(state, ev.dat2, NULL) && (ev.dat3 == 0 || !search_for(state, ev.dat3, NULL)) &&
               (ev.dat4 == 0 || !search_for(state, ev.dat4, NULL)))
      {
        event_jump(level, ev.dat);
      }
      break;

    case 71:
      if (((((intptr_t)map.map_y_pos - (intptr_t)&megaData1.mainmap) / sizeof(uint8_t *)) * 2) <=
          (unsigned)ev.dat2)
      {
        event_jump(level, ev.dat);
      }
      break;

    case 72:
      map.background3x1b = (ev.dat == 1);
      break;

    case 73:
      map.sky_enemy_over_all = (ev.dat == 1);
      break;

    case 74:  // Enemy Global BounceParams
      for (int i = 0; i < 100; i++) {
        if (ev.dat4 != 0 && enemy[i].link_num != ev.dat4) {
          continue;
        }

        if (ev.dat5 != -99) {
          enemy[i].x_min_bounce = ev.dat5;
        }

        if (ev.dat6 != -99) {
          enemy[i].y_min_bounce = ev.dat6;
        }

        if (ev.dat != -99) {
          enemy[i].x_max_bounce = ev.dat;
        }

        if (ev.dat2 != -99) {
          enemy[i].y_max_bounce = ev.dat2;
        }
      }
      break;

    case 75: {
      // Picks one of the link numbers eventdat to eventdat2 of enemies with
      // armor that don't move down, at random, for newPL; or skips events if
      // there is none.
      bool any = false;

      for (int i = 0; i < 100; i++) {
        if (enemy_avail[i] == 0 && enemy[i].y_speed == 0 && enemy[i].link_num >= ev.dat &&
            enemy[i].link_num <= ev.dat2)
        {
          any = true;
        }
      }

      if (any) {
        uint8_t link;
        uint8_t enemy_i;
        do {
          link = (mt_rand_r(&mt) % (ev.dat2 + 1 - ev.dat)) + ev.dat;
        } while (!(search_for(state, link, &enemy_i) && enemy[enemy_i].y_speed == 0));

        level.new_pl[ev.dat3 - 80] = link;
      }
      else {
        level.new_pl[ev.dat3 - 80] = 255;
        // Skip
        if (ev.dat4 > 0) {
          level.cur_loc = level.events[level.event_loc - 1 + ev.dat4].time - 1;
          level.event_loc += ev.dat4 - 1;
        }
      }

      break;
    }

    case 76:
      level.return_active = true;
      break;

    case 77:
      map.map_y_pos = &megaData1.mainmap[0][0];
      map.map_y_pos += ev.dat / 2;
      if (ev.dat2 > 0) {
        map.map_y2_pos = &megaData2.mainmap[0][0];
        map.map_y2_pos += ev.dat2 / 2;
      }
      else {
        map.map_y2_pos = &megaData2.mainmap[0][0];
        map.map_y2_pos += ev.dat / 2;
      }
      break;

    case 78:
      if (level.galaga_shot_freq < 10) {
        level.galaga_shot_freq++;
      }
      break;

    case 79:
      display.boss_bars[0].link_num = ev.dat;
      display.boss_bars[1].link_num = ev.dat2;
      break;

    case 80:  // skip events if in 2-player mode
      if (game.two_player_mode) {
        level.event_loc += ev.dat;
      }
      break;

    case 81:  // WRAP2
      map.bk_wrap2 = &megaData2.mainmap[0][0];
      map.bk_wrap2 += ev.dat / 2;
      map.bk_wrap2_to = &megaData2.mainmap[0][0];
      map.bk_wrap2_to += ev.dat2 / 2;
      break;

    case 82:  // Give SPECIAL WEAPON
      players.slots[0].items.special = ev.dat;
      players.shot_multi_pos[SHOT_SPECIAL] = 0;
      players.shot_repeat[SHOT_SPECIAL] = 0;
      players.shot_multi_pos[SHOT_SPECIAL2] = 0;
      players.shot_repeat[SHOT_SPECIAL2] = 0;
      break;

    default:
      logWarn("Event %05d %d has invalid type.", ev.time, ev.type);
      break;
  }

  level.event_loc++;
}

}  // namespace

void update_events(State& state, HudLog& hud) {
  Level& level = state.level;
  while (level.events[level.event_loc - 1].time <= level.cur_loc && level.event_loc <= level.max_event) {
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
  } while (!(level.events[i - 1].time >= level.cur_loc));
  level.event_loc = i - 1;
}

}  // namespace tyrx::sim
