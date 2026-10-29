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

// One pass of the game's level loop in C++: one level tick, from the
// level_loop label in JE_main (tyrian2.c) to the jump back to it, on a
// sim::State and the C code's other globals.  The tick's simulation
// (sim::level_tick) is complete before anything is drawn: it keeps a record
// for the drawing (sim/draw_record.hpp), from which render::draw_tick draws
// the tick into a draw list, and then the frame is presented and events are
// polled.

#pragma once

#include "render/draw_list.hpp"
#include "sim/sim.hpp"

namespace tyrx {

// What runs the passes provides, besides the State and the draw list: a
// notice that the simulation is complete and the drawing begins, presenting
// the frame, polling events, a notice just before main_keyboard_input opens
// the in-game help, and running a C loop that has its own drawing and works
// on the C globals (the pause screen, the in-game menu and help), once the
// tick's drawing so far is in the draw list.
struct Runner {
  virtual void drawing_begins() = 0;
  virtual void present() = 0;
  virtual void poll_events() = 0;
  virtual void in_game_help_ahead() = 0;
  virtual void c_loop(void (*loop)()) = 0;

 protected:
  ~Runner() = default;
};

// Runs one pass on state, with the level loop's own state in locals, adding
// the tick's drawing to list.
sim::LevelPass level_pass(sim::State& state, sim::LevelLocals& locals, render::DrawList& list,
                          Runner& runner);

}  // namespace tyrx
