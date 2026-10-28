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

// An event of the level: when the level reaches its time, it does what its
// type says, with data whose meaning depends on the type (JE_EventRecType).
struct Event {
  uint16_t time;  // eventtime
  uint8_t  type;  // eventtype
  int16_t  dat;   // eventdat
  int16_t  dat2;  // eventdat2
  int8_t   dat3;  // eventdat3
  uint8_t  dat4;  // eventdat4
  int8_t   dat5;  // eventdat5
  int8_t   dat6;  // eventdat6
};

// An explosion's sprite, for as long as it lasts (Explosion).
struct Explosion {
  int16_t  x;
  int16_t  y;
  int16_t  delta_y;  // how far it moves down each tick (deltaY)
  uint16_t sprite;
  uint8_t  ttl;             // ticks it has left, 0 for a free slot
  bool     fixed_position;  // neither animates nor moves down with the others (fixedPosition)
  bool     follow_player;   // if fixed, moves with the player (followPlayer)
};

// A big explosion that sets off explosions over several ticks
// (rep_explosion_type).
struct RepeatingExplosion {
  uint32_t x;
  uint32_t y;
  uint32_t ttl;    // the explosions it has left to set off
  uint32_t delay;  // ticks until the next
  bool     big;    // sets off large explosions
};

// A spark that moves and fades (superpixel_type).
struct Superpixel {
  uint32_t x;
  uint32_t y;
  int32_t  delta_x;
  int32_t  delta_y;
  uint8_t  color;
  uint32_t z;  // ticks it has left, 0 for a free slot, which it fades with
};

// A star of the starfield (StarfieldStar).
struct Star {
  uint16_t position;  // an offset into the screen, which relies on wrapping around
  int32_t  speed;
  uint8_t  color;
};

// A boss's armor bar (boss_bar_t).
struct BossBar {
  uint8_t link_num;  // the link number of the enemies it follows, 0 for none
  uint8_t armor;
  uint8_t color;  // flashes after a hit
};

// An enemy's shot (EnemyShotType).
struct EnemyShot {
  int16_t  x;           // sx
  int16_t  y;           // sy
  int16_t  x_speed;     // sxm
  int16_t  y_speed;     // sym
  int8_t   x_accel;     // sxc
  int8_t   y_accel;     // syc
  uint8_t  x_track;     // how fast it turns toward player 1, 0 if it doesn't (tx)
  uint8_t  y_track;     // ty
  uint8_t  damage;      // sdmg
  uint8_t  duration;    // ticks it has left
  uint16_t sprite;      // sgr
  uint16_t frame;       // animate
  uint16_t frames;      // animax, 0 if it doesn't animate
  uint8_t  c_fill[12];  // the C struct's padding, which the C code's hash covers (fill)
};

// A player's shot (PlayerShotDataType).
struct PlayerShot {
  int16_t x;        // shotX
  int16_t y;        // shotY
  int16_t x_speed;  // shotXM
  int16_t y_speed;  // shotYM
  int16_t x_accel;  // shotXC
  int16_t y_accel;  // shotYC

  // Shots that circle as they move.
  bool    circling;       // shotComplicated
  int16_t x_circle;       // shotDevX
  int16_t x_circle_dir;   // shotDirX
  int16_t x_circle_size;  // shotCirSizeX
  int16_t y_circle;       // shotDevY
  int16_t y_circle_dir;   // shotDirY
  int16_t y_circle_size;  // shotCirSizeY

  uint8_t  trail;   // the explosions it leaves, 255 for none (shotTrail)
  uint16_t sprite;  // shotGr
  uint16_t frame;   // shotAni
  uint16_t frames;  // shotAniMax

  uint8_t damage;          // shotDmg
  uint8_t blast_filter;    // shotBlastFilter
  uint8_t chain_reaction;  // the weapon of the shots a hit sets off (chainReaction)
  uint8_t player_num;      // playerNumber

  // Guided shots.
  uint8_t aim_at_enemy;   // the enemy's slot + 1, 0 if it isn't guided (aimAtEnemy)
  uint8_t aim_delay;      // aimDelay
  uint8_t aim_delay_max;  // aimDelayMax
};

// An enemy (JE_SingleEnemyType).
struct Enemy {
  uint16_t       type;          // its enemyDat entry (enemytype)
  void          *dat;           // &enemyDat[type], which nothing reads (enemydatofs)
  Sprite2_array *sprite_sheet;  // sprite2s
  uint8_t        size;          // 1 for a 2x2 enemy
  bool           ground;        // enemyground
  uint8_t        explosion;     // the explosions it's destroyed with (explonum)
  int16_t        value;         // the cash it's worth, or the pickup it is (evalue)
  bool           score_item;    // stays on screen at the sides (scoreitem)
  uint16_t       dies_into;     // the enemy it leaves when it's destroyed (enemydie)

  int16_t  x;             // ex
  int16_t  y;             // ey
  uint16_t map_offset;    // the map's x offset when it was last updated (mapoffset)
  int16_t  fixed_move_y;  // fixedmovey
  int8_t   x_speed;       // exc
  int8_t   y_speed;       // eyc

  // Homing in on player 1: how fast, from 90 up, 0 if it doesn't.
  uint8_t x_homing;      // xaccel
  uint8_t y_homing;      // yaccel
  int8_t  x_homing_dir;  // set but never read (exca)
  int8_t  y_homing_dir;  // eyca

  // Fixed acceleration: every accel_wait_max ticks, the speed changes by
  // accel_step, and the acceleration reverses when the speed reaches
  // reverse_speed.
  int8_t  x_accel;           // excc
  int8_t  y_accel;           // eycc
  int8_t  x_accel_wait;      // exccw
  int8_t  y_accel_wait;      // eyccw
  uint8_t x_accel_wait_max;  // exccwmax
  uint8_t y_accel_wait_max;  // eyccwmax
  int16_t x_accel_step;      // exccadd
  int16_t y_accel_step;      // eyccadd
  int8_t  x_reverse_speed;   // exrev
  int8_t  y_reverse_speed;   // eyrev

  // Where its speed reverses.
  int16_t x_min_bounce;  // xminbounce
  int16_t x_max_bounce;  // xmaxbounce
  int16_t y_min_bounce;  // yminbounce
  int16_t y_max_bounce;  // ymaxbounce

  // Animation: the frames are 1-based indexes into sprites, and a sprite of
  // 999 makes the enemy go.
  uint16_t sprites[20];        // egr
  uint8_t  frame;              // enemycycle
  uint8_t  first_frame;        // animin
  uint8_t  last_frame;         // ani
  uint8_t  animation;          // 0 still, 1 animating, 2 animates when it fires (aniactive)
  uint8_t  stop_frame;         // where animating stops, 0 for nowhere (animax)
  uint8_t  animation_at_stop;  // aniwhenfire
  uint8_t  filter;             // the sprite's colour filter, after a hit or while iced

  uint8_t armor;     // 255 for one that can't be destroyed (armorleft)
  uint8_t link_num;  // linked enemies are damaged together, and boss bars follow them (linknum)
  uint8_t iced;      // ticks it stays frozen
  // Damage: at damaged_armor, it changes to its damaged animation (frames
  // damaged_sprite to |damaged_frames|), or to damaged_sprite, or goes.
  bool     damaged;         // edamaged
  int8_t   damaged_armor;   // edlevel
  uint16_t damaged_sprite;  // edgr
  int8_t   damaged_frames;  // edani

  // A global flag that it sets when it's destroyed.
  bool    sets_flag;   // special
  uint8_t flag_num;    // flagnum
  bool    flag_value;  // setto

  // Its turrets: their weapons, how often they fire, ticks until they do, and
  // which of the weapon's shot positions each fires from next.
  uint8_t turrets[3];    // tur
  uint8_t shot_freq[3];  // freq
  uint8_t shot_wait[3];  // eshotwait
  uint8_t shot_pos[3];   // eshotmultipos

  // Launching enemies.
  uint16_t launch_type;     // launchtype
  uint8_t  launch_freq;     // launchfreq
  uint8_t  launch_wait;     // launchwait
  uint8_t  launch_special;  // launchspecial

  // The C struct's filler, which the C code's hash covers.
  uint8_t c_fillbyte;  // fillbyte
  uint8_t c_fill1;     // fill1
  uint8_t c_fill[3];   // fill
};

struct Explosions {
  Explosion slots[MAX_EXPLOSIONS];  // explosions
  uint16_t  move;                   // how far explosions that aren't fixed move down each tick (explodeMove)

  // The big explosions that set off explosions over several ticks.
  RepeatingExplosion repeating[MAX_REPEATING_EXPLOSIONS];  // rep_explosions

  // How far the player moved, for explosions that follow the player.
  int16_t follow_x;  // explosionFollowAmountX
  int16_t follow_y;  // explosionFollowAmountY
};

struct Superpixels {
  Superpixel slots[MAX_SUPERPIXELS];  // superpixels
  uint32_t   last;                    // the slot of the last one created (last_superpixel)
};

struct Starfield {
  Star    stars[MAX_STARS];  // their positions are offsets into the screen (starfield_stars)
  int32_t speed;             // how much faster than their own speed all stars move (starfield_speed)
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
  MtState mt;  // the random number generator that the game logic draws from (mt_global)
};

struct Sound {
  uint8_t  queue[8];  // the samples to play at the end of the tick, one per channel (soundQueue)
  uint8_t  level_song;
  bool     music_fade;
  uint16_t temp_volume;  // the music's volume while it fades
};

struct Display {
  // The screen filter, and its fades.
  int8_t filter;          // levelFilter
  int8_t filter_new;      // levelFilterNew
  int8_t brightness;      // levelBrightness
  int8_t brightness_chg;  // levelBrightnessChg
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
  BossBar boss_bars[2];  // boss_bar
  bool    port_config_change;
  bool    port_config_done;
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
  intptr_t  map_x_bp_pos;  // mapXbpPos
  uint8_t **bk_wrap1;      // BKwrap1
  uint8_t **bk_wrap1_to;   // BKwrap1to

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
  intptr_t  map_x2_bp_pos;  // mapX2bpPos
  uint8_t **bk_wrap2;       // BKwrap2
  uint8_t **bk_wrap2_to;    // BKwrap2to

  // Layer 3.
  uint8_t **map_y3_pos;
  uint16_t  map_y3;
  uint16_t  back_pos3;
  uint16_t  back_move3;
  uint16_t  map_x3;
  uint16_t  map_x3_pos;
  uint16_t  map_x3_ofs;
  uint16_t  old_map_x3_ofs;
  intptr_t  map_x3_bp_pos;  // mapX3bpPos
  uint8_t **bk_wrap3;       // BKwrap3
  uint8_t **bk_wrap3_to;    // BKwrap3to

  // The layer that JE_drawEnemy last updated enemies on.
  uint16_t temp_map_x_ofs;
  uint16_t temp_back_move;

  // Which layers there are, how they are drawn, and whether they scroll.
  bool    background2;
  bool    background2_not_transparent;  // background2notTransparent
  uint8_t background2_over;             // background2over
  bool    background3x1;
  bool    background3x1b;
  uint8_t background3_over;  // background3over
  bool    sky_enemy_over_all;
  bool    top_enemy_over;
  bool    stop_backgrounds;
  uint8_t stop_background_num;
};

struct Level {
  // The level's events, which some events change, and where the level is in them.
  Event    events[EVENT_MAXIMUM];  // eventRec
  uint16_t max_event;              // the number of events
  uint16_t event_loc;              // the next one
  uint16_t cur_loc;                // how far the level has scrolled, which events wait for
  uint16_t return_loc;             // where a jump back goes, if return_active
  bool     return_active;
  bool     force_events;
  bool     jump_section;
  bool     global_flags[10];  // which destroyed enemies set and events test

  // Random enemies.
  bool     enemies_active;
  uint16_t enemy_types[40];  // levelEnemy
  uint16_t enemy_types_max;  // levelEnemyMax
  uint16_t enemy_frequency;  // levelEnemyFrequency

  // What the events set for the enemies.
  bool     enemy_continual_damage;
  uint16_t super_enemy_254_jump;  // superEnemy254Jump
  bool     small_enemy_adjust;
  bool     random_explosions;

  // Galaga mode.
  uint16_t galaga_shot_freq;
  int32_t  galaga_life;

  // The level timer.
  bool     timer;            // levelTimer
  uint16_t timer_countdown;  // levelTimerCountdown
  uint16_t timer_jump_to;    // levelTimerJumpTo

  // Where the map screen goes after the level.
  uint16_t map_origin;
  uint16_t map_pnum;  // mapPNum
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
  uint16_t fx_wait;    // ticks until the next of the dead players' explosions (levelEndFxWait)
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
  Player slots[2];  // player

  // The weapons' shots.
  uint8_t shot_repeat[11];     // ticks until each kind of shot can fire again
  uint8_t shot_multi_pos[11];  // the pattern each kind of shot fires next

  // The generator's power, and the shield.
  uint32_t power;
  uint32_t last_power;   // as last drawn
  uint32_t power_add;    // gained per tick
  uint8_t  shield_wait;  // ticks until the shield recharges
  uint8_t  shield_cost;  // the power it takes (shieldT)

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
  float   link_gun_direction;  // linkGunDirec
  float   option_satellite_rotate;
  int16_t option_attachment_move;
  bool    option_attachment_linked;
  bool    option_attachment_return;

  // The ships' sprites.
  uint16_t       ship_gr;
  uint16_t       ship_gr2;
  Sprite2_array *ship_gr_ptr;
  Sprite2_array *ship_gr2_ptr;  // shipGr2ptr
};

struct Specials {
  // The special moves.
  uint8_t sf_current_code[2][21];  // the keys of each player's special moves so far
  uint8_t sf_executed[2];
  uint8_t wait;       // specialWait
  uint8_t next_wait;  // nextSpecialWait

  // The special weapon.
  uint16_t weapon_wpn;     // specialWeaponWpn
  int8_t   weapon_filter;  // specialWeaponFilter
  int8_t   weapon_freq;    // specialWeaponFreq
  bool     link_to_player;
  bool     spray;  // spraySpecial

  // The specials that last a while.
  uint16_t flare_duration;
  bool     flare_start;
  int8_t   flare_col_chg;
  uint8_t  zinglon_duration;
  uint8_t  astral_duration;
};

struct Enemies {
  Enemy    slots[100];   // enemy
  uint8_t  status[100];  // 1 for a free slot, 0 for an enemy, 2 for one without armor (enemyAvail)
  uint16_t on_screen;    // how many were updated in the tick (enemyOnScreen)
  uint16_t total;        // for the destruction ratio (totalEnemy)
  uint16_t killed;       // enemyKilled
  uint16_t offset;       // enemyOffset
};

struct PlayerShots {
  PlayerShot slots[MAX_PWEAPON + 1];  // playerShotData
  uint8_t    ttl[MAX_PWEAPON];        // ticks each shot has left, 0 for a free slot (shotAvail)

  // The last hit on an enemy (hit_enemy).
  uint8_t do_iced;
  bool    infinite_shot;
};

struct EnemyShots {
  EnemyShot slots[ENEMY_SHOT_MAX];      // enemyShot
  bool      available[ENEMY_SHOT_MAX];  // whether each slot is free (enemyShotAvail)
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
