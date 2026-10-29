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

// Ports of the background drawing, the filters of the playing area and the
// screen copies in backgrnd.c, and of the superpixels in varz.c.  The C
// filters process several pixels at a time (SWAR); these process one at a
// time, in an order that reads the same pixels.

#include "render.hpp"

#include <stddef.h>
#include <stdint.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace tyrx::render {

namespace {

// The rows of tiles a background draws: from y (-28 to 183), the part of the
// 28 rows of a tile row that is within the top 184 rows of the canvas.
struct TileRows {
  int y, first, end;  // canvas row of `first`, and the tile rows [first, end)
};

TileRows tile_rows(int y) {
  if (y < 0) {
    return {0, -y, 28};
  }
  if (y > 184 - 28) {
    return {y, 0, 184 - y};
  }
  return {y, 0, 28};
}

// A row of 12 opaque 24x28 tiles (port of blitBackground1Row).
void blit_background_1_row(Canvas dst, int x, int y, uint8_t **map) {
  TileRows const r = tile_rows(y);
  size_t         di = (size_t)r.y * dst.pitch + x;
  for (int ty = r.first; ty < r.end; ++ty) {
    for (int ti = 0; ti < 12; ++ti) {
      std::memcpy(&dst.pixels[di], map[ti] + ty * 24, 24);
      di += 24;
    }
    di += dst.pitch - 12 * 24;
  }
}

// A row of 12 tiles whose 0 pixels are transparent, with missing (null) tiles
// skipped (port of blitBackgroundRow and blitBackgroundRowBlend).
template <bool kBlend>
void blit_background_row(Canvas dst, int x, int y, uint8_t **map) {
  TileRows const r = tile_rows(y);
  size_t         di = (size_t)r.y * dst.pitch + x;
  for (int ty = r.first; ty < r.end; ++ty) {
    for (int ti = 0; ti < 12; ++ti) {
      if (map[ti] == nullptr) {
        di += 24;
        continue;
      }
      uint8_t const *data = map[ti] + ty * 24;
      for (int tx = 0; tx < 24; ++tx, ++di) {
        if (data[tx] == 0) {
          continue;
        }
        if constexpr (kBlend) {
          uint8_t const value = (data[tx] & 0x0F) + (dst.pixels[di] & 0x0F);
          dst.pixels[di] = (data[tx] & 0xF0) | (value >> 1);
        }
        else {
          dst.pixels[di] = data[tx];
        }
      }
    }
    di += dst.pitch - 12 * 24;
  }
}

// Draws the 8 tile rows of a background that cover the screen.
template <typename Row>
void draw_background(Row row, uint8_t **map, intptr_t map_x_bp_pos, uint16_t map_x_pos, uint16_t back_pos,
                     int map_width) {
  map = map + map_x_bp_pos - 12;
  for (int i = -1; i < 7; ++i) {
    row(map_x_pos, i * 28 + back_pos, map);
    map += map_width;
  }
}

// The value (low nibble) of a pixel plus brightness, or 0xF if the sum is
// 0x10..0x1E and 0x0 if it is negative or larger (port of filterScreenValue).
uint8_t adjust_value(uint8_t pixel, int8_t brightness) {
  int const     sum = (pixel & 0x0F) + brightness;
  uint8_t const value = sum < 0 || sum >= 0x1F ? 0x0 : sum >= 0x10 ? 0xF : sum;
  return (pixel & 0xF0) | value;
}

// Applies `op` to each pixel of the playing area (264x184 at x = 24).
template <typename Op>
void for_playing_area(Canvas dst, Op op) {
  for (int y = 0; y < 184; ++y) {
    uint8_t *row = dst.pixels + (size_t)y * dst.pitch + 24;
    for (int x = 0; x < 264; ++x) {
      row[x] = op(row[x]);
    }
  }
}

uint8_t dark(uint8_t pixel) { return (pixel & 0xF0) | ((pixel & 0x0F) >> 2); }

}  // namespace

void draw_background_1(Canvas dst, uint8_t **map, intptr_t map_x_bp_pos, uint16_t map_x_pos,
                       uint16_t back_pos) {
  draw_background([&](int x, int y, uint8_t **m) { blit_background_1_row(dst, x, y, m); }, map, map_x_bp_pos,
                  map_x_pos, back_pos, 14);
}

void draw_background_2(Canvas dst, uint8_t **map, intptr_t map_x_bp_pos, uint16_t map_x_pos,
                       uint16_t back_pos) {
  draw_background([&](int x, int y, uint8_t **m) { blit_background_row<false>(dst, x, y, m); }, map,
                  map_x_bp_pos, map_x_pos, back_pos, 14);
}

void draw_background_2_blend(Canvas dst, uint8_t **map, intptr_t map_x_bp_pos, uint16_t map_x_pos,
                             uint16_t back_pos) {
  draw_background([&](int x, int y, uint8_t **m) { blit_background_row<true>(dst, x, y, m); }, map,
                  map_x_bp_pos, map_x_pos, back_pos, 14);
}

void draw_background_3(Canvas dst, uint8_t **map, intptr_t map_x_bp_pos, uint16_t map_x_pos,
                       uint16_t back_pos) {
  draw_background([&](int x, int y, uint8_t **m) { blit_background_row<false>(dst, x, y, m); }, map,
                  map_x_bp_pos, map_x_pos, back_pos, 15);
}

void filter_screen(Canvas dst, int8_t col, int8_t brightness, bool filtration_avail,
                   bool explosion_transparent) {
  if (col != -99 && filtration_avail) {
    uint8_t const hue = (uint8_t)col << 4;
    for_playing_area(dst, [=](uint8_t p) { return (uint8_t)(hue | (p & 0x0F)); });
  }
  if (brightness != -99 && explosion_transparent && brightness != 0) {
    for_playing_area(dst, [=](uint8_t p) { return adjust_value(p, brightness); });
  }
}

void darken_background(Canvas dst, uint16_t neat) {
  uint8_t *s = dst.pixels + 24;
  for (int y = 184; y; --y) {
    for (int x = 264; x; --x) {
      int const above = y == 184 ? 0 : *(s - (dst.pitch - 1));
      *s = ((((*s & 0x0f) << 4) - (*s & 0x0f) + ((((x - neat - y) >> 2) + *(s - 2) + above) & 0x0f)) >> 4) |
           (*s & 0xf0);
      ++s;
    }
    s += dst.pitch - 264;
  }
}

// The wave in the lava and water filters shifts each 8-pixel chunk by an
// amount that depends on its position (`diw`, the index just past it in a
// 320-pixel-wide image).  Rows are filtered from the bottom up, because each
// reads the filtered row below it, and each row's chunks from right to left:
// shifted reads at the ends of a row reach into the other end of the row
// next to it, which is then filtered or not as in the C code.  The C code
// filters each chunk a machine word (kLanes pixels) at a time, from right to
// left, reading a word's pixels before writing them, which shows when the
// lava filter's source is its destination.
namespace {

constexpr int kLanes = UINTPTR_MAX >= UINT64_MAX ? 8 : UINTPTR_MAX >= UINT32_MAX ? 4 : 2;

template <typename Filter>
void filter_words(int x0, Filter filter) {
  for (int w = x0 + 8 - kLanes; w >= x0; w -= kLanes) {
    uint8_t out[kLanes];
    for (int i = 0; i < kLanes; ++i) {
      out[i] = filter(w + i, false);
    }
    for (int i = 0; i < kLanes; ++i) {
      filter(w + i, true, out[i]);
    }
  }
}

}  // namespace

void lava_filter(Canvas dst, Canvas src) {
  for (int y = 184; y >= 0; --y) {
    for (int x0 = 320 - 8; x0 >= 0; x0 -= 8) {
      unsigned const diw = y * 320 + x0 + 8;
      int const      wavey = std::abs((int)((diw >> 9) & 0xF) - 8) - 1;  // -1..7
      filter_words(x0, [&](int x, bool write, uint8_t pixel = 0) -> uint8_t {
        size_t const di = (size_t)y * dst.pitch + x, si = (size_t)y * src.pitch + x;
        if (write) {
          dst.pixels[di] = pixel;
          return pixel;
        }
        // Average of the values of the wavey source pixel (2x), and of the
        // wavey destination pixels below and above; values above the top
        // row count as 0.  The hue is red.
        int value = (src.pixels[si + wavey] & 0x0F) * 2 + (dst.pixels[di + wavey + dst.pitch] & 0x0F);
        if (y > 0) {
          value += dst.pixels[di + wavey - dst.pitch] & 0x0F;
        }
        return 0x70 | (value >> 2);
      });
    }
  }
}

void water_filter(Canvas dst, Canvas src, uint8_t hue_index) {
  uint8_t const hue = hue_index << 4;
  for (int y = 184; y >= 0; --y) {
    for (int x0 = 320 - 8; x0 >= 0; x0 -= 8) {
      unsigned const diw = y * 320 + x0 + 8;
      int const      wavey = std::abs((int)((diw >> 10) & 0x7) - 4) - 1;  // -1..3
      filter_words(x0, [&](int x, bool write, uint8_t pixel = 0) -> uint8_t {
        size_t const di = (size_t)y * dst.pitch + x, si = (size_t)y * src.pitch + x;
        if (write) {
          dst.pixels[di] = pixel;
          return pixel;
        }
        // Gray and cloud-blue pixels are copied.  Others become the average
        // of the values of the source pixel and the wavey destination pixel
        // below, in the water's hue.
        uint8_t const source = src.pixels[si];
        if ((source & 0x30) == 0) {
          return source;
        }
        uint8_t const value = (source & 0x0F) + (dst.pixels[di + wavey + dst.pitch] & 0x0F);
        return hue | (value >> 1);
      });
    }
  }
}

// Average of the values of the source and destination pixels, in ice blue.
void iced_blur_filter(Canvas dst, Canvas src) {
  for (int y = 0; y < 184; ++y) {
    for (int x = 0; x < 320; ++x) {
      uint8_t&      d = dst.pixels[(size_t)y * dst.pitch + x];
      uint8_t const s = src.pixels[(size_t)y * src.pitch + x];
      d = 0x80 | (((s & 0x0F) + (d & 0x0F)) >> 1);
    }
  }
}

// Average of the values of the source and destination pixels, in the source
// pixel's hue.
void blur_filter(Canvas dst, Canvas src) {
  for (int y = 0; y < 184; ++y) {
    for (int x = 0; x < 320; ++x) {
      uint8_t&      d = dst.pixels[(size_t)y * dst.pitch + x];
      uint8_t const s = src.pixels[(size_t)y * src.pitch + x];
      d = (s & 0xF0) | (((s & 0x0F) + (d & 0x0F)) >> 1);
    }
  }
}

// A light cone in front of the player, with the rest darkened.  x counts down
// from 264 at the left edge of the playing area, as in the C code.
void show_headlight(Canvas dst, Canvas src, int16_t player_x, int16_t player_y) {
  int lx = (player_x - 17) - (player_y + 12) + 1;  // light starting at X
  int rx = (player_x - 17) + (player_y + 12);      // light until X
  for (int row = 0; row < 184; ++row) {
    uint8_t       *d = dst.pixels + (size_t)row * dst.pitch;
    uint8_t const *s = src.pixels + (size_t)row * src.pitch + 24;
    auto const     at = [](int x) { return 264 - x; };  // column of x
    int            x = 264;
    if (lx <= rx + 1) {
      int const x1 = 264 - std::max(0, lx - 5);
      int const x2 = 264 - std::max(0, lx);
      int const x3 = 264 - std::min(rx, 264);
      int const x4 = 264 - std::min(rx + 5, 264);
      lx += 1;
      rx -= 1;
      // dark left side
      for (; x > x1; --x) {
        d[at(x)] = dark(s[at(x)]);
      }
      // gradient left side
      for (; x > x2; --x) {
        int const     light = 3 * (6 - (x - x2));
        uint8_t const pixel = s[at(x)];
        d[at(x)] = (pixel & 0xF0) | (((pixel & 0xF) + light) >> 2);
      }
      // light
      if (x2 > x3) {
        int const w = x2 - x3;
        std::memcpy(&d[at(x)], &s[at(x)], w);
        x -= w;
      }
      // gradient right side
      for (; x > x4; --x) {
        int const     light = 3 * (x - x4);
        uint8_t const pixel = s[at(x)];
        d[at(x)] = (pixel & 0xF0) | (((pixel & 0xF) + light) >> 2);
      }
    }
    // dark (right side)
    for (; x > 0; --x) {
      d[at(x)] = dark(s[at(x)]);
    }
  }
}

void show_flipped(Canvas dst, Canvas src) {
  for (int row = 0; row < 184; ++row) {
    std::memcpy(dst.pixels + (size_t)(183 - row) * dst.pitch, src.pixels + (size_t)row * src.pitch + 24, 264);
  }
}

void show_normal(Canvas dst, Canvas src) {
  for (int row = 0; row < 184; ++row) {
    std::memcpy(dst.pixels + (size_t)row * dst.pitch, src.pixels + (size_t)row * src.pitch + 24, 264);
  }
}

void update_and_draw_starfield(Canvas dst, ::StarfieldStar (&stars)[MAX_STARS], int move_speed) {
  constexpr int  kHue = 0x90;  // STARFIELD_HUE
  uint8_t *const p = dst.pixels;
  for (int i = MAX_STARS - 1; i >= 0; --i) {
    ::StarfieldStar& star = stars[i];
    star.position += (star.speed + move_speed) * dst.pitch;
    if (star.position >= 177 * dst.pitch) {
      continue;
    }
    if (p[star.position] == 0) {
      p[star.position] = star.color;
    }
    if (star.color - 4 < kHue) {
      continue;
    }

    // bright enough to draw the surrounding pixels
    if (p[star.position + 1] == 0) {
      p[star.position + 1] = star.color - 4;
    }
    if (star.position > 0 && p[star.position - 1] == 0) {
      p[star.position - 1] = star.color - 4;
    }
    if (p[star.position + dst.pitch] == 0) {
      p[star.position + dst.pitch] = star.color - 4;
    }
    if (star.position >= dst.pitch && p[star.position - dst.pitch] == 0) {
      p[star.position - dst.pitch] = star.color - 4;
    }
  }
}

void draw_superpixels(Canvas dst, ::superpixel_type (&superpixels)[MAX_SUPERPIXELS]) {
  for (int i = MAX_SUPERPIXELS; i--;) {
    ::superpixel_type& sp = superpixels[i];
    if (!sp.z) {
      continue;
    }
    sp.x += sp.delta_x;
    sp.y += sp.delta_y;
    if (sp.x < (unsigned)dst.w && sp.y < (unsigned)dst.h) {
      uint8_t *const s = dst.pixels + sp.y * dst.pitch + sp.x;
      *s = (((*s & 0x0f) + sp.z) >> 1) + sp.color;
      if (sp.x > 0) {
        *(s - 1) = (((*(s - 1) & 0x0f) + (sp.z >> 1)) >> 1) + sp.color;
      }
      if (sp.x < dst.w - 1u) {
        *(s + 1) = (((*(s + 1) & 0x0f) + (sp.z >> 1)) >> 1) + sp.color;
      }
      if (sp.y > 0) {
        *(s - dst.pitch) = (((*(s - dst.pitch) & 0x0f) + (sp.z >> 1)) >> 1) + sp.color;
      }
      if (sp.y < dst.h - 1u) {
        *(s + dst.pitch) = (((*(s + dst.pitch) & 0x0f) + (sp.z >> 1)) >> 1) + sp.color;
      }
    }
    sp.z--;
  }
}

}  // namespace tyrx::render
