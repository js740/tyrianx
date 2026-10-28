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

// What the game logic leaves for a tick's drawing about the objects it
// updates: those it drew, as the C code drew them.

#pragma once

#include "../gold/api.hpp"

#include <stdint.h>
#include <bitset>
#include <optional>
#include <vector>

namespace tyrx::sim {

// The surfaces that the C code's level tick draws on, which its VGAScreen
// points at one of.
enum class Surface : uint8_t {
  vga_screen_seg,  // VGAScreenSeg, the HUD's, which the playing area is copied onto at the end
  game_screen,     // game_screen
  vga_screen2,     // VGAScreen2, for the smoothies
};

// The explosions visible in a tick, for its drawing: the slots of those still
// on screen after the update moved them, and the last position and frame of
// those whose time ran out, as a new explosion may take their slots later in
// the tick.
struct EndedExplosion {
  int slot;
  int16_t x, y;
  uint16_t sprite;
};
struct VisibleExplosions {
  std::bitset<MAX_EXPLOSIONS> slots;
  std::vector<EndedExplosion> ended;
};

// An enemy as it was drawn, before it moved.
struct VisibleEnemy {
  int16_t x, y;
  uint16_t sprite;  // egr[enemycycle - 1]
  uint8_t filter, size;
  Sprite2_array *sprite2s;
};

// A player shot as it was drawn.
struct VisiblePlayerShot {
  int x, y;
  uint16_t sprite_frame;  // over 60000 for special shots
  bool shadow;
};

// A player as JE_playerMovement drew it, each part on the surface it was
// drawn on.
struct VisiblePlayer {
  uint16_t ship_gr;
  Sprite2_array *ship_gr_ptr;
  struct Trail {  // at the end of a level
    Surface screen;
    int x;
    std::vector<int> trail_y;
  };
  std::optional<Trail> trail;
  struct InsertCoin {  // in demos
    Surface screen;
  };
  std::optional<InsertCoin> insert_coin;
  struct Ship {
    Surface screen;
    int x, y;
    int sprite;
    uint16_t map_x2_ofs;
    bool background2, super_wild, invulnerable;
  };
  std::optional<Ship> ship;
  struct SpecialIcon {
    Surface screen;
    bool ready;
  };
  std::optional<SpecialIcon> special_icon;
  struct Zinglon {
    Surface screen;
    int x;
    uint8_t width;
  };
  std::optional<Zinglon> zinglon;
  struct Charge {
    Surface screen;
    int x, y;
    int sprite;
  };
  std::optional<Charge> charge;
  struct Sidekick {
    Surface screen;
    int x, y;
    unsigned sprite;
    unsigned style;
  };
  std::optional<Sidekick> sidekicks[2];
};

}  // namespace tyrx::sim
