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

// The input of each level tick, recorded so that the ticks can be replayed on
// it.  A level tick reads its input as:
// - the globals that event polling sets (keysactive, the mouse buttons and
//   position, focus, ...) and the joystick state: the input class of the state
//   registry (registry.inc), recorded at the top of the tick;
// - whether the input queues hold anything (keyboardHasInput, hasInput),
//   recorded at the top of the tick too, since the tick only clears them
//   (keyboardClearInput, mouseClearInput) and doesn't read their events;
// - the mouse's relative motion, which reading resets
//   (mouseGetRelativePosition), as the tick reads it; and
// - joystick polls (poll_joystick, push_joysticks_as_keyboard), which read the
//   joysticks themselves, as the tick polls them.
// The queues and the mouse's motion stay private to keyboard.c: the calls are
// wrapped at link time.
//
// A replay puts each tick's recorded input in place and answers its reads from
// the recording, with event polling and synthetic input suppressed, so the
// live input (the queues, SDL's events, the virtual joystick) is left as it
// was when the replay started, and is put back when it ends.  Round trips
// (roundtrip.hpp) replay their ticks this way.

#pragma once

#include <stdint.h>

#include <functional>

namespace tyrx::validate {

// Starts recording from the tick about to run, dropping any earlier
// recording.
void input_record_start();

// Called at the top of each level tick: records the input at the tick
// boundary, or while replaying, puts the recorded input of the tick in place.
void input_record_tick();

// Starts replaying the recording from its first tick.  The live input at this
// tick boundary is kept, to be put back when the replay stops.
void input_record_replay();

// Stops recording or replaying; after a replay, puts the live input back.
void input_record_stop();

// Whether a replay is running: event polling and synthetic input are
// suppressed.
bool input_record_replaying();

// A joystick poll (poll_joystick or push_joysticks_as_keyboard), which `poll`
// makes: recorded, or while replaying, answered from the recording.
void input_record_joystick_poll(std::function<void()> const& poll);

// Reads that replays made beyond what the recording had: a replay that read
// its input differently from the recorded ticks.
uint64_t input_record_misses();

}  // namespace tyrx::validate
