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

#include "../c/api.hpp"

#include <string>
#include <variant>
#include <vector>

namespace tyrx::sim {

// The HUD's changes, as the C code draws them.
namespace hud {

struct MessageBarErased {};  // the message timed out
struct TextWindow {          // JE_drawTextWindow
  bool        erase;         // a message was showing
  std::string text;
};
struct Shield {  // JE_drawShield
  bool     two;  // two-player HUD
  uint32_t shield[2];
  uint32_t shield_max;
};
struct Armor {  // JE_drawArmor
  bool     two;
  uint32_t armor[2];
};
struct WipeShieldArmor {  // JE_wipeShieldArmorBars
  bool     one;           // one-player HUD
  uint32_t shield;
  uint32_t armor;
};
struct WeaponPower {
  int32_t  x;
  int32_t  y;
  uint32_t item_power;
};
struct PowerBar {
  uint8_t  temp;       // power / 10
  uint32_t lastPower;  // before
};
struct PortConfig {  // JE_drawPortConfigButtons
  uint32_t weapon_mode;
};
struct Sidekick {  // JE_drawOptions, for one sidekick
  int32_t y;
  uint8_t icongr;
  int32_t ammo_max;
  int32_t ammo;
};
struct OptionLevel {  // JE_drawOptionLevel
  uint8_t sidekick_level;
};
struct ChargeLevel {
  uint8_t chargeLevel;
  uint8_t color;
};
struct SidekickAmmo {
  int32_t y;
  int32_t ammo_max;
  int32_t ammo;
  bool    discharged;  // after firing; otherwise after a refill
};

using Draw = std::variant<MessageBarErased, TextWindow, Shield, Armor, WipeShieldArmor, WeaponPower, PowerBar,
                          PortConfig, Sidekick, OptionLevel, ChargeLevel, SidekickAmmo>;

}  // namespace hud

// The HUD's changes in a tick, in order.
struct HudLog {
  std::vector<hud::Draw> draws;
};

}  // namespace tyrx::sim
