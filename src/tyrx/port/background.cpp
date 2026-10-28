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

// The backgrounds and the starfield's drawing (backgrnd.c).  Each background
// is drawn at most once a tick, from the scroll position it had there, which
// the simulation goes on to change; the drawing gets that position.

#include "draw.hpp"
#include "port.hpp"

#include <algorithm>

namespace tyrx::port {

void background_1_draw(Background1Drawn const& d) {
  if (d.clr) {
    draw::JE_clr256(surface(d.screen));
  }
  else {
    draw::draw_background_1(surface(d.screen), d.mapYPos, d.mapXbpPos, d.mapXPos, d.backPos);
  }
}

void background_2_draw(Background2Drawn const& d) {
  if (d.blend) {
    draw::draw_background_2_blend(surface(d.screen), d.mapY2Pos, d.mapX2bpPos, d.mapX2Pos, d.backPos2);
  }
  else {
    draw::draw_background_2(surface(d.screen), d.mapY2Pos, d.mapX2bpPos, d.mapX2Pos, d.backPos2,
                            d.background2, d.smoothies_1, d.mapXbpPos, d.mapXPos);
  }
}

void background_3_draw(Background3Drawn const& d) {
  draw::draw_background_3(surface(d.screen), d.mapY3Pos, d.mapX3bpPos, d.mapX3Pos, d.backPos3, d.backMove3);
}

// update_and_draw_starfield.  The stars move as they are drawn
// (sim::update_starfield), so the drawing gets them as they were before (as the
// C code records them), moved back.  Their speed is still the one they moved
// by, as only events change it, and they come before.
void starfield_draw(SDL_Surface *screen) {
  int const move_speed = state().starfield.speed;
  StarfieldStar before[MAX_STARS];
  std::ranges::copy(state().starfield.stars, before);
  for (StarfieldStar& star : before) {
    star.position -= (star.speed + move_speed) * screen->pitch;
  }
  draw::update_and_draw_starfield(screen, before, move_speed);
}

}  // namespace tyrx::port
