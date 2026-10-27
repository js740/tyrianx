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

// draw_background_1 (the scrolling is in the level loop), or JE_clr256.
void background_1_tick(SDL_Surface *screen, bool clr) {
  auto& backPos = state().map.back_pos;
  auto& mapYPos = state().map.map_y_pos;
  auto& mapXPos = state().map.map_x_pos;
  auto& mapXbpPos = state().map.map_x_bp_pos;
  draw_record().background_1 = Background1Drawn{screen, clr, mapYPos, mapXbpPos, mapXPos, backPos};
}

void background_1_draw(Background1Drawn const& d) {
  if (d.clr) {
    draw::JE_clr256(d.screen);
  }
  else {
    draw::draw_background_1(d.screen, d.mapYPos, d.mapXbpPos, d.mapXPos, d.backPos);
  }
}

// draw_background_2, or draw_background_2_blend.
void background_2_tick(SDL_Surface *screen, bool blend, std::optional<Background2Drawn>& drawn) {
  auto& backPos2 = state().map.back_pos2;
  auto& backMove2 = state().map.back_move2;
  auto& mapY2 = state().map.map_y2;
  auto& mapY2Pos = state().map.map_y2_pos;
  auto& mapXPos = state().map.map_x_pos;
  auto& mapX2Pos = state().map.map_x2_pos;
  auto& mapXbpPos = state().map.map_x_bp_pos;
  auto& mapX2bpPos = state().map.map_x2_bp_pos;
  auto& map2YDelay = state().map.map2_y_delay;
  auto& map2YDelayMax = state().map.map2_y_delay_max;
  auto& background2 = state().map.background2;
  if (map2YDelayMax > 1 && backMove2 < 2) {
    backMove2 = (map2YDelay == 1) ? 1 : 0;
  }

  drawn = Background2Drawn{screen,    blend,    mapY2Pos,         mapX2bpPos,
                           mapX2Pos,  backPos2, background2 != 0, smoothies[1] != 0,
                           mapXbpPos, mapXPos};

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

void background_2_draw(Background2Drawn const& d) {
  if (d.blend) {
    draw::draw_background_2_blend(d.screen, d.mapY2Pos, d.mapX2bpPos, d.mapX2Pos, d.backPos2);
  }
  else {
    draw::draw_background_2(d.screen, d.mapY2Pos, d.mapX2bpPos, d.mapX2Pos, d.backPos2, d.background2,
                            d.smoothies_1, d.mapXbpPos, d.mapXPos);
  }
}

// draw_background_3, which scrolls before drawing.
void background_3_tick(SDL_Surface *screen, std::optional<Background3Drawn>& drawn) {
  auto& backPos3 = state().map.back_pos3;
  auto& backMove3 = state().map.back_move3;
  auto& mapY3 = state().map.map_y3;
  auto& mapY3Pos = state().map.map_y3_pos;
  auto& mapX3Pos = state().map.map_x3_pos;
  auto& mapX3bpPos = state().map.map_x3_bp_pos;
  drawn = Background3Drawn{screen, mapY3Pos, mapX3bpPos, mapX3Pos, backPos3, backMove3};

  // Movement of background
  backPos3 += backMove3;

  if (backPos3 > 27) {
    backPos3 -= 28;
    mapY3--;
    mapY3Pos -= 15;  // Map Width
  }
}

void background_3_draw(Background3Drawn const& d) {
  draw::draw_background_3(d.screen, d.mapY3Pos, d.mapX3bpPos, d.mapX3Pos, d.backPos3, d.backMove3);
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
