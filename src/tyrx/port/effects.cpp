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
