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
// Drawing from the C++ game code: each function adds a call to the tick's draw
// list (../draw_list.hpp), with the same arguments as the C primitive of the
// same name records (../../validate/draw/wraps.cpp), instead of drawing.
// Primitives that the C code calls with game state take that state as
// arguments.
//------------------------------------------------------------------------------

#pragma once

#include "../draw_list.hpp"

namespace tyrx::render::draw {

using render::DrawCall;
using render::DrawList;
using render::DrawOp;

inline void emit(DrawList& list, DrawCall const& call, void const *blob = nullptr, size_t blob_size = 0) {
  list.append(call, blob, blob_size);
}

inline void blit_sprite(DrawList& list, SDL_Surface *s, int x, int y, unsigned table, unsigned index) {
  emit(list, {.op = DrawOp::blit_sprite, .dst = s, .args = {x, y, table, index}});
}

inline void blit_sprite_blend(DrawList& list, SDL_Surface *s, int x, int y, unsigned table, unsigned index) {
  emit(list, {.op = DrawOp::blit_sprite_blend, .dst = s, .args = {x, y, table, index}});
}

inline void blit_sprite_hv_unsafe(DrawList& list, SDL_Surface *s, int x, int y, unsigned table,
                                  unsigned index, uint8_t hue, int8_t value) {
  emit(list, {.op = DrawOp::blit_sprite_hv_unsafe, .dst = s, .args = {x, y, table, index, hue, value}});
}

inline void blit_sprite_hv(DrawList& list, SDL_Surface *s, int x, int y, unsigned table, unsigned index,
                           uint8_t hue, int8_t value) {
  emit(list, {.op = DrawOp::blit_sprite_hv, .dst = s, .args = {x, y, table, index, hue, value}});
}

inline void blit_sprite_hv_blend(DrawList& list, SDL_Surface *s, int x, int y, unsigned table, unsigned index,
                                 uint8_t hue, int8_t value) {
  emit(list, {.op = DrawOp::blit_sprite_hv_blend, .dst = s, .args = {x, y, table, index, hue, value}});
}

inline void blit_sprite_dark(DrawList& list, SDL_Surface *s, int x, int y, unsigned table, unsigned index,
                             bool black) {
  emit(list, {.op = DrawOp::blit_sprite_dark, .dst = s, .args = {x, y, table, index, black}});
}

#define TYRX_DRAW_SPRITE2(name)                                                                         \
  inline void name(DrawList& list, SDL_Surface *s, int x, int y, Sprite2_array sheet, unsigned index) { \
    emit(list, {.op = DrawOp::name, .dst = s, .sheet = sheet, .args = {x, y, index}});                  \
  }
#define TYRX_DRAW_SPRITE2_FILTER(name)                                                                \
  inline void name(DrawList& list, SDL_Surface *s, int x, int y, Sprite2_array sheet, unsigned index, \
                   uint8_t filter) {                                                                  \
    emit(list, {.op = DrawOp::name, .dst = s, .sheet = sheet, .args = {x, y, index, filter}});        \
  }
TYRX_DRAW_SPRITE2(blit_sprite2)
TYRX_DRAW_SPRITE2(blit_sprite2_clip)
TYRX_DRAW_SPRITE2(blit_sprite2_blend)
TYRX_DRAW_SPRITE2(blit_sprite2_darken)
TYRX_DRAW_SPRITE2_FILTER(blit_sprite2_filter)
TYRX_DRAW_SPRITE2_FILTER(blit_sprite2_filter_clip)
TYRX_DRAW_SPRITE2(blit_sprite2x2)
TYRX_DRAW_SPRITE2(blit_sprite2x2_clip)
TYRX_DRAW_SPRITE2(blit_sprite2x2_blend)
TYRX_DRAW_SPRITE2(blit_sprite2x2_darken)
TYRX_DRAW_SPRITE2_FILTER(blit_sprite2x2_filter)
TYRX_DRAW_SPRITE2_FILTER(blit_sprite2x2_filter_clip)
#undef TYRX_DRAW_SPRITE2
#undef TYRX_DRAW_SPRITE2_FILTER

inline void JE_pix(DrawList& list, SDL_Surface *s, int x, int y, uint8_t c) {
  emit(list, {.op = DrawOp::JE_pix, .dst = s, .args = {x, y, c}});
}

inline void JE_rectangle(DrawList& list, SDL_Surface *s, int a, int b, int c, int d, int e) {
  emit(list, {.op = DrawOp::JE_rectangle, .dst = s, .args = {a, b, c, d, e}});
}

inline void fill_rectangle_xy(DrawList& list, SDL_Surface *s, int x, int y, int x2, int y2, uint8_t color) {
  emit(list, {.op = DrawOp::fill_rectangle_xy, .dst = s, .args = {x, y, x2, y2, color}});
}

inline void JE_barShade(DrawList& list, SDL_Surface *s, int a, int b, int c, int d) {
  emit(list, {.op = DrawOp::JE_barShade, .dst = s, .args = {a, b, c, d}});
}

inline void JE_barBright(DrawList& list, SDL_Surface *s, int a, int b, int c, int d) {
  emit(list, {.op = DrawOp::JE_barBright, .dst = s, .args = {a, b, c, d}});
}

inline void draw_segmented_gauge(DrawList& list, SDL_Surface *s, int x, int y, uint8_t color,
                                 unsigned segment_width, unsigned segment_height, unsigned segment_value,
                                 unsigned value) {
  emit(list, {.op = DrawOp::draw_segmented_gauge,
              .dst = s,
              .args = {x, y, color, segment_width, segment_height, segment_value, value}});
}

inline void JE_darkenBackground(DrawList& list, SDL_Surface *s, uint16_t neat) {
  emit(list, {.op = DrawOp::JE_darkenBackground, .dst = s, .args = {neat}});
}

inline void draw_background_1(DrawList& list, SDL_Surface *s, uint8_t **mapYPos, intptr_t mapXbpPos,
                              uint16_t mapXPos, uint16_t backPos) {
  emit(list,
       {.op = DrawOp::draw_background_1, .dst = s, .map = mapYPos, .args = {mapXbpPos, mapXPos, backPos}});
}

// smoothies[1] draws background 2 at background 1's x position.
inline void draw_background_2(DrawList& list, SDL_Surface *s, uint8_t **mapY2Pos, intptr_t mapX2bpPos,
                              uint16_t mapX2Pos, uint16_t backPos2, bool background2, bool smoothies_1,
                              intptr_t mapXbpPos, uint16_t mapXPos) {
  emit(list, {.op = DrawOp::draw_background_2,
              .dst = s,
              .map = mapY2Pos,
              .args = {mapX2bpPos, mapX2Pos, backPos2, background2, smoothies_1, mapXbpPos, mapXPos}});
}

inline void draw_background_2_blend(DrawList& list, SDL_Surface *s, uint8_t **mapY2Pos, intptr_t mapX2bpPos,
                                    uint16_t mapX2Pos, uint16_t backPos2) {
  emit(list, {.op = DrawOp::draw_background_2_blend,
              .dst = s,
              .map = mapY2Pos,
              .args = {mapX2bpPos, mapX2Pos, backPos2}});
}

// Draws from the position before the scroll by backMove3 (as the C function
// scrolls before drawing).
inline void draw_background_3(DrawList& list, SDL_Surface *s, uint8_t **mapY3Pos, intptr_t mapX3bpPos,
                              uint16_t mapX3Pos, uint16_t backPos3, uint16_t backMove3) {
  emit(list, {.op = DrawOp::draw_background_3,
              .dst = s,
              .map = mapY3Pos,
              .args = {mapX3bpPos, mapX3Pos, backPos3, backMove3}});
}

// Draws the stars after moving them from where they are in `stars`.
inline void update_and_draw_starfield(DrawList& list, SDL_Surface                   *s,
                                      ::StarfieldStar const (&stars)[MAX_STARS], int move_speed) {
  emit(list, {.op = DrawOp::update_and_draw_starfield, .dst = s, .args = {move_speed}}, stars, sizeof stars);
}

inline void JE_clr256(DrawList& list, SDL_Surface *s) { emit(list, {.op = DrawOp::JE_clr256, .dst = s}); }

inline void lava_filter(DrawList& list, SDL_Surface *dst, SDL_Surface *src) {
  emit(list, {.op = DrawOp::lava_filter, .dst = dst, .src = src});
}

inline void water_filter(DrawList& list, SDL_Surface *dst, SDL_Surface *src, uint8_t hue) {
  emit(list, {.op = DrawOp::water_filter, .dst = dst, .src = src, .args = {hue}});
}

inline void iced_blur_filter(DrawList& list, SDL_Surface *dst, SDL_Surface *src) {
  emit(list, {.op = DrawOp::iced_blur_filter, .dst = dst, .src = src});
}

inline void blur_filter(DrawList& list, SDL_Surface *dst, SDL_Surface *src) {
  emit(list, {.op = DrawOp::blur_filter, .dst = dst, .src = src});
}

inline void JE_filterScreen(DrawList& list, SDL_Surface *s, int8_t col, int8_t brightness,
                            bool filtration_avail, bool explosion_transparent) {
  emit(list, {.op = DrawOp::JE_filterScreen,
              .dst = s,
              .args = {col, brightness, filtration_avail, explosion_transparent}});
}

inline void showFlipped(DrawList& list, SDL_Surface *dst, SDL_Surface *src) {
  emit(list, {.op = DrawOp::showFlipped, .dst = dst, .src = src});
}

inline void showNormal(DrawList& list, SDL_Surface *dst, SDL_Surface *src) {
  emit(list, {.op = DrawOp::showNormal, .dst = dst, .src = src});
}

inline void showHeadlight(DrawList& list, SDL_Surface *dst, SDL_Surface *src, int16_t player_x,
                          int16_t player_y) {
  emit(list, {.op = DrawOp::showHeadlight, .dst = dst, .src = src, .args = {player_x, player_y}});
}

inline void JE_drawSP(DrawList& list, SDL_Surface *s, ::superpixel_type const (&sp)[MAX_SUPERPIXELS]) {
  emit(list, {.op = DrawOp::JE_drawSP, .dst = s}, sp, sizeof sp);
}

// fonthand.c (text.cpp)
void JE_dString(DrawList& list, SDL_Surface *screen, int x, int y, char const *s, unsigned font);
void JE_outText(DrawList& list, SDL_Surface *screen, int x, int y, char const *s, unsigned colorbank,
                int brightness);
void JE_outTextModify(DrawList& list, SDL_Surface *screen, int x, int y, char const *s, unsigned filter,
                      unsigned brightness, unsigned font);
void JE_outTextAdjust(DrawList& list, SDL_Surface *screen, int x, int y, char const *s, unsigned filter,
                      int brightness, unsigned font, bool shadow);
void JE_outTextAndDarken(DrawList& list, SDL_Surface *screen, int x, int y, char const *s, unsigned colorbank,
                         unsigned brightness, unsigned font);
void JE_textShade(DrawList& list, SDL_Surface *screen, int x, int y, char const *s, unsigned colorbank,
                  int brightness, unsigned shadetype);

}  // namespace tyrx::render::draw
