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

// What a level tick's simulation records for its drawing.  The drawing
// works from the game state at the end of the tick, and from this record
// where that state no longer shows what the C code drew: values that the
// simulation goes on to change after drawing them, which parts were drawn,
// and on which surface.  Each part of the playing area is drawn at most once a
// tick, at a fixed point in the C code's order; the HUD is drawn as it
// changes, so its drawing is logged in order (HudLog).

#pragma once

#include "../gold/api.hpp"
#include "hud.hpp"
#include "visible.hpp"

#include <stdint.h>
#include <bitset>
#include <optional>
#include <string>
#include <vector>

namespace tyrx::sim {

// A copy or filter from one surface to another.
struct Copy {
  Surface dst, src;
};

// The scroll positions a background was drawn at.
struct Background1Drawn {
  Surface screen;
  bool clr;  // JE_clr256 instead (astral zone)
  uint8_t **mapYPos;
  intptr_t mapXbpPos;
  uint16_t mapXPos, backPos;
};
struct Background2Drawn {
  Surface screen;
  bool blend;
  uint8_t **mapY2Pos;
  intptr_t mapX2bpPos;
  uint16_t mapX2Pos, backPos2;
  bool background2, smoothies_1;  // smoothies[1] draws it at background 1's x position
  intptr_t mapXbpPos;
  uint16_t mapXPos;
};
struct Background3Drawn {  // before the scroll that comes first
  Surface screen;
  uint8_t **mapY3Pos;
  intptr_t mapX3bpPos;
  uint16_t mapX3Pos, backPos3, backMove3;
};

struct Darkened {
  Surface screen;
  uint16_t neat;
};

struct EnemiesDrawn {  // by one JE_drawEnemy
  Surface screen;
  uint16_t mapXOfs;
  std::vector<VisibleEnemy> drawn;
};

struct PlayerShotsDrawn {
  Surface screen;
  std::vector<VisiblePlayerShot> drawn;
};

struct EnemyShotsDrawn {  // drawn shots stay in their slots for the rest of the tick
  Surface screen;
  std::bitset<ENEMY_SHOT_MAX> drawn;
};

struct ExplosionsDrawn {
  Surface screen;
  VisibleExplosions visible;
};

struct DebugDrawn {
  Surface screen;
  std::string text[4];
};

struct MessageDrawn {  // displayTime
  Surface screen;
  uint8_t flash;
};

struct SuperpixelsDrawn {  // the ones that were active, before moving
  Surface screen;
  std::bitset<MAX_SUPERPIXELS> active;
};

struct FilterDrawn {  // before the fade moves on
  Surface screen;
  int8_t col, brightness;
};

struct BossBarsDrawn {
  Surface screen;
  unsigned bars;
  struct Bar {
    unsigned x;
    uint8_t armor, color;
  };
  Bar bar[2];
};

struct DrawRecord {
  HudLog hud;

  // The surface that the C code draws the playing area on next (VGAScreen).
  Surface screen = Surface::vga_screen_seg;

  // The playing area, in the C code's order.
  std::optional<Background1Drawn> background_1;
  std::optional<Surface> starfield;
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
  std::vector<VisiblePlayer> players;
  std::optional<EnemyShotsDrawn> enemy_shots;
  std::optional<Background3Drawn> background_3_over_1;
  std::optional<EnemiesDrawn> top_enemies_over, sky_enemies_over;
  std::optional<ExplosionsDrawn> explosions;
  std::optional<Background2Drawn> background_2_over_2;
  std::optional<Surface> warning;
  std::optional<DebugDrawn> debug;
  std::optional<MessageDrawn> message;
  std::optional<Surface> level_timer;
  std::optional<Surface> game_over;
  std::optional<SuperpixelsDrawn> superpixels;
  std::optional<FilterDrawn> filter;
  std::optional<BossBarsDrawn> boss_bars;
  std::optional<Surface> in_game_displays;
  bool show = false;  // the playing area copied to the screen (JE_starShowVGA)
};

}  // namespace tyrx::sim
