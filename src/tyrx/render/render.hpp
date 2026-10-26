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

// C++ ports of the game's drawing functions.
//
// Each function draws from explicit state instead of the C globals onto an
// 8-bit canvas, and must produce exactly the C function's pixels (checked by
// render_diff.cpp).

#pragma once

#include "../gold/api.hpp"

#include <cstdint>

namespace tyrx::render {

// An 8-bit (palettized) image, such as the pixels of an SDL_Surface.
struct Canvas {
  Uint8 *pixels;
  int pitch;
  int w, h;
};

// backgrnd.c
void draw_background_1(Canvas dst, JE_byte **map_y_pos, intptr_t map_x_bp_pos, JE_word map_x_pos,
                       JE_word back_pos);

}  // namespace tyrx::render
