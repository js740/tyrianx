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

// The parts of the level tick ported from the C code, as they are: they work
// on the C globals, with the C code's names.  Those that draw are split in
// two: their simulation (X_tick), which runs where the C code runs it and
// records what the drawing needs in the tick's draw record, and their drawing
// (X_draw), which draw_tick runs after the simulation is complete.
//
// The state of the subsystems moved into sim::State is in the State that the
// code running the tick provides (state()) instead of the C globals.  Code
// moves from here to sim/ as the globals it uses move into structures.

#pragma once

#include "../gold/api.hpp"
#include "../sim/sim.hpp"
#include "../sim/state.hpp"
#include "draw_record.hpp"

#include <vector>

namespace tyrx::port {

// The State the tick works on, set by the code running the tick (the shadow
// tick, check/shadow.cpp).
inline sim::State *& state_target() {
  static sim::State *state = nullptr;
  return state;
}

inline sim::State& state() { return *state_target(); }

// The C globals whose state sim::State holds.  These names hide them here, so
// that code in port/ can't use them by mistake.
struct InSimState {};
inline constexpr InSimState explosions, rep_explosions, explodeMove, explosionFollowAmountX,
    explosionFollowAmountY;

// background.cpp
void background_1_tick(SDL_Surface *screen, bool clr);  // draw_background_1, or JE_clr256
void background_1_draw(Background1Drawn const& d);
void background_2_tick(SDL_Surface *screen, bool blend,
                       std::optional<Background2Drawn>& drawn);  // draw_background_2, _blend
void background_2_draw(Background2Drawn const& d);
void background_3_tick(SDL_Surface *screen, std::optional<Background3Drawn>& drawn);  // draw_background_3
void background_3_draw(Background3Drawn const& d);
void starfield_tick(SDL_Surface *screen, int move_speed);  // update_and_draw_starfield
void starfield_draw(SDL_Surface *screen);

// effects.cpp
void superpixels_tick();  // JE_drawSP
void superpixels_draw(SuperpixelsDrawn const& d);
void filter_screen_tick();  // JE_filterScreen(levelFilter, levelBrightness)
void filter_screen_draw(FilterDrawn const& d);
void do_sp(JE_word x, JE_word y, JE_word num, JE_byte explowidth, JE_byte color);  // JE_doSP (superpixels)

// JE_setupExplosion, in sim/explosion.cpp
inline void setup_explosion(JE_integer x, JE_integer y, JE_integer deltaY, JE_integer type,
                            bool fixedPosition, bool followPlayer) {
  sim::setup_explosion(state().explosions.slots, x, y, deltaY, type, fixedPosition, followPlayer);
}

// JE_setupExplosionLarge, in sim/explosion.cpp
inline void setup_explosion_large(JE_boolean enemyGround, JE_byte exploNum, JE_integer x, JE_integer y) {
  sim::setup_explosion_large(state().explosions, enemyGround, exploNum, x, y);
}

// enemy.cpp
void draw_enemy_tick(int enemyOffset, std::optional<EnemiesDrawn>& drawn);  // JE_drawEnemy
void enemies_draw(EnemiesDrawn const& d);
Sint16 new_enemy(int enemyOffset, Uint16 eDatI, Sint16 uniqueShapeTableI);                  // JE_newEnemy
uint make_enemy(struct JE_SingleEnemyType *enemy, Uint16 eDatI, Sint16 uniqueShapeTableI);  // JE_makeEnemy

// events.cpp
void event_system();  // JE_eventSystem
void create_new_event_enemy(JE_byte enemyTypeOfs, JE_word enemyOffset,
                            Sint16 uniqueShapeTableI);  // JE_createNewEventEnemy
void event_jump(JE_word jump);                          // JE_eventJump
bool search_for(JE_byte PLType, JE_byte *out_index);    // JE_searchFor

// hud.cpp
void draw_shield_tick();                       // JE_drawShield
void draw_armor_tick();                        // JE_drawArmor
void wipe_shield_armor_bars_tick();            // JE_wipeShieldArmorBars
void draw_port_config_buttons_tick();          // JE_drawPortConfigButtons
void draw_text_window_tick(char const *text);  // JE_drawTextWindow
void draw_options_tick();                      // JE_drawOptions
void draw_option_level_tick();                 // JE_drawOptionLevel
void message_bar_erased_tick();
void weapon_power_tick(int x, int y, uint item_power);
void power_bar_tick(JE_byte temp, uint lastPower);
void charge_level_tick(JE_byte color);
void sidekick_ammo_tick(int y, int ammo_max, int ammo, bool discharged);
void hud_draw(hud::Draw const& draw);
void boss_bar_tick();  // draw_boss_bar
void boss_bar_draw(BossBarsDrawn const& d);
void in_game_displays_draw(SDL_Surface *screen);  // JE_inGameDisplays

// keyboard.cpp
void main_keyboard_input();  // JE_mainKeyboardInput

// player.cpp
void main_game_player_functions();  // JE_mainGamePlayerFunctions
void player_draw(PlayerDrawn const& d);
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
// player_shot_move_and_draw, adding the shot to `drawn` if drawn
bool player_shot_move(int shot_id, bool *out_is_special, int *out_shotx, int *out_shoty,
                      JE_integer *out_shot_damage, JE_byte *out_blast_filter, JE_byte *out_chain,
                      JE_byte *out_playerNum, JE_word *out_special_radiusw, JE_word *out_special_radiush,
                      std::vector<PlayerShotDrawn>& drawn);
void player_shots_draw(PlayerShotsDrawn const& d);
// shots.c, called as port::, as for player.c
JE_integer player_shot_create(JE_word portNum, uint bay_i, JE_word PX, JE_word PY, JE_word mouseX,
                              JE_word mouseY, JE_word wpNum, JE_byte playerNum);
void player_shot_set_direction(JE_integer shot_id, uint weapon_id, JE_real direction);

}  // namespace tyrx::port
