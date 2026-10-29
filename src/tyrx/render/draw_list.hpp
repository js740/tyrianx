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

//------------------------------------------------------------------------------
// Draw lists: a level tick's drawing as a sequence of calls to the game's
// drawing primitives, each with its arguments and, for primitives that draw
// from game state (backgrounds, superpixels, the starfield), that state as it
// was at the call.
//
// execute() draws a list's calls with the ports in render.hpp.  The validation
// records the C code's lists (../validate/draw/) to replay them with them.
//------------------------------------------------------------------------------

#pragma once

#include "../c/api.hpp"
#include "render.hpp"

#include <stddef.h>
#include <stdint.h>
#include <vector>

namespace tyrx::render {

// The drawing primitives.
// clang-format off
#define TYRX_DRAW_OPS(X) \
  /* sprite.c */ \
  X(blit_sprite) X(blit_sprite_blend) X(blit_sprite_hv_unsafe) X(blit_sprite_hv) X(blit_sprite_hv_blend) \
  X(blit_sprite_dark) X(blit_sprite2) X(blit_sprite2_clip) X(blit_sprite2_blend) X(blit_sprite2_darken) \
  X(blit_sprite2_filter) X(blit_sprite2_filter_clip) X(blit_sprite2x2) X(blit_sprite2x2_clip) \
  X(blit_sprite2x2_blend) X(blit_sprite2x2_darken) X(blit_sprite2x2_filter) X(blit_sprite2x2_filter_clip) \
  /* vga256d.c */ \
  X(JE_pix) X(JE_pix3) X(JE_rectangle) X(fill_rectangle_xy) X(JE_barShade) X(JE_barBright) \
  X(draw_segmented_gauge) \
  /* backgrnd.c */ \
  X(JE_darkenBackground) X(draw_background_1) X(draw_background_2) X(draw_background_2_blend) \
  X(draw_background_3) X(JE_filterScreen) X(lava_filter) X(water_filter) X(iced_blur_filter) X(blur_filter) \
  X(showHeadlight) X(showFlipped) X(showNormal) X(update_and_draw_starfield) \
  /* varz.c, video.c */ \
  X(JE_drawSP) X(JE_clr256)
// clang-format on

enum class DrawOp : uint8_t {
#define TYRX_DRAW_OP_ENUM(name) name,
  TYRX_DRAW_OPS(TYRX_DRAW_OP_ENUM)
#undef TYRX_DRAW_OP_ENUM
};

inline constexpr size_t kDrawOps = 0
#define TYRX_DRAW_OP_COUNT(name) +1
    TYRX_DRAW_OPS(TYRX_DRAW_OP_COUNT)
#undef TYRX_DRAW_OP_COUNT
    ;

// The primitive's name, as the C code calls it.
char const *op_name(DrawOp op);

struct DrawCall {
  DrawOp        op;
  SDL_Surface  *dst = nullptr;
  SDL_Surface  *src = nullptr;  // copies and filters
  Sprite2_array sheet = {};     // sprite2 primitives
  void const   *map = nullptr;  // background map position (into megaData1-3)
  intptr_t      args[8] = {};
  uint32_t      blob = 0;       // extra inputs, in DrawList::blob
  uint32_t      blob_size = 0;  // and their size
};

struct DrawList {
  uint64_t tick = 0;

  std::vector<DrawCall>      calls;
  std::vector<unsigned char> blob;

  // Adds a call, copying its extra inputs into the list.
  void append(DrawCall const& call, void const *blob = nullptr, size_t blob_size = 0);
};

// Draws a call with the ports in render.hpp (execute.cpp).
void execute(DrawCall const& call, unsigned char const *blob, Canvas dst, Canvas src);

}  // namespace tyrx::render
