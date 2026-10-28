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

// The explosions: ports of JE_setupExplosion and JE_setupExplosionLarge
// (varz.c), and of the explosions' updates in JE_main's level loop
// (tyrian2.c).

#include "sim.hpp"

#include <array>

namespace tyrx::sim {

namespace {

struct ExplosionData {
  uint16_t sprite;
  uint8_t ttl;
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

void setup_explosion(std::span<Explosion, MAX_EXPLOSIONS> explosions, int16_t x, int16_t y, int16_t delta_y,
                     int16_t type, bool fixed_position, bool follow_player) {
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

void setup_explosion_large(Explosions& explosions, bool enemy_ground, uint8_t explo_num, int16_t x,
                           int16_t y) {
  if (y < 0) {
    return;
  }

  if (enemy_ground) {
    setup_explosion(explosions.slots, x - 6, y - 14, 0, 2, false, false);
    setup_explosion(explosions.slots, x + 6, y - 14, 0, 4, false, false);
    setup_explosion(explosions.slots, x - 6, y, 0, 3, false, false);
    setup_explosion(explosions.slots, x + 6, y, 0, 5, false, false);
  }
  else {
    setup_explosion(explosions.slots, x - 6, y - 14, 0, 7, false, false);
    setup_explosion(explosions.slots, x + 6, y - 14, 0, 9, false, false);
    setup_explosion(explosions.slots, x - 6, y, 0, 8, false, false);
    setup_explosion(explosions.slots, x + 6, y, 0, 10, false, false);
  }

  bool big = explo_num > 10;
  if (big) {
    explo_num -= 10;
  }

  if (!explo_num) {
    return;
  }

  for (rep_explosion_type& r : explosions.repeating) {
    if (r.ttl != 0) {
      continue;
    }

    r.ttl = explo_num;
    r.delay = 2;
    r.x = x;
    r.y = y;
    r.big = big;
    return;
  }
}

bool update_repeating_explosions(Explosions& explosions, MtState& mt, int16_t back_move,
                                 std::span<uint8_t, 8> sound_queue) {
  bool still_exploding = false;
  for (rep_explosion_type& r : explosions.repeating) {
    if (r.ttl == 0) {
      continue;
    }

    still_exploding = true;

    if (r.delay > 0) {
      r.delay--;
      continue;
    }

    r.y += back_move + 1;
    int16_t x = r.x + (mt_rand_r(&mt) % 24) - 12;
    int16_t y = r.y + (mt_rand_r(&mt) % 27) - 24;

    if (r.big) {
      setup_explosion_large(explosions, false, 2, x, y);

      if (r.ttl == 1 || mt_rand_r(&mt) % 5 == 1) {
        sound_queue[7] = S_EXPLOSION_11;
      }
      else {
        sound_queue[6] = S_EXPLOSION_9;
      }

      r.delay = 4 + (mt_rand_r(&mt) % 3);
    }
    else {
      setup_explosion(explosions.slots, x, y, 0, 1, false, false);

      sound_queue[5] = S_EXPLOSION_4;

      r.delay = 3;
    }

    r.ttl--;
  }
  return still_exploding;
}

VisibleExplosions update_explosions(Explosions& explosions) {
  VisibleExplosions visible;
  for (int j = 0; j < MAX_EXPLOSIONS; j++) {
    Explosion& e = explosions.slots[j];
    if (e.ttl == 0) {
      continue;
    }

    if (!e.fixedPosition) {
      e.sprite++;
      e.y += explosions.move;
    }
    else if (e.followPlayer) {
      e.x += explosions.follow_x;
      e.y += explosions.follow_y;
    }
    e.y += e.deltaY;

    if (e.y > 200 - 14) {
      e.ttl = 0;
      continue;
    }

    visible.slots[j] = true;
    e.ttl--;
    if (e.ttl == 0) {
      visible.ended.push_back({j, e.x, e.y, e.sprite});
    }
  }
  return visible;
}

}  // namespace tyrx::sim
