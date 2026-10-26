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

// Round-trip test of snapshots (TYRX_ROUNDTRIP=K[,P]): every P ticks (default
// 10*K) outside demos, snapshot the game and input state, run K ticks, restore
// the snapshot, and check that replaying the K ticks gives the same hashes, i.e.
// that snapshots hold everything a tick depends on.

#pragma once

#include <cstdint>
#include <vector>

namespace tyrx::check {

// Whether TYRX_ROUNDTRIP is set (and valid).
bool round_trip_enabled();

// Called when a level starts: cancels a round trip in progress, since its
// snapshot points into the previous level's data.
void round_trip_level_start();

// Called at the top of each level tick.  At the end of a round trip's first
// pass, restores its snapshot and returns the tick to replay from; otherwise
// returns `tick`.
std::uint64_t round_trip_begin_tick(std::uint64_t tick);

// Whether a round trip is in progress, so that each tick's hashes are needed.
bool round_trip_active();

// Records the tick's block hashes on a round trip's first pass, or checks them
// against the recorded ones on the replay.  Returns whether the tick is a
// replay.
bool round_trip_end_tick(std::uint64_t tick, std::vector<std::uint64_t> const& hashes);

// Whether any replay differed or no round trip completed.
bool round_trip_failed();

}  // namespace tyrx::check
