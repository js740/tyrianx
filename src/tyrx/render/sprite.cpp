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

// Ports of the sprite blitters in sprite.c.  Positions are offsets into the
// canvas rather than pointers, so that positions outside it, which the C code
// computes but never writes to, are well defined.

#include "render.hpp"

#include <cstddef>
#include <cstring>

namespace tyrx::render {

namespace {

// Blits a run-length encoded sprite from a sprite table: 255 n skips n pixels,
// 254 ends a row, 253 skips a pixel, and any other byte is a pixel, which
// `op(pixel, dst)` turns into the canvas pixel.  Pixels above or below the
// canvas are skipped, and the first one below it ends the sprite.
template <typename Op>
void blit_table_sprite(Canvas dst, int x, int y, unsigned table, unsigned index, Op op) {
  if (index >= sprite_table[table].count || sprite_table[table].sprite[index].data == nullptr) {
    return;
  }
  Sprite const& s = sprite_table[table].sprite[index];
  Uint8 const *data = s.data;
  Uint8 const *const data_ul = data + s.size;
  unsigned const width = s.width;
  unsigned x_offset = 0;

  std::ptrdiff_t p = (std::ptrdiff_t)y * dst.pitch + x;
  std::ptrdiff_t const p_ul = (std::ptrdiff_t)dst.h * dst.pitch;
  for (; data < data_ul; ++data) {
    switch (*data) {
      case 255:  // transparent pixels; the next byte says how many
        ++data;
        p += *data;
        x_offset += *data;
        break;
      case 254:  // next pixel row
        p += width - x_offset;
        x_offset = width;
        break;
      case 253:  // 1 transparent pixel
        ++p;
        ++x_offset;
        break;
      default:
        if (p >= p_ul) {
          return;
        }
        if (p >= 0) {
          dst.pixels[p] = op(*data, dst.pixels[p]);
        }
        ++p;
        ++x_offset;
        break;
    }
    if (x_offset >= width) {
      p += dst.pitch - x_offset;
      x_offset = 0;
    }
  }
}

// The data of sprite `index` (1-based) in a sprite sheet, after its offset
// table.
Uint8 const *sheet_sprite(Sprite2_array sheet, unsigned index) {
  Uint16 offset;
  std::memcpy(&offset, sheet.data + 2 * (index - 1), sizeof offset);
  return sheet.data + SDL_SwapLE16(offset);
}

// Blits a sprite from a sprite sheet: each byte has a count of transparent
// pixels in its low nibble and a count of pixels that follow in its high
// nibble; 0 pixels ends a 12-pixel row, and 0x0F ends the sprite.
template <typename Op>
void blit_sheet_sprite(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index, Op op) {
  std::ptrdiff_t p = (std::ptrdiff_t)y * dst.pitch + x;
  std::ptrdiff_t const p_ul = (std::ptrdiff_t)dst.h * dst.pitch;
  for (Uint8 const *data = sheet_sprite(sheet, index); *data != 0x0f; ++data) {
    p += *data & 0x0f;
    unsigned count = (*data & 0xf0) >> 4;
    if (count == 0) {
      p += dst.pitch - 12;
      continue;
    }
    while (count--) {
      ++data;
      if (p >= p_ul) {
        return;
      }
      if (p >= 0) {
        dst.pixels[p] = op(*data, dst.pixels[p]);
      }
      ++p;
    }
  }
}

// The same, clipping on all edges.
template <typename Op>
void blit_sheet_sprite_clip(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index, Op op) {
  for (Uint8 const *data = sheet_sprite(sheet, index); *data != 0x0f; ++data) {
    if (y >= dst.h) {
      return;
    }
    Uint8 const skip_count = *data & 0x0f;
    Uint8 fill_count = (*data >> 4) & 0x0f;
    x += skip_count;
    if (fill_count == 0) {
      y += 1;
      x -= 12;
    }
    else if (y >= 0) {
      Uint8 *const row = dst.pixels + (std::ptrdiff_t)y * dst.pitch;
      do {
        ++data;
        if (x >= 0 && x < dst.pitch) {
          row[x] = op(*data, row[x]);
        }
        x += 1;
      } while (--fill_count);
    }
    else {
      data += fill_count;
      x += fill_count;
    }
  }
}

// A 2x2 block of sprites, at index, index + 1, index + 19 and index + 20.
template <typename Blit>
void blit_2x2(Blit blit, int x, int y, unsigned index) {
  blit(x, y, index);
  blit(x + 12, y, index + 1);
  blit(x, y + 14, index + 19);
  blit(x + 12, y + 14, index + 20);
}

Uint8 copy(Uint8 src, Uint8) { return src; }

Uint8 blend(Uint8 src, Uint8 dst) { return (src & 0xf0) | (((dst & 0x0f) + (src & 0x0f)) / 2); }

// The sum of a value and an adjustment, or 0xF on overflow and 0x0 on
// underflow (as blit_sprite_hv computes it).
Uint8 adjusted_value(Uint8 src, Sint8 value) {
  Uint8 v = (src & 0x0f) + value;
  if (v > 0xf) {
    v = (v >= 0x1f) ? 0x0 : 0xf;
  }
  return v;
}

}  // namespace

void blit_sprite(Canvas dst, int x, int y, unsigned table, unsigned index) {
  blit_table_sprite(dst, x, y, table, index, copy);
}

void blit_sprite_blend(Canvas dst, int x, int y, unsigned table, unsigned index) {
  blit_table_sprite(dst, x, y, table, index, blend);
}

// Unsafe because the value may overflow into the hue.
void blit_sprite_hv_unsafe(Canvas dst, int x, int y, unsigned table, unsigned index, Uint8 hue, Sint8 value) {
  Uint8 const h = hue << 4;
  blit_table_sprite(dst, x, y, table, index,
                    [=](Uint8 src, Uint8) { return (Uint8)(h | ((src & 0x0f) + value)); });
}

void blit_sprite_hv(Canvas dst, int x, int y, unsigned table, unsigned index, Uint8 hue, Sint8 value) {
  Uint8 const h = hue << 4;
  blit_table_sprite(dst, x, y, table, index,
                    [=](Uint8 src, Uint8) { return (Uint8)(h | adjusted_value(src, value)); });
}

void blit_sprite_hv_blend(Canvas dst, int x, int y, unsigned table, unsigned index, Uint8 hue, Sint8 value) {
  Uint8 const h = hue << 4;
  blit_table_sprite(dst, x, y, table, index, [=](Uint8 src, Uint8 d) {
    return (Uint8)(h | (((d & 0x0f) + adjusted_value(src, value)) / 2));
  });
}

void blit_sprite_dark(Canvas dst, int x, int y, unsigned table, unsigned index, bool black) {
  blit_table_sprite(dst, x, y, table, index, [=](Uint8, Uint8 d) {
    return black ? (Uint8)0x00 : (Uint8)((d & 0xf0) | ((d & 0x0f) / 2));
  });
}

void blit_sprite2(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index) {
  blit_sheet_sprite(dst, x, y, sheet, index, copy);
}

void blit_sprite2_clip(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index) {
  blit_sheet_sprite_clip(dst, x, y, sheet, index, copy);
}

void blit_sprite2_blend(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index) {
  blit_sheet_sprite(dst, x, y, sheet, index, blend);
}

void blit_sprite2_darken(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index) {
  blit_sheet_sprite(dst, x, y, sheet, index,
                    [](Uint8, Uint8 d) { return (Uint8)(((d & 0x0f) / 2) + (d & 0xf0)); });
}

void blit_sprite2_filter(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index, Uint8 filter) {
  blit_sheet_sprite(dst, x, y, sheet, index,
                    [=](Uint8 src, Uint8) { return (Uint8)(filter | (src & 0x0f)); });
}

void blit_sprite2_filter_clip(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index, Uint8 filter) {
  blit_sheet_sprite_clip(dst, x, y, sheet, index,
                         [=](Uint8 src, Uint8) { return (Uint8)(filter | (src & 0x0f)); });
}

void blit_sprite2x2(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index) {
  blit_2x2([&](int bx, int by, unsigned i) { blit_sprite2(dst, bx, by, sheet, i); }, x, y, index);
}

void blit_sprite2x2_clip(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index) {
  blit_2x2([&](int bx, int by, unsigned i) { blit_sprite2_clip(dst, bx, by, sheet, i); }, x, y, index);
}

void blit_sprite2x2_blend(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index) {
  blit_2x2([&](int bx, int by, unsigned i) { blit_sprite2_blend(dst, bx, by, sheet, i); }, x, y, index);
}

void blit_sprite2x2_darken(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index) {
  blit_2x2([&](int bx, int by, unsigned i) { blit_sprite2_darken(dst, bx, by, sheet, i); }, x, y, index);
}

void blit_sprite2x2_filter(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index, Uint8 filter) {
  blit_2x2([&](int bx, int by, unsigned i) { blit_sprite2_filter(dst, bx, by, sheet, i, filter); }, x, y,
           index);
}

void blit_sprite2x2_filter_clip(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index, Uint8 filter) {
  blit_2x2([&](int bx, int by, unsigned i) { blit_sprite2_filter_clip(dst, bx, by, sheet, i, filter); }, x, y,
           index);
}

}  // namespace tyrx::render
