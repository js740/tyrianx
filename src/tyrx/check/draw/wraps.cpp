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

// The wrapped drawing primitives (see record.hpp), linked with
// -Wl,--wrap for each.

#include "record.hpp"

#include <cstring>

using tyrx::check::draw::draw_call;
using tyrx::render::DrawCall;
using tyrx::render::DrawOp;

extern "C" {

// sprite.c

void __real_blit_sprite(SDL_Surface *, int, int, unsigned int, unsigned int);
void __real_blit_sprite_blend(SDL_Surface *, int, int, unsigned int, unsigned int);
void __real_blit_sprite_hv_unsafe(SDL_Surface *, int, int, unsigned int, unsigned int, uint8_t, int8_t);
void __real_blit_sprite_hv(SDL_Surface *, int, int, unsigned int, unsigned int, uint8_t, int8_t);
void __real_blit_sprite_hv_blend(SDL_Surface *, int, int, unsigned int, unsigned int, uint8_t, int8_t);
void __real_blit_sprite_dark(SDL_Surface *, int, int, unsigned int, unsigned int, bool);

void __wrap_blit_sprite(SDL_Surface *s, int x, int y, unsigned int table, unsigned int index) {
  draw_call({.op = DrawOp::blit_sprite, .dst = s, .args = {x, y, table, index}},
            [&] { __real_blit_sprite(s, x, y, table, index); });
}

void __wrap_blit_sprite_blend(SDL_Surface *s, int x, int y, unsigned int table, unsigned int index) {
  draw_call({.op = DrawOp::blit_sprite_blend, .dst = s, .args = {x, y, table, index}},
            [&] { __real_blit_sprite_blend(s, x, y, table, index); });
}

void __wrap_blit_sprite_hv_unsafe(SDL_Surface *s, int x, int y, unsigned int table, unsigned int index,
                                  uint8_t hue, int8_t value) {
  draw_call({.op = DrawOp::blit_sprite_hv_unsafe, .dst = s, .args = {x, y, table, index, hue, value}},
            [&] { __real_blit_sprite_hv_unsafe(s, x, y, table, index, hue, value); });
}

void __wrap_blit_sprite_hv(SDL_Surface *s, int x, int y, unsigned int table, unsigned int index, uint8_t hue,
                           int8_t value) {
  draw_call({.op = DrawOp::blit_sprite_hv, .dst = s, .args = {x, y, table, index, hue, value}},
            [&] { __real_blit_sprite_hv(s, x, y, table, index, hue, value); });
}

void __wrap_blit_sprite_hv_blend(SDL_Surface *s, int x, int y, unsigned int table, unsigned int index,
                                 uint8_t hue, int8_t value) {
  draw_call({.op = DrawOp::blit_sprite_hv_blend, .dst = s, .args = {x, y, table, index, hue, value}},
            [&] { __real_blit_sprite_hv_blend(s, x, y, table, index, hue, value); });
}

void __wrap_blit_sprite_dark(SDL_Surface *s, int x, int y, unsigned int table, unsigned int index,
                             bool black) {
  draw_call({.op = DrawOp::blit_sprite_dark, .dst = s, .args = {x, y, table, index, black}},
            [&] { __real_blit_sprite_dark(s, x, y, table, index, black); });
}

#define TYRX_WRAP_SPRITE2(name)                                                               \
  void __real_##name(SDL_Surface *, int, int, Sprite2_array, unsigned int);                   \
  void __wrap_##name(SDL_Surface *s, int x, int y, Sprite2_array sheet, unsigned int index) { \
    draw_call({.op = DrawOp::name, .dst = s, .sheet = sheet, .args = {x, y, index}},          \
              [&] { __real_##name(s, x, y, sheet, index); });                                 \
  }
#define TYRX_WRAP_SPRITE2_FILTER(name)                                                       \
  void __real_##name(SDL_Surface *, int, int, Sprite2_array, unsigned int, uint8_t);         \
  void __wrap_##name(SDL_Surface *s, int x, int y, Sprite2_array sheet, unsigned int index,  \
                     uint8_t filter) {                                                       \
    draw_call({.op = DrawOp::name, .dst = s, .sheet = sheet, .args = {x, y, index, filter}}, \
              [&] { __real_##name(s, x, y, sheet, index, filter); });                        \
  }

TYRX_WRAP_SPRITE2(blit_sprite2)
TYRX_WRAP_SPRITE2(blit_sprite2_clip)
TYRX_WRAP_SPRITE2(blit_sprite2_blend)
TYRX_WRAP_SPRITE2(blit_sprite2_darken)
TYRX_WRAP_SPRITE2_FILTER(blit_sprite2_filter)
TYRX_WRAP_SPRITE2_FILTER(blit_sprite2_filter_clip)
TYRX_WRAP_SPRITE2(blit_sprite2x2)
TYRX_WRAP_SPRITE2(blit_sprite2x2_clip)
TYRX_WRAP_SPRITE2(blit_sprite2x2_blend)
TYRX_WRAP_SPRITE2(blit_sprite2x2_darken)
TYRX_WRAP_SPRITE2_FILTER(blit_sprite2x2_filter)
TYRX_WRAP_SPRITE2_FILTER(blit_sprite2x2_filter_clip)

#undef TYRX_WRAP_SPRITE2
#undef TYRX_WRAP_SPRITE2_FILTER

// vga256d.c

void __real_JE_pix(SDL_Surface *, int, int, uint8_t);
void __real_JE_pix3(SDL_Surface *, int, int, uint8_t);
void __real_JE_rectangle(SDL_Surface *, int, int, int, int, int);
void __real_fill_rectangle_xy(SDL_Surface *, int, int, int, int, uint8_t);
void __real_JE_barShade(SDL_Surface *, int, int, int, int);
void __real_JE_barBright(SDL_Surface *, int, int, int, int);
void __real_draw_segmented_gauge(SDL_Surface *, int, int, uint8_t, unsigned, unsigned, unsigned, unsigned);

void __wrap_JE_pix(SDL_Surface *s, int x, int y, uint8_t c) {
  draw_call({.op = DrawOp::JE_pix, .dst = s, .args = {x, y, c}}, [&] { __real_JE_pix(s, x, y, c); });
}

void __wrap_JE_pix3(SDL_Surface *s, int x, int y, uint8_t c) {
  draw_call({.op = DrawOp::JE_pix3, .dst = s, .args = {x, y, c}}, [&] { __real_JE_pix3(s, x, y, c); });
}

void __wrap_JE_rectangle(SDL_Surface *s, int a, int b, int c, int d, int e) {
  draw_call({.op = DrawOp::JE_rectangle, .dst = s, .args = {a, b, c, d, e}},
            [&] { __real_JE_rectangle(s, a, b, c, d, e); });
}

void __wrap_fill_rectangle_xy(SDL_Surface *s, int x, int y, int x2, int y2, uint8_t color) {
  draw_call({.op = DrawOp::fill_rectangle_xy, .dst = s, .args = {x, y, x2, y2, color}},
            [&] { __real_fill_rectangle_xy(s, x, y, x2, y2, color); });
}

void __wrap_JE_barShade(SDL_Surface *s, int a, int b, int c, int d) {
  draw_call({.op = DrawOp::JE_barShade, .dst = s, .args = {a, b, c, d}},
            [&] { __real_JE_barShade(s, a, b, c, d); });
}

void __wrap_JE_barBright(SDL_Surface *s, int a, int b, int c, int d) {
  draw_call({.op = DrawOp::JE_barBright, .dst = s, .args = {a, b, c, d}},
            [&] { __real_JE_barBright(s, a, b, c, d); });
}

void __wrap_draw_segmented_gauge(SDL_Surface *s, int x, int y, uint8_t color, unsigned segment_width,
                                 unsigned segment_height, unsigned segment_value, unsigned value) {
  draw_call({.op = DrawOp::draw_segmented_gauge,
             .dst = s,
             .args = {x, y, color, segment_width, segment_height, segment_value, value}},
            [&] {
              __real_draw_segmented_gauge(s, x, y, color, segment_width, segment_height, segment_value,
                                          value);
            });
}

// backgrnd.c.  The background draws record the scroll state they draw from:
// the map row, the column offset, the x position and the y position.

void __real_JE_darkenBackground(uint16_t);
void __real_draw_background_1(SDL_Surface *);
void __real_draw_background_2(SDL_Surface *);
void __real_draw_background_2_blend(SDL_Surface *);
void __real_draw_background_3(SDL_Surface *);
void __real_JE_filterScreen(int8_t, int8_t);
void __real_lava_filter(SDL_Surface *, SDL_Surface *);
void __real_water_filter(SDL_Surface *, SDL_Surface *);
void __real_iced_blur_filter(SDL_Surface *, SDL_Surface *);
void __real_blur_filter(SDL_Surface *, SDL_Surface *);
void __real_showHeadlight(SDL_Surface *, SDL_Surface *, int16_t, int16_t);
void __real_showFlipped(SDL_Surface *, SDL_Surface *);
void __real_showNormal(SDL_Surface *, SDL_Surface *);
void __real_update_and_draw_starfield(SDL_Surface *, int);

void __wrap_JE_darkenBackground(uint16_t neat) {
  draw_call({.op = DrawOp::JE_darkenBackground, .dst = VGAScreen, .args = {neat}},
            [&] { __real_JE_darkenBackground(neat); });
}

// With the water smoothie (smoothies[1]), background 2 is drawn at background
// 1's x position.
void __wrap_draw_background_2(SDL_Surface *s) {
  draw_call({.op = DrawOp::draw_background_2,
             .dst = s,
             .map = mapY2Pos,
             .args = {mapX2bpPos, mapX2Pos, backPos2, background2, smoothies[1], mapXbpPos, mapXPos}},
            [&] { __real_draw_background_2(s); });
}

void __wrap_draw_background_2_blend(SDL_Surface *s) {
  draw_call({.op = DrawOp::draw_background_2_blend,
             .dst = s,
             .map = mapY2Pos,
             .args = {mapX2bpPos, mapX2Pos, backPos2}},
            [&] { __real_draw_background_2_blend(s); });
}

// Background 3 scrolls before drawing, so this records the state it starts
// from and the scroll step.
void __wrap_draw_background_3(SDL_Surface *s) {
  draw_call({.op = DrawOp::draw_background_3,
             .dst = s,
             .map = mapY3Pos,
             .args = {mapX3bpPos, mapX3Pos, backPos3, backMove3}},
            [&] { __real_draw_background_3(s); });
}

// Besides filtering, JE_filterScreen advances the level's brightness fade.
// Whether it filters also depends on two settings, which are recorded.
void __wrap_JE_filterScreen(int8_t col, int8_t brightness) {
  draw_call({.op = DrawOp::JE_filterScreen,
             .dst = VGAScreen,
             .args = {col, brightness, filtrationAvail, explosionTransparent}},
            [&] { __real_JE_filterScreen(col, brightness); });
}

#define TYRX_WRAP_COPY(name)                                                                   \
  void __wrap_##name(SDL_Surface *dst, SDL_Surface *src) {                                     \
    draw_call({.op = DrawOp::name, .dst = dst, .src = src}, [&] { __real_##name(dst, src); }); \
  }

TYRX_WRAP_COPY(lava_filter)
TYRX_WRAP_COPY(iced_blur_filter)
TYRX_WRAP_COPY(blur_filter)
TYRX_WRAP_COPY(showFlipped)
TYRX_WRAP_COPY(showNormal)

#undef TYRX_WRAP_COPY

void __wrap_water_filter(SDL_Surface *dst, SDL_Surface *src) {
  draw_call({.op = DrawOp::water_filter, .dst = dst, .src = src, .args = {smoothie_data[1]}},
            [&] { __real_water_filter(dst, src); });
}

void __wrap_showHeadlight(SDL_Surface *dst, SDL_Surface *src, int16_t player_x, int16_t player_y) {
  draw_call({.op = DrawOp::showHeadlight, .dst = dst, .src = src, .args = {player_x, player_y}},
            [&] { __real_showHeadlight(dst, src, player_x, player_y); });
}

void __wrap_update_and_draw_starfield(SDL_Surface *s, int move_speed) {
  draw_call(
      {.op = DrawOp::update_and_draw_starfield, .dst = s, .args = {move_speed}},
      [&] { __real_update_and_draw_starfield(s, move_speed); }, starfield_stars, sizeof starfield_stars);
}

// varz.c, video.c

void __real_JE_drawSP(void);
void __real_JE_clr256(SDL_Surface *);

void __wrap_JE_drawSP(void) {
  draw_call(
      {.op = DrawOp::JE_drawSP, .dst = VGAScreen}, [&] { __real_JE_drawSP(); }, superpixels,
      sizeof superpixels);
}

void __wrap_JE_clr256(SDL_Surface *s) {
  draw_call({.op = DrawOp::JE_clr256, .dst = s}, [&] { __real_JE_clr256(s); });
}

}  // extern "C"

void tyrx::check::draw::drawlist_background_1(SDL_Surface *s, void (*draw)(SDL_Surface *)) {
  draw_call(
      {.op = DrawOp::draw_background_1, .dst = s, .map = mapYPos, .args = {mapXbpPos, mapXPos, backPos}},
      [&] { draw(s); });
}

namespace {

// Sets a variable for the rest of the scope.
template <typename T>
class Restore {
 public:
  explicit Restore(T& v) : v_(v), saved_(v) {}
  Restore(Restore const&) = delete;
  ~Restore() { v_ = saved_; }

 private:
  T& v_;
  T saved_;
};

// The background scroll state that the background draws change after (or, for
// background 3, before) drawing.
struct BackgroundScroll {
  Restore<uint16_t> back_move2{backMove2}, back_pos2{backPos2}, back_pos3{backPos3}, map_y2{mapY2},
      map_y3{mapY3};
  Restore<uint8_t> map2_y_delay{map2YDelay};
  Restore<uint8_t **> map_y2_pos{mapY2Pos}, map_y3_pos{mapY3Pos};
};

}  // namespace

void tyrx::check::draw::drawlist_replay(DrawCall const& c, unsigned char const *blob, SDL_Surface *dst,
                                        SDL_Surface *src) {
  auto const& a = c.args;
  Restore<SDL_Surface *> vga_screen{VGAScreen};  // some primitives draw on VGAScreen
  switch (c.op) {
    case DrawOp::blit_sprite:
      __real_blit_sprite(dst, a[0], a[1], a[2], a[3]);
      break;
    case DrawOp::blit_sprite_blend:
      __real_blit_sprite_blend(dst, a[0], a[1], a[2], a[3]);
      break;
    case DrawOp::blit_sprite_hv_unsafe:
      __real_blit_sprite_hv_unsafe(dst, a[0], a[1], a[2], a[3], a[4], a[5]);
      break;
    case DrawOp::blit_sprite_hv:
      __real_blit_sprite_hv(dst, a[0], a[1], a[2], a[3], a[4], a[5]);
      break;
    case DrawOp::blit_sprite_hv_blend:
      __real_blit_sprite_hv_blend(dst, a[0], a[1], a[2], a[3], a[4], a[5]);
      break;
    case DrawOp::blit_sprite_dark:
      __real_blit_sprite_dark(dst, a[0], a[1], a[2], a[3], a[4]);
      break;
#define TYRX_REPLAY_SPRITE2(name)                  \
  case DrawOp::name:                               \
    __real_##name(dst, a[0], a[1], c.sheet, a[2]); \
    break;
#define TYRX_REPLAY_SPRITE2_FILTER(name)                 \
  case DrawOp::name:                                     \
    __real_##name(dst, a[0], a[1], c.sheet, a[2], a[3]); \
    break;
      TYRX_REPLAY_SPRITE2(blit_sprite2)
      TYRX_REPLAY_SPRITE2(blit_sprite2_clip)
      TYRX_REPLAY_SPRITE2(blit_sprite2_blend)
      TYRX_REPLAY_SPRITE2(blit_sprite2_darken)
      TYRX_REPLAY_SPRITE2_FILTER(blit_sprite2_filter)
      TYRX_REPLAY_SPRITE2_FILTER(blit_sprite2_filter_clip)
      TYRX_REPLAY_SPRITE2(blit_sprite2x2)
      TYRX_REPLAY_SPRITE2(blit_sprite2x2_clip)
      TYRX_REPLAY_SPRITE2(blit_sprite2x2_blend)
      TYRX_REPLAY_SPRITE2(blit_sprite2x2_darken)
      TYRX_REPLAY_SPRITE2_FILTER(blit_sprite2x2_filter)
      TYRX_REPLAY_SPRITE2_FILTER(blit_sprite2x2_filter_clip)
#undef TYRX_REPLAY_SPRITE2
#undef TYRX_REPLAY_SPRITE2_FILTER
    case DrawOp::JE_pix:
      __real_JE_pix(dst, a[0], a[1], a[2]);
      break;
    case DrawOp::JE_pix3:
      __real_JE_pix3(dst, a[0], a[1], a[2]);
      break;
    case DrawOp::JE_rectangle:
      __real_JE_rectangle(dst, a[0], a[1], a[2], a[3], a[4]);
      break;
    case DrawOp::fill_rectangle_xy:
      __real_fill_rectangle_xy(dst, a[0], a[1], a[2], a[3], a[4]);
      break;
    case DrawOp::JE_barShade:
      __real_JE_barShade(dst, a[0], a[1], a[2], a[3]);
      break;
    case DrawOp::JE_barBright:
      __real_JE_barBright(dst, a[0], a[1], a[2], a[3]);
      break;
    case DrawOp::draw_segmented_gauge:
      __real_draw_segmented_gauge(dst, a[0], a[1], a[2], a[3], a[4], a[5], a[6]);
      break;
    case DrawOp::JE_darkenBackground:
      VGAScreen = dst;
      __real_JE_darkenBackground(a[0]);
      break;
    case DrawOp::draw_background_1: {
      Restore<uint8_t **> map_y_pos{mapYPos};
      Restore<intptr_t> map_x_bp_pos{mapXbpPos};
      Restore<uint16_t> map_x_pos{mapXPos}, back_pos{backPos};
      mapYPos = static_cast<uint8_t **>(const_cast<void *>(c.map));
      mapXbpPos = a[0];
      mapXPos = a[1];
      backPos = a[2];
      __real_draw_background_1(dst);
      break;
    }
    case DrawOp::draw_background_2: {
      BackgroundScroll scroll;
      Restore<intptr_t> map_x2_bp_pos{mapX2bpPos}, map_x_bp_pos{mapXbpPos};
      Restore<uint16_t> map_x2_pos{mapX2Pos}, map_x_pos{mapXPos};
      Restore<bool> background_2{background2}, water{smoothies[1]};
      mapY2Pos = static_cast<uint8_t **>(const_cast<void *>(c.map));
      mapX2bpPos = a[0];
      mapX2Pos = a[1];
      backPos2 = a[2];
      background2 = a[3];
      smoothies[1] = a[4];
      mapXbpPos = a[5];
      mapXPos = a[6];
      __real_draw_background_2(dst);
      break;
    }
    case DrawOp::draw_background_2_blend: {
      BackgroundScroll scroll;
      Restore<intptr_t> map_x2_bp_pos{mapX2bpPos};
      Restore<uint16_t> map_x2_pos{mapX2Pos};
      mapY2Pos = static_cast<uint8_t **>(const_cast<void *>(c.map));
      mapX2bpPos = a[0];
      mapX2Pos = a[1];
      backPos2 = a[2];
      __real_draw_background_2_blend(dst);
      break;
    }
    case DrawOp::draw_background_3: {
      BackgroundScroll scroll;
      Restore<intptr_t> map_x3_bp_pos{mapX3bpPos};
      Restore<uint16_t> map_x3_pos{mapX3Pos}, back_move3{backMove3};
      mapY3Pos = static_cast<uint8_t **>(const_cast<void *>(c.map));
      mapX3bpPos = a[0];
      mapX3Pos = a[1];
      backPos3 = a[2];
      backMove3 = a[3];
      __real_draw_background_3(dst);
      break;
    }
    case DrawOp::JE_filterScreen: {
      Restore<bool> filter_fade{filterFade}, filter_fade_start{filterFadeStart};
      Restore<bool> filtration_avail{filtrationAvail}, explosion_transparent{explosionTransparent};
      Restore<int8_t> level_brightness{levelBrightness}, level_brightness_chg{levelBrightnessChg};
      Restore<int8_t> level_filter{levelFilter};
      filtrationAvail = a[2];
      explosionTransparent = a[3];
      VGAScreen = dst;
      __real_JE_filterScreen(a[0], a[1]);
      break;
    }
    case DrawOp::lava_filter:
      __real_lava_filter(dst, src);
      break;
    case DrawOp::water_filter: {
      Restore<uint8_t> hue{smoothie_data[1]};
      smoothie_data[1] = a[0];
      __real_water_filter(dst, src);
      break;
    }
    case DrawOp::iced_blur_filter:
      __real_iced_blur_filter(dst, src);
      break;
    case DrawOp::blur_filter:
      __real_blur_filter(dst, src);
      break;
    case DrawOp::showHeadlight:
      __real_showHeadlight(dst, src, a[0], a[1]);
      break;
    case DrawOp::showFlipped:
      __real_showFlipped(dst, src);
      break;
    case DrawOp::showNormal:
      __real_showNormal(dst, src);
      break;
    case DrawOp::update_and_draw_starfield: {
      decltype(starfield_stars) saved;
      std::memcpy(saved, starfield_stars, sizeof saved);
      std::memcpy(starfield_stars, blob, sizeof starfield_stars);
      __real_update_and_draw_starfield(dst, a[0]);
      std::memcpy(starfield_stars, saved, sizeof saved);
      break;
    }
    case DrawOp::JE_drawSP: {
      decltype(superpixels) saved;
      std::memcpy(saved, superpixels, sizeof saved);
      std::memcpy(superpixels, blob, sizeof superpixels);
      VGAScreen = dst;
      __real_JE_drawSP();
      std::memcpy(superpixels, saved, sizeof saved);
      break;
    }
    case DrawOp::JE_clr256:
      __real_JE_clr256(dst);
      break;
  }
}
