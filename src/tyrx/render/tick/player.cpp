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

// The players' drawing (JE_playerMovement's, in mainint.c, and
// JE_doSpecialShot's, in varz.c).  Their update is in ../sim/player.cpp and
// ../sim/special.cpp: the draw record gets the values that each part is drawn
// with where the update goes on to change them (positions, animation frames
// and charge).

#include "draw.hpp"
#include "tick.hpp"

namespace tyrx::render {

namespace {

// The ship as drawn: in the fade-out at the end of a level, its trail.
void draw_ship_trail(SDL_Surface *screen, JE_word shipGr_, Sprite2_array *shipGrPtr_, int x, int trail_y) {
  if (shipGr_ == 0) {
    draw::blit_sprite2x2(screen, x - 17, trail_y - 7, *shipGrPtr_, 13);
    draw::blit_sprite2x2(screen, x + 7, trail_y - 7, *shipGrPtr_, 51);
  }
  else if (shipGr_ == 1) {
    draw::blit_sprite2x2(screen, x - 17, trail_y - 7, *shipGrPtr_, 220);
    draw::blit_sprite2x2(screen, x + 7, trail_y - 7, *shipGrPtr_, 222);
  }
  else {
    draw::blit_sprite2x2(screen, x - 5, trail_y - 7, *shipGrPtr_, shipGr_);
  }
}

void draw_ship(SDL_Surface *screen, JE_word shipGr_, Sprite2_array *shipGrPtr_,
               sim::VisiblePlayer::Ship const& d) {
  int const x = d.x, y = d.y;
  int const ship_sprite = d.sprite;
  JE_word const mapX2Ofs = d.map_x2_ofs;

  if (shipGr_ == 0) {
    if (d.background2) {
      draw::blit_sprite2x2_darken(screen, x - 17 - mapX2Ofs + 30, y - 7 + shadowYDist, *shipGrPtr_,
                                  ship_sprite + 13);
      draw::blit_sprite2x2_darken(screen, x + 7 - mapX2Ofs + 30, y - 7 + shadowYDist, *shipGrPtr_,
                                  ship_sprite + 51);
      if (d.super_wild) {
        draw::blit_sprite2x2_darken(screen, x - 16 - mapX2Ofs + 30, y - 7 + shadowYDist, *shipGrPtr_,
                                    ship_sprite + 13);
        draw::blit_sprite2x2_darken(screen, x + 6 - mapX2Ofs + 30, y - 7 + shadowYDist, *shipGrPtr_,
                                    ship_sprite + 51);
      }
    }
  }
  else if (shipGr_ == 1) {
    if (d.background2) {
      draw::blit_sprite2x2_darken(screen, x - 17 - mapX2Ofs + 30, y - 7 + shadowYDist, *shipGrPtr_, 220);
      draw::blit_sprite2x2_darken(screen, x + 7 - mapX2Ofs + 30, y - 7 + shadowYDist, *shipGrPtr_, 222);
    }
  }
  else {
    if (d.background2) {
      draw::blit_sprite2x2_darken(screen, x - 5 - mapX2Ofs + 30, y - 7 + shadowYDist, *shipGrPtr_,
                                  ship_sprite);
      if (d.super_wild) {
        draw::blit_sprite2x2_darken(screen, x - 4 - mapX2Ofs + 30, y - 7 + shadowYDist, *shipGrPtr_,
                                    ship_sprite);
      }
    }
  }

  if (d.invulnerable) {
    if (shipGr_ == 0) {
      draw::blit_sprite2x2_blend(screen, x - 17, y - 7, *shipGrPtr_, ship_sprite + 13);
      draw::blit_sprite2x2_blend(screen, x + 7, y - 7, *shipGrPtr_, ship_sprite + 51);
    }
    else if (shipGr_ == 1) {
      draw::blit_sprite2x2_blend(screen, x - 17, y - 7, *shipGrPtr_, 220);
      draw::blit_sprite2x2_blend(screen, x + 7, y - 7, *shipGrPtr_, 222);
    }
    else {
      draw::blit_sprite2x2_blend(screen, x - 5, y - 7, *shipGrPtr_, ship_sprite);
    }
  }
  else {
    if (shipGr_ == 0) {
      draw::blit_sprite2x2(screen, x - 17, y - 7, *shipGrPtr_, ship_sprite + 13);
      draw::blit_sprite2x2(screen, x + 7, y - 7, *shipGrPtr_, ship_sprite + 51);
    }
    else if (shipGr_ == 1) {
      draw::blit_sprite2x2(screen, x - 17, y - 7, *shipGrPtr_, 220);
      draw::blit_sprite2x2(screen, x + 7, y - 7, *shipGrPtr_, 222);

      switch (ship_sprite) {
        case 5:
          draw::blit_sprite2(screen, x - 17, y + 7, *shipGrPtr_, 40);
          break;
        case 3:
          draw::blit_sprite2(screen, x - 17, y + 7, *shipGrPtr_, 39);
          break;
        case -1:
          draw::blit_sprite2(screen, x + 19, y + 7, *shipGrPtr_, 58);
          break;
        case -3:
          draw::blit_sprite2(screen, x + 19, y + 7, *shipGrPtr_, 59);
          break;
      }
    }
    else {
      draw::blit_sprite2x2(screen, x - 5, y - 7, *shipGrPtr_, ship_sprite);
    }
  }
}

}  // namespace

// The player's parts, in the order JE_playerMovement draws them.
void player_draw(sim::VisiblePlayer const& p) {
  if (p.trail) {
    for (int trail_y : p.trail->trail_y) {
      draw_ship_trail(surface(p.trail->screen), p.ship_gr, p.ship_gr_ptr, p.trail->x, trail_y);
    }
  }
  if (p.insert_coin) {
    draw::JE_dString(surface(p.insert_coin->screen), 115, 10, miscText[7],
                     SMALL_FONT_SHAPES);  // insert coin
  }
  if (p.ship) {
    draw_ship(surface(p.ship->screen), p.ship_gr, p.ship_gr_ptr, *p.ship);
  }
  if (p.special_icon) {
    draw::blit_sprite2(surface(p.special_icon->screen), 47, 4, spriteSheet9, p.special_icon->ready ? 94 : 93);
  }
  if (p.zinglon) {
    SDL_Surface *const screen = surface(p.zinglon->screen);
    int const x = p.zinglon->x, temp = p.zinglon->width;
    draw::JE_barBright(screen, x + 7 - temp, 0, x + 7 + temp, 184);
    draw::JE_barBright(screen, x + 7 - temp - 2, 0, x + 7 + temp + 2, 184);
  }
  if (p.charge) {
    draw::blit_sprite2(surface(p.charge->screen), p.charge->x, p.charge->y, spriteSheet10, p.charge->sprite);
  }
  for (std::optional<sim::VisiblePlayer::Sidekick> const& sidekick : p.sidekicks) {
    if (!sidekick) {
      continue;
    }

    SDL_Surface *const screen = surface(sidekick->screen);
    if (sidekick->style == 1 || sidekick->style == 2) {
      draw::blit_sprite2x2(screen, sidekick->x - 6, sidekick->y, spriteSheet10, sidekick->sprite);
    }
    else {
      draw::blit_sprite2(screen, sidekick->x, sidekick->y, spriteSheet9, sidekick->sprite);
    }
  }
}

}  // namespace tyrx::render
