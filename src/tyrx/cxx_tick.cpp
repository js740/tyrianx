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

#include "cxx_tick.hpp"

#include "gold/api.hpp"
#include "gold/sim_state.hpp"
#include "port/draw.hpp"
#include "port/level.hpp"
#include "port/port.hpp"
#include "render/draw_list.hpp"
#include "runtime.hpp"

#include <cstdio>
#include <cstdlib>

namespace tyrx {

namespace {

render::Canvas canvas(SDL_Surface *s) {
  if (s == nullptr) {
    return {nullptr, 0, 0, 0};
  }
  return {static_cast<Uint8 *>(s->pixels), s->pitch, s->w, s->h};
}

class CxxTick final : public port::Runner {
 public:
  void level_start() {
    gold::read_sim_state(state_);
    owned_ = true;
    port::level_locals() = {};
  }

  TyrxPass level_pass() {
    if (isNetworkGame) {
      owned_ = false;
      return TYRX_PASS_C;
    }
    if (!owned_) {
      gold::read_sim_state(state_);
      owned_ = true;
    }

    gold::scramble_sim_state_globals();
    port::state_target() = &state_;
    port::draw::target() = &list_;
    port::runner_target() = this;
    port::LevelPass const pass = port::level_pass();
    flush();
    port::runner_target() = nullptr;
    port::draw::target() = nullptr;
    port::state_target() = nullptr;
    gold::write_sim_state(state_);

    switch (pass) {
      case port::LevelPass::level_loop:
        return TYRX_PASS_LEVEL_LOOP;
      case port::LevelPass::start_level:
        return TYRX_PASS_START_LEVEL;
      case port::LevelPass::unsupported:
        break;
    }
    std::fprintf(stderr, "tyrx: the C++ level tick can't run this pass\n");
    std::exit(EXIT_FAILURE);
  }

  void drawing_begins() override {}

  void present() override {
    flush();
    JE_showVGA();
  }

  void poll_events() override { handleSdlEvents(); }

  void in_game_help_ahead() override {}

  // The C loop draws on the screen as the C code has drawn the tick by then,
  // and works on the C globals.
  void c_loop(void (*loop)()) override {
    port::draw_so_far();
    flush();
    gold::write_sim_state(state_);
    loop();
    gold::read_sim_state(state_);
    gold::scramble_sim_state_globals();
  }

 private:
  // Draws the calls that the pass has made so far.
  void flush() {
    for (render::DrawCall const& c : list_.calls) {
      render::execute(c, list_.blob.data() + c.blob, canvas(c.dst), canvas(c.src));
    }
    list_.calls.clear();
    list_.blob.clear();
  }

  sim::State state_;
  bool owned_ = false;  // whether state_ is the game's state, or the C globals are
  render::DrawList list_;
};

CxxTick& cxx_tick() {
  static CxxTick t;
  return t;
}

bool const enabled = [] {
  if (env("TYRX_CXX_TICK") == nullptr) {
    return false;
  }
  for (char const *check : {"TYRX_SHADOW", "TYRX_DRAWLIST_CHECK", "TYRX_ROUNDTRIP", "TYRX_REWIND"}) {
    if (env(check) != nullptr) {
      std::fprintf(stderr, "tyrx: TYRX_CXX_TICK doesn't run with %s\n", check);
      std::exit(EXIT_FAILURE);
    }
  }
  if (!gold::sim_state_bound()) {
    std::exit(EXIT_FAILURE);
  }
  tyrx_hooks.level_pass = [] { return cxx_tick().level_pass(); };
  return true;
}();

}  // namespace

bool cxx_tick_enabled() { return enabled; }

void cxx_tick_level_start() {
  if (enabled) {
    cxx_tick().level_start();
  }
}

}  // namespace tyrx
