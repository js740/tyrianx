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
// The per-tick state hash log and the state dump:
//
//   TYRX_HASHLOG=FILE       write per-tick state hashes to FILE
//   TYRX_DUMP_TICK=N        dump the full hashed state at tick N ...
//   TYRX_DUMP=FILE          ... to FILE (default: tyrx-dump-N.txt)
//
// tools/compare_hashlogs.py compares two hash logs.
//------------------------------------------------------------------------------

#pragma once

#include <stdint.h>
#include <vector>

namespace tyrx::validate {

// Whether the hash log is being written, so each tick's hashes are needed.
bool hashlog_enabled();

// Logs the start of a level, `level` counting from 1, at `tick`.
void hashlog_level_start(unsigned level, uint64_t tick);

// Logs a tick's block hashes (block_hashes), and dumps the state if it is
// the tick to dump.
void hashlog_tick(uint64_t tick, std::vector<uint64_t> const& hashes);

}  // namespace tyrx::validate
