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
// The draw record of a level tick, which the simulation keeps
// (../sim/draw_record.hpp), and the surfaces it names.
//------------------------------------------------------------------------------

#pragma once

#include "../gold/api.hpp"
#include "../sim/draw_record.hpp"

namespace tyrx::port {

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

// The name of a surface that VGAScreen points at.
inline Surface surface_name(SDL_Surface *s) {
  if (s == game_screen) {
    return Surface::game_screen;
  }
  if (s == VGAScreen2) {
    return Surface::vga_screen2;
  }
  assert(s == VGAScreenSeg);
  return Surface::vga_screen_seg;
}

// The record of the tick being simulated, or last simulated.
DrawRecord& draw_record();

// Draws the tick, after its simulation.
void draw_tick(DrawRecord const& r);

}  // namespace tyrx::port
