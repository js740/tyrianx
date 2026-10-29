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

// The C++ game.  It runs each pass of the game's level loop (level.hpp) in
// place of the C code's, through the game's level_tick_substitute hook
// (../hooks.h), on a sim::State that it owns during the level: it reads the
// State from the C globals when the level starts and after C code that works
// on them has run (the pause screen, the in-game menu and help), and writes it
// back after each pass, for the C code that runs between ticks and after the
// level.  Network games' passes run in C.  It rewinds with TYRX_REWIND
// (rewind.hpp).

#pragma once

#include "sim/state.hpp"

namespace tyrx {

// What the validation of the C++ game against the C game (validate/, in
// builds with TYRX_VALIDATE=on) adds to the game, which it sets during static
// initialization.  It is null in other builds.
struct Validation {
  // Whether the C++ game runs the levels in place of the C code (it does in
  // other builds).
  virtual bool cxx_game() = 0;

  // The game's hooks (../hooks.h): level_start after the C++ game's own,
  // level_tick in level_tick_substitute, before the C++ game substitutes for
  // the tick.
  virtual void level_start() = 0;
  virtual void level_tick() = 0;
  virtual void level_ended() = 0;

  // A pass of the C++ game is about to run on state, which it has read from
  // the C globals, or has ended.  A pass ends for a C loop too, and begins
  // again after it.
  virtual void pass_begins(sim::State const& state) = 0;
  virtual void pass_ends() = 0;

 protected:
  ~Validation() = default;
};

Validation *& validation();

// The C globals have been restored to an earlier tick's state (by the
// validation's round trips): the next pass reads the State from them.
void game_c_state_restored();

}  // namespace tyrx
