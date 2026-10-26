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

// The backgrounds and the starfield (backgrnd.c).  Background 1 is drawn by
// the level loop, which scrolls it; backgrounds 2 and 3 and the starfield
// move as they are drawn.

#include "draw.hpp"
#include "port.hpp"

namespace tyrx::port {

// draw_background_2, or draw_background_2_blend.
void background_2(SDL_Surface *screen, bool blend) {
  if (map2YDelayMax > 1 && backMove2 < 2) {
    backMove2 = (map2YDelay == 1) ? 1 : 0;
  }

  if (blend) {
    draw::draw_background_2_blend(screen, mapY2Pos, mapX2bpPos, mapX2Pos, backPos2);
  }
  else {
    draw::draw_background_2(screen, mapY2Pos, mapX2bpPos, mapX2Pos, backPos2, background2 != 0,
                            smoothies[1] != 0, mapXbpPos, mapXPos);
  }

  if (--map2YDelay != 0) {
    return;
  }

  // Set Movement of background
  map2YDelay = map2YDelayMax;

  backPos2 += backMove2;

  if (backPos2 > 27) {
    backPos2 -= 28;
    mapY2--;
    mapY2Pos -= 14;  // Map Width
  }
}

// draw_background_3, which scrolls before drawing.
void background_3(SDL_Surface *screen) {
  draw::draw_background_3(screen, mapY3Pos, mapX3bpPos, mapX3Pos, backPos3, backMove3);

  // Movement of background
  backPos3 += backMove3;

  if (backPos3 > 27) {
    backPos3 -= 28;
    mapY3--;
    mapY3Pos -= 15;  // Map Width
  }
}

// update_and_draw_starfield.
void starfield(SDL_Surface *screen, int move_speed) {
  draw::update_and_draw_starfield(screen, starfield_stars, move_speed);

  for (int i = MAX_STARS - 1; i >= 0; --i) {
    StarfieldStar *star = &starfield_stars[i];

    star->position += (star->speed + move_speed) * screen->pitch;
  }
}

}  // namespace tyrx::port
