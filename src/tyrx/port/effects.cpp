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

// Superpixels, explosions and the screen filter (JE_drawSP, JE_doSP and
// JE_setupExplosionLarge in varz.c, and JE_filterScreen in backgrnd.c).

#include "draw.hpp"
#include "port.hpp"

#include <bitset>
#include <cstring>

namespace tyrx::port {

// Superpixels move and fade as they are drawn.  The drawing gets the
// superpixels as they were before (as the C code records them), rebuilt from
// the ones that were active.
void superpixels_tick() {
  std::bitset<MAX_SUPERPIXELS> active;
  for (int i = MAX_SUPERPIXELS; i--;) {
    superpixel_type& sp = superpixels[i];
    if (sp.z) {
      active[i] = true;
      sp.x += sp.delta_x;
      sp.y += sp.delta_y;
      sp.z--;
    }
  }
  draw_record().superpixels = SuperpixelsDrawn{VGAScreen, active};
}

void superpixels_draw(SuperpixelsDrawn const& d) {
  superpixel_type before[MAX_SUPERPIXELS];
  std::memcpy(before, superpixels, sizeof before);
  for (int i = 0; i < MAX_SUPERPIXELS; ++i) {
    if (d.active[i]) {
      before[i].x -= before[i].delta_x;
      before[i].y -= before[i].delta_y;
      before[i].z++;
    }
  }
  draw::JE_drawSP(d.screen, before);
}

// The filter is drawn with the filter and brightness from before the fade
// moves on.
void filter_screen_tick() {
  draw_record().filter = FilterDrawn{VGAScreen, levelFilter, levelBrightness};

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
  draw::JE_filterScreen(d.screen, d.col, d.brightness, filtrationAvail, explosionTransparent);
}

// Port of JE_setupExplosionLarge (varz.c).
void setup_explosion_large(JE_boolean enemyGround, JE_byte exploNum, JE_integer x, JE_integer y) {
  if (y < 0) {
    return;
  }

  if (enemyGround) {
    setup_explosion(x - 6, y - 14, 0, 2, false, false);
    setup_explosion(x + 6, y - 14, 0, 4, false, false);
    setup_explosion(x - 6, y, 0, 3, false, false);
    setup_explosion(x + 6, y, 0, 5, false, false);
  }
  else {
    setup_explosion(x - 6, y - 14, 0, 7, false, false);
    setup_explosion(x + 6, y - 14, 0, 9, false, false);
    setup_explosion(x - 6, y, 0, 8, false, false);
    setup_explosion(x + 6, y, 0, 10, false, false);
  }

  bool big;

  if (exploNum > 10) {
    exploNum -= 10;
    big = true;
  }
  else {
    big = false;
  }

  if (!exploNum) {
    return;
  }

  for (int i = 0; i < MAX_REPEATING_EXPLOSIONS; i++) {
    if (rep_explosions[i].ttl != 0) {
      continue;
    }

    rep_explosions[i].ttl = exploNum;
    rep_explosions[i].delay = 2;
    rep_explosions[i].x = x;
    rep_explosions[i].y = y;
    rep_explosions[i].big = big;
    break;
  }
}

// Port of JE_doSP (varz.c).
// superpixels
void do_sp(JE_word x, JE_word y, JE_word num, JE_byte explowidth, JE_byte color) {
  for (temp = 0; temp < num; temp++) {
    JE_real tempr = mt_rand_lt1() * (2 * M_PI);
    signed int tempy = roundf(cosf(tempr) * mt_rand_1() * explowidth);
    signed int tempx = roundf(sinf(tempr) * mt_rand_1() * explowidth);

    if (++last_superpixel >= MAX_SUPERPIXELS) {
      last_superpixel = 0;
    }
    superpixels[last_superpixel].x = tempx + x;
    superpixels[last_superpixel].y = tempy + y;
    superpixels[last_superpixel].delta_x = tempx;
    superpixels[last_superpixel].delta_y = tempy + 1;
    superpixels[last_superpixel].color = color;
    superpixels[last_superpixel].z = 15;
  }
}

}  // namespace tyrx::port
