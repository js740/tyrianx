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

// The parts of the level tick ported from the C code, as they are: they work
// on the C globals, with the C code's names.  Those that draw are split in
// two: their simulation (X_tick), which runs where the C code runs it and
// records what the drawing needs in the tick's draw record, and their drawing
// (X_draw), which draw_tick runs after the simulation is complete.
//
// The state of the subsystems moved into sim::State is in the State that the
// code running the tick provides (state()) instead of the C globals.  Code
// moves from here to sim/ as the globals it uses move into structures.

#pragma once

#include "../gold/api.hpp"
#include "../sim/hidden.hpp"
#include "../sim/sim.hpp"
#include "../sim/state.hpp"
#include "draw_record.hpp"

#include <iterator>
#include <utility>
#include <vector>

// COUNTOF counts the State's std::arrays too.
#undef COUNTOF
#define COUNTOF(x) std::size(x)

namespace tyrx::port {

// The State the tick works on, set by the code running the tick (the shadow
// tick, check/shadow.cpp).
inline sim::State *& state_target() {
  static sim::State *state = nullptr;
  return state;
}

inline sim::State& state() { return *state_target(); }

// The C globals that sim::State holds are hidden here (../sim/hidden.hpp).
using namespace hidden;

// background.cpp
void background_1_draw(Background1Drawn const& d);  // draw_background_1, or JE_clr256
void background_2_draw(Background2Drawn const& d);  // draw_background_2, or _blend
void background_3_draw(Background3Drawn const& d);  // draw_background_3
void starfield_draw(SDL_Surface *screen);

// effects.cpp
void superpixels_draw(SuperpixelsDrawn const& d);
void filter_screen_draw(FilterDrawn const& d);

// enemy.cpp
void enemies_draw(EnemiesDrawn const& d);

// hud.cpp
void hud_draw(hud::Draw const& draw);
void boss_bar_draw(BossBarsDrawn const& d);
void in_game_displays_draw(SDL_Surface *screen);  // JE_inGameDisplays

// player.cpp
void player_draw(sim::VisiblePlayer const& p);

// shots.cpp
void player_shots_draw(PlayerShotsDrawn const& d);

}  // namespace tyrx::port
