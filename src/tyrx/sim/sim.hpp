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
#include "state.hpp"

#include <array>
#include <bitset>
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

// What JE_playerDamage did: the damage that got through the shield, and the
// shields and armor that it drew in the HUD (JE_wipeShieldArmorBars,
// JE_drawShield and JE_drawArmor).
struct PlayerDamage {
  JE_byte through;
  uint armor_before_limit;            // player 1's, as JE_wipeShieldArmorBars drew it
  std::array<uint, 2> shield, armor;  // both players', the armor after JE_drawArmor limited it to 28
};

// varz.c: JE_playerDamage, which damages the player's shield, and its armor
// if the damage gets through, and limits both players' armor to 28 (in
// JE_drawArmor).
PlayerDamage damage_player(State& state, Player& player, JE_byte damage);

// The enemy shots visible in a tick, and the damage they did to the players.
struct EnemyShotsUpdate {
  std::bitset<ENEMY_SHOT_MAX> visible;  // which stay in their slots for the rest of the tick
  std::vector<PlayerDamage> damage;     // in order, for the HUD
};

// tyrian2.c: the enemy shots' update in JE_main's level loop ("Draw Enemy
// Shots"), which moves them and ends those that time out, leave the screen or
// hit a player.
EnemyShotsUpdate update_enemy_shots(State& state);

// backgrnd.c: the stars' update in update_and_draw_starfield, which moves
// them down a screen whose rows are pitch bytes apart.
void update_starfield(Starfield& starfield, int pitch);

}  // namespace tyrx::sim
