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

// The boss bar (draw_boss_bar in tyrian2.c) and the in-game displays
// (JE_inGameDisplays in mainint.c), which the level tick draws at its end.

#include "draw.hpp"
#include "port.hpp"

#include <cstdio>
#include <cstring>

namespace tyrx::port {

namespace {

// Port of JE_barX.
void bar_x(SDL_Surface *screen, JE_word x1, JE_word y1, JE_word x2, JE_word y2, JE_byte col) {
  draw::fill_rectangle_xy(screen, x1, y1, x2, y1, col + 1);
  draw::fill_rectangle_xy(screen, x1, y1 + 1, x2, y2 - 1, col);
  draw::fill_rectangle_xy(screen, x1, y2, x2, y2, col - 1);
}

}  // namespace

// Port of draw_boss_bar: the bars follow the most damaged enemy of their link
// group, and flash after a hit.
void draw_boss_bars() {
  for (unsigned b = 0; b < COUNTOF(boss_bar); b++) {
    if (boss_bar[b].link_num == 0) {
      continue;
    }
    unsigned armor = 256;  // higher than armor max
    // find most damaged
    for (unsigned e = 0; e < COUNTOF(enemy); e++) {
      if (enemyAvail[e] == 1 || enemy[e].linknum != boss_bar[b].link_num) {
        continue;
      }

      if (enemy[e].armorleft < armor) {
        armor = enemy[e].armorleft;
      }
    }
    // boss dead?
    if (armor > 255 || armor == 0) {
      boss_bar[b].link_num = 0;
    }
    else {
      boss_bar[b].armor = (armor == 255) ? 254 : armor;  // 255 would make the bar too long
    }
  }

  unsigned const bars = (boss_bar[0].link_num != 0 ? 1 : 0) + (boss_bar[1].link_num != 0 ? 1 : 0);

  // if only one bar left, make it the first one
  if (bars == 1 && boss_bar[0].link_num == 0) {
    std::memcpy(&boss_bar[0], &boss_bar[1], sizeof(boss_bar_t));
    boss_bar[1].link_num = 0;
  }

  for (unsigned b = 0; b < bars; b++) {
    unsigned const x = (bars == 2) ? ((b == 0) ? 125 : 185)
                                   : ((levelTimer) ? 250 : 155);  // level timer and boss bar would overlap
    bar_x(VGAScreen, x - 25, 7, x + 25, 12, 115);
    bar_x(VGAScreen, x - (boss_bar[b].armor / 10), 7, x + (boss_bar[b].armor + 5) / 10, 12,
          118 + boss_bar[b].color);
    if (boss_bar[b].color > 0) {
      boss_bar[b].color--;
    }
  }
}

// Port of JE_inGameDisplays, which only draws.
void in_game_displays(SDL_Surface *screen) {
  char stemp[21];
  char tempstr[256];

  for (unsigned i = 0; i < ((twoPlayerMode && !galagaMode) ? 2 : 1); ++i) {
    std::snprintf(tempstr, sizeof(tempstr), "%lu", player[i].cash);
    draw::JE_textShade(screen, 30 + 200 * i, 175, tempstr, 2, 4, FULL_SHADE);
  }

  // Special Weapon?
  if (player[0].items.special > 0) {
    draw::blit_sprite2x2(screen, 25, 1, spriteSheet10, special[player[0].items.special].itemgraphic);
  }

  // Lives Left
  if (onePlayerAction || twoPlayerMode) {
    for (int temp = 0; temp < (onePlayerAction ? 1 : 2); temp++) {
      unsigned const extra_lives = *player[temp].lives - 1;

      int const y = (temp == 0 && player[0].items.special > 0) ? 35 : 15;
      JE_word x = (temp == 0) ? 30 : 270;  // tempW in the C code

      if (extra_lives >= 5) {
        draw::blit_sprite2(screen, x, y, spriteSheet9, 285);
        x = (temp == 0) ? 45 : 250;
        std::sprintf(tempstr, "%d", extra_lives);
        draw::JE_textShade(screen, x, y + 3, tempstr, 15, 1, FULL_SHADE);
      }
      else if (extra_lives >= 1) {
        for (unsigned i = 0; i < extra_lives; ++i) {
          draw::blit_sprite2(screen, x, y, spriteSheet9, 285);
          x += (temp == 0) ? 12 : -12;
        }
      }

      std::strcpy(stemp, (temp == 0) ? miscText[49 - 1] : miscText[50 - 1]);
      if (isNetworkGame) {
        std::strcpy(stemp, JE_getName(temp + 1));
      }

      x = (temp == 0) ? 28 : (285 - JE_textWidth(stemp, TINY_FONT));
      draw::JE_textShade(screen, x, y - 7, stemp, 2, 6, FULL_SHADE);
    }
  }

  // Super Bombs!!
  for (unsigned i = 0; i < COUNTOF(player); ++i) {
    int x = (i == 0) ? 30 : 270;
    for (unsigned j = player[i].superbombs; j > 0; --j) {
      draw::blit_sprite2(screen, x, 160, spriteSheet9, 304);
      x += (i == 0) ? 12 : -12;
    }
  }

  if (youAreCheating) {
    draw::JE_outText(screen, 90, 170, "Cheaters always prosper.", 3, 4);
  }
}

}  // namespace tyrx::port
