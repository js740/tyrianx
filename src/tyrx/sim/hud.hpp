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

// The HUD: the gauges and messages that the C code draws on VGAScreenSeg as
// they change, wherever that happens in the tick, so they overlap.  The game
// logic logs them in order (HudLog), and the tick's drawing draws the log.

#pragma once

#include "../gold/api.hpp"

#include <array>
#include <string>
#include <variant>
#include <vector>

namespace tyrx::sim {

// The HUD's changes, as the C code draws them.
namespace hud {

struct MessageBarErased {};  // the message timed out
struct TextWindow {          // JE_drawTextWindow
  bool erase;                // a message was showing
  std::string text;
};
struct Shield {  // JE_drawShield
  bool two;      // two-player HUD
  std::array<uint, 2> shield;
  uint shield_max;
};
struct Armor {  // JE_drawArmor
  bool two;
  std::array<uint, 2> armor;
};
struct WipeShieldArmor {  // JE_wipeShieldArmorBars
  bool one;               // one-player HUD
  uint shield, armor;
};
struct WeaponPower {
  int x, y;
  uint item_power;
};
struct PowerBar {
  JE_byte temp;    // power / 10
  uint lastPower;  // before
};
struct PortConfig {  // JE_drawPortConfigButtons
  uint weapon_mode;
};
struct Sidekick {  // JE_drawOptions, for one sidekick
  int y;
  JE_byte icongr;
  int ammo_max, ammo;
};
struct OptionLevel {  // JE_drawOptionLevel
  Uint8 sidekick_level;
};
struct ChargeLevel {
  JE_byte chargeLevel, color;
};
struct SidekickAmmo {
  int y, ammo_max, ammo;
  bool discharged;  // after firing; otherwise after a refill
};

using Draw = std::variant<MessageBarErased, TextWindow, Shield, Armor, WipeShieldArmor, WeaponPower, PowerBar,
                          PortConfig, Sidekick, OptionLevel, ChargeLevel, SidekickAmmo>;

}  // namespace hud

// The HUD's changes in a tick, in order.
struct HudLog {
  std::vector<hud::Draw> draws;

  // The C code draws the shields and armor with VGAScreen set to the HUD's
  // surface, and leaves it at game_screen after (its "side-effect of
  // game_screen"), where the rest of the tick then draws.  The game logic
  // sets this where the C code does that, and the code running it applies it.
  bool game_screen = false;
};

}  // namespace tyrx::sim
