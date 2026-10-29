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

#include "../c/api.hpp"

#include <stdint.h>
#include <bitset>
#include <vector>

namespace tyrx::sim {

// The explosions visible in a tick, for its drawing: the slots of those still
// on screen after the update moved them, and the last position and frame of
// those whose time ran out, as a new explosion may take their slots later in
// the tick.
struct EndedExplosion {
  int32_t  slot;
  int16_t  x;
  int16_t  y;
  uint16_t sprite;
};
struct VisibleExplosions {
  std::bitset<MAX_EXPLOSIONS> slots;
  std::vector<EndedExplosion> ended;
};

// An enemy as it was drawn, before it moved.
struct VisibleEnemy {
  int16_t        x;
  int16_t        y;
  uint16_t       sprite;  // sprites[frame - 1]
  uint8_t        filter;
  uint8_t        size;
  Sprite2_array *sprite_sheet;
};

}  // namespace tyrx::sim
