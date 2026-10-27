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

// The superpixels, sparks that move and fade: ports of JE_doSP and of the
// update in JE_drawSP (varz.c).

#include "sim.hpp"

#include <cmath>

namespace tyrx::sim {

void do_sp(Superpixels& superpixels, MtState& mt, JE_word x, JE_word y, JE_word num, JE_byte explowidth,
           JE_byte color) {
  // JE_doSP counts in a JE_byte (the global temp)
  for (JE_byte i = 0; i < num; i++) {
    JE_real tempr = mt_rand_lt1_r(&mt) * (2 * M_PI);
    signed int tempy = roundf(cosf(tempr) * mt_rand_1_r(&mt) * explowidth);
    signed int tempx = roundf(sinf(tempr) * mt_rand_1_r(&mt) * explowidth);

    if (++superpixels.last >= MAX_SUPERPIXELS) {
      superpixels.last = 0;
    }
    superpixel_type& sp = superpixels.slots[superpixels.last];
    sp.x = tempx + x;
    sp.y = tempy + y;
    sp.delta_x = tempx;
    sp.delta_y = tempy + 1;
    sp.color = color;
    sp.z = 15;
  }
}

std::bitset<MAX_SUPERPIXELS> update_superpixels(Superpixels& superpixels) {
  std::bitset<MAX_SUPERPIXELS> active;
  for (int i = MAX_SUPERPIXELS; i--;) {
    superpixel_type& sp = superpixels.slots[i];
    if (!sp.z) {
      continue;
    }

    active[i] = true;
    sp.x += sp.delta_x;
    sp.y += sp.delta_y;
    sp.z--;
  }
  return active;
}

}  // namespace tyrx::sim
