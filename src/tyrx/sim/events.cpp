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

// The level's events: a port of JE_eventJump (tyrian2.c).

#include "sim.hpp"

namespace tyrx::sim {

void event_jump(Level& level, JE_word jump) {
  if (jump == 65535) {
    level.cur_loc = level.return_loc;
  }
  else {
    level.return_loc = level.cur_loc + 1;
    level.cur_loc = jump;
  }

  JE_word i = 0;
  do {
    i++;
  } while (!(level.events[i - 1].eventtime >= level.cur_loc));
  level.event_loc = i - 1;
}

}  // namespace tyrx::sim
