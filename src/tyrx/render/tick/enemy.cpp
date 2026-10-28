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

// The enemies' drawing (JE_drawEnemy's, in tyrian2.c).  Their update, which
// JE_drawEnemy does too, is in ../sim/enemy.cpp: each enemy is drawn, then
// moves, fires and launches enemies.  Nearly everything it was drawn with can
// change later in the tick: its position, its frame and filter (when hit)
// and, if it goes or is destroyed, its slot.  So the draw record gets each
// drawn enemy as the update drew it.

#include "draw.hpp"
#include "tick.hpp"

namespace tyrx::render {

namespace {

// Port of blit_enemy.
void blit_enemy(SDL_Surface *surface, sim::VisibleEnemy const& e, JE_word mapXOfs, signed int x_offset,
                signed int y_offset, signed int sprite_offset) {
  if (e.sprite2s == nullptr) {
    return;
  }

  int const x = e.x + x_offset + mapXOfs, y = e.y + y_offset;
  unsigned int const index = e.sprite + sprite_offset;

  if (e.filter != 0) {
    draw::blit_sprite2_filter(surface, x, y, *e.sprite2s, index, e.filter);
  }
  else {
    draw::blit_sprite2(surface, x, y, *e.sprite2s, index);
  }
}

}  // namespace

void enemies_draw(EnemiesDrawn const& d) {
  SDL_Surface *const screen = surface(d.screen);
  JE_word const mapXOfs = d.mapXOfs;
  for (sim::VisibleEnemy const& e : d.drawn) {
    // 2x2 enemy
    if (e.size == 1) {
      if (e.y > -13) {
        blit_enemy(screen, e, mapXOfs, -6, -7, 0);
        blit_enemy(screen, e, mapXOfs, 6, -7, 1);
      }
      if (e.y > -26 && e.y < 182) {
        blit_enemy(screen, e, mapXOfs, -6, 7, 19);
        blit_enemy(screen, e, mapXOfs, 6, 7, 20);
      }
    }
    else {
      if (e.y > -13) {
        blit_enemy(screen, e, mapXOfs, 0, 0, 0);
      }
    }
  }
}

}  // namespace tyrx::render
