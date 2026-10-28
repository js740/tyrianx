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

// C++ ports of the game's drawing primitives.
//
// Each function draws from explicit inputs instead of the C globals onto an
// 8-bit canvas, and must produce exactly the C function's pixels.  This is
// checked call by call (check/render_diff.cpp) and by replaying the C code's
// recorded draw lists with these functions (check/draw/record.cpp).  Sprite data is read
// from the C code's sprite tables and sheets, which only change between levels.

#pragma once

#include "../gold/api.hpp"

#include <stdint.h>

namespace tyrx::render {

// An 8-bit (palettized) image, such as the pixels of an SDL_Surface.
struct Canvas {
  uint8_t *pixels;
  int      pitch;
  int      w, h;
};

// The C drawing code expects all surfaces to be 320 pixels wide, and some of it
// steps between rows by the pitch of VGAScreen rather than of the surface it
// draws on; the ports use the canvas's pitch, which is the same.

// sprite.c: run-length encoded sprites from the sprite tables (sprite_table).
// None of these clip on the left or right edges.
void blit_sprite(Canvas dst, int x, int y, unsigned table, unsigned index);
void blit_sprite_blend(Canvas dst, int x, int y, unsigned table, unsigned index);
void blit_sprite_hv_unsafe(Canvas dst, int x, int y, unsigned table, unsigned index, uint8_t hue,
                           int8_t value);
void blit_sprite_hv(Canvas dst, int x, int y, unsigned table, unsigned index, uint8_t hue, int8_t value);
void blit_sprite_hv_blend(Canvas dst, int x, int y, unsigned table, unsigned index, uint8_t hue,
                          int8_t value);
void blit_sprite_dark(Canvas dst, int x, int y, unsigned table, unsigned index, bool black);

// sprite.c: 12-pixel-wide sprites from sprite sheets (Sprite2_array).  The
// _clip variants clip on all edges, the others only at the top and bottom of
// the canvas.
void blit_sprite2(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index);
void blit_sprite2_clip(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index);
void blit_sprite2_blend(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index);
void blit_sprite2_darken(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index);
void blit_sprite2_filter(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index, uint8_t filter);
void blit_sprite2_filter_clip(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index, uint8_t filter);
void blit_sprite2x2(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index);
void blit_sprite2x2_clip(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index);
void blit_sprite2x2_blend(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index);
void blit_sprite2x2_darken(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index);
void blit_sprite2x2_filter(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index, uint8_t filter);
void blit_sprite2x2_filter_clip(Canvas dst, int x, int y, Sprite2_array sheet, unsigned index,
                                uint8_t filter);

// vga256d.c, video.c
void pix(Canvas dst, int x, int y, uint8_t c);
void pix3(Canvas dst, int x, int y, uint8_t c);
void rectangle(Canvas dst, int x1, int y1, int x2, int y2, int color);
void fill_rectangle_xy(Canvas dst, int x, int y, int x2, int y2, uint8_t color);
void fill_rectangle_wh(Canvas dst, int x, int y, unsigned w, unsigned h, uint8_t color);
void bar_shade(Canvas dst, int x1, int y1, int x2, int y2);
void bar_bright(Canvas dst, int x1, int y1, int x2, int y2);
void draw_segmented_gauge(Canvas dst, int x, int y, uint8_t color, unsigned segment_width,
                          unsigned segment_height, unsigned segment_value, unsigned value);
void clear(Canvas dst);

// backgrnd.c: backgrounds, drawn from their scroll position.  map points to the
// map's first visible row, map_x_bp_pos is the column offset within it.
void draw_background_1(Canvas dst, uint8_t **map, intptr_t map_x_bp_pos, uint16_t map_x_pos,
                       uint16_t back_pos);
void draw_background_2(Canvas dst, uint8_t **map, intptr_t map_x_bp_pos, uint16_t map_x_pos,
                       uint16_t back_pos);
void draw_background_2_blend(Canvas dst, uint8_t **map, intptr_t map_x_bp_pos, uint16_t map_x_pos,
                             uint16_t back_pos);
void draw_background_3(Canvas dst, uint8_t **map, intptr_t map_x_bp_pos, uint16_t map_x_pos,
                       uint16_t back_pos);

// backgrnd.c: filters of the playing area (264x184 at x = 24), and the copy of
// it to the screen.
void filter_screen(Canvas dst, int8_t col, int8_t brightness, bool filtration_avail,
                   bool explosion_transparent);
void darken_background(Canvas dst, uint16_t neat);
void lava_filter(Canvas dst, Canvas src);
void water_filter(Canvas dst, Canvas src, uint8_t hue);
void iced_blur_filter(Canvas dst, Canvas src);
void blur_filter(Canvas dst, Canvas src);
void show_headlight(Canvas dst, Canvas src, int16_t player_x, int16_t player_y);
void show_flipped(Canvas dst, Canvas src);
void show_normal(Canvas dst, Canvas src);

// backgrnd.c, varz.c: the starfield and superpixels move as they are drawn.
void update_and_draw_starfield(Canvas dst, StarfieldStar (&stars)[MAX_STARS], int move_speed);
void draw_superpixels(Canvas dst, superpixel_type (&superpixels)[MAX_SUPERPIXELS]);

}  // namespace tyrx::render
