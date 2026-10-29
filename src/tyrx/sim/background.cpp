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
// The map's three background layers in the level loop: ports of the scrolling
// that draw_background_2 and draw_background_3 do as they draw (backgrnd.c),
// with what their drawing needs, and of JE_checkSmoothies (tyrian2.c).

#include "sim.hpp"

namespace tyrx::sim {

void background_1(State const& state, DrawRecord& record, bool clr) {
  Map const& map = state.map;
  record.background_1 =
      Background1Drawn{record.screen, clr, map.map_y_pos, map.map_x_bp_pos, map.map_x_pos, map.back_pos};
}

void background_2(State& state, DrawRecord& record, bool blend, std::optional<Background2Drawn>& drawn) {
  Map& map = state.map;

  if (map.map2_y_delay_max > 1 && map.back_move2 < 2) {
    map.back_move2 = (map.map2_y_delay == 1) ? 1 : 0;
  }

  drawn =
      Background2Drawn{record.screen,    blend,         map.map_y2_pos,       map.map_x2_bp_pos,
                       map.map_x2_pos,   map.back_pos2, map.background2 != 0, state.display.smoothies[1] != 0,
                       map.map_x_bp_pos, map.map_x_pos};

  if (--map.map2_y_delay != 0) {
    return;
  }

  // Set Movement of background
  map.map2_y_delay = map.map2_y_delay_max;

  map.back_pos2 += map.back_move2;

  if (map.back_pos2 > 27) {
    map.back_pos2 -= 28;
    map.map_y2--;
    map.map_y2_pos -= 14;  // Map Width
  }
}

void background_3(State& state, DrawRecord& record, std::optional<Background3Drawn>& drawn) {
  Map& map = state.map;

  drawn = Background3Drawn{record.screen,  map.map_y3_pos, map.map_x3_bp_pos,
                           map.map_x3_pos, map.back_pos3,  map.back_move3};

  // Movement of background
  map.back_pos3 += map.back_move3;

  if (map.back_pos3 > 27) {
    map.back_pos3 -= 28;
    map.map_y3--;
    map.map_y3_pos -= 15;  // Map Width
  }
}

void check_smoothies(State& state) {
  uint8_t const processor = state.game.processor_type;
  auto const&   smoothies = state.display.smoothies;
  state.display.any_smoothies = (processor > 2 && (smoothies[1 - 1] || smoothies[2 - 1])) ||
                                (processor > 1 && (smoothies[3 - 1] || smoothies[4 - 1] || smoothies[5 - 1]));
}

}  // namespace tyrx::sim
