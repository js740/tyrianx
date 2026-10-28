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

// The HUD's gauges and messages: ports of JE_drawShield, JE_drawArmor and so
// on (varz.c and mainint.c), which log them (hud.hpp), with the game logic
// that some of them do.

#include "sim.hpp"

namespace tyrx::sim {

namespace {

bool two_player_hud(State const& state) { return state.game.two_player_mode && !state.game.galaga_mode; }

}  // namespace

void hud_shield(State const& state, HudLog& hud) {
  auto const& players = state.players.slots;
  hud.draws.push_back(
      hud::Shield{two_player_hud(state), {players[0].shield, players[1].shield}, players[0].shield_max});
}

void hud_armor(State& state, HudLog& hud) {
  auto& players = state.players.slots;
  for (Player& p : players) {
    if (p.armor > 28) {
      p.armor = 28;
    }
  }

  hud.draws.push_back(hud::Armor{two_player_hud(state), {players[0].armor, players[1].armor}});
}

void hud_wipe_shield_armor(State const& state, HudLog& hud) {
  auto const& players = state.players.slots;
  hud.draws.push_back(hud::WipeShieldArmor{!two_player_hud(state), players[0].shield, players[0].armor});
}

void hud_port_config(State const& state, HudLog& hud) {
  if (state.game.two_player_mode) {
    return;
  }

  hud.draws.push_back(hud::PortConfig{state.players.slots[0].weapon_mode});
}

void hud_text_window(State& state, HudLog& hud, char const *text) {
  uint16_t&  text_erase = state.display.text_erase;
  bool const erase = text_erase > 0;  // erase current text

  text_erase = 100;
  hud.draws.push_back(hud::TextWindow{erase, text});
}

void hud_options(State& state, HudLog& hud) {
  bool const two = state.game.two_player_mode;
  Player&    player = state.players.slots[two ? 1 : 0];

  for (unsigned i = 0; i < std::size(player.sidekick); ++i) {
    JE_OptionType const& option = options[player.items.sidekick[i]];

    player.sidekick[i].ammo = player.sidekick[i].ammo_max = option.ammo;

    player.sidekick[i].ammo_refill_ticks = player.sidekick[i].ammo_refill_ticks_max =
        (105 - player.sidekick[i].ammo) * 4;

    player.sidekick[i].style = option.tr;

    player.sidekick[i].animation_enabled = (option.option == 1);
    player.sidekick[i].animation_frame = 0;

    player.sidekick[i].charge = 0;
    player.sidekick[i].charge_ticks = 20;

    // draw initial sidekick HUD
    int const y = hud_sidekick_y[two ? 1 : 0][i];
    hud.draws.push_back(
        hud::Sidekick{y, option.icongr, player.sidekick[i].ammo_max, player.sidekick[i].ammo});
  }

  hud_option_level(state, hud);
}

void hud_option_level(State const& state, HudLog& hud) {
  if (!state.game.two_player_mode) {
    return;
  }

  hud.draws.push_back(hud::OptionLevel{state.players.slots[1].items.sidekick_level});
}

void hud_charge_level(State const& state, HudLog& hud, uint8_t color) {
  hud.draws.push_back(hud::ChargeLevel{state.players.charge_level, color});
}

void hud_sidekick_ammo(HudLog& hud, int y, int ammo_max, int ammo, bool discharged) {
  hud.draws.push_back(hud::SidekickAmmo{y, ammo_max, ammo, discharged});
}

}  // namespace tyrx::sim
