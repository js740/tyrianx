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

// The game logic of the level tick: each function takes the state it works
// on (state.hpp) instead of using the C globals, and leaves what it draws
// for the tick's drawing (visible.hpp).
//
// Each function must match the C code it ports exactly.

#pragma once

#include "../c/api.hpp"
#include "hidden.hpp"
#include "hud.hpp"
#include "state.hpp"
#include "visible.hpp"

#include <bitset>
#include <span>
#include <vector>

namespace tyrx::sim {

// The C globals that the State holds are hidden here (hidden.hpp).
using namespace hidden;

// Random numbers: the game logic draws from an MtState with the reentrant C
// functions (mt_rand_r etc. in mtrand.h) rather than a C++ copy of the
// generator.

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
uint8_t make_enemy(State& state, Enemy& enemy, uint16_t e_dat_i, int16_t unique_shape_table_i);

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
void set_player_shot_direction(PlayerShot& shot, unsigned weapon_id, float direction);

// backgrnd.c: the stars' update in update_and_draw_starfield, which moves
// them down a screen whose rows are pitch bytes apart.
void update_starfield(Starfield& starfield, int pitch);

}  // namespace tyrx::sim
