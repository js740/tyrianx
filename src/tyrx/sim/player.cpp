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
// The players: ports of JE_mainGamePlayerFunctions, JE_playerMovement and
// JE_playerCollide (mainint.c), JE_playerDamage, JE_getShipInfo, JE_SGr and
// JE_portConfigs (varz.c), and the power-ups of player.c, with the players'
// part of JE_main's level loop (tyrian2.c).  Network games aren't supported,
// so the network code is left out.
//
// The players' movement reads the raw input from the C globals (keyboard,
// mouse, joystick) and the demo file, as the C code does, in one place
// (Movement::read_input).

#include "sim.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace tyrx::sim {

namespace {

bool all_players_dead(State const& state) {
  auto const& players = state.players.slots;
  return !players[0].is_alive && (!state.game.two_player_mode || !players[1].is_alive);
}

// playDemoKeys (demo.c), which reads the demo's input, whose state is static
// there, and moves player 1 and sets the buttons in the C globals: it gets the
// State's player 1's position there, and the State gets the result.
bool play_demo_keys(State& state) {
  auto& players = state.players.slots;
  auto& button = state.input.button;
  ::player[0].x = players[0].x;
  ::player[0].y = players[0].y;
  std::ranges::copy(button, ::button);
  bool const more = playDemoKeys();
  players[0].x = ::player[0].x;
  players[0].y = ::player[0].y;
  std::ranges::copy(::button, std::begin(button));
  return more;
}

// JE_SGr: the ship graphic of edited ship ship, whose sprites may be in
// extraShapes.
uint16_t s_gr(uint16_t ship, Sprite2_array *& ptr) {
  static uint16_t const GR[15] = {233, 157, 195, 271, 81,  0,   119, 5,
                                  43,  81,  119, 157, 195, 233, 271};  // [1..15]

  uint16_t const graphic = extraShips[(ship - 1) * 15];
  if (graphic > 7) {
    ptr = &extraShapes;
  }

  return GR[graphic - 1];
}

// player.c's power-ups.
void calc_purple_balls_needed(Player& player) {
  static unsigned const purple_balls_required[12] = {1, 1, 2, 4, 8, 12, 16, 20, 25, 30, 40, 50};

  player.purple_balls_needed = purple_balls_required[player.lives()];
}

bool power_up_weapon(State& state, Player& player, unsigned port) {
  bool const can_power_up = player.items.weapon[port].id != 0 &&   // not None
                            player.items.weapon[port].power < 11;  // not at max power
  if (can_power_up) {
    ++player.items.weapon[port].power;
    state.players.shot_multi_pos[port] = 0;  // TODO: should be part of Player structure

    calc_purple_balls_needed(player);
  }
  // cash consolation prize
  else {
    player.cash += 1000;
  }

  return can_power_up;
}

void handle_got_purple_ball(State& state, Player& player) {
  if (player.purple_balls_needed > 1) {
    --player.purple_balls_needed;
  }
  else {
    power_up_weapon(state, player, player.is_dragonwing ? REAR_WEAPON : FRONT_WEAPON);
  }
}

// One player's part of JE_playerMovement: their death or respawn, input,
// movement, firing and sidekicks, in the C code's order.  The player's
// mouseX and mouseY are where they were before they moved.
struct Movement {
  State&         state;
  DrawRecord&    record;
  Player&        player;
  uint8_t        input_device;
  uint8_t        num;  // 1 or 2
  uint16_t       ship_gr;
  VisiblePlayer& visible;

  // What the input asks for: the mouse's or the analog joystick's movement,
  // and the acceleration it gives.
  int16_t mouse_xc = 0, mouse_yc = 0;
  int16_t accel_xc = 0, accel_yc = 0;
  bool    link_gun_analog = false;
  float   link_gun_angle = 0;

  void run();
  bool update_dead();
  void read_input();
  void link();
  void fade_out();
  void move();
  void follow_linked();
  void place();
  void fire();
  void fire_sidekicks();
  void place_sidekicks();
  void animate_sidekicks();
};

void Movement::run() {
  auto& ending = state.ending;

  if (num == 2 || !state.game.two_player_mode) {
    if (player.weapon_mode > weaponPort[player.items.weapon[REAR_WEAPON].id].opnum) {
      player.weapon_mode = 1;
    }
  }

  // The C code starts again (goto redo) after a respawn.
  do {
    mouse_xc = 0;
    mouse_yc = 0;
    accel_xc = 0;
    accel_yc = 0;

    link_gun_analog = false;
    link_gun_angle = 0;
  } while (update_dead());

  if (!player.is_alive) {
    state.explosions.follow_x = state.explosions.follow_y = 0;
    return;
  }

  if (!ending.end_level) {
    read_input();

    moveOk = true;

    // Street-Fighter codes
    sf_codes(state, num, player.x, player.y, player.mouse_x, player.mouse_y);

    if (moveOk) {
      link();
    }
  }

  if (ending.countdown > 0 && all_players_dead(state)) {
    ending.really_end_level = true;
  }

  fade_out();

  if (state.game.play_demo) {
    visible.insert_coin = VisiblePlayer::InsertCoin{record.screen};
  }

  if (player.is_alive && !ending.end_level) {
    if (!state.players.two_player_linked || num < 2) {
      move();
    }
    else {
      follow_linked();
    }
  }

  if (!ending.end_level) {
    place();
  }

  if (moveOk && player.is_alive && !ending.end_level) {
    fire();
  }

  if (!((num == 2 || !state.game.two_player_mode) && !ending.end_level)) {
    return;
  }

  animate_sidekicks();
}

// A player who isn't alive explodes, then respawns if there are lives left,
// in arcade games; with constantDie, one who is dies again.  Returns whether
// the player respawned.
bool Movement::update_dead() {
  auto&    game = state.game;
  auto&    ending = state.ending;
  auto&    sound_queue = state.sound.queue;
  MtState& mt = state.random.mt;

  if (player.is_alive) {
    if (!state.cheats.constant_die || player.exploding_ticks != 0) {
      return false;
    }

    // finished exploding?  start dying again
    player.shield = 0;

    if (player.armor > 0) {
      --player.armor;
    }
    else {
      player.is_alive = false;
      player.exploding_ticks = 60;
      ending.countdown = 40;
    }

    hud_wipe_shield_armor(state, record.hud);
    hud_armor(state, record.hud);
    record.screen = Surface::game_screen;  // side-effect of game_screen

    // as if instant death weren't enough, player also gets infinite lives in order to enjoy an infinite
    // number of deaths -_-
    Player& player1 = state.players.slots[0];
    if (player1.lives() < 11) {
      ++player1.lives();
    }
    return false;
  }

  if (player.exploding_ticks > 0) {
    --player.exploding_ticks;

    if (ending.fx_wait > 0) {
      ending.fx_wait--;
    }
    else {
      ending.fx_wait = (mt_rand_r(&mt) % 6) + 3;
      if ((mt_rand_r(&mt) % 3) == 1) {
        sound_queue[6] = S_EXPLOSION_9;
      }
      else {
        sound_queue[5] = S_EXPLOSION_11;
      }
    }

    int explosion_x = player.x + (mt_rand_r(&mt) % 32) - 16;
    int explosion_y = player.y + (mt_rand_r(&mt) % 32) - 16;
    setup_explosion_large(state.explosions, false, 0, explosion_x, explosion_y + 7);
    setup_explosion_large(state.explosions, false, 0, player.x, player.y + 7);

    if (ending.countdown > 0) {
      ending.countdown--;
    }
    return false;
  }

  // if arcade mode
  if (!(game.two_player_mode || game.one_player_action)) {
    return false;
  }

  // respawn if any extra lives
  if (player.lives() <= 1) {
    if (game.galaga_mode) {
      game.two_player_mode = false;
    }
    return false;
  }

  --player.lives();

  ending.really_end_level = false;
  state.players.shot_multi_pos[num - 1] = 0;
  calc_purple_balls_needed(player);
  state.players.two_player_linked = false;
  if (game.galaga_mode) {
    game.two_player_mode = false;
  }
  player.y = 160;
  player.invulnerable_ticks = 100;
  player.is_alive = true;
  ending.end_level = false;

  if (game.galaga_mode || state.progress.episode_num == 4) {
    player.armor = player.initial_armor;
  }
  else {
    player.armor = player.initial_armor / 2;
  }

  if (game.galaga_mode) {
    player.shield = 0;
  }
  else {
    player.shield = player.shield_max / 2;
  }

  hud_armor(state, record.hud);
  hud_shield(state, record.hud);
  record.screen = Surface::game_screen;  // side-effect of game_screen
  return true;
}

// The input: the demo's, the joysticks', the mouse's and the keyboard's, from
// the C globals.  Keys and joystick directions move the player; the mouse and
// analog joysticks ask for a movement (mouse_xc, mouse_yc), and both
// accelerate the player.
void Movement::read_input() {
  auto& game = state.game;
  auto& button = state.input.button;

  player.mouse_x = player.x;
  player.mouse_y = player.y;
  button[1 - 1] = false;
  button[2 - 1] = false;
  button[3 - 1] = false;
  button[4 - 1] = false;

  // --- Movement Routine Beginning ---

  if (game.play_demo || game.record_demo) {
    input_device = 1;  // keyboard is required device for demo recording
  }

  // demo playback input
  if (game.play_demo && !play_demo_keys(state)) {
    state.ending.end_level = true;
    state.ending.countdown = 40;
  }

  // joystick input
  if ((input_device == 0 || input_device >= 3) && joysticks > 0) {
    int j = input_device == 0 ? 0 : input_device - 3;
    int j_max = input_device == 0 ? joysticks : input_device - 3 + 1;
    for (; j < j_max; j++) {
      poll_joystick(j);

      if (joystick[j].analog) {
        mouse_xc += joystick_axis_reduce(j, joystick[j].x);
        mouse_yc += joystick_axis_reduce(j, joystick[j].y);

        link_gun_analog = joystick_analog_angle(j, &link_gun_angle);
      }
      else {
        player.x += (joystick[j].direction[3] ? -CURRENT_KEY_SPEED : 0) +
                    (joystick[j].direction[1] ? CURRENT_KEY_SPEED : 0);
        player.y += (joystick[j].direction[0] ? -CURRENT_KEY_SPEED : 0) +
                    (joystick[j].direction[2] ? CURRENT_KEY_SPEED : 0);
      }

      button[0] |= joystick[j].action[0];
      button[1] |= joystick[j].action[2];
      button[2] |= joystick[j].action[3];
      button[3] |= joystick[j].action_pressed[1];

      ingamemenu_pressed |= joystick[j].action_pressed[4];
      pause_pressed |= joystick[j].action_pressed[5];
    }
  }

  // mouse input
  if ((input_device == 0 || input_device == 2) && has_mouse) {
    button[0] |= (mouseButtonsDown & SDL_BUTTON_LMASK) != 0;
    button[1] |= (mouseButtonsDown & SDL_BUTTON_RMASK) != 0;
    button[2] |= (mouseButtonsDown & (mouse_has_three_buttons ? SDL_BUTTON_MMASK : SDL_BUTTON_RMASK)) != 0;

    int32_t mouse_xr;
    int32_t mouse_yr;
    mouseGetRelativePosition(&mouse_xr, &mouse_yr);
    mouse_xc += mouse_xr;
    mouse_yc += mouse_yr;
  }

  // keyboard input
  if ((input_device == 0 || input_device == 1) && !game.play_demo) {
    if (keysactive[keySettings[KEY_SETTING_UP]]) {
      player.y -= CURRENT_KEY_SPEED;
    }
    if (keysactive[keySettings[KEY_SETTING_DOWN]]) {
      player.y += CURRENT_KEY_SPEED;
    }

    if (keysactive[keySettings[KEY_SETTING_LEFT]]) {
      player.x -= CURRENT_KEY_SPEED;
    }
    if (keysactive[keySettings[KEY_SETTING_RIGHT]]) {
      player.x += CURRENT_KEY_SPEED;
    }

    button[0] |= keysactive[keySettings[KEY_SETTING_FIRE]];
    button[3] |= keysactive[keySettings[KEY_SETTING_CHANGE_FIRE]];
    button[1] |= keysactive[keySettings[KEY_SETTING_LEFT_SIDEKICK]];
    button[2] |= keysactive[keySettings[KEY_SETTING_RIGHT_SIDEKICK]];

    if (state.cheats.constant_play) {
      for (unsigned int i = 0; i < 4; i++) {
        button[i] = true;
      }

      ++player.y;
      player.x += state.cheats.constant_last_x;
    }

    if (game.record_demo) {
      recordDemoKeys();
    }
  }

  if (state.display.smoothies[9 - 1]) {
    player.mouse_y = player.y - (player.mouse_y - player.y);
    mouse_yc = -mouse_yc;
  }

  accel_xc += player.x - player.mouse_x;
  accel_yc += player.y - player.mouse_y;

  mouse_xc = std::clamp<int16_t>(mouse_xc, -30, 30);
  mouse_yc = std::clamp<int16_t>(mouse_yc, -30, 30);

  if (mouse_xc > 0) {
    player.x += (mouse_xc + 3) / 4;
  }
  else if (mouse_xc < 0) {
    player.x += (mouse_xc - 3) / 4;
  }
  if (mouse_yc > 0) {
    player.y += (mouse_yc + 3) / 4;
  }
  else if (mouse_yc < 0) {
    player.y += (mouse_yc - 3) / 4;
  }

  if (mouse_xc > 3) {
    accel_xc++;
  }
  else if (mouse_xc < -2) {
    accel_xc--;
  }
  if (mouse_yc > 2) {
    accel_yc++;
  }
  else if (mouse_yc < -2) {
    accel_yc--;
  }
}

// Two-player linking: the players link when the second meets the first, and
// the first breaks it with a sidekick button; linked, the second aims the
// turret.
void Movement::link() {
  auto&       players = state.players.slots;
  auto const& button = state.input.button;
  bool const  galaga = state.game.galaga_mode;
  bool&       linked = state.players.two_player_linked;
  float&      direction = state.players.link_gun_direction;

  if (state.game.two_player_mode && !linked && player.x == player.mouse_x && player.y == player.mouse_y &&
      abs(players[0].x - players[1].x) < 8 && abs(players[0].y - players[1].y) < 8 && players[0].is_alive &&
      players[1].is_alive && !galaga)
  {
    linked = true;
  }

  if (num == 1 && (button[3 - 1] || button[2 - 1]) && !galaga) {
    linked = false;
  }

  if (!(state.game.two_player_mode && linked && num == 2 &&
        (player.x != player.mouse_x || player.y != player.mouse_y)))
  {
    return;
  }

  if (!button[0]) {
    if (!galaga) {
      linked = false;
    }
    return;
  }

  if (link_gun_analog) {
    direction = link_gun_angle;
  }
  else {
    float target;

    if (abs(player.x - player.mouse_x) > abs(player.y - player.mouse_y)) {
      target = (player.x - player.mouse_x > 0) ? M_PI_2 : (M_PI + M_PI_2);
    }
    else {
      target = (player.y - player.mouse_y > 0) ? 0 : M_PI;
    }

    if (fabsf(direction - target) < 0.3f) {
      direction = target;
    }
    else if (direction < target && direction - target > -3.24f) {
      direction += 0.2f;
    }
    else if (direction - target < M_PI) {
      direction -= 0.2f;
    }
    else {
      direction += 0.2f;
    }
  }

  if (direction >= (2 * M_PI)) {
    direction -= (2 * M_PI);
  }
  else if (direction < 0) {
    direction += (2 * M_PI);
  }
}

// End Level Fade-Out: the ship warps up, leaving a trail.
void Movement::fade_out() {
  auto& ending = state.ending;

  if (!(player.is_alive && ending.end_level)) {
    return;
  }

  if (ending.countdown == 0) {
    ending.really_end_level = true;
    return;
  }

  player.y -= ending.warp;
  if (player.y < -200) {
    ending.really_end_level = true;
  }

  int trail_spacing = 1;
  int trail_y = player.y;
  int num_trails = abs(41 - ending.countdown);
  if (num_trails > 20) {
    num_trails = 20;
  }

  for (int i = 0; i < num_trails; i++) {
    trail_y += trail_spacing;
    trail_spacing++;
  }

  for (int i = 1; i < num_trails; i++) {
    trail_y -= trail_spacing;
    trail_spacing--;

    if (trail_y <= 0 || trail_y >= 170) {
      continue;
    }

    if (!visible.trail) {
      visible.trail = VisiblePlayer::Trail{record.screen, player.x, {}};
    }
    visible.trail->trail_y.push_back(trail_y);
  }
}

// The ship's movement: friction, the acceleration, and the history of its
// positions that trailing sidekicks follow.
void Movement::move() {
  // if not dragonwing
  if (!state.game.two_player_mode || state.players.ship_gr2 != 0) {
    if (player.sidekick[LEFT_SIDEKICK].style == 0) {
      player.sidekick[LEFT_SIDEKICK].x = player.mouse_x - 14;
      player.sidekick[LEFT_SIDEKICK].y = player.mouse_y;
    }

    if (player.sidekick[RIGHT_SIDEKICK].style == 0) {
      player.sidekick[RIGHT_SIDEKICK].x = player.mouse_x + 16;
      player.sidekick[RIGHT_SIDEKICK].y = player.mouse_y;
    }
  }

  if (player.x_friction_ticks > 0) {
    --player.x_friction_ticks;
  }
  else {
    player.x_friction_ticks = 1;

    if (player.x_velocity < 0) {
      ++player.x_velocity;
    }
    else if (player.x_velocity > 0) {
      --player.x_velocity;
    }
  }

  if (player.y_friction_ticks > 0) {
    --player.y_friction_ticks;
  }
  else {
    player.y_friction_ticks = 2;

    if (player.y_velocity < 0) {
      ++player.y_velocity;
    }
    else if (player.y_velocity > 0) {
      --player.y_velocity;
    }
  }

  player.x_velocity += accel_xc;
  player.y_velocity += accel_yc;

  player.x_velocity = std::clamp(player.x_velocity, -4, 4);
  player.y_velocity = std::clamp(player.y_velocity, -4, 4);

  player.x += player.x_velocity;
  player.y += player.y_velocity;

  // if player moved, add new ship x, y history entry
  if (player.x - player.mouse_x == 0 && player.y - player.mouse_y == 0) {
    return;
  }

  size_t const n = std::size(player.old_x);
  for (size_t i = 1; i < n; ++i) {
    player.old_x[i - 1] = player.old_x[i];
    player.old_y[i - 1] = player.old_y[i];
  }
  player.old_x[n - 1] = player.x;
  player.old_y[n - 1] = player.y;
}

// The second player linked to the first: the turret, its direction marker,
// and its shots.
void Movement::follow_linked() {
  auto&       players = state.players.slots;
  auto&       shot_repeat = state.players.shot_repeat;
  auto&       shot_multi_pos = state.players.shot_multi_pos;
  float const direction = state.players.link_gun_direction;

  if (ship_gr == 0) {
    player.x = players[0].x - 1;
  }
  else {
    player.x = players[0].x;
  }
  player.y = players[0].y + 8;

  player.x_velocity = players[0].x_velocity;
  player.y_velocity = 4;

  // turret direction marker/shield
  shot_multi_pos[SHOT_MISC] = 0;
  create_player_shot(state, 0, SHOT_MISC, player.x + 1 + roundf(sinf(direction + 0.2f) * 26),
                     player.y + roundf(cosf(direction + 0.2f) * 26), player.mouse_x, player.mouse_y, 148,
                     num);
  shot_multi_pos[SHOT_MISC] = 0;
  create_player_shot(state, 0, SHOT_MISC, player.x + 1 + roundf(sinf(direction - 0.2f) * 26),
                     player.y + roundf(cosf(direction - 0.2f) * 26), player.mouse_x, player.mouse_y, 148,
                     num);
  shot_multi_pos[SHOT_MISC] = 0;
  create_player_shot(state, 0, SHOT_MISC, player.x + 1 + roundf(sinf(direction) * 26),
                     player.y + roundf(cosf(direction) * 26), player.mouse_x, player.mouse_y, 147, num);

  if (shot_repeat[SHOT_REAR] > 0) {
    --shot_repeat[SHOT_REAR];
    return;
  }

  if (!state.input.button[1 - 1]) {
    return;
  }

  shot_multi_pos[SHOT_REAR] = 0;
  int16_t const b =
      create_player_shot(state, 0, SHOT_REAR, player.x + 1 + roundf(sinf(direction) * 20),
                         player.y + roundf(cosf(direction) * 20), player.mouse_x, player.mouse_y,
                         linkGunWeapons[player.items.weapon[REAR_WEAPON].id - 1], num);
  set_player_shot_direction(state.player_shots.slots[b], player.items.weapon[REAR_WEAPON].id, direction);
}

// The ship's place on the screen and banking, which the explosions that
// follow it, its drawing and its sparks go by.
void Movement::place() {
  auto&   shot_repeat = state.players.shot_repeat;
  int8_t& constant_last_x = state.cheats.constant_last_x;

  if (player.x > 256) {
    player.x = 256;
    constant_last_x = -constant_last_x;
  }
  if (player.x < 40) {
    player.x = 40;
    constant_last_x = -constant_last_x;
  }

  if (player.y > 160) {
    player.y = 160;
  }

  if (player.y < 10) {
    player.y = 10;
  }

  // Determines the ship banking sprite to display, depending on horizontal velocity and acceleration
  int ship_banking = player.x_velocity / 2 + (player.x - player.mouse_x) / 6;
  ship_banking = std::max(-2, std::min(ship_banking, 2));

  int const ship_sprite = ship_banking * 2 + ship_gr;

  Explosions& e = state.explosions;
  e.follow_x = player.x - player.last_x_explosion_follow;
  e.follow_y = player.y - player.last_y_explosion_follow;

  if (e.follow_y < 0) {
    e.follow_y = 0;
  }

  player.last_x_explosion_follow = player.x;
  player.last_y_explosion_follow = player.y;

  visible.ship = VisiblePlayer::Ship{record.screen,
                                     player.x,
                                     player.y,
                                     ship_sprite,
                                     state.map.map_x2_ofs,
                                     (bool)state.map.background2,
                                     (bool)state.cheats.super_wild,
                                     player.invulnerable_ticks > 0};

  if (player.invulnerable_ticks > 0) {
    --player.invulnerable_ticks;
  }
  else if (ship_gr == 1) {
    // NortSparks, from the side the ship banks to
    uint16_t sparks_x = 0;
    int      sparks_banking = 0;
    switch (ship_sprite) {
      case 5:
        sparks_x = player.x - 7;
        sparks_banking = -2;
        break;
      case 3:
        sparks_x = player.x - 7;
        sparks_banking = -1;
        break;
      case -1:
        sparks_x = player.x + 9;
        sparks_banking = 1;
        break;
      case -3:
        sparks_x = player.x + 9;
        sparks_banking = 2;
        break;
    }
    if (sparks_banking != 0) {
      if (shot_repeat[SHOT_NORTSPARKS] > 0) {
        --shot_repeat[SHOT_NORTSPARKS];
      }
      else {
        MtState& mt = state.random.mt;
        create_player_shot(state, 0, SHOT_NORTSPARKS, sparks_x + (mt_rand_r(&mt) % 8) - 4,
                           player.y + (mt_rand_r(&mt) % 8) - 4, player.mouse_x, player.mouse_y, 671, 1);
        shot_repeat[SHOT_NORTSPARKS] = abs(sparks_banking) - 1;
      }
    }
  }

  // Options Location
  // if dragonwing
  if (num == 2 && ship_gr == 0) {
    if (player.sidekick[LEFT_SIDEKICK].style == 0) {
      player.sidekick[LEFT_SIDEKICK].x = player.x - 14 + ship_banking * 2;
      player.sidekick[LEFT_SIDEKICK].y = player.y;
    }

    if (player.sidekick[RIGHT_SIDEKICK].style == 0) {
      player.sidekick[RIGHT_SIDEKICK].x = player.x + 17 + ship_banking * 2;
      player.sidekick[RIGHT_SIDEKICK].y = player.y;
    }
  }
}

// PLAYER SHOT Change and Creation: the rear weapon's mode, the special, the
// weapons, the second player's charge, the superbomb and the sidekicks.
void Movement::fire() {
  auto&       game = state.game;
  auto&       players = state.players;
  auto&       shot_repeat = players.shot_repeat;
  auto&       shot_multi_pos = players.shot_multi_pos;
  auto const& button = state.input.button;
  Display&    display = state.display;

  player.delta_x_shot_move = player.x - player.last_x_shot_move;
  player.delta_y_shot_move = player.y - player.last_y_shot_move;

  // PLAYER SHOT Change
  if (button[4 - 1]) {
    display.port_config_change = true;
    if (display.port_config_done) {
      shot_multi_pos[SHOT_REAR] = 0;

      uint8_t const sa = game.super_arcade_mode;
      if (sa != SA_NONE && sa <= SA_NORTSHIPZ) {
        shot_multi_pos[SHOT_SPECIAL] = 0;
        shot_multi_pos[SHOT_SPECIAL2] = 0;
        if (players.slots[0].items.special == SASpecialWeapon[sa - 1]) {
          players.slots[0].items.special = SASpecialWeaponB[sa - 1];
          player.weapon_mode = 2;
        }
        else {
          players.slots[0].items.special = SASpecialWeapon[sa - 1];
          player.weapon_mode = 1;
        }
      }
      else if (++player.weapon_mode > port_configs(state)) {
        player.weapon_mode = 1;
      }

      hud_port_config(state, record.hud);
      display.port_config_done = false;
    }
  }

  // SpecialShot
  if (!game.galaga_mode) {
    do_special_shot(state, record, num, player.armor, player.shield, visible);
  }

  // Normal Main Weapons
  if (!(players.two_player_linked && num == 2)) {
    int const min = game.two_player_mode ? num : 1;
    int const max = game.two_player_mode ? num : 2;

    for (int i = min - 1; i < max; i++) {
      unsigned const item = player.items.weapon[i].id;

      if (item <= 0) {
        continue;
      }

      if (shot_repeat[i] > 0) {
        --shot_repeat[i];
      }
      else if (button[1 - 1]) {
        unsigned const item_power = game.galaga_mode ? 0 : player.items.weapon[i].power - 1,
                       item_mode = (i == REAR_WEAPON) ? player.weapon_mode - 1 : 0;

        create_player_shot(state, item, i, player.x, player.y, player.mouse_x, player.mouse_y,
                           weaponPort[item].op[item_mode][item_power], num);
      }
    }
  }

  // Super Charge Weapons
  if (num == 2) {
    if (!players.two_player_linked) {
      visible.charge = VisiblePlayer::Charge{record.screen, player.x + (ship_gr == 0) + 1, player.y - 13,
                                             77 + players.charge_level + players.charge_gr * 19};
    }

    if (players.charge_gr_wait > 0) {
      players.charge_gr_wait--;
    }
    else {
      players.charge_gr++;
      if (players.charge_gr == 4) {
        players.charge_gr = 0;
      }
      players.charge_gr_wait = 3;
    }

    if (players.charge_level > 0) {
      hud_charge_level(state, record.hud, 193);
    }

    if (players.charge_wait > 0) {
      players.charge_wait--;
    }
    else {
      if (players.charge_level < players.charge_max) {
        players.charge_level++;
      }

      players.charge_wait = 28 - player.items.weapon[REAR_WEAPON].power * 2;
      if (game.difficulty_level > DIFFICULTY_HARD) {
        players.charge_wait -= 5;
      }
    }

    if (players.charge_level > 0) {
      hud_charge_level(state, record.hud, 204);
    }

    if (shot_repeat[SHOT_P2_CHARGE] > 0) {
      --shot_repeat[SHOT_P2_CHARGE];
    }
    else if (button[1 - 1] && (!players.two_player_linked || players.charge_level > 0)) {
      shot_multi_pos[SHOT_P2_CHARGE] = 0;
      create_player_shot(
          state, 16, SHOT_P2_CHARGE, player.x, player.y, player.mouse_x, player.mouse_y,
          chargeGunWeapons[players.slots[1].items.weapon[REAR_WEAPON].id - 1] + players.charge_level, num);

      if (players.charge_level > 0) {
        hud_charge_level(state, record.hud, 193);
      }

      players.charge_level = 0;
      players.charge_wait = 30 - player.items.weapon[REAR_WEAPON].power * 2;
    }
  }

  // SUPER BOMB
  uint8_t const bomber = num == 0 ? 1 : num;  // Get whether player 1 or 2
  Player&       bomber_player = players.slots[bomber - 1];

  if (bomber_player.superbombs > 0) {
    if (shot_repeat[SHOT_P1_SUPERBOMB + bomber - 1] > 0) {
      --shot_repeat[SHOT_P1_SUPERBOMB + bomber - 1];
    }
    else if (button[3 - 1] || button[2 - 1]) {
      --bomber_player.superbombs;
      shot_multi_pos[SHOT_P1_SUPERBOMB + bomber - 1] = 0;
      create_player_shot(state, 16, SHOT_P1_SUPERBOMB + bomber - 1, player.x, player.y, player.mouse_x,
                         player.mouse_y, 535, num);
    }
  }

  place_sidekicks();

  // if player has sidekicks
  if (num == 2 || !game.two_player_mode) {
    fire_sidekicks();
  }
}

// The sidekicks' places: trailing, front-mounted, orbiting, and the right
// front-mounted one that detaches and comes back.
void Movement::place_sidekicks() {
  auto&        players = state.players;
  auto const&  button = state.input.button;
  auto&        left = player.sidekick[LEFT_SIDEKICK];
  auto&        right = player.sidekick[RIGHT_SIDEKICK];
  size_t const history = std::size(player.old_x);

  if (left.style == 4 && right.style == 4) {
    players.option_satellite_rotate += 0.2f;
  }
  else if (left.style == 4 || right.style == 4) {
    players.option_satellite_rotate += 0.15f;
  }

  float const rotate = players.option_satellite_rotate;

  switch (left.style) {
    case 1:  // trailing
    case 3:
      left.x = player.old_x[history / 2 - 1];
      left.y = player.old_y[history / 2 - 1];
      break;
    case 2:  // front-mounted
      left.x = player.x;
      left.y = std::max(10, player.y - 20);
      break;
    case 4:  // orbiting
      left.x = player.x + roundf(sinf(rotate) * 20);
      left.y = player.y + roundf(cosf(rotate) * 20);
      break;
  }

  switch (right.style) {
    case 4:  // orbiting
      right.x = player.x - roundf(sinf(rotate) * 20);
      right.y = player.y - roundf(cosf(rotate) * 20);
      break;
    case 1:  // trailing
    case 3:
      right.x = player.old_x[0];
      right.y = player.old_y[0];
      break;
    case 2: {  // front-mounted
      int16_t& move = players.option_attachment_move;
      bool&    comes_back = players.option_attachment_return;

      if (players.option_attachment_linked) {
        right.x = player.x;
        right.y = player.y - 20;
        if (button[3 - 1]) {
          players.option_attachment_linked = false;
          comes_back = false;
          move = -20;
          state.sound.queue[3] = S_WEAPON_26;
        }
      }
      // sidekick needs to catch up to player
      else if (move < -2) {
        right.y += move / 2;
        move += 1 + comes_back;
        setup_explosion(state.explosions.slots, right.x + 1, right.y + 10, 0, 0, false, false);
      }
      else {
        right.y += move / 2;

        int step = comes_back ? 2 : 0;

        if (right.y > (player.y - 20) + 5) {
          step = 2;
          move -= 1 + comes_back;
        }
        else if (right.y > (player.y - 20) - 0) {
          step = 3;
          if (move > 0) {
            move--;
          }
          else {
            move++;
          }
        }
        else if (right.y > (player.y - 20) - 5) {
          step = 2;
          move++;
        }
        else if (move < 2 + comes_back * 4) {
          move += 1 + comes_back;
        }

        if (comes_back) {
          step = step * 2;
        }
        if (abs(right.x - player.x) < step) {
          step = 1;
        }

        if (right.x > player.x) {
          right.x -= step;
        }
        else if (right.x < player.x) {
          right.x += step;
        }

        if (abs(right.y - (player.y - 20)) + abs(right.x - player.x) < 8) {
          players.option_attachment_linked = true;
          state.sound.queue[2] = S_CLINK;
        }

        if (button[3 - 1]) {
          comes_back = true;
        }
      }

      if (right.y < 10) {
        right.y = 10;
      }
      break;
    }
  }
}

// The sidekicks fire, and refill their ammo.
void Movement::fire_sidekicks() {
  auto&       shot_repeat = state.players.shot_repeat;
  auto&       shot_multi_pos = state.players.shot_multi_pos;
  auto const& button = state.input.button;

  for (unsigned i = 0; i < std::size(player.items.sidekick); ++i) {
    unsigned const       shot_i = (i == 0) ? SHOT_LEFT_SIDEKICK : SHOT_RIGHT_SIDEKICK;
    JE_OptionType const& option = options[player.items.sidekick[i]];

    auto& sidekick = player.sidekick[i];

    if (option.wport <= 0) {
      continue;
    }

    // fire/refill sidekick
    if (shot_repeat[shot_i] > 0) {
      --shot_repeat[shot_i];
      continue;
    }

    int const ammo_max = sidekick.ammo_max;
    int const hud_y = hud_sidekick_y[state.game.two_player_mode ? 1 : 0][i] + 13;

    // has infinite ammo
    if (ammo_max <= 0) {
      if (button[0] || button[1 + i]) {
        create_player_shot(state, option.wport, shot_i, sidekick.x, sidekick.y, player.mouse_x,
                           player.mouse_y, option.wpnum + sidekick.charge, num);

        if (sidekick.charge > 0) {
          shot_multi_pos[shot_i] = 0;
          sidekick.charge = 0;
        }
        sidekick.charge_ticks = 20;
        sidekick.animation_enabled = true;
      }
      continue;
    }

    // sidekick has limited ammo
    if (sidekick.ammo_refill_ticks > 0) {
      --sidekick.ammo_refill_ticks;
    }
    // refill one ammo
    else {
      sidekick.ammo_refill_ticks = sidekick.ammo_refill_ticks_max;

      if (sidekick.ammo < ammo_max) {
        ++sidekick.ammo;
      }

      // draw sidekick refill ammo gauge
      hud_sidekick_ammo(record.hud, hud_y, ammo_max, sidekick.ammo, false);
    }

    if (button[1 + i] && sidekick.ammo > 0) {
      create_player_shot(state, option.wport, shot_i, sidekick.x, sidekick.y, player.mouse_x, player.mouse_y,
                         option.wpnum + sidekick.charge, num);

      --sidekick.ammo;
      if (sidekick.charge > 0) {
        shot_multi_pos[shot_i] = 0;
        sidekick.charge = 0;
      }
      sidekick.charge_ticks = 20;
      sidekick.animation_enabled = true;

      // draw sidekick discharge ammo gauge
      hud_sidekick_ammo(record.hud, hud_y, ammo_max, sidekick.ammo, true);
    }
  }
}

// The sidekicks' animation and charge, and their drawing.
void Movement::animate_sidekicks() {
  for (unsigned i = 0; i < std::size(player.sidekick); ++i) {
    JE_OptionType const& option = options[player.items.sidekick[i]];

    auto& sidekick = player.sidekick[i];

    if (option.option > 0) {
      if (sidekick.animation_enabled) {
        if (++sidekick.animation_frame >= option.ani) {
          sidekick.animation_frame = 0;
          sidekick.animation_enabled = (option.option == 1);
        }
      }

      unsigned const sprite = option.gr[sidekick.animation_frame] + sidekick.charge;

      visible.sidekicks[i] =
          VisiblePlayer::Sidekick{record.screen, sidekick.x, sidekick.y, sprite, sidekick.style};
    }

    if (--sidekick.charge_ticks != 0) {
      continue;
    }

    if (sidekick.charge < option.pwr) {
      ++sidekick.charge;
    }
    sidekick.charge_ticks = 20;
  }
}

// JE_mainGamePlayerFunctions: the players' movement, then the parallax map
// scrolling, which follows them.
void main_game_player_functions(State& state, DrawRecord& record, std::vector<VisiblePlayer>& visible) {
  auto& game = state.game;
  auto& ending = state.ending;
  auto& players = state.players;
  Map&  map = state.map;

  // PLAYER MOVEMENT/MOUSE ROUTINES

  if (ending.end_level && ending.countdown > 0) {
    ending.countdown--;
    ending.warp++;
  }

  // Reset Street-Fighter commands
  std::ranges::fill(state.specials.sf_executed, 0);

  state.display.port_config_change = false;

  auto const movement = [&](int i, uint8_t input_device, uint16_t ship_gr, Sprite2_array *ship_gr_ptr) {
    VisiblePlayer& v = visible.emplace_back();
    v.ship_gr = ship_gr;
    v.ship_gr_ptr = ship_gr_ptr;
    Movement{state, record, players.slots[i], input_device, uint8_t(i + 1), ship_gr, v}.run();
  };

  if (game.two_player_mode) {
    movement(0, !game.galaga_mode ? inputDevice[0] : 0, players.ship_gr, players.ship_gr_ptr);
    movement(1, !game.galaga_mode ? inputDevice[1] : 0, players.ship_gr2, players.ship_gr2_ptr);
  }
  else {
    movement(0, 0, players.ship_gr, players.ship_gr_ptr);
  }

  // == Parallax Map Scrolling ==
  uint16_t const center_x =
      game.two_player_mode ? (players.slots[0].x + players.slots[1].x) / 2 : players.slots[0].x;

  uint16_t const ofs = floorf((float)(260 - (center_x - 36)) / (260 - 36) * (24 * 3) - 1);
  map.map_x3_ofs = ofs;
  map.map_x3_pos = map.map_x3_ofs % 24;
  map.map_x3_bp_pos = 1 - (map.map_x3_ofs / 24);

  map.map_x2_ofs = (ofs * 2) / 3;
  map.map_x2_pos = map.map_x2_ofs % 24;
  map.map_x2_bp_pos = 1 - (map.map_x2_ofs / 24);

  map.old_map_x_ofs = map.map_x_ofs;
  map.map_x_ofs = map.map_x2_ofs / 2;
  map.map_x_pos = map.map_x_ofs % 24;
  map.map_x_bp_pos = 1 - (map.map_x_ofs / 24);

  if (map.background3x1) {
    map.map_x3_ofs = map.map_x_ofs;
    map.map_x3_pos = map.map_x_pos;
    map.map_x3_bp_pos = map.map_x_bp_pos - 1;
  }
}

}  // namespace

namespace {

// The message for a pickup, in the message bar.
void pickup_message(State& state, HudLog& hud, char const *who, char const *what) {
  char text[256];
  snprintf(text, sizeof text, "%s %s", who, what);
  hud_text_window(state, hud, text);
}

// JE_playerCollide: the player's collisions with enemies, which pick up
// items, or damage both.
void collide(State& state, DrawRecord& record, Player& player, uint8_t num) {
  auto& game = state.game;
  auto& players = state.players.slots;
  auto& shot_repeat = state.players.shot_repeat;
  auto& shot_multi_pos = state.players.shot_multi_pos;
  auto& sound_queue = state.sound.queue;
  auto& enemies = state.enemies;

  for (int z = 0; z < 100; z++) {
    if (enemies.status[z] == 1) {
      continue;
    }

    Enemy& e = enemies.slots[z];

    int enemy_screen_x = e.x + e.map_offset;

    if (abs(player.x - enemy_screen_x) >= 12 || abs(player.y - e.y) >= 14) {
      continue;
    }

    // Collide
    int evalue = e.value;
    if (evalue > 29999) {
      // spawn dragonwing in galaga mode, otherwise just a purple ball
      if (evalue == 30000) {
        player.cash += 100;

        if (!game.galaga_mode) {
          handle_got_purple_ball(state, player);
        }
        else {
          // spawn the dragonwing?
          if (game.two_player_mode) {
            player.cash += 2400;
          }
          game.two_player_mode = true;
          state.players.two_player_linked = true;
          players[1].items.weapon[REAR_WEAPON].power = 1;
          players[1].armor = 10;
          players[1].is_alive = true;
        }
        enemies.status[z] = 1;
        sound_queue[7] = S_POWERUP;
      }
      else if (game.super_arcade_mode != SA_NONE && evalue > 30000) {
        shot_multi_pos[SHOT_FRONT] = 0;
        shot_repeat[SHOT_FRONT] = 10;

        uint16_t const weapon = SAWeapon[game.super_arcade_mode - 1][evalue - 30000 - 1];

        // if picked up already-owned weapon, power weapon up
        if (weapon == players[0].items.weapon[FRONT_WEAPON].id) {
          player.cash += 1000;
          power_up_weapon(state, player, FRONT_WEAPON);
        }
        // else weapon also gives purple ball
        else {
          handle_got_purple_ball(state, player);
        }

        players[0].items.weapon[FRONT_WEAPON].id = weapon;
        player.cash += 200;
        sound_queue[7] = S_POWERUP;
        enemies.status[z] = 1;
      }
      else if (evalue > 32100) {
        if (num == 1) {
          player.cash += 250;
          players[0].items.special = evalue - 32100;
          shot_multi_pos[SHOT_SPECIAL] = 0;
          shot_repeat[SHOT_SPECIAL] = 10;
          shot_multi_pos[SHOT_SPECIAL2] = 0;
          shot_repeat[SHOT_SPECIAL2] = 0;

          if (game.two_player_mode) {
            pickup_message(state, record.hud, miscText[43 - 1], special[evalue - 32100].name);
          }
          else {
            pickup_message(state, record.hud, miscText[64 - 1], special[evalue - 32100].name);
          }
          sound_queue[7] = S_POWERUP;
          enemies.status[z] = 1;
        }
      }
      else if (evalue > 32000) {
        if (num == 2) {
          enemies.status[z] = 1;
          pickup_message(state, record.hud, miscText[44 - 1], options[evalue - 32000].name);

          // if picked up a different sidekick than player already has, then reset sidekicks to least
          // powerful, else power them up
          if (evalue - 32000u != players[1].items.sidekick_series) {
            players[1].items.sidekick_series = evalue - 32000;
            players[1].items.sidekick_level = 101;
          }
          else if (players[1].items.sidekick_level < 103) {
            ++players[1].items.sidekick_level;
          }

          unsigned const level = players[1].items.sidekick_level - 100 - 1;
          for (unsigned i = 0; i < std::size(players[1].items.sidekick); ++i) {
            players[1].items.sidekick[i] = optionSelect[players[1].items.sidekick_series][level][i];
          }

          shot_multi_pos[SHOT_LEFT_SIDEKICK] = 0;
          shot_multi_pos[SHOT_RIGHT_SIDEKICK] = 0;
          hud_options(state, record.hud);
          sound_queue[7] = S_POWERUP;
        }
        else if (game.one_player_action) {
          enemies.status[z] = 1;
          pickup_message(state, record.hud, miscText[64 - 1], options[evalue - 32000].name);

          for (unsigned i = 0; i < std::size(players[0].items.sidekick); ++i) {
            players[0].items.sidekick[i] = evalue - 32000;
          }
          shot_multi_pos[SHOT_LEFT_SIDEKICK] = 0;
          shot_multi_pos[SHOT_RIGHT_SIDEKICK] = 0;

          hud_options(state, record.hud);
          sound_queue[7] = S_POWERUP;
        }
        if (enemies.status[z] == 1) {
          player.cash += 250;
        }
      }
      else if (evalue > 31000) {
        player.cash += 250;
        if (num == 2) {
          pickup_message(state, record.hud, miscText[44 - 1], weaponPort[evalue - 31000].name);
          players[1].items.weapon[REAR_WEAPON].id = evalue - 31000;
          shot_multi_pos[SHOT_REAR] = 0;
          enemies.status[z] = 1;
          sound_queue[7] = S_POWERUP;
        }
        else if (game.one_player_action) {
          pickup_message(state, record.hud, miscText[64 - 1], weaponPort[evalue - 31000].name);
          players[0].items.weapon[REAR_WEAPON].id = evalue - 31000;
          shot_multi_pos[SHOT_REAR] = 0;
          enemies.status[z] = 1;
          sound_queue[7] = S_POWERUP;

          // does this ever happen?
          if (players[0].items.weapon[REAR_WEAPON].power == 0) {
            players[0].items.weapon[REAR_WEAPON].power = 1;
          }
        }
      }
      else if (evalue > 30000) {
        if (num == 1 && game.two_player_mode) {
          pickup_message(state, record.hud, miscText[43 - 1], weaponPort[evalue - 30000].name);
          players[0].items.weapon[FRONT_WEAPON].id = evalue - 30000;
          shot_multi_pos[SHOT_FRONT] = 0;
          enemies.status[z] = 1;
          sound_queue[7] = S_POWERUP;
        }
        else if (game.one_player_action) {
          pickup_message(state, record.hud, miscText[64 - 1], weaponPort[evalue - 30000].name);
          players[0].items.weapon[FRONT_WEAPON].id = evalue - 30000;
          shot_multi_pos[SHOT_FRONT] = 0;
          enemies.status[z] = 1;
          sound_queue[7] = S_POWERUP;
        }

        if (enemies.status[z] == 1) {
          players[0].items.special = specialArcadeWeapon[evalue - 30000 - 1];
          if (players[0].items.special > 0) {
            shot_multi_pos[SHOT_SPECIAL] = 0;
            shot_repeat[SHOT_SPECIAL] = 0;
            shot_multi_pos[SHOT_SPECIAL2] = 0;
            shot_repeat[SHOT_SPECIAL2] = 0;
          }
          player.cash += 250;
        }
      }
    }
    else if (evalue > 20000) {
      if (state.players.two_player_linked) {
        // share the armor evenly between linked players
        for (Player& p : players) {
          p.armor += (evalue - 20000) / std::size(players);
          if (p.armor > 28) {
            p.armor = 28;
          }
        }
      }
      else {
        player.armor += evalue - 20000;
        if (player.armor > 28) {
          player.armor = 28;
        }
      }
      enemies.status[z] = 1;
      hud_armor(state, record.hud);
      record.screen = Surface::game_screen;  // side-effect of game_screen
      sound_queue[7] = S_POWERUP;
    }
    else if (evalue > 10000 && enemies.status[z] == 2) {
      if (!state.ending.bonus_level) {
        play_song(30);  // Zanac
        state.ending.bonus_level = true;
        state.ending.next_level = evalue - 10000;
        enemies.status[z] = 1;
        state.display.display_time = 150;
      }
    }
    else if (e.score_item) {
      enemies.status[z] = 1;
      sound_queue[7] = S_ITEM;
      if (evalue == 1) {
        state.progress.cube_max++;
        sound_queue[3] = V_DATA_CUBE;
      }
      // got front weapon powerup
      else if (evalue == -1) {
        if (game.two_player_mode) {
          pickup_message(state, record.hud, miscText[43 - 1], miscText[45 - 1]);
        }
        else {
          hud_text_window(state, record.hud, miscText[45 - 1]);
        }

        power_up_weapon(state, players[0], FRONT_WEAPON);
        sound_queue[7] = S_POWERUP;
      }
      // got rear weapon powerup
      else if (evalue == -2) {
        if (game.two_player_mode) {
          pickup_message(state, record.hud, miscText[44 - 1], miscText[46 - 1]);
        }
        else {
          hud_text_window(state, record.hud, miscText[46 - 1]);
        }

        power_up_weapon(state, game.two_player_mode ? players[1] : players[0], REAR_WEAPON);
        sound_queue[7] = S_POWERUP;
      }
      else if (evalue == -3) {
        // picked up orbiting asteroid killer
        shot_multi_pos[SHOT_MISC] = 0;
        create_player_shot(state, 0, SHOT_MISC, player.x, player.y, players[0].mouse_x, players[0].mouse_y,
                           104, num);
        state.player_shots.ttl[z] = 0;
      }
      else if (evalue == -4) {
        if (players[num - 1].superbombs < 10) {
          ++players[num - 1].superbombs;
        }
      }
      else if (evalue == -5) {
        players[0].items.weapon[FRONT_WEAPON].id = 25;  // HOT DOG!
        players[0].items.weapon[REAR_WEAPON].id = 26;
        players[1].items.weapon[REAR_WEAPON].id = 26;

        players[0].last_items = players[0].items;

        for (Player& p : players) {
          p.weapon_mode = 1;
        }

        std::ranges::fill(shot_multi_pos, 0);
      }
      else if (state.players.two_player_linked) {
        // players get equal share of pick-up cash when linked
        for (Player& p : players) {
          p.cash += evalue / std::size(players);
        }
      }
      else {
        player.cash += evalue;
      }
      setup_explosion(state.explosions.slots, enemy_screen_x, e.y, 0, enemyDat[e.type].explosiontype, true,
                      false);
    }
    // explosiontype & 1 == 0: not ground enemy
    else if (player.invulnerable_ticks == 0 && enemies.status[z] == 0 &&
             (enemyDat[e.type].explosiontype & 1) == 0)
    {
      int armorleft = e.armor;
      if (armorleft > state.players.damage_rate) {
        armorleft = state.players.damage_rate;
      }

      damage_player(state, record, player, armorleft);

      // player ship gets push-back from collision
      if (e.armor > 0) {
        player.x_velocity += (e.x_speed * e.armor) / 2;
        player.y_velocity += (e.y_speed * e.armor) / 2;
      }

      int armorleft2 = e.armor;
      if (armorleft2 == 255) {
        armorleft2 = 30000;
      }

      uint8_t link = e.link_num;
      if (link == 0) {
        link = 255;
      }

      if (armorleft2 > armorleft) {
        // damage enemy
        if (e.armor != 255) {
          e.armor -= armorleft;
        }
        sound_queue[5] = S_ENEMY_HIT;
        continue;
      }

      // kill enemy, and those linked to it; the C code counts them in
      // temp2 (Scratch)
      uint8_t& temp2 = state.scratch.temp2;
      for (temp2 = 0; temp2 < 100; temp2++) {
        if (enemies.status[temp2] == 1) {
          continue;
        }

        Enemy&        killed = enemies.slots[temp2];
        uint8_t const linknum = killed.link_num;
        bool const linked = link != 255 && (link == linknum || link - 100 == linknum ||
                                            (linknum > 40 && linknum / 20 == link / 20 && linknum <= link));
        if (temp2 != z && !linked) {
          continue;
        }

        int killed_screen_x = killed.x + killed.map_offset;

        killed.link_num = 0;

        enemies.status[temp2] = 1;

        if (enemyDat[killed.type].esize == 1) {
          setup_explosion_large(state.explosions, killed.ground, killed.explosion, killed_screen_x, killed.y);
          sound_queue[6] = S_EXPLOSION_9;
        }
        else {
          setup_explosion(state.explosions.slots, killed_screen_x, killed.y, 0, 1, false, false);
          sound_queue[5] = S_EXPLOSION_4;
        }
      }
      enemies.status[z] = 1;
    }
  }
}

}  // namespace

std::vector<VisiblePlayer> update_players(State& state, DrawRecord& record) {
  auto& players = state.players.slots;

  std::vector<VisiblePlayer> visible;

  // Player movement indicators for shots that track your ship
  for (Player& p : players) {
    p.last_x_shot_move = p.x;
    p.last_y_shot_move = p.y;
  }

  // =================================
  // =======Collisions Detection======
  // =================================

  for (unsigned i = 0; i < (state.game.two_player_mode ? 2 : 1); ++i) {
    if (players[i].is_alive && !state.ending.end_level) {
      collide(state, record, players[i], i + 1);
    }
  }

  if (state.ending.first_game_over) {
    main_game_player_functions(state, record, visible);  // --------PLAYER DRAW+MOVEMENT---------
  }
  return visible;
}

void get_ship_info(State& state) {
  auto& players = state.players;
  auto& slots = players.slots;

  players.ship_gr_ptr = &spriteSheet9;
  players.ship_gr2_ptr = &spriteSheet9;

  players.power_add = powerSys[slots[0].items.generator].power;

  bool const extra_ship = slots[0].items.ship > 90;
  if (extra_ship) {
    uint8_t base = (slots[0].items.ship - 91) * 15;
    players.ship_gr = s_gr(slots[0].items.ship - 90, players.ship_gr_ptr);
    slots[0].armor = extraShips[base + 7];
  }
  else {
    players.ship_gr = ships[slots[0].items.ship].shipgraphic;
    slots[0].armor = ships[slots[0].items.ship].dmg;
  }

  bool const extra_ship2 = slots[1].items.ship > 90;
  if (extra_ship2) {
    uint8_t base2 = (slots[1].items.ship - 91) * 15;
    players.ship_gr2 = s_gr(slots[1].items.ship - 90, players.ship_gr2_ptr);
    slots[1].armor = extraShips[base2 + 7];  // bug?
  }
  else {
    players.ship_gr2 = 0;
    slots[1].armor = 10;
  }

  for (unsigned i = 0; i < std::size(slots); ++i) {
    slots[i].initial_armor = slots[i].armor;

    unsigned const ani =
        ((i == 0 && extra_ship) || (i == 1 && extra_ship2)) ? 2 : ships[slots[i].items.ship].ani;

    if (ani == 0) {
      slots[i].shot_hit_area_x = 12;
      slots[i].shot_hit_area_y = 10;
    }
    else {
      slots[i].shot_hit_area_x = 11;
      slots[i].shot_hit_area_y = 14;
    }
  }
}

uint16_t port_configs(State const& state) {
  unsigned const player_index = state.game.two_player_mode ? 1 : 0;
  return weaponPort[state.players.slots[player_index].items.weapon[REAR_WEAPON].id].opnum;
}

uint8_t damage_player(State& state, DrawRecord& record, Player& player, uint8_t damage) {
  auto&   sound_queue = state.sound.queue;
  uint8_t through = 0;
  sound_queue[7] = S_SHIELD_HIT;

  // Player Damage Routines
  if (player.shield < damage) {
    through = damage;
    damage -= player.shield;
    player.shield = 0;

    // Through Shields - Now Armor
    if (damage > 0) {
      if (player.armor < damage) {
        player.armor = 0;

        if (player.is_alive && !state.cheats.you_are_cheating) {
          state.level.timer = false;
          player.is_alive = false;
          player.exploding_ticks = 60;
          state.ending.countdown = 40;
          state.sound.temp_volume = tyrMusicVolume;
          sound_queue[1] = S_EXPLOSION_22;
        }
      }
      else {
        player.armor -= damage;
        sound_queue[7] = S_HULL_HIT;
      }
    }
  }
  else {
    player.shield -= damage;

    auto&      slots = state.explosions.slots;
    bool const follow = !state.game.two_player_mode;
    setup_explosion(slots, player.x - 17, player.y - 12, 0, 14, false, follow);
    setup_explosion(slots, player.x - 5, player.y - 12, 0, 15, false, follow);
    setup_explosion(slots, player.x + 7, player.y - 12, 0, 16, false, follow);
    setup_explosion(slots, player.x + 19, player.y - 12, 0, 17, false, follow);

    setup_explosion(slots, player.x - 17, player.y + 2, 0, 18, false, follow);
    setup_explosion(slots, player.x + 19, player.y + 2, 0, 19, false, follow);

    setup_explosion(slots, player.x - 17, player.y + 16, 0, 20, false, follow);
    setup_explosion(slots, player.x - 5, player.y + 16, 0, 21, false, follow);
    setup_explosion(slots, player.x + 7, player.y + 16, 0, 22, false, follow);
  }

  hud_wipe_shield_armor(state, record.hud);
  hud_shield(state, record.hud);
  hud_armor(state, record.hud);
  record.screen = Surface::game_screen;  // side-effect of game_screen

  return through;
}

}  // namespace tyrx::sim
