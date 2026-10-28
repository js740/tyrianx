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

// The players: a port of JE_playerDamage (varz.c).

#include "sim.hpp"

namespace tyrx::sim {

JE_byte damage_player(State& state, HudLog& hud, Player& player, JE_byte damage) {
  auto& sound_queue = state.sound.queue;
  JE_byte through = 0;
  sound_queue[7] = S_SHIELD_HIT;

  // Player Damage Routines
  if (player.shield < damage) {
    through = damage;
    damage -= player.shield;
    player.shield = 0;

    // Through Shields - Now Armor
    if (damage > 0) {
      if (player.armor < damage) {
        player.armor = 0;

        if (player.is_alive && !state.cheats.you_are_cheating) {
          state.level.timer = false;
          player.is_alive = false;
          player.exploding_ticks = 60;
          state.ending.countdown = 40;
          state.sound.temp_volume = tyrMusicVolume;
          sound_queue[1] = S_EXPLOSION_22;
        }
      }
      else {
        player.armor -= damage;
        sound_queue[7] = S_HULL_HIT;
      }
    }
  }
  else {
    player.shield -= damage;

    auto& slots = state.explosions.slots;
    bool const follow = !state.game.two_player_mode;
    setup_explosion(slots, player.x - 17, player.y - 12, 0, 14, false, follow);
    setup_explosion(slots, player.x - 5, player.y - 12, 0, 15, false, follow);
    setup_explosion(slots, player.x + 7, player.y - 12, 0, 16, false, follow);
    setup_explosion(slots, player.x + 19, player.y - 12, 0, 17, false, follow);

    setup_explosion(slots, player.x - 17, player.y + 2, 0, 18, false, follow);
    setup_explosion(slots, player.x + 19, player.y + 2, 0, 19, false, follow);

    setup_explosion(slots, player.x - 17, player.y + 16, 0, 20, false, follow);
    setup_explosion(slots, player.x - 5, player.y + 16, 0, 21, false, follow);
    setup_explosion(slots, player.x + 7, player.y + 16, 0, 22, false, follow);
  }

  hud_wipe_shield_armor(state, hud);
  hud_shield(state, hud);
  hud_armor(state, hud);
  hud.game_screen = true;

  return through;
}

}  // namespace tyrx::sim
