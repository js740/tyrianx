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
// The shadow tick (TYRX_SHADOW=1, with TYRX_VCLOCK): checks the C++ level loop
// (port/level.hpp) against the C code, tick by tick.  At the top of each level
// tick, after the C code has run the previous one:
// 1. Saves the C code's state, all of its writable data (gold/memory.hpp).
// 2. Restores the state from the start of the previous tick, and points the
//    screen surfaces at copies of them as they were then.
// 3. Runs the C++ level pass on that state, with the side effects that the C
//    code has already had suppressed: sound, music, input, presenting.
// 4. Compares the hashed game state, the draw list and the pixels drawn with
//    the C code's.
// 5. Restores the state saved in step 1.
//
// Ticks in which the C code loaded sprites or opened a pause or menu screen
// are skipped, and a pass that would do so, or present or poll by other means,
// is abandoned.  A summary goes to stderr at exit, and exits that Tyrian X
// initiates return status 9 if anything differed.
//------------------------------------------------------------------------------

#pragma once

#include "../gold/api.hpp"

#include <stddef.h>
#include <stdint.h>
#include <functional>

namespace tyrx::check {

bool shadow_enabled();

// Reads from a file with `read`, recording the data in C ticks and giving the
// shadow pass the same data (for the fileRead wraps).
size_t shadow_file_read(File *file, void *data, size_t size, size_t (*read)(File *, void *, size_t));

// Polls joysticks with `poll`, recording what it leaves in the joystick array
// in C ticks, and giving the shadow pass the same (for the joystick wraps).
void shadow_joystick_poll(std::function<void()> const& poll);

// Whether the C++ level pass is running in the shadow of a C tick.
bool shadow_active();

// Abandons the shadow pass.
[[noreturn]] void shadow_abort(char const *why);

// Marks the current C tick as one that can't be checked.
void shadow_skip_tick(char const *why);

// Called when a level starts, at the top of each level tick, and when a level
// ends.
void shadow_level_start();
void shadow_level_tick();
void shadow_level_ended();

// Number of ticks whose shadow pass differed from the C tick.
uint64_t shadow_mismatches();

}  // namespace tyrx::check
