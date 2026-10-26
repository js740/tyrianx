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

// Ports of the text drawing in fonthand.c, adding the sprite calls they make
// to the draw list (draw.hpp).

#include "draw.hpp"

namespace tyrx::port::draw {

namespace {

constexpr int kDefaultBrightness = -3;

// Calls `glyph(sprite_id)` for each character of `s` that has a sprite in
// `font`, and toggles `bright` between 0 and `bright_on` at each '~'; spaces
// advance x by 6, glyphs by their width plus 1.  `check_font` is the table
// whose sprites must exist (the C code checks TINY_FONT for some text in other
// fonts).
template <typename Glyph>
void for_glyphs(int& x, char const *s, unsigned font, int bright_on, int check_font, Glyph glyph) {
  int bright = 0;
  for (int i = 0; s[i] != '\0'; ++i) {
    int const sprite_id = fontMap[(unsigned char)s[i]];
    switch (s[i]) {
      case ' ':
        x += 6;
        break;
      case '~':
        if (bright_on != 0) {
          bright = (bright == 0) ? bright_on : 0;
          break;
        }
        [[fallthrough]];
      default:
        if (sprite_id != -1 && (check_font < 0 || sprite_exists(check_font, sprite_id))) {
          glyph(sprite_id, bright);
          x += sprite(font, sprite_id)->width + 1;
        }
        break;
    }
  }
}

}  // namespace

void JE_dString(SDL_Surface *screen, int x, int y, char const *s, unsigned font) {
  for_glyphs(x, s, font, 2, -1, [&](int id, int bright) {
    draw::blit_sprite_dark(screen, x + 2, y + 2, font, id, false);
    draw::blit_sprite_hv_unsafe(screen, x, y, font, id, 0xf, kDefaultBrightness + bright);
  });
}

void JE_outText(SDL_Surface *screen, int x, int y, char const *s, unsigned colorbank, int brightness) {
  for_glyphs(x, s, TINY_FONT, 4, TINY_FONT, [&](int id, int bright) {
    if (brightness >= 0) {
      draw::blit_sprite_hv_unsafe(screen, x, y, TINY_FONT, id, colorbank, brightness + bright);
    }
    else {
      draw::blit_sprite_dark(screen, x, y, TINY_FONT, id, true);
    }
  });
}

void JE_outTextModify(SDL_Surface *screen, int x, int y, char const *s, unsigned filter, unsigned brightness,
                      unsigned font) {
  // No '~' handling: it is drawn if the font has a sprite for it.
  for_glyphs(x, s, font, 0, -1,
             [&](int id, int) { draw::blit_sprite_hv_blend(screen, x, y, font, id, filter, brightness); });
}

void JE_outTextAdjust(SDL_Surface *screen, int x, int y, char const *s, unsigned filter, int brightness,
                      unsigned font, bool shadow) {
  for_glyphs(x, s, font, 4, TINY_FONT, [&](int id, int bright) {
    if (shadow) {
      draw::blit_sprite_dark(screen, x + 2, y + 2, font, id, false);
    }
    draw::blit_sprite_hv(screen, x, y, font, id, filter, brightness + bright);
  });
}

void JE_outTextAndDarken(SDL_Surface *screen, int x, int y, char const *s, unsigned colorbank,
                         unsigned brightness, unsigned font) {
  for_glyphs(x, s, font, 4, TINY_FONT, [&](int id, int bright) {
    draw::blit_sprite_dark(screen, x + 1, y + 1, font, id, false);
    draw::blit_sprite_hv_unsafe(screen, x, y, font, id, colorbank, brightness + bright);
  });
}

void JE_textShade(SDL_Surface *screen, int x, int y, char const *s, unsigned colorbank, int brightness,
                  unsigned shadetype) {
  switch (shadetype) {
    case PART_SHADE:
      draw::JE_outText(screen, x + 1, y + 1, s, 0, -1);
      draw::JE_outText(screen, x, y, s, colorbank, brightness);
      break;
    case FULL_SHADE:
      draw::JE_outText(screen, x - 1, y, s, 0, -1);
      draw::JE_outText(screen, x + 1, y, s, 0, -1);
      draw::JE_outText(screen, x, y - 1, s, 0, -1);
      draw::JE_outText(screen, x, y + 1, s, 0, -1);
      draw::JE_outText(screen, x, y, s, colorbank, brightness);
      break;
    case DARKEN:
      draw::JE_outTextAndDarken(screen, x + 1, y + 1, s, colorbank, brightness, TINY_FONT);
      break;
    case TRICK:
      draw::JE_outTextModify(screen, x, y, s, colorbank, brightness, TINY_FONT);
      break;
  }
}

}  // namespace tyrx::port::draw
