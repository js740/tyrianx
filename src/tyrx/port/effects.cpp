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

// Superpixels and the screen filter (JE_drawSP in varz.c, and JE_filterScreen
// in backgrnd.c).

#include "draw.hpp"
#include "port.hpp"

namespace tyrx::port {

// Port of JE_drawSP: the superpixels move and fade as they are drawn.
void draw_sp() {
  draw::JE_drawSP(VGAScreen, superpixels);
  for (int i = MAX_SUPERPIXELS; i--;) {
    superpixel_type& sp = superpixels[i];
    if (sp.z) {
      sp.x += sp.delta_x;
      sp.y += sp.delta_y;
      sp.z--;
    }
  }
}

// Port of JE_filterScreen, which moves the fade on, then filters with the
// filter and brightness it was called with.
void filter_screen(JE_shortint col, JE_shortint brightness) {
  draw::JE_filterScreen(VGAScreen, col, brightness, filtrationAvail, explosionTransparent);

  if (!filterFade) {
    return;
  }

  levelBrightness += levelBrightnessChg;
  if ((filterFadeStart && levelBrightness < -14) || levelBrightness > 14) {
    levelBrightnessChg = -levelBrightnessChg;
    filterFadeStart = false;
    levelFilter = levelFilterNew;
  }
  if (!filterFadeStart && levelBrightness == 0) {
    filterFade = false;
    levelBrightness = -99;
  }
}

}  // namespace tyrx::port
