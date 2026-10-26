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

// The parts of the level tick ported from the C code, as they are: they work on
// the C globals, with the C code's names.  Those that draw add their calls to
// the tick's draw list (draw.hpp) where the C code draws.
//
// Code moves from here to sim/ as the globals it uses move into structures.

#pragma once

#include "../gold/api.hpp"
#include "../sim/sim.hpp"

namespace tyrx::port {

// background.cpp
void background_2(SDL_Surface *screen, bool blend);   // draw_background_2, _blend
void background_3(SDL_Surface *screen);               // draw_background_3
void starfield(SDL_Surface *screen, int move_speed);  // update_and_draw_starfield

// effects.cpp
void draw_sp();                                               // JE_drawSP
void filter_screen(JE_shortint col, JE_shortint brightness);  // JE_filterScreen
void setup_explosion_large(JE_boolean enemyGround, JE_byte exploNum, JE_integer x,
                           JE_integer y);                                          // JE_setupExplosionLarge
void do_sp(JE_word x, JE_word y, JE_word num, JE_byte explowidth, JE_byte color);  // JE_doSP (superpixels)

// JE_setupExplosion, in sim/explosion.cpp
inline void setup_explosion(JE_integer x, JE_integer y, JE_integer deltaY, JE_integer type,
                            bool fixedPosition, bool followPlayer) {
  sim::setup_explosion(explosions, x, y, deltaY, type, fixedPosition, followPlayer);
}

// enemy.cpp
void draw_enemy(int enemyOffset);                                                           // JE_drawEnemy
Sint16 new_enemy(int enemyOffset, Uint16 eDatI, Sint16 uniqueShapeTableI);                  // JE_newEnemy
uint make_enemy(struct JE_SingleEnemyType *enemy, Uint16 eDatI, Sint16 uniqueShapeTableI);  // JE_makeEnemy

// events.cpp
void event_system();  // JE_eventSystem
void create_new_event_enemy(JE_byte enemyTypeOfs, JE_word enemyOffset,
                            Sint16 uniqueShapeTableI);  // JE_createNewEventEnemy
void event_jump(JE_word jump);                          // JE_eventJump
bool search_for(JE_byte PLType, JE_byte *out_index);    // JE_searchFor

// hud.cpp
void draw_shield();                          // JE_drawShield
void draw_armor();                           // JE_drawArmor
void wipe_shield_armor_bars();               // JE_wipeShieldArmorBars
void draw_port_config_buttons();             // JE_drawPortConfigButtons
void draw_text_window(char const *text);     // JE_drawTextWindow
void draw_options();                         // JE_drawOptions
void draw_option_level();                    // JE_drawOptionLevel
void draw_boss_bars();                       // draw_boss_bar
void in_game_displays(SDL_Surface *screen);  // JE_inGameDisplays

// keyboard.cpp
void main_keyboard_input();  // JE_mainKeyboardInput

// player.cpp
void main_game_player_functions();                             // JE_mainGamePlayerFunctions
void player_collide(Player *this_player, JE_byte playerNum_);  // JE_playerCollide
JE_byte player_damage(JE_byte temp, Player *this_player);      // JE_playerDamage
void sf_codes(JE_byte playerNum_, JE_integer PX_, JE_integer PY_, JE_integer mouseX_,
              JE_integer mouseY_);                // JE_SFCodes
JE_word s_gr(JE_word ship, Sprite2_array **ptr);  // JE_SGr
void get_ship_info();                             // JE_getShipInfo
JE_word port_configs();                           // JE_portConfigs
// player.c, called as port:: (argument-dependent lookup also finds the C
// functions of the same names)
void calc_purple_balls_needed(Player *this_player);
bool power_up_weapon(Player *this_player, uint port);
void handle_got_purple_ball(Player *this_player);

// shots.cpp
// shots.c, called as port::, as for player.c
bool player_shot_move_and_draw(int shot_id, bool *out_is_special, int *out_shotx, int *out_shoty,
                               JE_integer *out_shot_damage, JE_byte *out_blast_filter, JE_byte *out_chain,
                               JE_byte *out_playerNum, JE_word *out_special_radiusw,
                               JE_word *out_special_radiush);
JE_integer player_shot_create(JE_word portNum, uint bay_i, JE_word PX, JE_word PY, JE_word mouseX,
                              JE_word mouseY, JE_word wpNum, JE_byte playerNum);
void player_shot_set_direction(JE_integer shot_id, uint weapon_id, JE_real direction);

}  // namespace tyrx::port
