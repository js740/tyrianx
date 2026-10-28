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
// Recording the C code's draw lists (render/draw_list.hpp), as the reference
// that the C++ game code's drawing must reproduce.
//
// Every pixel the game writes goes through a drawing primitive in sprite.c,
// vga256d.c, backgrnd.c, varz.c (JE_drawSP) or video.c (JE_clr256), which the
// level code calls across object files, so each is wrapped at link time
// (wraps.cpp).  A call made from inside another recorded call (text drawn with
// sprites, say) is part of the outer call and not recorded separately.
//
//   TYRX_DRAWLIST=FILE      write each level tick's draw list to FILE
//   TYRX_DRAWLIST_CHECK=1   check that the lists are complete and sufficient:
//                           that no pixels change outside recorded calls, and
//                           that replaying each tick's list, on copies of the
//                           surfaces as they were at the start of the tick,
//                           reproduces them, both with the C primitives and
//                           with their C++ ports in render/
//
// While a shadow pass runs the C++ level tick (../shadow.hpp), calls to the C
// primitives go to the list the C++ code draws into (render/tick/draw.hpp) instead.
//------------------------------------------------------------------------------

#pragma once

#include "../../gold/api.hpp"
#include "../../render/draw_list.hpp"

#include <cstddef>
#include <cstdint>

namespace tyrx::check::draw {

using render::DrawCall;
using render::DrawList;
using render::DrawOp;

bool drawlist_enabled();

// The draw list of the current (or just finished) level tick.
DrawList const& drawlist_current();

// Number of failures TYRX_DRAWLIST_CHECK found: pixel changes outside recorded
// calls, and ticks whose replay differed.
std::uint64_t drawlist_check_failures();

// Called at the top of each level tick, and when a level ends.
void drawlist_level_tick();
void drawlist_level_ended();

// Runs draw_background_1 through `draw`, recording it (draw_background_1 is
// wrapped in ../render_diff.cpp).
void drawlist_background_1(SDL_Surface *s, void (*draw)(SDL_Surface *));

// Runs a recorded call again with the C primitive, drawing on `dst` (and
// reading `src`) instead of the surfaces it was recorded with.  Game state that
// the primitive reads is set from the record for the call and restored after.
void drawlist_replay(DrawCall const& call, unsigned char const *blob, SDL_Surface *dst, SDL_Surface *src);

namespace detail {
bool drawlist_begin(DrawCall const& call, void const *blob = nullptr, std::size_t blob_size = 0);
void drawlist_end();
}  // namespace detail

// Runs a drawing primitive, recording it if it is not nested in another.
template <typename F>
void draw_call(DrawCall const& call, F&& run, void const *blob = nullptr, std::size_t blob_size = 0) {
  if (!drawlist_enabled()) {
    run();
    return;
  }
  bool const outer = detail::drawlist_begin(call, blob, blob_size);
  run();
  if (outer) {
    detail::drawlist_end();
  }
}

}  // namespace tyrx::check::draw
