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

namespace tyrx::sim {

struct Explosions {
  Explosion slots[MAX_EXPLOSIONS];
  rep_explosion_type repeating[MAX_REPEATING_EXPLOSIONS];  // which set off explosions over ticks
  uint16_t move;               // how far explosions that aren't fixed move down each tick
  int16_t follow_x, follow_y;  // how far the player moved, for explosions that follow the player
};

struct Superpixels {
  superpixel_type slots[MAX_SUPERPIXELS];
  unsigned int last;  // the slot of the last one created
};

struct Starfield {
  StarfieldStar stars[MAX_STARS];  // their positions are offsets into the screen
  int speed;                       // how much faster than their own speed all stars move
};

// The game's modes and settings.
struct Game {
  bool two_player_mode;
  bool one_player_action;
  bool galaga_mode;
  uint8_t super_arcade_mode;
  uint8_t super_arcade_power_up;
  int8_t difficulty_level;
  int8_t initial_difficulty;
  int8_t old_difficulty_level;
  bool super_tyrian;
  uint8_t processor_type;
  bool pentium_mode;
  uint16_t frame_count_max;
  uint8_t fast_play;
  uint8_t game_speed;
  bool play_demo;
  bool record_demo;
  bool xmas;
  bool display_score;
  bool rich_mode;
};

// The cheats and debug modes.
struct Cheats {
  bool you_are_cheating;
  bool super_wild;
  bool wild;
  uint8_t do_iced;
  bool infinite_shot;
  bool constant_play;
  bool constant_die;
  int8_t constant_last_x;
  bool super_pause;
  bool global_flags[10];
};

// Where the game is: the episode, the level, and the items and data cubes found.
struct Progress {
  uint8_t episode_num;
  uint8_t initial_episode_num;
  bool episode_avail[EPISODE_MAX];
  uint8_t lvl_file_num;
  char level_name[11];
  char last_level_name[11];
  bool load_level_ok;
  uint16_t cube_list[4];
  uint16_t cube_max;
  uint16_t last_cube_max;
  uint8_t item_avail[9][10];
  uint8_t item_avail_max[9];
  uint16_t editor_level;
  uint8_t editor_item_avail[100];
  uint8_t secret_hint;
  bool use_last_bank;
  bool trent_win;
  bool jump_back_to_episode1;
  bool extra_game;
  bool game_loaded;
  bool game_just_loaded;
  bool game_has_repeated;
  bool do_not_save_backup;
};

struct Random {
  MtState mt;  // the random number generator that the game logic draws from
};

struct Sound {
  uint8_t queue[8];  // the samples to play at the end of the tick, one per channel
  bool music_fade;
  uint16_t temp_volume;  // the music's volume while it fades
  uint8_t level_song;
};

struct Display {
  // The screen filter, and its fades.
  int8_t filter, filter_new, brightness, brightness_chg;
  bool filtration_avail, filter_active, filter_fade, filter_fade_start;
  bool explosion_transparent;

  // The smoothies, effects on parts of the screen.
  bool smoothies[9];
  uint8_t smoothie_data[9];
  bool any_smoothies;

  // Flashes, warnings and texts.
  uint8_t flash;
  int8_t flash_change;
  uint8_t warning_col;
  int8_t warning_col_change;
  bool warning_red;
  uint8_t warning_sound_delay;
  bool level_warning_display;
  uint8_t level_warning_lines;
  char level_warning_text[10][61];
  uint16_t text_erase;
  uint8_t display_time;
  uint16_t neat;
  uint8_t star_show_vga_special_code;
  bool star_active;

  boss_bar_t boss_bars[2];
  bool port_config_change, port_config_done;
};

struct Map {
  // Where each of the three map layers is, and how far it moved in the tick.
  uint16_t back_pos, back_pos2, back_pos3;
  uint16_t back_move, back_move2, back_move3;
  uint16_t map_x, map_y, map_x2, map_x3, map_y2, map_y3;
  uint8_t **map_y_pos, **map_y2_pos, **map_y3_pos;
  uint16_t map_x_pos, old_map_x_ofs, map_x_ofs, map_x2_ofs, map_x2_pos, map_x3_pos, old_map_x3_ofs,
      map_x3_ofs, temp_map_x_ofs;
  intptr_t map_x_bp_pos, map_x2_bp_pos, map_x3_bp_pos;
  uint8_t map1_y_delay, map1_y_delay_max, map2_y_delay, map2_y_delay_max;
  uint16_t temp_back_move;
  uint8_t **bk_wrap1, **bk_wrap2, **bk_wrap3, **bk_wrap1_to, **bk_wrap2_to, **bk_wrap3_to;

  // Which layers there are, how they are drawn, and whether they scroll.
  bool stop_backgrounds;
  uint8_t stop_background_num;
  bool background2, background2_not_transparent, background3x1, background3x1b;
  uint8_t background2_over, background3_over;
  bool sky_enemy_over_all, top_enemy_over;
};

struct Level {
  // The level's events, which some events change, and where the level is in them.
  JE_EventRecType events[EVENT_MAXIMUM];
  uint16_t max_event, event_loc;  // the number of events, and the next one
  uint16_t cur_loc;               // how far the level has scrolled, which events wait for
  uint16_t return_loc;            // where a jump back goes, if return_active
  bool return_active;
  bool force_events;
  bool jump_section;

  // Random enemies.
  bool enemies_active;
  uint16_t enemy_types[40];
  uint16_t enemy_types_max, enemy_frequency;

  bool enemy_continual_damage;
  uint16_t super_enemy_254_jump;
  bool random_explosions;
  bool small_enemy_adjust;
  uint16_t galaga_shot_freq;
  int32_t galaga_life;

  // The level timer.
  bool timer;
  uint16_t timer_countdown, timer_jump_to;

  // Where the map screen goes after the level.
  uint16_t map_origin, map_pnum;
  uint8_t map_planet[5], map_section[5];
  uint8_t new_pl[10];
};

struct Ending {
  bool end_level, really_end_level, wait_to_end_level, player_end_level, ready_to_end_level;
  uint8_t countdown;  // levelEnd: ticks until the level ends
  uint16_t fx_wait;   // ticks until the next of the dead players' explosions
  int8_t warp;        // levelEndWarp
  bool first_game_over, all_players_gone, enemy_still_exploding;
  bool stopped_demo;

  // What comes next.
  bool bonus_level, bonus_level_current, normal_bonus_level_current;
  uint8_t main_level, next_level;
};

struct Input {
  bool button[4];         // fire, left sidekick, right sidekick, fire mode
  bool fire_button_held;  // for the special weapon, which fires once per press
};

struct Players {
  Player slots[2];

  uint8_t shot_repeat[11];                // ticks until each kind of shot can fire again
  uint8_t shot_multi_pos[11];             // the pattern each kind of shot fires next
  unsigned power, last_power, power_add;  // the generator's power, as last drawn, and gained per tick
  uint8_t shield_wait, shield_cost;       // ticks until the shield recharges, and the power it takes
  uint8_t charge_wait, charge_level, charge_max, charge_gr, charge_gr_wait;  // the charge weapon
  uint8_t damage_rate;        // the most armor that a collision takes
  uint16_t armor_ship_delay;  // ticks until the low-armor ship shows again

  // Two-player linked mode, and the sidekicks that attach to the ship.
  bool two_player_linked;
  float link_gun_direction;
  float option_satellite_rotate;
  int16_t option_attachment_move;
  bool option_attachment_linked, option_attachment_return;

  // The ships' sprites.
  uint16_t ship_gr, ship_gr2;
  Sprite2_array *ship_gr_ptr, *ship_gr2_ptr;
};

struct Specials {
  uint8_t wait, next_wait;
  uint8_t sf_current_code[2][21];  // the keys of each player's special moves so far
  uint8_t sf_executed[2];
  bool spray;

  // The special weapon.
  uint16_t weapon_wpn;
  int8_t weapon_filter, weapon_freq;
  bool link_to_player;

  // The specials that last a while.
  uint16_t flare_duration;
  bool flare_start;
  int8_t flare_col_chg;
  uint8_t zinglon_duration, astral_duration;
};

struct Enemies {
  JE_SingleEnemyType slots[100];
  uint8_t status[100];     // 1 for a free slot, 0 for an enemy, 2 for one without armor
  uint16_t on_screen;      // how many were updated in the tick
  uint16_t total, killed;  // for the destruction ratio
  uint16_t offset;         // enemyOffset
};

struct PlayerShots {
  PlayerShotDataType slots[MAX_PWEAPON + 1];
  uint8_t ttl[MAX_PWEAPON];  // ticks each shot has left, 0 for a free slot
};

struct EnemyShots {
  EnemyShotType slots[ENEMY_SHOT_MAX];
  bool available[ENEMY_SHOT_MAX];  // whether each slot is free
};

// The C code's scratch global temp2, which most code writes before reading
// it, but which carries a value between functions, and between ticks:
// JE_specialComplete reads it as the special's power when the fire button
// sets the special off, so it gets whatever code last wrote there.  That is
// most often a player shot's blast filter (player_shot_move_and_draw), or 100
// after a destruction, which counts the enemies in it; the sound routine
// leaves it at 8 at the end of the tick.
struct Scratch {
  uint8_t temp2;
};

struct State {
  Game game;
  Cheats cheats;
  Progress progress;
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
  Scratch scratch;
};

}  // namespace tyrx::sim
