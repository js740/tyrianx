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
// The frame log, for comparing what two runs show:
//
//   TYRX_FRAMELOG=FILE      write a line per frame presented (JE_showVGA) to
//                           FILE: the tick, and a hash of the screen's pixels
//                           and the palette
//------------------------------------------------------------------------------

#pragma once

namespace tyrx::validate {

// Logs the frame that JE_showVGA is about to present.
void framelog_frame();

}  // namespace tyrx::validate
