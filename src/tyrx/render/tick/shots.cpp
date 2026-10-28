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

// The player shots' drawing (player_shot_move_and_draw's, in shots.c).  Their
// update is in ../sim/player_shot.cpp.  A shot that hits can be freed, and
// its slot taken by a chain-reaction shot later in the same loop, so the draw
// record gets each drawn shot's position and frame, and whether it has a
// shadow.

#include "draw.hpp"
#include "tick.hpp"

namespace tyrx::render {

void player_shots_draw(PlayerShotsDrawn const& d) {
  SDL_Surface *const screen = surface(d.screen);
  for (sim::VisiblePlayerShot const& shot : d.drawn) {
    uint16_t const sprite_frame = shot.sprite_frame;
    int const      x = shot.x + 1, y = shot.y;
    if (sprite_frame > 60000) {
      draw::blit_sprite_blend(screen, x, y, OPTION_SHAPES, sprite_frame - 60001);
    }
    else if (sprite_frame > 500) {
      if (shot.shadow) {
        draw::blit_sprite2_darken(screen, x, y + shadowYDist, spriteSheet12, sprite_frame - 500);
      }
      draw::blit_sprite2(screen, x, y, spriteSheet12, sprite_frame - 500);
    }
    else {
      if (shot.shadow) {
        draw::blit_sprite2_darken(screen, x, y + shadowYDist, spriteSheet8, sprite_frame);
      }
      draw::blit_sprite2(screen, x, y, spriteSheet8, sprite_frame);
    }
  }
}

}  // namespace tyrx::render
