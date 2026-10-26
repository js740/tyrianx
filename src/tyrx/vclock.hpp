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

// Virtual clock (TYRX_VCLOCK=1), which replaces wall-clock time so that runs are
// fast and deterministic: SDL_Delay advances it instantly, and time() starts at
// 2000-01-01 00:00:00 UTC.

#pragma once

#include <SDL.h>

namespace tyrx {

// Whether TYRX_VCLOCK is set.
bool vclock_enabled();

// Milliseconds since startup on the virtual clock.
Uint32 vclock_now();

}  // namespace tyrx
