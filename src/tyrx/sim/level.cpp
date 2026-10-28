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
// One tick of the game's level loop (from the level_loop label in JE_main,
// tyrian2.c, to the jump back to it): its simulation, in the C code's order,
// which records the tick's drawing in the draw record.  The frame (the
// drawing, the wait for the frame's deadline, the present and the event poll)
// is left to the code running it.
//
// Besides the State, the tick changes the C code's sound and music, as the C
// code does, plays the sound queue, and reads the input for stopping demos,
// pausing, the menu and the key commands from the C globals.

#include "sim.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace tyrx::sim {

namespace {

bool all_players_dead(State const& state) {
  auto const& players = state.players.slots;
  return !players[0].is_alive && (!state.game.two_player_mode || !players[1].is_alive);
}

// Whether background 2 is drawn where the C code draws it over things: not
// with smoothies that draw it themselves.
bool background_2_drawn(State const& state) {
  uint8_t const processor = state.game.processor_type;
  auto const&   smoothies = state.display.smoothies;
  return !(smoothies[2 - 1] && processor < 4) && !(smoothies[1 - 1] && processor == 3);
}

// Background 2, blended in wild mode.
void background_2_over(State& state, DrawRecord& record, std::optional<Background2Drawn>& drawn) {
  background_2(state, record, state.cheats.wild && !state.map.background2_not_transparent, drawn);
}

// A smoothie's filter from game_screen to the surface, which the rest draws
// on game_screen.
void smoothie(DrawRecord& record, std::optional<Copy>& copy) {
  copy = Copy{Surface::game_screen, record.screen};
  record.screen = Surface::game_screen;
}

// One JE_drawEnemy: the enemies before slot end, on a layer that has scrolled
// map_x_ofs sideways and moved back_move down.
void enemies(State& state, DrawRecord& record, int end, uint16_t map_x_ofs, uint16_t back_move,
             std::optional<EnemiesDrawn>& drawn) {
  drawn = EnemiesDrawn{record.screen, map_x_ofs, update_enemies(state, end, map_x_ofs, back_move)};
}

// The start of the tick: the smoothie screen code, background wrapping, and
// music.
void tick_start(State& state) {
  Game&   game = state.game;
  Sound&  sound = state.sound;
  Map&    map = state.map;
  Ending& ending = state.ending;
  auto&   players = state.players.slots;
  auto&   smoothies = state.display.smoothies;
  // tempScreenSeg = game_screen; // side-effect of game_screen

  if (isNetworkGame) {
    smoothies[9 - 1] = false;
    smoothies[6 - 1] = false;
  }
  else {
    state.display.star_show_vga_special_code = smoothies[9 - 1] + (smoothies[6 - 1] << 1);
  }

  // Background Wrapping
  if (map.map_y_pos <= map.bk_wrap1) {
    map.map_y_pos = map.bk_wrap1_to;
  }
  if (map.map_y2_pos <= map.bk_wrap2) {
    map.map_y2_pos = map.bk_wrap2_to;
  }
  if (map.map_y3_pos <= map.bk_wrap3) {
    map.map_y3_pos = map.bk_wrap3_to;
  }

  ending.all_players_gone =
      all_players_dead(state) &&
      ((players[0].lives() == 1 && players[0].exploding_ticks == 0) ||
       (!game.one_player_action && !game.two_player_mode)) &&
      ((players[1].lives() == 1 && players[1].exploding_ticks == 0) || !game.two_player_mode);

  // -----MUSIC FADE------
  if (sound.music_fade) {
    if (sound.temp_volume > 10) {
      sound.temp_volume--;
      set_volume(sound.temp_volume, fxVolume);
    }
    else {
      sound.music_fade = false;
    }
  }

  if (!ending.all_players_gone && ending.countdown > 0 && ending.end_level) {
    play_song(9);
    sound.music_fade = false;
  }
  else if (!playing && ending.first_game_over) {
    play_song(sound.level_song - 1);
  }
}

// The HUD at the top of the tick: message bar, shield generator, weapon power
// and power bar.
void hud_update(State& state, LevelLocals& locals, DrawRecord& record) {
  Game&    game = state.game;
  Players& players = state.players;
  auto&    slots = players.slots;
  HudLog&  hud = record.hud;

  if (state.ending.end_level) {
    return;
  }

  // draw HUD
  record.screen = Surface::vga_screen_seg;  // side-effect of game_screen

  // -----------------------Message Bar------------------------
  if (state.display.text_erase > 0 && --state.display.text_erase == 0) {
    hud.draws.push_back(hud::MessageBarErased{});  // in-game message area
  }

  // ------------------------Shield Gen-------------------------
  if (game.galaga_mode) {
    for (Player& p : slots) {
      p.shield = 0;
    }

    // spawned dragonwing died :(
    if (slots[1].lives() == 0 || slots[1].armor == 0) {
      game.two_player_mode = false;
    }

    int32_t& galaga_life = state.level.galaga_life;
    if (slots[0].cash >= (unsigned)galaga_life) {
      state.sound.queue[6] = S_EXPLOSION_11;
      state.sound.queue[7] = S_SOUL_OF_ZINGLON;

      if (slots[0].lives() < 11) {
        ++slots[0].lives();
      }
      else {
        slots[0].cash += 1000;
      }

      if (galaga_life == 10000) {
        galaga_life = 20000;
      }
      else {
        galaga_life += 25000;
      }
    }
  }
  // not galagaMode
  else if (game.two_player_mode) {
    if (--players.shield_wait == 0) {
      players.shield_wait = 15;

      for (Player& p : slots) {
        if (p.shield < p.shield_max && p.is_alive) {
          ++p.shield;
        }
      }

      hud_shield(state, hud);
    }
  }
  else if (slots[0].is_alive && slots[0].shield < slots[0].shield_max && players.power > players.shield_cost)
  {
    if (--players.shield_wait == 0) {
      players.shield_wait = 15;

      players.power -= players.shield_cost;

      ++slots[0].shield;
      if (slots[1].shield < slots[0].shield_max) {
        ++slots[1].shield;
      }

      hud_shield(state, hud);
    }
  }

  // ---------------------Weapon Display-------------------------
  for (unsigned i = 0; i < 2; ++i) {
    unsigned const item_power = slots[game.two_player_mode ? i : 0].items.weapon[i].power;

    if (locals.old_weapon_bar[i] == item_power) {
      continue;
    }

    locals.old_weapon_bar[i] = item_power;

    int const x = game.two_player_mode ? 286 : 289;
    int const y = (i == 0) ? (game.two_player_mode ? 6 : 17) : (game.two_player_mode ? 100 : 38);

    hud.draws.push_back(hud::WeaponPower{x, y, item_power});
  }

  // ------------------------Power Bar-------------------------
  if (game.two_player_mode || game.one_player_action) {
    players.power = 900;
  }
  else {
    players.power += players.power_add;
    if (players.power > 900) {
      players.power = 900;
    }

    uint8_t const bar = players.power / 10;

    if (bar != players.last_power) {
      hud.draws.push_back(hud::PowerBar{bar, players.last_power});

      players.last_power = bar;
    }
  }

  state.map.old_map_x3_ofs = state.map.map_x3_ofs;

  state.enemies.on_screen = 0;
}

// Smoothies, background 1, the starfield and background 2 under the enemies.
void backgrounds_under(State& state, DrawRecord& record, Host& host) {
  uint8_t const processor = state.game.processor_type;
  auto const&   smoothies = state.display.smoothies;
  auto const&   smoothie_data = state.display.smoothie_data;
  Map&          map = state.map;
  Level&        level = state.level;
  uint8_t const astral_duration = state.specials.astral_duration;

  // SMOOTHIES!
  check_smoothies(state);
  if (state.display.any_smoothies) {
    record.screen = Surface::vga_screen2;  // this makes things complicated, but we do it anyway :(
  }

  // --- BACKGROUNDS ---
  // --- BACKGROUND 1 ---

  if (level.force_events && !map.back_move) {
    level.cur_loc++;
  }

  if (map.map1_y_delay_max > 1 && map.back_move < 2) {
    map.back_move = (map.map1_y_delay == 1) ? 1 : 0;
  }

  // Draw background, or JE_clr256 in the astral zone
  background_1(state, record, astral_duration != 0);

  // Set Movement of background 1
  if (--map.map1_y_delay == 0) {
    map.map1_y_delay = map.map1_y_delay_max;

    level.cur_loc += map.back_move;

    map.back_pos += map.back_move;

    if (map.back_pos > 27) {
      map.back_pos -= 28;
      map.map_y--;
      map.map_y_pos -= 14;  // Map Width
    }
  }

  if (state.display.star_active || astral_duration > 0) {
    update_starfield(state.starfield, host.pitch(record.screen));
    record.starfield = record.screen;
  }

  if (processor > 1 && smoothies[5 - 1]) {
    smoothie(record, record.iced_blur_1);
  }

  // -----------------------BACKGROUNDS------------------------
  // -----------------------BACKGROUND 2------------------------
  if (map.background2_over == 3) {
    background_2(state, record, false, record.background_2_under);
    map.background2 = true;
  }

  if (map.background2_over == 0 && background_2_drawn(state)) {
    background_2_over(state, record, record.background_2_under);
  }

  if (smoothies[0] && processor > 2 && smoothie_data[0] == 0) {
    smoothie(record, record.lava_1);
  }
  if (smoothies[2 - 1] && processor > 2) {
    smoothie(record, record.water);
  }
}

// The ground, sky and top enemies, with the background 2 and 3 layers and
// smoothie filters between them, and new random enemies.
void enemies_under(State& state, DrawRecord& record) {
  uint8_t const processor = state.game.processor_type;
  auto const&   smoothies = state.display.smoothies;
  auto const&   smoothie_data = state.display.smoothie_data;
  Map&          map = state.map;
  Level&        level = state.level;
  uint16_t&     on_screen = state.enemies.on_screen;
  MtState&      mt = state.random.mt;

  // -----------------------Ground Enemy------------------------
  uint16_t last_on_screen = on_screen;

  enemies(state, record, 50, map.map_x_ofs, map.back_move, record.ground_enemies);
  enemies(state, record, 100, map.map_x_ofs, map.back_move, record.ground_enemies_2);

  if ((on_screen == 0 || on_screen == last_on_screen) && map.stop_background_num == 1) {
    map.stop_background_num = 9;
  }

  if (smoothies[0] && processor > 2 && smoothie_data[0] > 0) {
    smoothie(record, record.lava_2);
  }

  if (state.cheats.super_wild) {
    state.display.neat += 3;
    record.darken_1 = Darkened{record.screen, state.display.neat};
  }

  // -----------------------BACKGROUNDS------------------------
  // -----------------------BACKGROUND 2------------------------
  if (map.background2_over == 1 && background_2_drawn(state)) {
    background_2_over(state, record, record.background_2_over_1);
  }

  if (state.cheats.super_wild) {
    state.display.neat++;
    record.darken_2 = Darkened{record.screen, state.display.neat};
  }

  if (map.background3_over == 2) {
    background_3(state, record, record.background_3_over_2);
  }

  // New Enemy
  if (level.enemies_active && mt_rand_r(&mt) % 100 > level.enemy_frequency) {
    uint16_t const type = level.enemy_types[mt_rand_r(&mt) % level.enemy_types_max];
    if (type == 2) {
      state.sound.queue[3] = S_WEAPON_7;
    }
    new_enemy(state, 0, type, 0);
  }

  if (processor > 1 && smoothies[3 - 1]) {
    smoothie(record, record.iced_blur_2);
  }
  if (processor > 1 && smoothies[4 - 1]) {
    smoothie(record, record.blur);
  }

  // Draw Sky Enemy
  if (!map.sky_enemy_over_all) {
    last_on_screen = on_screen;

    enemies(state, record, 25, map.map_x2_ofs, 0, record.sky_enemies_under);

    if (on_screen == last_on_screen && map.stop_background_num == 2) {
      map.stop_background_num = 9;
    }
  }

  if (map.background3_over == 0) {
    background_3(state, record, record.background_3_over_0);
  }

  // Draw Top Enemy
  if (!map.top_enemy_over) {
    enemies(state, record, 75, (map.background3x1 == 0) ? map.old_map_x3_ofs : map.map_x_ofs, map.back_move3,
            record.top_enemies_under);
  }
}

// The enemy shots, and background 3 and the enemies over them.
void enemy_shots_and_over(State& state, DrawRecord& record) {
  Map&      map = state.map;
  uint16_t& on_screen = state.enemies.on_screen;

  // MAIN DRAWING IS STOPPED STARTING HERE
  if (!state.ending.end_level) {
    // Draw Enemy Shots
    Surface const screen = record.screen;
    record.enemy_shots = EnemyShotsDrawn{screen, update_enemy_shots(state, record)};
  }

  if (map.background3_over == 1) {
    background_3(state, record, record.background_3_over_1);
  }

  // Draw Top Enemy
  if (map.top_enemy_over) {
    enemies(state, record, 75, (map.background3x1 == 0) ? map.old_map_x3_ofs : map.old_map_x_ofs,
            map.back_move3, record.top_enemies_over);
  }

  if (!map.sky_enemy_over_all) {
    return;
  }

  // Draw Sky Enemy
  uint16_t const last_on_screen = on_screen;

  enemies(state, record, 25, map.map_x2_ofs, 0, record.sky_enemies_over);

  if (on_screen == last_on_screen && map.stop_background_num == 2) {
    map.stop_background_num = 9;
  }
}

// Sequenced explosions, and the explosions.
void explosions(State& state, DrawRecord& record) {
  // -------------------------- Sequenced Explosions -------------------------
  state.ending.enemy_still_exploding =
      update_repeating_explosions(state.explosions, state.random.mt, state.map.back_move2, state.sound.queue);

  // ---------------------------- Draw Explosions ----------------------------
  record.explosions = ExplosionsDrawn{record.screen, update_explosions(state.explosions)};

  if (!state.display.port_config_change) {
    state.display.port_config_done = true;
  }
}

// Background 2 over everything, the low armor warning, and random explosions.
void warning(State& state, DrawRecord& record) {
  Game const& game = state.game;
  Display&    display = state.display;
  auto&       players = state.players.slots;
  MtState&    mt = state.random.mt;

  // -----------------------BACKGROUNDS------------------------
  // -----------------------BACKGROUND 2------------------------
  if (state.map.background2_over == 2 && background_2_drawn(state)) {
    background_2_over(state, record, record.background_2_over_2);
  }

  // -------------------------Warning---------------------------
  bool const player1_low = players[0].is_alive && players[0].armor < 6;
  bool const player2_low = players[1].is_alive && players[1].armor < 6;
  if (player1_low || (game.two_player_mode && !game.galaga_mode && player2_low)) {
    int const armor_amount = player1_low ? players[0].armor : players[1].armor;

    uint16_t& delay = state.players.armor_ship_delay;
    if (delay > 0) {
      delay--;
    }
    else {
      if (int const b = new_enemy(state, 50, 560, 0); b > 0) {
        Enemy& e = state.enemies.slots[b - 1];
        e.dies_into = 560 + (mt_rand_r(&mt) % 3) + 1;
        e.y_speed -= state.map.back_move3;
        e.armor = 4;
      }
      delay = 500;
    }

    if ((player1_low && (!isNetworkGame || thisPlayerNum == 1)) ||
        (game.two_player_mode && player2_low && (!isNetworkGame || thisPlayerNum == 2)))
    {
      uint16_t const sound_delay = armor_amount * 4 + 8;
      if (display.warning_sound_delay > sound_delay) {
        display.warning_sound_delay = sound_delay;
      }

      if (display.warning_sound_delay > 1) {
        display.warning_sound_delay--;
      }
      else {
        state.sound.queue[7] = S_WARNING;
        display.warning_sound_delay = sound_delay;
      }

      display.warning_col += display.warning_col_change;
      if (display.warning_col > 113 + (14 - (armor_amount * 2))) {
        display.warning_col_change = -display.warning_col_change;
        display.warning_col = 113 + (14 - (armor_amount * 2));
      }
      else if (display.warning_col < 113) {
        display.warning_col_change = -display.warning_col_change;
      }
      record.warning = record.screen;
    }
  }

  // ------- Random Explosions --------
  if (state.level.random_explosions && mt_rand_r(&mt) % 10 == 1) {
    setup_explosion_large(state.explosions, false, 20, mt_rand_r(&mt) % 280, mt_rand_r(&mt) % 180);
  }
}

// The sound routine, which plays the sound queue, and the return event.
void sound(State& state) {
  auto& queue = state.sound.queue;

  // =================================
  // =======The Sound Routine=========
  // =================================
  if (state.ending.first_game_over) {
    // The C code counts the channels in temp2 (Scratch).
    uint8_t& temp2 = state.scratch.temp2;
    for (temp2 = 0; temp2 < std::size(queue); temp2++) {
      if (queue[temp2] == S_NONE) {
        continue;
      }

      uint8_t const sample = queue[temp2];
      uint8_t       volume;
      if (temp2 == 3) {
        volume = fxPlayVol;
      }
      else if (sample == 15) {
        volume = fxPlayVol / 4;
      }
      // Lightning
      else {
        volume = fxPlayVol / 2;
      }

      multiSamplePlay(soundSamples[sample - 1], soundSampleCount[sample - 1], temp2, volume);

      queue[temp2] = S_NONE;
    }
  }

  if (state.level.return_active && state.enemies.on_screen == 0) {
    event_jump(state.level, 65535);
    state.level.return_active = false;
  }
}

// The debug display, the bonus message, the speed mode and the level timer.
void texts(State& state, DrawRecord& record) {
  Display&    display = state.display;
  Level&      level = state.level;
  auto const& players = state.players.slots;
  char        buffer[256];

  // -------      DEbug      ---------
  debugTime = SDL_GetTicks();

  if (debug) {
    char smoothies[10];
    for (size_t i = 0; i < 9; i++) {
      smoothies[i] = '0' + display.smoothies[i];
    }
    smoothies[9] = '\0';
    record.debug = DebugDrawn{record.screen, {}};
    sprintf(buffer, "SM = %s", smoothies);
    record.debug->text[0] = buffer;

    sprintf(buffer, "Memory left = %d", -1);
    record.debug->text[1] = buffer;
    sprintf(buffer, "Enemies onscreen = %d", state.enemies.on_screen);
    record.debug->text[2] = buffer;

    debugHist += debugTime - lastDebugTime;
    debugHistCount++;
    char fps[32];
    sprintf(fps, "%2.3f", debugHistCount / (debugHist * 1e-3f));
    sprintf(buffer, "X:%d Y:%-5d  %s FPS  %d %d %d %d", (state.map.map_x - 1) * 12 + players[0].x,
            level.cur_loc, fps, players[0].x_velocity, players[0].y_velocity, players[0].x, players[0].y);
    record.debug->text[3] = buffer;
    lastDebugTime = debugTime;
  }

  if (display.display_time > 0) {
    display.display_time--;
    record.message = MessageDrawn{record.screen, display.flash};
    display.flash += display.flash_change;
    if (display.flash > 4 || display.flash == 0) {
      display.flash_change = -display.flash_change;
    }
  }

  // Pentium Speed Mode?
  if (state.game.pentium_mode) {
    state.game.frame_count_max = (state.game.frame_count_max == 2) ? 3 : 2;
  }

  // --------  Level Timer    ---------
  if (!level.timer || level.timer_countdown <= 0) {
    return;
  }

  level.timer_countdown--;
  if (level.timer_countdown == 0) {
    event_jump(level, level.timer_jump_to);
  }

  if (level.timer_countdown > 200) {
    if (level.timer_countdown % 100 == 0) {
      state.sound.queue[7] = S_WARNING;
    }

    if (level.timer_countdown % 10 == 0) {
      state.sound.queue[6] = S_CLICK;
    }
  }
  else if (level.timer_countdown % 20 == 0) {
    state.sound.queue[7] = S_WARNING;
  }

  record.level_timer = record.screen;
}

// Game over, and the input for stopping demos, pausing, the menu and cheats.
// Returns true if the in-game menu or help opened, which ends the tick
// without a frame (the C code jumps back to level_loop).
bool game_over_and_input(State& state, DrawRecord& record, Host& host) {
  Game const& game = state.game;
  Ending&     ending = state.ending;
  auto&       players = state.players.slots;

  // GAME OVER
  if (!state.cheats.constant_play && !state.cheats.constant_die && ending.all_players_gone) {
    if (players[0].exploding_ticks > 0 || players[1].exploding_ticks > 0) {
      if (game.galaga_mode) {
        players[1].exploding_ticks = 0;
      }

      state.sound.music_fade = true;
    }
    else {
      if (game.play_demo || ending.normal_bonus_level_current || ending.bonus_level_current) {
        ending.really_end_level = true;
      }
      else {
        record.game_over = record.screen;  // game over
      }

      if (ending.first_game_over) {
        if (!game.play_demo) {
          play_song(SONG_GAMEOVER);
          set_volume(tyrMusicVolume, fxVolume);
        }
        ending.first_game_over = false;
      }

      if (!game.play_demo) {
        push_joysticks_as_keyboard();

        if (hasInput(INPUT_NO_MOTION)) {
          ending.really_end_level = true;
        }
      }

      if (isNetworkGame) {
        ending.really_end_level = true;
      }
    }
  }

  // input stops demo
  if (game.play_demo) {
    push_joysticks_as_keyboard();

    if (hasInput(INPUT_NO_MOTION)) {
      ending.really_end_level = true;

      ending.stopped_demo = true;
    }
    return false;
  }

  // input handling for pausing, menu, cheats

  // Ensure gameplay input does not affect pause or menu.
  mouseClearInput();

  if (keyboardHasInput()) {
    // Pause, menu, and cheats are triggered on keysactive, so this is fine.
    keyboardClearInput();

    skipStarShowVGA = false;
    if (keysactive[SDL_SCANCODE_F1]) {
      host.in_game_help_ahead();  // main_keyboard_input opens help
    }
    main_keyboard_input(state, record, host);
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
      host.c_loop(JE_pauseGame);
    }
  }

  if (ingamemenu_pressed) {
    ingamemenu_pressed = false;

    if (isNetworkGame) {
      inGameMenuRequest = true;
    }
    else {
      yourInGameMenuRequest = true;
      host.c_loop(JE_doInGameSetup);
      skipStarShowVGA = true;
    }
  }

  return false;
}

// The filter on the playing area, drawn with the filter and brightness from
// before the fade moves on.
void filter_screen(State& state, DrawRecord& record) {
  Display& display = state.display;
  record.filter = FilterDrawn{record.screen, display.filter, display.brightness};

  if (!display.filter_fade) {
    return;
  }

  display.brightness += display.brightness_chg;
  if ((display.filter_fade_start && display.brightness < -14) || display.brightness > 14) {
    display.brightness_chg = -display.brightness_chg;
    display.filter_fade_start = false;
    display.filter = display.filter_new;
  }
  if (!display.filter_fade_start && display.brightness == 0) {
    display.filter_fade = false;
    display.brightness = -99;
  }
}

// draw_boss_bar: the bars follow the most damaged enemy of their link group,
// and flash after a hit; the drawing gets where they are and how they look.
void boss_bars(State& state, DrawRecord& record) {
  auto&       bars = state.display.boss_bars;
  auto const& enemies = state.enemies;

  for (BossBar& bar : bars) {
    if (bar.link_num == 0) {
      continue;
    }
    unsigned armor = 256;  // higher than armor max
    // find most damaged
    for (unsigned e = 0; e < std::size(enemies.slots); e++) {
      if (enemies.status[e] == 1 || enemies.slots[e].link_num != bar.link_num) {
        continue;
      }

      if (enemies.slots[e].armor < armor) {
        armor = enemies.slots[e].armor;
      }
    }
    // boss dead?
    if (armor > 255 || armor == 0) {
      bar.link_num = 0;
    }
    else {
      bar.armor = (armor == 255) ? 254 : armor;  // 255 would make the bar too long
    }
  }

  unsigned const count = (bars[0].link_num != 0 ? 1 : 0) + (bars[1].link_num != 0 ? 1 : 0);

  // if only one bar left, make it the first one
  if (count == 1 && bars[0].link_num == 0) {
    bars[0] = bars[1];
    bars[1].link_num = 0;
  }

  BossBarsDrawn drawn{record.screen, count, {}};
  for (unsigned b = 0; b < count; b++) {
    unsigned const x = (count == 2)
                           ? ((b == 0) ? 125 : 185)
                           : ((state.level.timer) ? 250 : 155);  // level timer and boss bar would overlap
    drawn.bar[b] = {x, bars[b].armor, bars[b].color};
    if (bars[b].color > 0) {
      bars[b].color--;
    }
  }
  record.boss_bars = drawn;
}

// The changes that the C code makes after JE_starShowVGA, which read no input
// and draw nothing: start the backgrounds again if no enemies are on screen,
// and end the level when it is ready to.
void level_end_checks(State& state) {
  Map&    map = state.map;
  Ending& ending = state.ending;

  // Start backgrounds if no enemies on screen
  // End level if number of enemies left to kill equals 0.
  if (map.stop_background_num == 9 && map.back_move == 0 && !ending.enemy_still_exploding) {
    map.back_move = 1;
    map.back_move2 = 2;
    map.back_move3 = 3;
    state.explosions.move = 2;
    map.stop_background_num = 0;
    map.stop_backgrounds = false;
    if (ending.wait_to_end_level) {
      ending.end_level = true;
      ending.countdown = 40;
    }
    if (ending.all_players_gone) {
      ending.really_end_level = true;
    }
  }

  if (ending.end_level || state.enemies.on_screen != 0) {
    return;
  }

  if (ending.ready_to_end_level && !ending.enemy_still_exploding) {
    if (state.level.timer_countdown > 0) {
      state.level.timer = false;
    }
    ending.ready_to_end_level = false;
    ending.end_level = true;
    ending.countdown = 40;
    if (ending.all_players_gone) {
      ending.really_end_level = true;
    }
  }
  if (map.stop_backgrounds) {
    map.stop_backgrounds = false;
    map.back_move = 1;
    map.back_move2 = 2;
    map.back_move3 = 3;
    state.explosions.move = 2;
  }
}

}  // namespace

LevelPass level_tick(State& state, LevelLocals& locals, DrawRecord& record, Host& host, bool& frame) {
  frame = false;

  if (isNetworkGame) {
    return LevelPass::unsupported;
  }

  tick_start(state);

  hud_update(state, locals, record);

  // use game_screen for all the generic drawing functions
  record.screen = Surface::game_screen;

  // ---------------------------EVENTS-------------------------
  update_events(state, record.hud);

  backgrounds_under(state, record, host);

  enemies_under(state, record);

  // Player Shot Images
  record.player_shots = PlayerShotsDrawn{record.screen, update_player_shots(state)};

  // --------PLAYER DRAW+MOVEMENT---------
  record.players = update_players(state, record);

  enemy_shots_and_over(state, record);

  explosions(state, record);

  warning(state, record);

  sound(state);

  texts(state, record);

  if (game_over_and_input(state, record, host)) {
    return LevelPass::level_loop;
  }

  // Network games aren't supported (level_tick returns early).

  // Test
  record.superpixels = SuperpixelsDrawn{record.screen, update_superpixels(state.superpixels)};

  // Filtration
  if (state.display.filter_active) {
    filter_screen(state, record);
  }

  boss_bars(state, record);

  record.in_game_displays = record.screen;

  // The C code makes these changes after JE_starShowVGA; they read no input
  // and draw nothing, so they can come before the frame.
  level_end_checks(state);

  record.screen = Surface::vga_screen_seg;  // side-effect of game_screen

  record.show = !state.ending.player_end_level && !skipStarShowVGA;  // JE_starShowVGA copies the playing area
  frame = true;

  // Other Network Functions
  JE_handleChat();

  if (state.ending.really_end_level) {
    return LevelPass::start_level;
  }

  return LevelPass::level_loop;
}

}  // namespace tyrx::sim
