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

// The C++ game, which attaches to the C code through the game's hooks
// (../hooks.h).

#pragma once

namespace tyrx {

// What the validation of the C++ game against the C game (validate/, in
// builds with TYRX_VALIDATE=on) adds to the game, which it sets during static
// initialization.  It is null in other builds.
struct Validation {
  // The game's hooks (../hooks.h).
  virtual void level_start() = 0;
  virtual void level_tick() = 0;
  virtual void level_ended() = 0;

 protected:
  ~Validation() = default;
};

Validation *& validation();

}  // namespace tyrx
