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

// Ports of the background drawing in backgrnd.c.

#include "render.hpp"

#include <cstring>

namespace tyrx::render {

namespace {

// Port of blitBackground1Row: a row of 12 opaque 24x28 tiles at (x, y),
// clipped to the top 184 pixel rows.  Tiles reach beyond the visible area
// (24, 0)-(288, 184) on the left and right.
void blit_background_1_row(Canvas dst, int x, int y, JE_byte **map) {
  int ty = 0;
  int th = 28;
  if (y < 0) {
    ty = -y;
    y = 0;
  }
  else if (y > 184 - th) {
    th = 184 - y;
  }

  std::size_t di = (std::size_t)y * dst.pitch + x;
  for (; ty < th; ++ty) {
    for (int ti = 0; ti < 12; ++ti) {
      std::memcpy(&dst.pixels[di], map[ti] + ty * 24, 24);
      di += 24;
    }
    di += dst.pitch - 12 * 24;
  }
}

}  // namespace

// Port of draw_background_1.  map_y_pos points to the top row of the
// background 1 map that is on screen, and map_x_bp_pos is the column offset
// within it.
void draw_background_1(Canvas dst, JE_byte **map_y_pos, intptr_t map_x_bp_pos, JE_word map_x_pos,
                       JE_word back_pos) {
  JE_byte **map = map_y_pos + map_x_bp_pos - 12;
  for (int i = -1; i < 7; ++i) {
    blit_background_1_row(dst, map_x_pos, i * 28 + back_pos, map);
    map += 14;
  }
}

}  // namespace tyrx::render
