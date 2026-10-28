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

// The HUD: the gauges and messages on VGAScreenSeg (JE_drawShield,
// JE_drawArmor and so on in varz.c and mainint.c), which are drawn as they
// change, the boss bar (draw_boss_bar in tyrian2.c) and the in-game displays
// (JE_inGameDisplays in mainint.c).

#include "draw.hpp"
#include "tick.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <string>

namespace tyrx::render {

namespace {

// Port of JE_barX.
void bar_x(SDL_Surface *screen, uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t col) {
  draw::fill_rectangle_xy(screen, x1, y1, x2, y1, col + 1);
  draw::fill_rectangle_xy(screen, x1, y1 + 1, x2, y2 - 1, col);
  draw::fill_rectangle_xy(screen, x1, y2, x2, y2, col - 1);
}

// Port of JE_dBar3.
void dbar3(SDL_Surface *surface, int16_t x, int16_t y, int16_t num, int16_t col) {
  uint8_t zWait = 2;

  col += 2;

  for (uint8_t z = 0; z <= num; z++) {
    draw::JE_rectangle(surface, x, y - 1, x + 8, y, col);  // <MXD> SEGa000
    if (zWait > 0) {
      zWait--;
    }
    else {
      col++;
      zWait = 1;
    }
    y -= 2;
  }
}

}  // namespace

void hud_draw(hud::Draw const& draw) {
  SDL_Surface *const screen = VGAScreenSeg;
  struct Visitor {
    SDL_Surface *screen;

    void operator()(hud::MessageBarErased const&) const {
      draw::blit_sprite(screen, 16, 189, OPTION_SHAPES, 36);  // in-game message area
    }

    void operator()(hud::TextWindow const& d) const {
      if (d.erase) {
        draw::blit_sprite(screen, 16, 189, OPTION_SHAPES, 36);  // in-game text area
      }
      draw::JE_outText(screen, 20, 190, d.text.c_str(), 0, 4);
    }

    void operator()(hud::Shield const& d) const {
      if (d.two) {
        for (unsigned i = 0; i < std::size(d.shield); ++i) {
          dbar3(screen, 270, 60 + 134 * i, roundf(d.shield[i] * 0.8f), 144);
        }
      }
      else {
        dbar3(screen, 270, 194, d.shield[0], 144);
        if (d.shield[0] != d.shield_max) {
          unsigned const y = 193 - (d.shield_max * 2);
          draw::JE_rectangle(screen, 270, y, 278, y, 68);  // <MXD> SEGa000
        }
      }
    }

    void operator()(hud::Armor const& d) const {
      if (d.two) {
        for (unsigned i = 0; i < std::size(d.armor); ++i) {
          dbar3(screen, 307, 60 + 134 * i, roundf(d.armor[i] * 0.8f), 224);
        }
      }
      else {
        dbar3(screen, 307, 194, d.armor[0], 224);
      }
    }

    void operator()(hud::WipeShieldArmor const& d) const {
      if (d.one) {
        draw::fill_rectangle_xy(screen, 270, 137, 278, 194 - d.shield * 2, 0);
      }
      else {
        draw::fill_rectangle_xy(screen, 270, 60 - 44, 278, 60, 0);
        draw::fill_rectangle_xy(screen, 270, 194 - 44, 278, 194, 0);
      }
      if (d.one) {
        draw::fill_rectangle_xy(screen, 307, 137, 315, 194 - d.armor * 2, 0);
      }
      else {
        draw::fill_rectangle_xy(screen, 307, 60 - 44, 315, 60, 0);
        draw::fill_rectangle_xy(screen, 307, 194 - 44, 315, 194, 0);
      }
    }

    void operator()(hud::WeaponPower const& d) const {
      int x = d.x;
      draw::fill_rectangle_xy(screen, x, d.y, x + 1 + 10 * 2, d.y + 2, 0);

      for (unsigned j = 1; j <= d.item_power; ++j) {
        draw::JE_rectangle(screen, x, d.y, x + 1, d.y + 2, 115 + j);  // SEGa000
        x += 2;
      }
    }

    void operator()(hud::PowerBar const& d) const {
      if (d.temp > d.lastPower) {
        draw::fill_rectangle_xy(screen, 269, 113 - 11 - d.temp, 276, 114 - 11 - d.lastPower,
                                113 + d.temp / 7);
      }
      else {
        draw::fill_rectangle_xy(screen, 269, 113 - 11 - d.lastPower, 276, 114 - 11 - d.temp, 0);
      }
    }

    void operator()(hud::PortConfig const& d) const {
      if (d.weapon_mode == 1) {
        draw::blit_sprite(screen, 285, 44, OPTION_SHAPES, 18);  // lit
        draw::blit_sprite(screen, 302, 44, OPTION_SHAPES, 19);  // unlit
      }
      // == 2
      else {
        draw::blit_sprite(screen, 285, 44, OPTION_SHAPES, 19);  // unlit
        draw::blit_sprite(screen, 302, 44, OPTION_SHAPES, 18);  // lit
      }
    }

    void operator()(hud::Sidekick const& d) const {
      draw::fill_rectangle_xy(screen, 284, d.y, 284 + 28, d.y + 15, 0);
      if (d.icongr > 0) {
        draw::blit_sprite(screen, 284, d.y, OPTION_SHAPES, d.icongr - 1);  // sidekick HUD icon
      }
      draw::draw_segmented_gauge(screen, 284, d.y + 13, 112, 2, 2, MAX(1, d.ammo_max / 10), d.ammo);
    }

    void operator()(hud::OptionLevel const& d) const {
      for (int temp = 1; temp <= 3; temp++) {
        draw::fill_rectangle_xy(screen, 268, 127 + (temp - 1) * 6, 269, 127 + 3 + (temp - 1) * 6,
                                193 + ((d.sidekick_level - 100) == temp) * 11);
      }
    }

    void operator()(hud::ChargeLevel const& d) const {
      draw::fill_rectangle_xy(screen, 269, 107 + (d.chargeLevel - 1) * 3, 275, 108 + (d.chargeLevel - 1) * 3,
                              d.color);
    }

    void operator()(hud::SidekickAmmo const& d) const {
      if (d.discharged) {
        draw::fill_rectangle_xy(screen, 284, d.y, 312, d.y + 2, 0);
      }
      draw::draw_segmented_gauge(screen, 284, d.y, 112, 2, 2, MAX(1, d.ammo_max / 10), d.ammo);
    }
  };
  std::visit(Visitor{screen}, draw);
}

void boss_bar_draw(BossBarsDrawn const& d) {
  for (unsigned b = 0; b < d.bars; b++) {
    BossBarsDrawn::Bar const& bar = d.bar[b];
    bar_x(surface(d.screen), bar.x - 25, 7, bar.x + 25, 12, 115);
    bar_x(surface(d.screen), bar.x - (bar.armor / 10), 7, bar.x + (bar.armor + 5) / 10, 12, 118 + bar.color);
  }
}

// JE_inGameDisplays: only draws, from state that nothing changes later in the
// tick.
void in_game_displays_draw(sim::State const& state, SDL_Surface *screen) {
  auto const& twoPlayerMode = state.game.two_player_mode;
  auto const& onePlayerAction = state.game.one_player_action;
  auto const& galagaMode = state.game.galaga_mode;
  auto const& youAreCheating = state.cheats.you_are_cheating;
  auto const& player = state.players.slots;
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
      uint16_t x = (temp == 0) ? 30 : 270;  // tempW in the C code

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
  for (unsigned i = 0; i < std::size(player); ++i) {
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

}  // namespace tyrx::render
