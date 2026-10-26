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

#include "c_api.hpp"
#include "c_state.hpp"
#include "rewind.hpp"
#include "runtime.hpp"
#include "vclock.hpp"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

namespace tyrx {

namespace {

bool in_item_screen = false;  // see __wrap_JE_itemScreen

// The virtual joystick has the axes and buttons that the default assignments
// (reset_joystick_assignments) use: a stick, and fire, change fire, left and
// right sidekick, menu and pause.
constexpr int kJoystickAxes = 2, kJoystickButtons = 6;

class InputDriver {
 public:
  explicit InputDriver(long seed)
      : seed_(static_cast<std::uint64_t>(seed)),
        press_rewind_(env("TYRX_INPUT_REWIND") != nullptr),
        press_commands_(env("TYRX_INPUT_COMMANDS") != nullptr),
        midtick_(env("TYRX_INPUT_MIDTICK") != nullptr),
        wants_joystick_(env("TYRX_INPUT_JOYSTICK") != nullptr) {
    if (press_rewind_ && press_commands_ && rewind_key() == kCommandModifier) {
      std::fprintf(stderr, "tyrx: TYRX_INPUT_COMMANDS is ignored with TYRX_INPUT_REWIND\n");
      press_commands_ = false;
    }
  }

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
      between_levels_ = true;
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

  bool wants_joystick() const { return wants_joystick_; }

  // Called by __wrap_init_joysticks with the virtual joystick it opened.
  void set_joystick(SDL_Joystick *handle) { joystick_ = handle; }

  // The game's key state was restored, so push the next tick's keys again.
  // The virtual joystick isn't part of the game state, so it still holds the
  // input of the tick before the restore; at the top of the tick, it is put
  // back as the tick first saw it.
  void on_snapshot_restored() {
    pushed_tick_ = UINT64_MAX;
    joystick_stale_ = joystick_ != nullptr;
  }

  // Called at the top of each level tick, before it runs.
  void on_level_tick() {
    last_tick_ms_ = vclock_now();
    tick_ms_valid_ = true;
    if (between_levels_) {
      level_first_tick_ = current_tick();
      between_levels_ = false;
    }
    if (joystick_stale_ && !midtick_) {
      // The first tick of a level ran with everything released.
      if (current_tick() == level_first_tick_) {
        release_joystick();
      }
      else {
        push_joystick(current_tick());
      }
    }
    joystick_stale_ = false;
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
    if (press_rewind_) {
      set_key(rewind_key(), next_tick % 400 >= 360);
    }
    if (press_commands_) {
      // Backspace+Minus cycles the level filter (JE_mainKeyboardInput): a
      // command that the game handles in the middle of a tick.
      bool const down = next_tick % 300 == 150;
      set_key(kCommandModifier, down);
      set_key(SDL_SCANCODE_MINUS, down);
    }
    if (joystick_) {
      push_joystick(next_tick);
    }
  }

  // The virtual joystick's stick and its fire and sidekick buttons follow runs
  // like the keys' (with seeds of their own), and the change-fire button is
  // down for one tick in every 200.  Menu and pause are never pressed.  The
  // game sees the new state at its next poll_joystick().
  void push_joystick(std::uint64_t tick) {
    auto const down = [tick, this](std::size_t i) { return key_down(kKeys.size() + i, tick); };
    SDL_JoystickSetVirtualAxis(joystick_, 0, axis_value(down(0), down(1)));  // left, right
    SDL_JoystickSetVirtualAxis(joystick_, 1, axis_value(down(2), down(3)));  // up, down
    SDL_JoystickSetVirtualButton(joystick_, 0, down(4));                     // fire
    SDL_JoystickSetVirtualButton(joystick_, 1, tick % 200 == 100);           // change fire
    SDL_JoystickSetVirtualButton(joystick_, 2, down(5));                     // left sidekick
    SDL_JoystickSetVirtualButton(joystick_, 3, down(6));                     // right sidekick
  }

  static Sint16 axis_value(bool negative, bool positive) {
    return static_cast<Sint16>((positive - negative) * SDL_JOYSTICK_AXIS_MAX);
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

  void release_all() {
    for (SDL_Scancode k : kKeys) {
      if (keysactive[k]) {
        key(k, false);
      }
    }
    for (SDL_Scancode k : {rewind_key(), kCommandModifier, SDL_SCANCODE_MINUS}) {
      if (keysactive[k]) {
        key(k, false);
      }
    }
    if (joystick_) {
      release_joystick();
    }
  }

  void release_joystick() {
    for (int axis = 0; axis < kJoystickAxes; ++axis) {
      SDL_JoystickSetVirtualAxis(joystick_, axis, 0);
    }
    for (int button = 0; button < kJoystickButtons; ++button) {
      SDL_JoystickSetVirtualButton(joystick_, button, 0);
    }
  }

  std::uint64_t seed_;
  bool press_rewind_;
  bool press_commands_;
  bool midtick_;
  bool wants_joystick_;
  SDL_Joystick *joystick_ = nullptr;
  bool joystick_stale_ = false;
  bool between_levels_ = true;
  std::uint64_t level_first_tick_ = 0;
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

}  // namespace tyrx

// Linked with -Wl,--wrap=JE_itemScreen, so the driver knows when the shop menu
// is up.
extern "C" void __real_JE_itemScreen(void);

extern "C" void __wrap_JE_itemScreen(void) {
  tyrx::in_item_screen = true;
  __real_JE_itemScreen();
  tyrx::in_item_screen = false;
}

// Linked with -Wl,--wrap=init_joysticks, to register the joystick array, which
// holds each joystick's previous state and repeat timer, as input state.  With
// TYRX_INPUT_JOYSTICK, the game gets one virtual joystick with the default
// assignments in place of any real ones, even with --no-joystick.
// deinit_joysticks closes it as usual.
extern "C" void __real_init_joysticks(void);

extern "C" void __wrap_init_joysticks(void) {
  tyrx::InputDriver *const d = tyrx::input_driver();
  if (d == nullptr || !d->wants_joystick()) {
    __real_init_joysticks();
    if (joysticks > 0) {
      tyrx::register_heap_block("joystick[]", tyrx::StateClass::input, joystick,
                                joysticks * sizeof *joystick);
    }
    return;
  }

  SDL_Joystick *handle = nullptr;
  if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == 0) {
    SDL_JoystickEventState(SDL_IGNORE);
    int const index = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, tyrx::kJoystickAxes,
                                                tyrx::kJoystickButtons, 0);
    handle = index < 0 ? nullptr : SDL_JoystickOpen(index);
  }
  if (handle == nullptr) {
    std::fprintf(stderr, "tyrx: could not open a virtual joystick: %s\n", SDL_GetError());
    std::exit(EXIT_FAILURE);
  }

  ignore_joystick = false;
  joysticks = 1;
  joystick = static_cast<Joystick *>(std::calloc(1, sizeof *joystick));
  joystick[0].handle = handle;
  reset_joystick_assignments(0);
  tyrx::register_heap_block("joystick[]", tyrx::StateClass::input, joystick, sizeof *joystick);
  d->set_joystick(handle);
}
