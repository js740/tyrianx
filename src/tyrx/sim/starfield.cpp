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

// The starfield: a port of the update in update_and_draw_starfield
// (backgrnd.c).

#include "sim.hpp"

namespace tyrx::sim {

void update_starfield(Starfield& starfield, int pitch) {
  for (int i = MAX_STARS - 1; i >= 0; --i) {
    StarfieldStar& star = starfield.stars[i];
    star.position += (star.speed + starfield.speed) * pitch;
  }
}

}  // namespace tyrx::sim
