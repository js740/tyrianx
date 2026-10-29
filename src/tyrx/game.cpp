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
#include "c/state.hpp"
#include "level.hpp"
#include "render/draw_list.hpp"
#include "rewind.hpp"

namespace tyrx {

namespace {

render::Canvas canvas(SDL_Surface *s) {
  if (s == nullptr) {
    return {nullptr, 0, 0, 0};
  }
  return {static_cast<uint8_t *>(s->pixels), s->pitch, s->w, s->h};
}

class Game final : public Runner {
 public:
  bool enabled() {
    static bool const enabled = validation() == nullptr || validation()->cxx_game();
    return enabled;
  }

  void level_start() {
    c::read_state(state_);
    owned_ = true;
    locals_ = {};
    rewind_level_start();
  }

  void c_state_restored() { owned_ = false; }

  TickSubstituteResult level_pass() {
    if (isNetworkGame) {
      owned_ = false;
      return LEVEL_TICK_NOT_SUBSTITUTED;
    }
    if (!owned_) {
      c::read_state(state_);
      owned_ = true;
    }
    rewind_before_pass(state_, locals_);

    pass_begins();
    sim::LevelPass const pass = tyrx::level_pass(state_, locals_, list_, *this);
    flush();
    pass_ends();
    c::write_state(state_);
    return pass == sim::LevelPass::start_level ? LEVEL_TICK_STOPPING : LEVEL_TICK_SUBSTITUTED;
  }

  void drawing_begins() override {}

  void present() override {
    flush();
    JE_showVGA();
  }

  void poll_events() override { handleSdlEvents(); }

  void in_game_help_ahead() override {}

  // The C loop draws on the screen as the tick has drawn it so far, and works
  // on the C globals.
  void c_loop(void (*loop)()) override {
    flush();
    pass_ends();
    c::write_state(state_);
    loop();
    c::read_state(state_);
    pass_begins();
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

  void pass_begins() {
    if (Validation *v = validation()) {
      v->pass_begins(state_);
    }
  }

  void pass_ends() {
    if (Validation *v = validation()) {
      v->pass_ends();
    }
  }

  sim::State       state_;
  sim::LevelLocals locals_;
  bool             owned_ = false;  // whether state_ is the game's state, or the C globals are
  render::DrawList list_;
};

Game& game() {
  static Game g;
  return g;
}

// Set during static initialization, before main() runs.
[[maybe_unused]] bool const hooks_installed = [] {
  game_hooks.level_initialized = [] {
    if (game().enabled()) {
      game().level_start();
    }
    if (Validation *v = validation()) {
      v->level_start();
    }
  };
  game_hooks.level_tick_substitute = [] {
    if (Validation *v = validation()) {
      v->level_tick();
    }
    return game().enabled() ? game().level_pass() : LEVEL_TICK_NOT_SUBSTITUTED;
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

void game_c_state_restored() { game().c_state_restored(); }

}  // namespace tyrx
