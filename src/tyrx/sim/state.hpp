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
  Explosion          slots[MAX_EXPLOSIONS];
  rep_explosion_type repeating[MAX_REPEATING_EXPLOSIONS];  // which set off explosions over ticks
  uint16_t           move;      // how far explosions that aren't fixed move down each tick
  int16_t            follow_x;  // how far the player moved, for explosions that follow the player
  int16_t            follow_y;
};

struct Superpixels {
  superpixel_type slots[MAX_SUPERPIXELS];
  uint32_t        last;  // the slot of the last one created
};

struct Starfield {
  StarfieldStar stars[MAX_STARS];  // their positions are offsets into the screen
  int32_t       speed;             // how much faster than their own speed all stars move
};

// The game's modes and settings.
struct Game {
  // The players and the mode.
  bool    two_player_mode;
  bool    one_player_action;
  bool    galaga_mode;
  bool    super_tyrian;
  uint8_t super_arcade_mode;
  uint8_t super_arcade_power_up;

  // The difficulty.
  int8_t difficulty_level;
  int8_t initial_difficulty;
  int8_t old_difficulty_level;

  // The speed, and the detail.
  uint8_t  fast_play;
  uint8_t  game_speed;
  uint16_t frame_count_max;
  bool     pentium_mode;
  uint8_t  processor_type;

  // Demos.
  bool play_demo;
  bool record_demo;

  // Options.
  bool display_score;
  bool rich_mode;
  bool xmas;
};

// The cheats and debug modes.
struct Cheats {
  bool   you_are_cheating;
  bool   wild;
  bool   super_wild;
  bool   super_pause;
  bool   constant_play;
  bool   constant_die;
  int8_t constant_last_x;
};

// Where the game is: the episode, the level, and the items and data cubes found.
struct Progress {
  // The episode, and the level.
  uint8_t episode_num;
  uint8_t initial_episode_num;
  bool    episode_avail[EPISODE_MAX];
  bool    jump_back_to_episode1;
  bool    extra_game;
  bool    trent_win;
  uint8_t lvl_file_num;
  char    level_name[11];
  char    last_level_name[11];
  bool    load_level_ok;

  // The items and data cubes found.
  uint8_t  item_avail[9][10];
  uint8_t  item_avail_max[9];
  uint16_t cube_list[4];
  uint16_t cube_max;
  uint16_t last_cube_max;
  uint8_t  secret_hint;

  // The level editor's.
  uint16_t editor_level;
  uint8_t  editor_item_avail[100];

  // Saved games.
  bool game_loaded;
  bool game_just_loaded;
  bool game_has_repeated;  // only the first play-through gets a high score
  bool do_not_save_backup;
};

struct Random {
  MtState mt;  // the random number generator that the game logic draws from
};

struct Sound {
  uint8_t  queue[8];  // the samples to play at the end of the tick, one per channel
  uint8_t  level_song;
  bool     music_fade;
  uint16_t temp_volume;  // the music's volume while it fades
};

struct Display {
  // The screen filter, and its fades.
  int8_t filter;
  int8_t filter_new;
  int8_t brightness;
  int8_t brightness_chg;
  bool   filtration_avail;
  bool   filter_active;
  bool   filter_fade;
  bool   filter_fade_start;
  bool   explosion_transparent;

  // The smoothies, effects on parts of the screen, and how the playing area
  // is shown.
  bool     smoothies[9];
  uint8_t  smoothie_data[9];
  bool     any_smoothies;
  uint8_t  star_show_vga_special_code;
  uint16_t neat;
  bool     star_active;

  // The low armor warning.
  uint8_t warning_col;
  int8_t  warning_col_change;
  bool    warning_red;
  uint8_t warning_sound_delay;

  // The level's warning.
  bool    level_warning_display;
  uint8_t level_warning_lines;
  char    level_warning_text[10][61];

  // Messages.
  uint16_t text_erase;
  bool     use_last_bank;
  uint8_t  display_time;
  uint8_t  flash;
  int8_t   flash_change;

  // The boss bars, and the port config buttons.
  boss_bar_t boss_bars[2];
  bool       port_config_change;
  bool       port_config_done;
};

// The map's three layers, and how they move.
struct Map {
  // Layer 1.
  uint8_t **map_y_pos;
  uint16_t  map_y;
  uint16_t  back_pos;
  uint16_t  back_move;
  uint8_t   map1_y_delay;
  uint8_t   map1_y_delay_max;
  uint16_t  map_x;
  uint16_t  map_x_pos;
  uint16_t  map_x_ofs;
  uint16_t  old_map_x_ofs;
  intptr_t  map_x_bp_pos;
  uint8_t **bk_wrap1;
  uint8_t **bk_wrap1_to;

  // Layer 2.
  uint8_t **map_y2_pos;
  uint16_t  map_y2;
  uint16_t  back_pos2;
  uint16_t  back_move2;
  uint8_t   map2_y_delay;
  uint8_t   map2_y_delay_max;
  uint16_t  map_x2;
  uint16_t  map_x2_pos;
  uint16_t  map_x2_ofs;
  intptr_t  map_x2_bp_pos;
  uint8_t **bk_wrap2;
  uint8_t **bk_wrap2_to;

  // Layer 3.
  uint8_t **map_y3_pos;
  uint16_t  map_y3;
  uint16_t  back_pos3;
  uint16_t  back_move3;
  uint16_t  map_x3;
  uint16_t  map_x3_pos;
  uint16_t  map_x3_ofs;
  uint16_t  old_map_x3_ofs;
  intptr_t  map_x3_bp_pos;
  uint8_t **bk_wrap3;
  uint8_t **bk_wrap3_to;

  // The layer that JE_drawEnemy last updated enemies on.
  uint16_t temp_map_x_ofs;
  uint16_t temp_back_move;

  // Which layers there are, how they are drawn, and whether they scroll.
  bool    background2;
  bool    background2_not_transparent;
  uint8_t background2_over;
  bool    background3x1;
  bool    background3x1b;
  uint8_t background3_over;
  bool    sky_enemy_over_all;
  bool    top_enemy_over;
  bool    stop_backgrounds;
  uint8_t stop_background_num;
};

struct Level {
  // The level's events, which some events change, and where the level is in them.
  JE_EventRecType events[EVENT_MAXIMUM];
  uint16_t        max_event;   // the number of events
  uint16_t        event_loc;   // the next one
  uint16_t        cur_loc;     // how far the level has scrolled, which events wait for
  uint16_t        return_loc;  // where a jump back goes, if return_active
  bool            return_active;
  bool            force_events;
  bool            jump_section;
  bool            global_flags[10];  // which destroyed enemies set and events test

  // Random enemies.
  bool     enemies_active;
  uint16_t enemy_types[40];
  uint16_t enemy_types_max;
  uint16_t enemy_frequency;

  // What the events set for the enemies.
  bool     enemy_continual_damage;
  uint16_t super_enemy_254_jump;
  bool     small_enemy_adjust;
  bool     random_explosions;

  // Galaga mode.
  uint16_t galaga_shot_freq;
  int32_t  galaga_life;

  // The level timer.
  bool     timer;
  uint16_t timer_countdown;
  uint16_t timer_jump_to;

  // Where the map screen goes after the level.
  uint16_t map_origin;
  uint16_t map_pnum;
  uint8_t  map_planet[5];
  uint8_t  map_section[5];
  uint8_t  new_pl[10];
};

struct Ending {
  // The level's end.
  bool     end_level;
  bool     ready_to_end_level;
  bool     wait_to_end_level;
  bool     player_end_level;
  bool     really_end_level;
  uint8_t  countdown;  // levelEnd: ticks until the level ends
  int8_t   warp;       // levelEndWarp
  uint16_t fx_wait;    // ticks until the next of the dead players' explosions
  bool     enemy_still_exploding;

  // Game over.
  bool first_game_over;
  bool all_players_gone;
  bool stopped_demo;

  // What comes next.
  bool    bonus_level;
  bool    bonus_level_current;
  bool    normal_bonus_level_current;
  uint8_t main_level;
  uint8_t next_level;
};

struct Input {
  bool button[4];         // fire, left sidekick, right sidekick, fire mode
  bool fire_button_held;  // for the special weapon, which fires once per press
};

struct Players {
  Player slots[2];

  // The weapons' shots.
  uint8_t shot_repeat[11];     // ticks until each kind of shot can fire again
  uint8_t shot_multi_pos[11];  // the pattern each kind of shot fires next

  // The generator's power, and the shield.
  uint32_t power;
  uint32_t last_power;   // as last drawn
  uint32_t power_add;    // gained per tick
  uint8_t  shield_wait;  // ticks until the shield recharges
  uint8_t  shield_cost;  // the power it takes

  // The charge weapon.
  uint8_t charge_wait;
  uint8_t charge_level;
  uint8_t charge_max;
  uint8_t charge_gr;
  uint8_t charge_gr_wait;

  // Damage.
  uint8_t  damage_rate;       // the most armor that a collision takes
  uint16_t armor_ship_delay;  // ticks until the low-armor ship shows again

  // Two-player linked mode, and the sidekicks that attach to the ship.
  bool    two_player_linked;
  float   link_gun_direction;
  float   option_satellite_rotate;
  int16_t option_attachment_move;
  bool    option_attachment_linked;
  bool    option_attachment_return;

  // The ships' sprites.
  uint16_t       ship_gr;
  uint16_t       ship_gr2;
  Sprite2_array *ship_gr_ptr;
  Sprite2_array *ship_gr2_ptr;
};

struct Specials {
  // The special moves.
  uint8_t sf_current_code[2][21];  // the keys of each player's special moves so far
  uint8_t sf_executed[2];
  uint8_t wait;
  uint8_t next_wait;

  // The special weapon.
  uint16_t weapon_wpn;
  int8_t   weapon_filter;
  int8_t   weapon_freq;
  bool     link_to_player;
  bool     spray;

  // The specials that last a while.
  uint16_t flare_duration;
  bool     flare_start;
  int8_t   flare_col_chg;
  uint8_t  zinglon_duration;
  uint8_t  astral_duration;
};

struct Enemies {
  JE_SingleEnemyType slots[100];
  uint8_t            status[100];  // 1 for a free slot, 0 for an enemy, 2 for one without armor
  uint16_t           on_screen;    // how many were updated in the tick
  uint16_t           total;        // for the destruction ratio
  uint16_t           killed;
  uint16_t           offset;  // enemyOffset
};

struct PlayerShots {
  PlayerShotDataType slots[MAX_PWEAPON + 1];
  uint8_t            ttl[MAX_PWEAPON];  // ticks each shot has left, 0 for a free slot

  // The last hit on an enemy (hit_enemy).
  uint8_t do_iced;
  bool    infinite_shot;
};

struct EnemyShots {
  EnemyShotType slots[ENEMY_SHOT_MAX];
  bool          available[ENEMY_SHOT_MAX];  // whether each slot is free
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
  Game        game;
  Cheats      cheats;
  Progress    progress;
  Random      random;
  Explosions  explosions;
  Superpixels superpixels;
  Starfield   starfield;
  Sound       sound;
  Display     display;
  Map         map;
  Level       level;
  Ending      ending;
  Input       input;
  Players     players;
  Specials    specials;
  Enemies     enemies;
  PlayerShots player_shots;
  EnemyShots  enemy_shots;
  Scratch     scratch;
};

}  // namespace tyrx::sim
