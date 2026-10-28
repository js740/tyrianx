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
#include "draw_record.hpp"
#include "hidden.hpp"
#include "hud.hpp"
#include "state.hpp"
#include "visible.hpp"

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
void setup_explosion(std::span<Explosion, MAX_EXPLOSIONS> explosions, int16_t x, int16_t y, int16_t delta_y,
                     int16_t type, bool fixed_position, bool follow_player);

// varz.c: JE_setupExplosionLarge
void setup_explosion_large(Explosions& explosions, bool enemy_ground, uint8_t explo_num, int16_t x,
                           int16_t y);

// tyrian2.c: the repeating explosions' update in JE_main's level loop
// ("Sequenced Explosions"), which sets off their explosions, with their
// sounds in the sound queue.  Returns whether any is still exploding.
bool update_repeating_explosions(Explosions& explosions, MtState& mt, int16_t back_move,
                                 std::span<uint8_t, 8> sound_queue);

// tyrian2.c: the explosions' update in JE_main's level loop ("Draw
// Explosions"), which moves them and ends those whose time ran out or that
// left the screen.
VisibleExplosions update_explosions(Explosions& explosions);

// varz.c: JE_doSP, which creates num superpixels around (x, y)
void do_sp(Superpixels& superpixels, MtState& mt, uint16_t x, uint16_t y, uint16_t num, uint8_t explowidth,
           uint8_t color);

// varz.c: the superpixels' update in JE_drawSP, which moves and fades them.
// Returns the slots of those it updated, which the drawing draws.
std::bitset<MAX_SUPERPIXELS> update_superpixels(Superpixels& superpixels);

// tyrian2.c: JE_makeEnemy, which makes enemy one of type e_dat_i (enemyDat),
// with the sprites of shape table unique_shape_table_i if that is over 0.
// Returns the slot's status (Enemies::status) for the enemy.
uint8_t make_enemy(State& state, JE_SingleEnemyType& enemy, uint16_t e_dat_i, int16_t unique_shape_table_i);

// tyrian2.c: JE_newEnemy, which makes an enemy (make_enemy) in the first free
// slot of the 25 from first.  Returns the slot + 1, or 0 if none was free.
int new_enemy(State& state, int first, uint16_t e_dat_i, int16_t unique_shape_table_i);

// tyrian2.c: JE_drawEnemy, which "actually does a whole lot more than just
// drawing": the update of the 25 enemies before slot end, on a layer that has
// scrolled map_x_ofs sideways and moved back_move down in the tick (kept in
// the Map, as tempMapXOfs and tempBackMove).  Each is animated, moves, fires
// and launches enemies.  Returns those on screen, as they were drawn.
std::vector<VisibleEnemy> update_enemies(State& state, int end, uint16_t map_x_ofs, uint16_t back_move);

// tyrian2.c: a player shot's hit on the enemy in slot b, in JE_main's level
// loop ("Player Shot Images"), with the shot's damage and blast filter.  A
// damage of 99 ices the enemy, and 250 or more passes through enemies
// (Cheats::infinite_shot); the hit takes 99 to 0.  It damages or destroys
// the enemy and the enemies linked to it, and a destruction leaves the
// filter at 100.  The Zinglon's shot neither explodes nor makes sparks.
// Returns the enemy's armor before the hit.
int hit_enemy(State& state, int b, int16_t& damage, uint8_t& filter, bool zinglon, int shot_x, int shot_y,
              uint8_t player_num);

// How a level tick ended: like the C code, back to level_loop for the next
// tick, or to start_level to end the level; or not at all, for network games,
// which aren't supported.
enum class LevelPass { level_loop, start_level, unsupported };

// State the C code keeps in JE_main's local variables, which outlive a tick.
struct LevelLocals {
  unsigned old_weapon_bar[2] = {0, 0};  // weapon power bars are only redrawn when they change
};

// What the level tick needs from the code running it: the C code's loops
// with their own drawing and input (the pause screen, the in-game menu and
// help), which work on the C globals and run in the middle of the tick, a
// notice just before the key commands open the help, and the surfaces' pitch,
// which the stars' positions depend on.
struct Host {
  virtual void in_game_help_ahead() = 0;
  virtual void c_loop(void (*loop)()) = 0;
  virtual int  pitch(Surface surface) = 0;

 protected:
  ~Host() = default;
};

// level.cpp, tyrian2.c: one tick of JE_main's level loop, from the level_loop
// label to the jump back to it, which records the tick's drawing in record
// (which starts empty).  Sets frame if the tick ends with a frame
// (JE_starShowVGA): it doesn't when the in-game menu or help opened.
LevelPass level_tick(State& state, LevelLocals& locals, DrawRecord& record, Host& host, bool& frame);

// background.cpp, backgrnd.c: the map's backgrounds in the level loop.
// background_1 records layer 1 (or JE_clr256 if clr), which the level loop
// scrolls; background_2 and background_3 record their layers and scroll them,
// as draw_background_2 and draw_background_3 do.  background_2 is blended if
// blend.
void background_1(State const& state, DrawRecord& record, bool clr);
void background_2(State& state, DrawRecord& record, bool blend, std::optional<Background2Drawn>& drawn);
void background_3(State& state, DrawRecord& record, std::optional<Background3Drawn>& drawn);

// tyrian2.c: JE_checkSmoothies, whether any smoothies need VGAScreen2.
void check_smoothies(State& state);

// keyboard.cpp, mainint.c: JE_mainKeyboardInput, the key commands.
void main_keyboard_input(State& state, DrawRecord& record, Host& host);

// tyrian2.c: the events' part of JE_main's level loop: runs the events that
// the level has reached (JE_eventSystem).
void update_events(State& state, HudLog& hud);

// tyrian2.c: JE_eventJump, which jumps to the level position jump, or back
// to where the last jump came from if jump is 65535, and to the first event
// at or after it.
void event_jump(Level& level, uint16_t jump);

// The HUD's gauges and messages (hud.cpp), which log them in the HUD's log.
void hud_shield(State const& state, HudLog& hud);             // JE_drawShield
void hud_armor(State& state, HudLog& hud);                    // JE_drawArmor, which limits the armor to 28
void hud_wipe_shield_armor(State const& state, HudLog& hud);  // JE_wipeShieldArmorBars
void hud_port_config(State const& state, HudLog& hud);        // JE_drawPortConfigButtons
void hud_text_window(State& state, HudLog& hud,
                     char const *text);                  // JE_drawTextWindow, which sets textErase
void hud_options(State& state, HudLog& hud);             // JE_drawOptions, which sets up the sidekicks
void hud_option_level(State const& state, HudLog& hud);  // JE_drawOptionLevel
void hud_charge_level(State const& state, HudLog& hud, uint8_t color);
void hud_sidekick_ammo(HudLog& hud, int y, int ammo_max, int ammo, bool discharged);

// tyrian2.c: the players' part of JE_main's level loop: their collisions
// (JE_playerCollide), then their movement, firing, specials and sidekicks
// (JE_mainGamePlayerFunctions and JE_playerMovement, in mainint.c), and the
// map's sideways scrolling, which follows them.  Returns the players drawn.
std::vector<VisiblePlayer> update_players(State& state, DrawRecord& record);

// varz.c: JE_getShipInfo, which sets up the players' ships: their sprites,
// armor, hit areas and generator.
void get_ship_info(State& state);

// varz.c: JE_portConfigs, the number of modes of the rear weapon.
uint16_t port_configs(State const& state);

// varz.c: JE_doSpecialShot, the specials of player player_num, whose armor
// and shield pay for them: those set off by the special-move codes or the
// fire button, and the flares and the Zinglon while they last.
void do_special_shot(State& state, DrawRecord& record, uint8_t player_num, unsigned& armor, unsigned& shield,
                     VisiblePlayer& visible);

// mainint.c: JE_SFCodes, which follows player player_num's input (the
// movement from (mouse_x, mouse_y) to (px, py), and the fire button) through
// the ship's special-move codes.
void sf_codes(State& state, uint8_t player_num, int16_t px, int16_t py, int16_t mouse_x, int16_t mouse_y);

// varz.c: JE_playerDamage, which damages the player's shield, and its armor
// if the damage gets through, and draws them in the HUD (which limits both
// players' armor to 28).  The C code draws them with VGAScreen set to the
// HUD's surface, and leaves it at game_screen after (its "side-effect of
// game_screen"), where the rest of the tick then draws.  Returns the damage if
// it got through the shield.
uint8_t damage_player(State& state, DrawRecord& record, Player& player, uint8_t damage);

// tyrian2.c: the enemy shots' update in JE_main's level loop ("Draw Enemy
// Shots"), which moves them and ends those that time out, leave the screen or
// hit a player.  Returns those drawn, which stay in their slots for the rest
// of the tick.
std::bitset<ENEMY_SHOT_MAX> update_enemy_shots(State& state, DrawRecord& record);

// tyrian2.c: the player shots' update in JE_main's level loop ("Player Shot
// Images"): each moves (player_shot_move_and_draw, in shots.c), then hits
// the enemies it collides with (hit_enemy), or sets off a chain reaction.
// Returns the shots drawn.
std::vector<VisiblePlayerShot> update_player_shots(State& state);

// shots.c: player_shot_create, which fires player player_num's weapon wp_num
// from port port_num's bay bay_i at (px, py), if there is power for it.
// Returns the slot of the last shot made, or MAX_PWEAPON if one couldn't be.
int16_t create_player_shot(State& state, uint16_t port_num, unsigned bay_i, uint16_t px, uint16_t py,
                           uint16_t mouse_x, uint16_t mouse_y, uint16_t wp_num, uint8_t player_num);

// shots.c: player_shot_set_direction, which turns a linked weapon's shot.
void set_player_shot_direction(PlayerShotDataType& shot, unsigned weapon_id, float direction);

// backgrnd.c: the stars' update in update_and_draw_starfield, which moves
// them down a screen whose rows are pitch bytes apart.
void update_starfield(Starfield& starfield, int pitch);

}  // namespace tyrx::sim
