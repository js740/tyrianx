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

// Runtime support shared by the hooks and the debugging tools.

#pragma once

#include <cstdint>

namespace tyrx {

// The value of environment variable `name`, or nullptr if it is unset or empty.
char const *env(char const *name);

// The value of environment variable `name` as a number, or `fallback` if it is
// unset or empty.
long env_long(char const *name, long fallback);

// The number of level ticks run so far, i.e. the index of the next one.
std::uint64_t current_tick();

}  // namespace tyrx
