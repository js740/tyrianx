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

// Rewind (TYRX_REWIND=TICKS): a snapshot of each of the last TICKS level ticks.
// While the rewind key is held, each tick drops the newest snapshot and
// restores the one before it; the game then simulates (and draws) one tick from
// there, so every frame steps back one tick.  Input is not restored, so
// releasing the key resumes play with the keys actually held.
//
//   TYRX_REWIND_KEY=NAME    rewind key, as an SDL scancode name (default Backspace)
//   TYRX_REWIND_VERIFY=1    check that each rewind restores exactly the state that
//                           was captured

#pragma once

#include <SDL.h>

namespace tyrx {

// Called when a level starts: drops the snapshots, since they point into the
// previous level's data.
void rewind_level_start();

// Called at the top of each level tick; does nothing in demos and network games.
void rewind_tick();

// The key that rewinds while held.
SDL_Scancode rewind_key();

// Whether a rewind restored a state other than the captured one
// (TYRX_REWIND_VERIFY).
bool rewind_verify_failed();

}  // namespace tyrx
