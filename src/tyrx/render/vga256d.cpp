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

// Ports of the pixel, rectangle and gauge drawing in vga256d.c, and of
// JE_clr256 in video.c.

#include "render.hpp"

#include <stddef.h>
#include <algorithm>
#include <cstring>

namespace tyrx::render {

namespace {

// Sets the pixel at offset i, which the C code doesn't check against the start
// of the canvas.
void set(Canvas dst, ptrdiff_t i, uint8_t c) {
  if (i >= 0 && i < (ptrdiff_t)dst.pitch * dst.h) {
    dst.pixels[i] = c;
  }
}

}  // namespace

void pix(Canvas dst, int x, int y, uint8_t c) {
  if (x < dst.pitch && y < dst.h) {
    set(dst, (ptrdiff_t)y * dst.pitch + x, c);
  }
}

void pix3(Canvas dst, int x, int y, uint8_t c) {
  pix(dst, x, y, c);
  pix(dst, x - 1, y, c);
  pix(dst, x + 1, y, c);
  pix(dst, x, y - 1, c);
  pix(dst, x, y + 1, c);
}

void rectangle(Canvas dst, int a, int b, int c, int d, int e) {
  if (!(a < dst.pitch && b < dst.h && c < dst.pitch && d < dst.h)) {
    return;
  }
  std::memset(&dst.pixels[b * dst.pitch + a], e, c - a + 1);  // top
  std::memset(&dst.pixels[d * dst.pitch + a], e, c - a + 1);  // bottom
  for (int i = (b + 1) * dst.pitch + a; i < d * dst.pitch + a; i += dst.pitch) {
    dst.pixels[i] = e;  // left
  }
  for (int i = (b + 1) * dst.pitch + c; i < d * dst.pitch + c; i += dst.pitch) {
    dst.pixels[i] = e;  // right
  }
}

// SDL_FillRect, clipped to the canvas.
void fill_rectangle_wh(Canvas dst, int x, int y, unsigned w, unsigned h, uint8_t color) {
  int const x1 = std::max(x, 0), y1 = std::max(y, 0);
  int const x2 = std::min<long long>((long long)x + (int)w, dst.w),
            y2 = std::min<long long>((long long)y + (int)h, dst.h);
  for (int ry = y1; ry < y2; ++ry) {
    if (x2 > x1) {
      std::memset(&dst.pixels[ry * dst.pitch + x1], color, x2 - x1);
    }
  }
}

void fill_rectangle_xy(Canvas dst, int x, int y, int x2, int y2, uint8_t color) {
  fill_rectangle_wh(dst, x, y, x2 - x + 1, y2 - y + 1, color);
}

void bar_shade(Canvas dst, int a, int b, int c, int d) {
  if (!(a < dst.pitch && b < dst.h && c < dst.pitch && d < dst.h)) {
    return;
  }
  int const width = c - a + 1;
  for (int i = b * dst.pitch + a; i <= d * dst.pitch + a; i += dst.pitch) {
    for (int j = 0; j < width; ++j) {
      uint8_t& p = dst.pixels[i + j];
      p = ((p & 0x0F) >> 1) | (p & 0xF0);
    }
  }
}

void bar_bright(Canvas dst, int a, int b, int c, int d) {
  if (!(a < dst.pitch && b < dst.h && c < dst.pitch && d < dst.h)) {
    return;
  }
  int const width = c - a + 1;
  for (int i = b * dst.pitch + a; i <= d * dst.pitch + a; i += dst.pitch) {
    for (int j = 0; j < width; ++j) {
      uint8_t& p = dst.pixels[i + j];
      uint8_t const ah = p & 0xF0;
      uint8_t al = (p & 0x0F) + 2;
      if (al > 0x0F) {
        al = 0x0F;
      }
      p = al + ah;
    }
  }
}

void draw_segmented_gauge(Canvas dst, int x, int y, uint8_t color, unsigned segment_width,
                          unsigned segment_height, unsigned segment_value, unsigned value) {
  unsigned const segments = value / segment_value, partial_segment = value % segment_value;
  for (unsigned i = 0; i < segments; ++i) {
    fill_rectangle_wh(dst, x, y, segment_width, segment_height, color + 12);
    x += segment_width + 1;
  }
  if (partial_segment > 0) {
    fill_rectangle_wh(dst, x, y, segment_width, segment_height,
                      color + (12 * partial_segment / segment_value));
  }
}

void clear(Canvas dst) {
  for (int y = 0; y < dst.h; ++y) {
    std::memset(&dst.pixels[y * dst.pitch], 0, dst.w);
  }
}

}  // namespace tyrx::render
