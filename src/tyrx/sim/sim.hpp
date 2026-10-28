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

// C++ ports of the game logic whose state is encapsulated: each function
// takes the state it works on instead of using the C globals.  Code moves
// here from port/ as the globals it uses move into structures.
//
// Each function must match the C code it ports exactly.  The shadow tick
// checks that for the code the level tick runs, and check/diff.cpp checks the
// functions it wraps call by call (TYRX_DIFF).

#pragma once

#include "../gold/api.hpp"
#include "hidden.hpp"
#include "hud.hpp"
#include "state.hpp"

#include <array>
#include <bitset>
#include <optional>
#include <span>
#include <vector>

namespace tyrx::sim {

// The C globals that the State holds are hidden here (hidden.hpp).
using namespace hidden;

// Random numbers: ports draw from an MtState with the reentrant C functions
// (mt_rand_r etc. in mtrand.h) rather than a C++ copy of the generator.

// varz.c
void setup_explosion(std::span<Explosion, MAX_EXPLOSIONS> explosions, JE_integer x, JE_integer y,
                     JE_integer delta_y, JE_integer type, bool fixed_position, bool follow_player);

// varz.c: JE_setupExplosionLarge
void setup_explosion_large(Explosions& explosions, bool enemy_ground, JE_byte explo_num, JE_integer x,
                           JE_integer y);

// tyrian2.c: the repeating explosions' update in JE_main's level loop
// ("Sequenced Explosions"), which sets off their explosions, with their
// sounds in the sound queue.  Returns whether any is still exploding.
bool update_repeating_explosions(Explosions& explosions, MtState& mt, JE_integer back_move,
                                 std::span<JE_byte, 8> sound_queue);

// The explosions visible in a tick, for its drawing: the slots of those still
// on screen after the update moved them, and the last position and frame of
// those whose time ran out, as a new explosion may take their slots later in
// the tick.
struct EndedExplosion {
  int slot;
  JE_integer x, y;
  JE_word sprite;
};
struct VisibleExplosions {
  std::bitset<MAX_EXPLOSIONS> slots;
  std::vector<EndedExplosion> ended;
};

// tyrian2.c: the explosions' update in JE_main's level loop ("Draw
// Explosions"), which moves them and ends those whose time ran out or that
// left the screen.
VisibleExplosions update_explosions(Explosions& explosions);

// varz.c: JE_doSP, which creates num superpixels around (x, y)
void do_sp(Superpixels& superpixels, MtState& mt, JE_word x, JE_word y, JE_word num, JE_byte explowidth,
           JE_byte color);

// varz.c: the superpixels' update in JE_drawSP, which moves and fades them.
// Returns the slots of those it updated, which the drawing draws.
std::bitset<MAX_SUPERPIXELS> update_superpixels(Superpixels& superpixels);

// tyrian2.c: JE_makeEnemy, which makes enemy one of type e_dat_i (enemyDat),
// with the sprites of shape table unique_shape_table_i if that is over 0.
// Returns the slot's status (Enemies::status) for the enemy.
JE_byte make_enemy(State& state, JE_SingleEnemyType& enemy, JE_word e_dat_i, Sint16 unique_shape_table_i);

// tyrian2.c: JE_newEnemy, which makes an enemy (make_enemy) in the first free
// slot of the 25 from first.  Returns the slot + 1, or 0 if none was free.
int new_enemy(State& state, int first, JE_word e_dat_i, Sint16 unique_shape_table_i);

// An enemy as it was drawn, before it moved.
struct VisibleEnemy {
  JE_integer x, y;
  JE_word sprite;  // egr[enemycycle - 1]
  JE_byte filter, size;
  Sprite2_array *sprite2s;
};

// tyrian2.c: JE_drawEnemy, which "actually does a whole lot more than just
// drawing": the update of the 25 enemies before slot end, on a layer that has
// scrolled map_x_ofs sideways and moved back_move down in the tick (kept in
// the Map, as tempMapXOfs and tempBackMove).  Each is animated, moves, fires
// and launches enemies.  Returns those on screen, as they were drawn.
std::vector<VisibleEnemy> update_enemies(State& state, int end, JE_word map_x_ofs, JE_word back_move);

// tyrian2.c: a player shot's hit on the enemy in slot b, in JE_main's level
// loop ("Player Shot Images"), with the shot's damage and blast filter.  A
// damage of 99 ices the enemy, and 250 or more passes through enemies
// (Cheats::infinite_shot); the hit takes 99 to 0.  It damages or destroys
// the enemy and the enemies linked to it, and a destruction leaves the
// filter at 100.  The Zinglon's shot neither explodes nor makes sparks.
// Returns the enemy's armor before the hit.
int hit_enemy(State& state, int b, JE_integer& damage, JE_byte& filter, bool zinglon, int shot_x, int shot_y,
              JE_byte player_num);

// tyrian2.c: JE_eventJump, which jumps to the level position jump, or back
// to where the last jump came from if jump is 65535, and to the first event
// at or after it.
void event_jump(Level& level, JE_word jump);

// The HUD's gauges and messages (hud.cpp), which log them in the HUD's log.
void hud_shield(State const& state, HudLog& hud);             // JE_drawShield
void hud_armor(State& state, HudLog& hud);                    // JE_drawArmor, which limits the armor to 28
void hud_wipe_shield_armor(State const& state, HudLog& hud);  // JE_wipeShieldArmorBars
void hud_port_config(State const& state, HudLog& hud);        // JE_drawPortConfigButtons
void hud_text_window(State& state, HudLog& hud,
                     char const *text);                  // JE_drawTextWindow, which sets textErase
void hud_options(State& state, HudLog& hud);             // JE_drawOptions, which sets up the sidekicks
void hud_option_level(State const& state, HudLog& hud);  // JE_drawOptionLevel
void hud_charge_level(State const& state, HudLog& hud, JE_byte color);
void hud_sidekick_ammo(HudLog& hud, int y, int ammo_max, int ammo, bool discharged);

// A player as JE_playerMovement drew it.  Each part says whether the C code
// drew it on game_screen, as the HUD's drawing had left VGAScreen there
// (HudLog::game_screen).
struct VisiblePlayer {
  JE_word ship_gr;
  Sprite2_array *ship_gr_ptr;
  struct Trail {  // at the end of a level
    bool game_screen;
    int x;
    std::vector<int> trail_y;
  };
  std::optional<Trail> trail;
  struct InsertCoin {  // in demos
    bool game_screen;
  };
  std::optional<InsertCoin> insert_coin;
  struct Ship {
    bool game_screen;
    int x, y;
    int sprite;
    JE_word map_x2_ofs;
    bool background2, super_wild, invulnerable;
  };
  std::optional<Ship> ship;
  struct SpecialIcon {
    bool game_screen;
    bool ready;
  };
  std::optional<SpecialIcon> special_icon;
  struct Zinglon {
    bool game_screen;
    int x;
    JE_byte width;
  };
  std::optional<Zinglon> zinglon;
  struct Charge {
    bool game_screen;
    int x, y;
    int sprite;
  };
  std::optional<Charge> charge;
  struct Sidekick {
    bool game_screen;
    int x, y;
    uint sprite;
    uint style;
  };
  std::array<std::optional<Sidekick>, 2> sidekicks;
};

// tyrian2.c: the players' part of JE_main's level loop: their collisions
// (JE_playerCollide), then their movement, firing, specials and sidekicks
// (JE_mainGamePlayerFunctions and JE_playerMovement, in mainint.c), and the
// map's sideways scrolling, which follows them.  Returns the players drawn.
std::vector<VisiblePlayer> update_players(State& state, HudLog& hud);

// varz.c: JE_getShipInfo, which sets up the players' ships: their sprites,
// armor, hit areas and generator.
void get_ship_info(State& state);

// varz.c: JE_portConfigs, the number of modes of the rear weapon.
JE_word port_configs(State const& state);

// varz.c: JE_doSpecialShot, the specials of player player_num, whose armor
// and shield pay for them: those set off by the special-move codes or the
// fire button, and the flares and the Zinglon while they last.
void do_special_shot(State& state, HudLog& hud, JE_byte player_num, uint& armor, uint& shield,
                     VisiblePlayer& visible);

// mainint.c: JE_SFCodes, which follows player player_num's input (the
// movement from (mouse_x, mouse_y) to (px, py), and the fire button) through
// the ship's special-move codes.
void sf_codes(State& state, JE_byte player_num, JE_integer px, JE_integer py, JE_integer mouse_x,
              JE_integer mouse_y);

// varz.c: JE_playerDamage, which damages the player's shield, and its armor
// if the damage gets through, and draws them in the HUD (which limits both
// players' armor to 28).  Returns the damage if it got through the shield.
JE_byte damage_player(State& state, HudLog& hud, Player& player, JE_byte damage);

// tyrian2.c: the enemy shots' update in JE_main's level loop ("Draw Enemy
// Shots"), which moves them and ends those that time out, leave the screen or
// hit a player.  Returns those drawn, which stay in their slots for the rest
// of the tick.
std::bitset<ENEMY_SHOT_MAX> update_enemy_shots(State& state, HudLog& hud);

// A player shot as it was drawn.
struct VisiblePlayerShot {
  int x, y;
  JE_word sprite_frame;  // over 60000 for special shots
  bool shadow;
};

// tyrian2.c: the player shots' update in JE_main's level loop ("Player Shot
// Images"): each moves (player_shot_move_and_draw, in shots.c), then hits
// the enemies it collides with (hit_enemy), or sets off a chain reaction.
// Returns the shots drawn.
std::vector<VisiblePlayerShot> update_player_shots(State& state);

// shots.c: player_shot_create, which fires player player_num's weapon wp_num
// from port port_num's bay bay_i at (px, py), if there is power for it.
// Returns the slot of the last shot made, or MAX_PWEAPON if one couldn't be.
JE_integer create_player_shot(State& state, JE_word port_num, uint bay_i, JE_word px, JE_word py,
                              JE_word mouse_x, JE_word mouse_y, JE_word wp_num, JE_byte player_num);

// shots.c: player_shot_set_direction, which turns a linked weapon's shot.
void set_player_shot_direction(PlayerShotDataType& shot, uint weapon_id, JE_real direction);

// backgrnd.c: the stars' update in update_and_draw_starfield, which moves
// them down a screen whose rows are pitch bytes apart.
void update_starfield(Starfield& starfield, int pitch);

}  // namespace tyrx::sim
