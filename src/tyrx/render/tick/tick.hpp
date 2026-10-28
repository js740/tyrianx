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

// The drawing of a level tick, after its simulation (../../sim/level.cpp):
// the draw calls of the C code's level loop, in its order, from the game state
// at the end of the tick and the draw record (../../sim/draw_record.hpp), added
// to a draw list (draw.hpp).

#pragma once

#include "../../gold/api.hpp"
#include "../../sim/sim.hpp"

#include <cassert>

namespace tyrx::render {

// The C globals that sim::State holds are hidden here (../../sim/hidden.hpp).
using namespace hidden;

namespace hud = sim::hud;
using sim::Background1Drawn, sim::Background2Drawn, sim::Background3Drawn, sim::BossBarsDrawn, sim::Copy,
    sim::Darkened, sim::DebugDrawn, sim::DrawRecord, sim::EnemiesDrawn, sim::EnemyShotsDrawn,
    sim::ExplosionsDrawn, sim::FilterDrawn, sim::MessageDrawn, sim::PlayerShotsDrawn, sim::SuperpixelsDrawn,
    sim::Surface;

// The surface that the record names.
inline SDL_Surface *surface(Surface s) {
  switch (s) {
    case Surface::vga_screen_seg:
      return VGAScreenSeg;
    case Surface::game_screen:
      return game_screen;
    case Surface::vga_screen2:
      return VGAScreen2;
  }
  return nullptr;
}

// The record's name for a surface that VGAScreen points at.
inline Surface record_surface(SDL_Surface *s) {
  if (s == game_screen) {
    return Surface::game_screen;
  }
  if (s == VGAScreen2) {
    return Surface::vga_screen2;
  }
  assert(s == VGAScreenSeg);
  return Surface::vga_screen_seg;
}

// Draws the tick, after its simulation, from the state at its end.
void draw_tick(sim::State const& state, DrawRecord const& r);

// The parts' drawing, which draw_tick runs in the C code's order.

// background.cpp
void background_1_draw(Background1Drawn const& d);  // draw_background_1, or JE_clr256
void background_2_draw(Background2Drawn const& d);  // draw_background_2, or _blend
void background_3_draw(Background3Drawn const& d);  // draw_background_3
void starfield_draw(sim::State const& state, SDL_Surface *screen);

// effects.cpp
void superpixels_draw(sim::State const& state, SuperpixelsDrawn const& d);
void filter_screen_draw(sim::State const& state, FilterDrawn const& d);

// enemy.cpp
void enemies_draw(EnemiesDrawn const& d);

// hud.cpp
void hud_draw(hud::Draw const& draw);
void boss_bar_draw(BossBarsDrawn const& d);
void in_game_displays_draw(sim::State const& state, SDL_Surface *screen);  // JE_inGameDisplays

// player.cpp
void player_draw(sim::VisiblePlayer const& p);

// shots.cpp
void player_shots_draw(PlayerShotsDrawn const& d);

}  // namespace tyrx::render
