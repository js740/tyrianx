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

// Access to the OpenTyrian C code from C++.  The C headers are C++-clean, so
// they only need C linkage.

#pragma once

extern "C" {
#include "../backgrnd.h"
#include "../config.h"
#include "../demo.h"
#include "../editship.h"
#include "../episodes.h"
#include "../fonthand.h"
#include "../game_menu.h"
#include "../hooks.h"
#include "../joystick.h"
#include "../keyboard.h"
#include "../lvllib.h"
#include "../mainint.h"
#include "../mtrand.h"
#include "../musmast.h"
#include "../network.h"
#include "../nortsong.h"
#include "../params.h"
#include "../player.h"
#include "../shots.h"
#include "../sprite.h"
#include "../tyrian2.h"
#include "../varz.h"
#include "../xmas.h"
}
