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
// The game's level loop in C++.  level_pass runs one pass of it: one level
// tick, from the level_loop label in JE_main (tyrian2.c) to the jump back to
// it, on the C++ game's state (sim::State) and the C code's other globals.  The
// tick's simulation is complete before anything is drawn: it keeps a record
// for the drawing (draw_record.hpp), from which draw_tick then draws the tick,
// and then the frame is presented and events are polled.  It is checked
// against the C code by running it in the shadow of each C tick
// (check/shadow.hpp).
//------------------------------------------------------------------------------

#pragma once

namespace tyrx::port {

// How a pass ended: like the C code, back to level_loop for the next tick, or
// to start_level to end the level; or not at all, for network games.
enum class LevelPass { level_loop, start_level, unsupported };

LevelPass level_pass();

// State the C code keeps in JE_main's local variables, which outlive a pass.
struct LevelLocals {
  unsigned old_weapon_bar[2] = {0, 0};  // weapon power bars are only redrawn when they change
};

LevelLocals& level_locals();

// What runs the passes (the shadow tick, check/shadow.cpp) provides, with the
// State and the draw list: a notice that the simulation is complete and the drawing begins,
// presenting the frame, polling events, a notice just before
// main_keyboard_input opens the in-game help, and running a C loop that has
// its own drawing and works on the C globals (the pause screen, the in-game
// menu and help).
struct Runner {
  virtual void drawing_begins() = 0;
  virtual void present() = 0;
  virtual void poll_events() = 0;
  virtual void in_game_help_ahead() = 0;
  virtual void c_loop(void (*loop)()) = 0;

 protected:
  ~Runner() = default;
};

inline Runner *& runner_target() {
  static Runner *runner = nullptr;
  return runner;
}

inline void drawing_begins() { runner_target()->drawing_begins(); }
inline void present() { runner_target()->present(); }
inline void poll_events() { runner_target()->poll_events(); }
inline void in_game_help_ahead() { runner_target()->in_game_help_ahead(); }
inline void c_loop(void (*loop)()) { runner_target()->c_loop(loop); }

}  // namespace tyrx::port
