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

//------------------------------------------------------------------------------
// What a level tick's simulation records for its drawing.  The drawing
// (draw_tick, after the simulation) works from the game state at the end of
// the tick, and from this record where that state no longer shows what the
// C code drew: values that the simulation goes on to change after drawing
// them, which parts were drawn, and on which surface.  Each part of the
// playing area is drawn at most once a tick, at a fixed point in the C code's
// order; the HUD is drawn as it changes, so its drawing is logged in order.
//------------------------------------------------------------------------------

#pragma once

#include "../gold/api.hpp"
#include "../sim/sim.hpp"

#include <array>
#include <bitset>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace tyrx::port {

// The HUD's log, kept by ../sim/ (hud.hpp).
namespace hud = sim::hud;

// A copy or filter from one surface to another.
struct Copy {
  SDL_Surface *dst, *src;
};

// The scroll positions a background was drawn at.
struct Background1Drawn {
  SDL_Surface *screen;
  bool clr;  // JE_clr256 instead (astral zone)
  JE_byte **mapYPos;
  intptr_t mapXbpPos;
  JE_word mapXPos, backPos;
};
struct Background2Drawn {
  SDL_Surface *screen;
  bool blend;
  JE_byte **mapY2Pos;
  intptr_t mapX2bpPos;
  JE_word mapX2Pos, backPos2;
  bool background2, smoothies_1;  // smoothies[1] draws it at background 1's x position
  intptr_t mapXbpPos;
  JE_word mapXPos;
};
struct Background3Drawn {  // before the scroll that comes first
  SDL_Surface *screen;
  JE_byte **mapY3Pos;
  intptr_t mapX3bpPos;
  JE_word mapX3Pos, backPos3, backMove3;
};

struct Darkened {
  SDL_Surface *screen;
  JE_word neat;
};

struct EnemiesDrawn {  // by one JE_drawEnemy
  SDL_Surface *screen;
  JE_word mapXOfs;
  std::vector<sim::VisibleEnemy> drawn;
};

struct PlayerShotsDrawn {
  SDL_Surface *screen;
  std::vector<sim::VisiblePlayerShot> drawn;
};

struct PlayerDrawn {    // by one JE_playerMovement
  SDL_Surface *screen;  // VGAScreen when the players' update began
  sim::VisiblePlayer player;
};

struct EnemyShotsDrawn {  // drawn shots stay in their slots for the rest of the tick
  SDL_Surface *screen;
  std::bitset<ENEMY_SHOT_MAX> drawn;
};

struct ExplosionsDrawn {
  SDL_Surface *screen;
  sim::VisibleExplosions visible;
};

struct DebugDrawn {
  SDL_Surface *screen;
  std::array<std::string, 4> text;
};

struct MessageDrawn {  // displayTime
  SDL_Surface *screen;
  JE_byte flash;
};

struct SuperpixelsDrawn {  // the ones that were active, before moving
  SDL_Surface *screen;
  std::bitset<MAX_SUPERPIXELS> active;
};

struct FilterDrawn {  // before the fade moves on
  SDL_Surface *screen;
  JE_shortint col, brightness;
};

struct BossBarsDrawn {
  SDL_Surface *screen;
  unsigned bars;
  struct Bar {
    unsigned x;
    Uint8 armor, color;
  };
  std::array<Bar, 2> bar;
};

struct DrawRecord {
  sim::HudLog hud;

  // The playing area, in the C code's order.
  std::optional<Background1Drawn> background_1;
  SDL_Surface *starfield = nullptr;
  std::optional<Copy> iced_blur_1;  // smoothie 5
  std::optional<Background2Drawn> background_2_under;
  std::optional<Copy> lava_1;  // smoothie 1, without data
  std::optional<Copy> water;   // smoothie 2
  std::optional<EnemiesDrawn> ground_enemies, ground_enemies_2;
  std::optional<Copy> lava_2;  // smoothie 1, with data
  std::optional<Darkened> darken_1;
  std::optional<Background2Drawn> background_2_over_1;
  std::optional<Darkened> darken_2;
  std::optional<Background3Drawn> background_3_over_2;
  std::optional<Copy> iced_blur_2, blur;  // smoothies 3 and 4
  std::optional<EnemiesDrawn> sky_enemies_under;
  std::optional<Background3Drawn> background_3_over_0;
  std::optional<EnemiesDrawn> top_enemies_under;
  std::optional<PlayerShotsDrawn> player_shots;
  std::vector<PlayerDrawn> players;
  std::optional<EnemyShotsDrawn> enemy_shots;
  std::optional<Background3Drawn> background_3_over_1;
  std::optional<EnemiesDrawn> top_enemies_over, sky_enemies_over;
  std::optional<ExplosionsDrawn> explosions;
  std::optional<Background2Drawn> background_2_over_2;
  SDL_Surface *warning = nullptr;
  std::optional<DebugDrawn> debug;
  std::optional<MessageDrawn> message;
  SDL_Surface *level_timer = nullptr;
  SDL_Surface *game_over = nullptr;
  std::optional<SuperpixelsDrawn> superpixels;
  std::optional<FilterDrawn> filter;
  std::optional<BossBarsDrawn> boss_bars;
  SDL_Surface *in_game_displays = nullptr;
  bool show = false;  // the playing area copied to the screen (JE_starShowVGA)
};

// The record of the tick being simulated, or last simulated.
DrawRecord& draw_record();

// Draws the tick, after its simulation.
void draw_tick(DrawRecord const& r);

}  // namespace tyrx::port
