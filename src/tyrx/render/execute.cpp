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

// Draws a draw call with the ports in render.hpp.

#include "draw_list.hpp"

#include <cstring>

namespace tyrx::render {

void execute(DrawCall const& c, unsigned char const *blob, Canvas dst, Canvas src) {
  auto const& a = c.args;
  JE_byte **const map = static_cast<JE_byte **>(const_cast<void *>(c.map));
  switch (c.op) {
    case DrawOp::blit_sprite:
      blit_sprite(dst, a[0], a[1], a[2], a[3]);
      break;
    case DrawOp::blit_sprite_blend:
      blit_sprite_blend(dst, a[0], a[1], a[2], a[3]);
      break;
    case DrawOp::blit_sprite_hv_unsafe:
      blit_sprite_hv_unsafe(dst, a[0], a[1], a[2], a[3], a[4], a[5]);
      break;
    case DrawOp::blit_sprite_hv:
      blit_sprite_hv(dst, a[0], a[1], a[2], a[3], a[4], a[5]);
      break;
    case DrawOp::blit_sprite_hv_blend:
      blit_sprite_hv_blend(dst, a[0], a[1], a[2], a[3], a[4], a[5]);
      break;
    case DrawOp::blit_sprite_dark:
      blit_sprite_dark(dst, a[0], a[1], a[2], a[3], a[4] != 0);
      break;
    case DrawOp::blit_sprite2:
      blit_sprite2(dst, a[0], a[1], c.sheet, a[2]);
      break;
    case DrawOp::blit_sprite2_clip:
      blit_sprite2_clip(dst, a[0], a[1], c.sheet, a[2]);
      break;
    case DrawOp::blit_sprite2_blend:
      blit_sprite2_blend(dst, a[0], a[1], c.sheet, a[2]);
      break;
    case DrawOp::blit_sprite2_darken:
      blit_sprite2_darken(dst, a[0], a[1], c.sheet, a[2]);
      break;
    case DrawOp::blit_sprite2_filter:
      blit_sprite2_filter(dst, a[0], a[1], c.sheet, a[2], a[3]);
      break;
    case DrawOp::blit_sprite2_filter_clip:
      blit_sprite2_filter_clip(dst, a[0], a[1], c.sheet, a[2], a[3]);
      break;
    case DrawOp::blit_sprite2x2:
      blit_sprite2x2(dst, a[0], a[1], c.sheet, a[2]);
      break;
    case DrawOp::blit_sprite2x2_clip:
      blit_sprite2x2_clip(dst, a[0], a[1], c.sheet, a[2]);
      break;
    case DrawOp::blit_sprite2x2_blend:
      blit_sprite2x2_blend(dst, a[0], a[1], c.sheet, a[2]);
      break;
    case DrawOp::blit_sprite2x2_darken:
      blit_sprite2x2_darken(dst, a[0], a[1], c.sheet, a[2]);
      break;
    case DrawOp::blit_sprite2x2_filter:
      blit_sprite2x2_filter(dst, a[0], a[1], c.sheet, a[2], a[3]);
      break;
    case DrawOp::blit_sprite2x2_filter_clip:
      blit_sprite2x2_filter_clip(dst, a[0], a[1], c.sheet, a[2], a[3]);
      break;
    case DrawOp::JE_pix:
      pix(dst, a[0], a[1], a[2]);
      break;
    case DrawOp::JE_pix3:
      pix3(dst, a[0], a[1], a[2]);
      break;
    case DrawOp::JE_rectangle:
      rectangle(dst, a[0], a[1], a[2], a[3], a[4]);
      break;
    case DrawOp::fill_rectangle_xy:
      fill_rectangle_xy(dst, a[0], a[1], a[2], a[3], a[4]);
      break;
    case DrawOp::JE_barShade:
      bar_shade(dst, a[0], a[1], a[2], a[3]);
      break;
    case DrawOp::JE_barBright:
      bar_bright(dst, a[0], a[1], a[2], a[3]);
      break;
    case DrawOp::draw_segmented_gauge:
      draw_segmented_gauge(dst, a[0], a[1], a[2], a[3], a[4], a[5], a[6]);
      break;
    case DrawOp::JE_darkenBackground:
      darken_background(dst, a[0]);
      break;
    case DrawOp::draw_background_1:
      draw_background_1(dst, map, a[0], a[1], a[2]);
      break;
    case DrawOp::draw_background_2:
      // background2
      if (a[3] != 0) {
        bool const water = a[4] != 0;
        draw_background_2(dst, map, water ? a[5] : a[0], water ? a[6] : a[1], a[2]);
      }
      break;
    case DrawOp::draw_background_2_blend:
      draw_background_2_blend(dst, map, a[0], a[1], a[2]);
      break;
    case DrawOp::draw_background_3: {
      // Recorded before it scrolls, which it does before drawing.
      JE_word back_pos = a[2] + a[3];
      JE_byte **m = map;
      if (back_pos > 27) {
        back_pos -= 28;
        m -= 15;
      }
      draw_background_3(dst, m, a[0], a[1], back_pos);
      break;
    }
    case DrawOp::JE_filterScreen:
      filter_screen(dst, a[0], a[1], a[2] != 0, a[3] != 0);
      break;
    case DrawOp::lava_filter:
      lava_filter(dst, src);
      break;
    case DrawOp::water_filter:
      water_filter(dst, src, a[0]);
      break;
    case DrawOp::iced_blur_filter:
      iced_blur_filter(dst, src);
      break;
    case DrawOp::blur_filter:
      blur_filter(dst, src);
      break;
    case DrawOp::showHeadlight:
      show_headlight(dst, src, a[0], a[1]);
      break;
    case DrawOp::showFlipped:
      show_flipped(dst, src);
      break;
    case DrawOp::showNormal:
      show_normal(dst, src);
      break;
    case DrawOp::update_and_draw_starfield: {
      StarfieldStar stars[MAX_STARS];
      std::memcpy(stars, blob, sizeof stars);
      update_and_draw_starfield(dst, stars, a[0]);
      break;
    }
    case DrawOp::JE_drawSP: {
      superpixel_type sp[MAX_SUPERPIXELS];
      std::memcpy(sp, blob, sizeof sp);
      draw_superpixels(dst, sp);
      break;
    }
    case DrawOp::JE_clr256:
      clear(dst);
      break;
  }
}

}  // namespace tyrx::render
