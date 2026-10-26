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

#include "input_driver.hpp"

#include "../gold/api.hpp"
#include "../runtime.hpp"
#include "vclock.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <initializer_list>

namespace tyrx::check {

namespace {

bool in_item_screen = false;  // see __wrap_JE_itemScreen

class InputDriver {
 public:
  explicit InputDriver(long seed)
      : seed_(static_cast<std::uint64_t>(seed)),
        press_commands_(env("TYRX_INPUT_COMMANDS") != nullptr),
        midtick_(env("TYRX_INPUT_MIDTICK") != nullptr) {}

  void advance(Uint32 from, Uint32 to) {
    if (!focused_) {
      SDL_Event ev{};
      ev.type = SDL_WINDOWEVENT;
      ev.window.event = SDL_WINDOWEVENT_FOCUS_GAINED;
      SDL_PushEvent(&ev);
      focused_ = true;
    }

    bool const in_level = tick_ms_valid_ && to - last_tick_ms_ < 200;
    if (!in_level) {
      release_all();
      for (Uint32 t = from / 350 + 1; t * 350 <= to; ++t) {
        SDL_Scancode const k = menu_key();
        key(k, true);
        key(k, false);
      }
      return;
    }

    if (!midtick_) {
      push_input(current_tick());
    }
  }

  // The game's key state was restored, so push the next tick's keys again.
  void on_snapshot_restored() { pushed_tick_ = UINT64_MAX; }

  // Called at the top of each level tick, before it runs.
  void on_level_tick() {
    last_tick_ms_ = vclock_now();
    tick_ms_valid_ = true;
    if (midtick_) {
      push_input(current_tick() + 1);
    }
  }

 private:
  // Sets the input for tick `next_tick`, once per tick: events go out only for
  // keys whose state differs from the game's, so what the game sees depends
  // only on the seed and the tick, even after a snapshot is restored.  They go
  // out during the frame-pacing wait before the tick, as real input usually
  // arrives, or with TYRX_INPUT_MIDTICK at the top of the tick before: after
  // the game has polled events for it, so they arrive while it runs.  That
  // shows whether the game reads input only at tick boundaries.
  void push_input(std::uint64_t next_tick) {
    if (next_tick == pushed_tick_) {
      return;
    }
    pushed_tick_ = next_tick;
    for (std::size_t i = 0; i < kKeys.size(); ++i) {
      set_key(kKeys[i], key_down(i, next_tick));
    }
    if (press_commands_) {
      // Backspace+Minus cycles the level filter (JE_mainKeyboardInput): a
      // command that the game handles in the middle of a tick.
      bool const down = next_tick % 300 == 150;
      set_key(kCommandModifier, down);
      set_key(SDL_SCANCODE_MINUS, down);
    }
  }

  static void set_key(SDL_Scancode scancode, bool down) {
    if (down != keysactive[scancode]) {
      key(scancode, down);
    }
  }

  static constexpr SDL_Scancode kCommandModifier = SDL_SCANCODE_BACKSPACE;

  static std::uint64_t mix(std::uint64_t x) {  // splitmix64 finalizer
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
    return x ^ (x >> 31);
  }

  // Each key keeps its state for runs of 2 to 9 ticks (fixed per key) and is
  // down in about a third of the runs.
  bool key_down(std::size_t i, std::uint64_t tick) const {
    std::uint64_t const key_seed = mix(seed_ + 0x9e3779b97f4a7c15ull * (i + 1));
    std::uint64_t const run = tick / (2 + key_seed % 8);
    return mix(key_seed ^ run) % 3 == 0;
  }

  static constexpr std::array kKeys = {
      SDL_SCANCODE_UP,    SDL_SCANCODE_DOWN,   SDL_SCANCODE_LEFT,  SDL_SCANCODE_RIGHT,
      SDL_SCANCODE_SPACE, SDL_SCANCODE_RETURN, SDL_SCANCODE_LCTRL, SDL_SCANCODE_LALT,
  };

  // In the item screen, go to "Play Next Level" and pick the first planet;
  // elsewhere (title screen, episode and difficulty selection, ...), Enter.
  static SDL_Scancode menu_key() {
    if (!in_item_screen) {
      return SDL_SCANCODE_RETURN;
    }
    JE_integer const menu = curMenu;
    JE_byte const sel = curSel[menu];
    constexpr JE_integer kFullGame = 0, kPlayNextLevel = 3;  // game_menu.c
    if (menu == kFullGame) {
      return sel == 6 ? SDL_SCANCODE_RETURN : SDL_SCANCODE_DOWN;
    }
    if (menu == kPlayNextLevel) {
      return SDL_SCANCODE_RETURN;
    }
    return SDL_SCANCODE_ESCAPE;
  }

  static void key(SDL_Scancode scancode, bool down) {
    SDL_Event ev{};
    ev.type = down ? SDL_KEYDOWN : SDL_KEYUP;
    ev.key.state = down ? SDL_PRESSED : SDL_RELEASED;
    ev.key.keysym.scancode = scancode;
    ev.key.keysym.sym = SDL_GetKeyFromScancode(scancode);
    SDL_PushEvent(&ev);
  }

  static void release_all() {
    for (SDL_Scancode k : kKeys) {
      if (keysactive[k]) {
        key(k, false);
      }
    }
    for (SDL_Scancode k : {kCommandModifier, SDL_SCANCODE_MINUS}) {
      if (keysactive[k]) {
        key(k, false);
      }
    }
  }

  std::uint64_t seed_;
  bool press_commands_;
  bool midtick_;
  std::uint64_t pushed_tick_ = UINT64_MAX;
  bool focused_ = false;
  bool tick_ms_valid_ = false;
  Uint32 last_tick_ms_ = 0;
};

InputDriver *input_driver() {
  static InputDriver *driver = []() -> InputDriver * {
    long const seed = env_long("TYRX_INPUT_SEED", -1);
    return vclock_enabled() && seed >= 0 ? new InputDriver(seed) : nullptr;
  }();
  return driver;
}

}  // namespace

void input_driver_advance(Uint32 from, Uint32 to) {
  if (InputDriver *d = input_driver()) {
    d->advance(from, to);
  }
}

void input_driver_level_tick() {
  if (InputDriver *d = input_driver()) {
    d->on_level_tick();
  }
}

void input_driver_snapshot_restored() {
  if (InputDriver *d = input_driver()) {
    d->on_snapshot_restored();
  }
}

}  // namespace tyrx::check

// Linked with -Wl,--wrap=JE_itemScreen, so the driver knows when the shop menu
// is up.
extern "C" void __real_JE_itemScreen(void);

extern "C" void __wrap_JE_itemScreen(void) {
  tyrx::check::in_item_screen = true;
  __real_JE_itemScreen();
  tyrx::check::in_item_screen = false;
}
