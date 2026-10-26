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

// Port of JE_setupExplosion (varz.c).

#include "sim.hpp"

#include <array>

namespace tyrx::sim {

namespace {

struct ExplosionData {
  JE_word sprite;
  JE_byte ttl;
};

// Indexed directly by explosion type (types are effectively 0-based here,
// despite the original's [1..53] comment).
// clang-format off
constexpr std::array<ExplosionData, 53> kExplosionData = {{
    {144, 7},  {120, 12}, {190, 12}, {209, 12}, {152, 12}, {171, 12},
    {133, 7},  // white smoke
    {1, 12},   {20, 12},  {39, 12},  {58, 12},  {110, 3},  {76, 7},   {91, 3},
    {227, 3},  {230, 3},  {233, 3},  {252, 3},  {246, 3},  {249, 3},  {265, 3},
    {268, 3},  {271, 3},  {236, 3},  {239, 3},  {242, 3},  {261, 3},  {274, 3},
    {277, 3},  {280, 3},  {299, 3},  {284, 3},  {287, 3},  {290, 3},  {293, 3},
    {165, 8},  // coin values
    {184, 8},  {203, 8},  {222, 8},  {168, 8},  {187, 8},  {206, 8},  {225, 10},
    {169, 10}, {188, 10}, {207, 20}, {226, 14}, {170, 14}, {189, 14}, {208, 14},
    {246, 14}, {227, 14}, {265, 14},
}};
// clang-format on

}  // namespace

void setup_explosion(std::span<Explosion, MAX_EXPLOSIONS> explosions, JE_integer x, JE_integer y,
                     JE_integer delta_y, JE_integer type, bool fixed_position, bool follow_player) {
  if (y <= -16 || y >= 190) {
    return;
  }

  for (Explosion& e : explosions) {
    if (e.ttl != 0) {
      continue;
    }

    e.x = x;
    e.y = y;
    if (type == 6) {
      e.y += 12;
      e.x += 2;
    }
    else if (type == 98) {
      type = 6;  // like white smoke, but without the offset
    }
    e.sprite = kExplosionData[type].sprite;
    e.ttl = kExplosionData[type].ttl;
    e.followPlayer = follow_player;
    e.fixedPosition = fixed_position;
    e.deltaY = delta_y;
    return;
  }
}

}  // namespace tyrx::sim
