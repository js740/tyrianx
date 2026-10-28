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

// The drawing of the superpixels (JE_drawSP in varz.c), and the screen filter
// (JE_filterScreen in backgrnd.c).

#include "draw.hpp"
#include "port.hpp"

#include <algorithm>

namespace tyrx::port {

// Superpixels move and fade as they are drawn.  The drawing gets the
// superpixels as they were before (as the C code records them), rebuilt from
// the ones that were active.
void superpixels_draw(SuperpixelsDrawn const& d) {
  superpixel_type before[MAX_SUPERPIXELS];
  std::ranges::copy(state().superpixels.slots, before);
  for (int i = 0; i < MAX_SUPERPIXELS; ++i) {
    if (d.active[i]) {
      before[i].x -= before[i].delta_x;
      before[i].y -= before[i].delta_y;
      before[i].z++;
    }
  }
  draw::JE_drawSP(surface(d.screen), before);
}

// The filter is drawn with the filter and brightness from before the fade
// moves on.
void filter_screen_tick() {
  auto& levelFilter = state().display.filter;
  auto& levelFilterNew = state().display.filter_new;
  auto& levelBrightness = state().display.brightness;
  auto& levelBrightnessChg = state().display.brightness_chg;
  auto& filterFade = state().display.filter_fade;
  auto& filterFadeStart = state().display.filter_fade_start;
  draw_record().filter = FilterDrawn{draw_record().screen, levelFilter, levelBrightness};

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

void filter_screen_draw(FilterDrawn const& d) {
  auto& filtrationAvail = state().display.filtration_avail;
  auto& explosionTransparent = state().display.explosion_transparent;
  draw::JE_filterScreen(surface(d.screen), d.col, d.brightness, filtrationAvail, explosionTransparent);
}

}  // namespace tyrx::port
