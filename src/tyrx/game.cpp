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

#include "game.hpp"

#include "c/api.hpp"

namespace tyrx {

namespace {

// Set during static initialization, before main() runs.
[[maybe_unused]] bool const hooks_installed = [] {
  game_hooks.level_initialized = [] {
    if (Validation *v = validation()) {
      v->level_start();
    }
  };
  game_hooks.level_tick_substitute = [] {
    if (Validation *v = validation()) {
      v->level_tick();
    }
    return LEVEL_TICK_NOT_SUBSTITUTED;
  };
  game_hooks.level_ended = [] {
    if (Validation *v = validation()) {
      v->level_ended();
    }
  };
  return true;
}();

}  // namespace

Validation *& validation() {
  static Validation *v = nullptr;
  return v;
}

}  // namespace tyrx
