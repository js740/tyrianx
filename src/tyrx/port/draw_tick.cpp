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

// The drawing of a level tick, after its simulation: the draw calls of the C
// code's level loop, in its order, from the game state at the end of the tick
// and the draw record (draw_record.hpp).

#include "draw.hpp"
#include "draw_record.hpp"
#include "port.hpp"

#include <cstdio>

namespace tyrx::port {

DrawRecord& draw_record() {
  static DrawRecord record;
  return record;
}

namespace {

// The shots drawn stay as they are, and in their slots, for the rest of the
// tick.
void enemy_shots_draw(EnemyShotsDrawn const& d) {
  for (int z = 0; z < ENEMY_SHOT_MAX; z++) {
    if (!d.drawn[z]) {
      continue;
    }

    if (enemyShot[z].sgr >= 500) {
      draw::blit_sprite2(d.screen, enemyShot[z].sx, enemyShot[z].sy, spriteSheet12,
                         enemyShot[z].sgr + enemyShot[z].animate - 500);
    }
    else {
      draw::blit_sprite2(d.screen, enemyShot[z].sx, enemyShot[z].sy, spriteSheet8,
                         enemyShot[z].sgr + enemyShot[z].animate);
    }
  }
}

// Explosions that ended may have had their slots taken, so those are drawn
// from the record.
void explosions_draw(ExplosionsDrawn const& d) {
  auto e = d.ended.begin();
  for (int j = 0; j < MAX_EXPLOSIONS; j++) {
    if (!d.drawn[j]) {
      continue;
    }

    Explosion const& explosion = state().explosions.slots[j];
    JE_integer x = explosion.x, y = explosion.y;
    JE_word sprite = explosion.sprite;
    if (e != d.ended.end() && e->j == j) {
      x = e->x, y = e->y, sprite = e->sprite;
      ++e;
    }
    if (explosionTransparent) {
      draw::blit_sprite2_blend(d.screen, x, y, explosionSpriteSheet, sprite + 1);
    }
    else {
      draw::blit_sprite2(d.screen, x, y, explosionSpriteSheet, sprite + 1);
    }
  }
}

void copy_draw(std::optional<Copy> const& copy, void (*filter)(SDL_Surface *, SDL_Surface *)) {
  if (copy) {
    filter(copy->dst, copy->src);
  }
}

void darken_draw(std::optional<Darkened> const& d) {
  if (d) {
    draw::JE_darkenBackground(d->screen, d->neat);
  }
}

template <typename T, typename Draw>
void draw_if(std::optional<T> const& drawn, Draw draw) {
  if (drawn) {
    draw(*drawn);
  }
}

}  // namespace

void draw_tick(DrawRecord const& r) {
  // The HUD, on its own surface, whose drawing commutes with the playing
  // area's until the playing area is copied onto it at the end.
  for (hud::Draw const& d : r.hud) {
    hud_draw(d);
  }

  // --- BACKGROUNDS ---
  draw_if(r.background_1, background_1_draw);
  if (r.starfield != nullptr) {
    starfield_draw(r.starfield);
  }
  copy_draw(r.iced_blur_1, draw::iced_blur_filter);
  draw_if(r.background_2_under, background_2_draw);
  copy_draw(r.lava_1, draw::lava_filter);
  if (r.water) {
    draw::water_filter(r.water->dst, r.water->src, smoothie_data[1]);
  }

  // -----------------------Ground Enemy------------------------
  draw_if(r.ground_enemies, enemies_draw);
  draw_if(r.ground_enemies_2, enemies_draw);
  copy_draw(r.lava_2, draw::lava_filter);
  darken_draw(r.darken_1);
  draw_if(r.background_2_over_1, background_2_draw);
  darken_draw(r.darken_2);
  draw_if(r.background_3_over_2, background_3_draw);
  copy_draw(r.iced_blur_2, draw::iced_blur_filter);
  copy_draw(r.blur, draw::blur_filter);
  draw_if(r.sky_enemies_under, enemies_draw);
  draw_if(r.background_3_over_0, background_3_draw);
  draw_if(r.top_enemies_under, enemies_draw);

  // Player Shot Images
  draw_if(r.player_shots, player_shots_draw);

  // --------PLAYER DRAW+MOVEMENT---------
  for (PlayerDrawn const& p : r.players) {
    player_draw(p);
  }

  // Draw Enemy Shots
  draw_if(r.enemy_shots, enemy_shots_draw);
  draw_if(r.background_3_over_1, background_3_draw);
  draw_if(r.top_enemies_over, enemies_draw);
  draw_if(r.sky_enemies_over, enemies_draw);

  // ---------------------------- Draw Explosions ----------------------------
  draw_if(r.explosions, explosions_draw);
  draw_if(r.background_2_over_2, background_2_draw);

  // -------------------------Warning---------------------------
  if (r.warning != nullptr) {
    draw::fill_rectangle_xy(r.warning, 24, 181, 138, 183, warningCol);
    draw::fill_rectangle_xy(r.warning, 175, 181, 287, 183, warningCol);
    draw::fill_rectangle_xy(r.warning, 24, 0, 287, 3, warningCol);

    draw::JE_outText(r.warning, 140, 178, "WARNING", 7, (warningCol % 16) / 2);
  }

  // -------      DEbug      ---------
  if (r.debug) {
    draw::JE_outText(r.debug->screen, 30, 70, r.debug->text[0].c_str(), 4, 0);
    draw::JE_outText(r.debug->screen, 30, 80, r.debug->text[1].c_str(), 4, 0);
    draw::JE_outText(r.debug->screen, 30, 90, r.debug->text[2].c_str(), 6, 0);
    draw::JE_outText(r.debug->screen, 45, 175, r.debug->text[3].c_str(), 15, 3);
  }

  if (r.message) {
    draw::JE_outTextAndDarken(r.message->screen, 90, 10, miscText[59], 15, (JE_byte)r.message->flash - 8,
                              FONT_SHAPES);
  }

  // --------  Level Timer    ---------
  if (r.level_timer != nullptr) {
    char buffer[256];
    draw::JE_textShade(r.level_timer, 140, 6, miscText[66], 7, (levelTimerCountdown % 20) / 3, FULL_SHADE);
    std::sprintf(buffer, "%.1f", levelTimerCountdown / 100.0f);
    draw::JE_dString(r.level_timer, 100, 2, buffer, SMALL_FONT_SHAPES);
  }

  // GAME OVER
  if (r.game_over != nullptr) {
    draw::JE_dString(r.game_over, 120, 60, miscText[21], FONT_SHAPES);  // game over
  }

  // Test
  draw_if(r.superpixels, superpixels_draw);

  // Filtration
  draw_if(r.filter, filter_screen_draw);

  draw_if(r.boss_bars, boss_bar_draw);

  if (r.in_game_displays != nullptr) {
    in_game_displays_draw(r.in_game_displays);
  }

  if (!r.show) {
    return;
  }

  // JE_starShowVGA
  if (starShowVGASpecialCode == 1) {
    draw::showFlipped(VGAScreenSeg, game_screen);
  }
  else if (starShowVGASpecialCode == 2 && processorType >= 2) {
    draw::showHeadlight(VGAScreenSeg, game_screen, player[0].x, player[0].y);
  }
  else {
    draw::showNormal(VGAScreenSeg, game_screen);
  }
}

}  // namespace tyrx::port
