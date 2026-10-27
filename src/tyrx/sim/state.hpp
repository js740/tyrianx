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

// The C++ game's state: the state that the C globals hold in the C game, moved
// into structures, a subsystem at a time.  The C++ code works on a State that
// the code running the tick provides (port::state()).
//
// While the C game runs, it is the reference: gold/sim_state.hpp copies the C
// globals that a State holds into it and back.

#pragma once

#include "../gold/api.hpp"

#include <array>

namespace tyrx::sim {

struct Explosions {
  std::array<Explosion, MAX_EXPLOSIONS> slots;
  std::array<rep_explosion_type, MAX_REPEATING_EXPLOSIONS> repeating;  // which set off explosions over ticks
  JE_word move;                   // how far explosions that aren't fixed move down each tick
  JE_integer follow_x, follow_y;  // how far the player moved, for explosions that follow the player
};

struct Superpixels {
  std::array<superpixel_type, MAX_SUPERPIXELS> slots;
  unsigned int last;  // the slot of the last one created
};

struct Starfield {
  std::array<StarfieldStar, MAX_STARS> stars;  // their positions are offsets into the screen
  int speed;                                   // how much faster than their own speed all stars move
};

struct Random {
  MtState mt;  // the random number generator that the game logic draws from
};

struct Sound {
  std::array<JE_byte, 8> queue;  // the samples to play at the end of the tick, one per channel
  JE_boolean music_fade;
  JE_word temp_volume;  // the music's volume while it fades
  JE_byte level_song;
};

struct Display {
  // The screen filter, and its fades.
  JE_shortint filter, filter_new, brightness, brightness_chg;
  JE_boolean filtration_avail, filter_active, filter_fade, filter_fade_start;
  JE_boolean explosion_transparent;

  // The smoothies, effects on parts of the screen.
  std::array<JE_boolean, 9> smoothies;
  std::array<JE_byte, 9> smoothie_data;
  JE_boolean any_smoothies;

  // Flashes, warnings and texts.
  JE_byte flash;
  JE_shortint flash_change;
  JE_byte warning_col;
  JE_shortint warning_col_change;
  JE_boolean warning_red;
  JE_byte warning_sound_delay;
  JE_boolean level_warning_display;
  JE_byte level_warning_lines;
  std::array<std::array<char, 61>, 10> level_warning_text;
  JE_word text_erase;
  JE_byte display_time;
  JE_word neat;
  JE_byte star_show_vga_special_code;
  JE_boolean star_active;

  std::array<boss_bar_t, 2> boss_bars;
  JE_boolean port_config_change, port_config_done;
};

struct Map {
  // Where each of the three map layers is, and how far it moved in the tick.
  JE_word back_pos, back_pos2, back_pos3;
  JE_word back_move, back_move2, back_move3;
  JE_word map_x, map_y, map_x2, map_x3, map_y2, map_y3;
  JE_byte **map_y_pos, **map_y2_pos, **map_y3_pos;
  JE_word map_x_pos, old_map_x_ofs, map_x_ofs, map_x2_ofs, map_x2_pos, map_x3_pos, old_map_x3_ofs, map_x3_ofs,
      temp_map_x_ofs;
  intptr_t map_x_bp_pos, map_x2_bp_pos, map_x3_bp_pos;
  JE_byte map1_y_delay, map1_y_delay_max, map2_y_delay, map2_y_delay_max;
  JE_word temp_back_move;
  JE_byte **bk_wrap1, **bk_wrap2, **bk_wrap3, **bk_wrap1_to, **bk_wrap2_to, **bk_wrap3_to;

  // Which layers there are, how they are drawn, and whether they scroll.
  JE_boolean stop_backgrounds;
  JE_byte stop_background_num;
  JE_boolean background2, background2_not_transparent, background3x1, background3x1b;
  JE_byte background2_over, background3_over;
  JE_boolean sky_enemy_over_all, top_enemy_over;
};

struct Level {
  // The level's events, which some events change, and where the level is in them.
  std::array<JE_EventRecType, EVENT_MAXIMUM> events;
  JE_word max_event, event_loc;  // the number of events, and the next one
  JE_word cur_loc;               // how far the level has scrolled, which events wait for
  JE_word return_loc;            // where a jump back goes, if return_active
  JE_boolean return_active;
  JE_boolean force_events;
  JE_boolean jump_section;

  // Random enemies.
  JE_boolean enemies_active;
  std::array<JE_word, 40> enemy_types;
  JE_word enemy_types_max, enemy_frequency;

  JE_boolean enemy_continual_damage;
  JE_word super_enemy_254_jump;
  JE_boolean random_explosions;
  JE_boolean small_enemy_adjust;
  JE_word galaga_shot_freq;
  JE_longint galaga_life;

  // The level timer.
  JE_boolean timer;
  JE_word timer_countdown, timer_jump_to;

  // Where the map screen goes after the level.
  JE_word map_origin, map_pnum;
  std::array<JE_byte, 5> map_planet, map_section;
  std::array<JE_byte, 10> new_pl;
};

struct Ending {
  JE_boolean end_level, really_end_level, wait_to_end_level, player_end_level, ready_to_end_level;
  JE_byte countdown;  // levelEnd: ticks until the level ends
  JE_word fx_wait;    // ticks until the next of the dead players' explosions
  JE_shortint warp;   // levelEndWarp
  JE_boolean first_game_over, all_players_gone, enemy_still_exploding;
  bool stopped_demo;

  // What comes next.
  JE_boolean bonus_level, bonus_level_current, normal_bonus_level_current;
  JE_byte main_level, next_level;
};

struct Input {
  std::array<bool, 4> button;  // fire, left sidekick, right sidekick, fire mode
  JE_boolean fire_button_held;  // for the special weapon, which fires once per press
};

struct Players {
  std::array<Player, 2> slots;

  std::array<JE_byte, 11> shot_repeat;     // ticks until each kind of shot can fire again
  std::array<JE_byte, 11> shot_multi_pos;  // the pattern each kind of shot fires next
  uint power, last_power, power_add;       // the generator's power, as last drawn, and gained per tick
  JE_byte shield_wait, shield_cost;        // ticks until the shield recharges, and the power it takes
  JE_byte charge_wait, charge_level, charge_max, charge_gr, charge_gr_wait;  // the charge weapon
  JE_byte damage_rate;       // the most armor that a collision takes
  JE_word armor_ship_delay;  // ticks until the low-armor ship shows again

  // Two-player linked mode, and the sidekicks that attach to the ship.
  JE_boolean two_player_linked;
  JE_real link_gun_direction;
  JE_real option_satellite_rotate;
  JE_integer option_attachment_move;
  JE_boolean option_attachment_linked, option_attachment_return;

  // The ships' sprites.
  JE_word ship_gr, ship_gr2;
  Sprite2_array *ship_gr_ptr, *ship_gr2_ptr;
};

struct Specials {
  JE_byte wait, next_wait;
  std::array<std::array<JE_byte, 21>, 2> sf_current_code;  // the keys of each player's special moves so far
  std::array<JE_byte, 2> sf_executed;
  JE_boolean spray;

  // The special weapon.
  JE_word weapon_wpn;
  JE_shortint weapon_filter, weapon_freq;
  JE_boolean link_to_player;

  // The specials that last a while.
  JE_word flare_duration;
  JE_boolean flare_start;
  JE_shortint flare_col_chg;
  JE_byte zinglon_duration, astral_duration;
};

struct Enemies {
  std::array<JE_SingleEnemyType, 100> slots;
  std::array<JE_byte, 100> status;  // 1 for a free slot, 0 for an enemy, 2 for one without armor
  JE_word on_screen;                // how many were updated in the tick
  JE_word total, killed;            // for the destruction ratio
};

struct PlayerShots {
  std::array<PlayerShotDataType, MAX_PWEAPON + 1> slots;
  std::array<JE_byte, MAX_PWEAPON> ttl;  // ticks each shot has left, 0 for a free slot
};

struct EnemyShots {
  std::array<EnemyShotType, ENEMY_SHOT_MAX> slots;
  std::array<JE_boolean, ENEMY_SHOT_MAX> available;  // whether each slot is free
};

struct State {
  Random random;
  Explosions explosions;
  Superpixels superpixels;
  Starfield starfield;
  Sound sound;
  Display display;
  Map map;
  Level level;
  Ending ending;
  Input input;
  Players players;
  Specials specials;
  Enemies enemies;
  PlayerShots player_shots;
  EnemyShots enemy_shots;
};

}  // namespace tyrx::sim
