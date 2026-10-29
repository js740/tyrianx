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

#include "../c/api.hpp"
#include "../env.hpp"
#include "roundtrip.hpp"
#include "runtime.hpp"
#include "state.hpp"
#include "vclock.hpp"

#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <span>

namespace tyrx::validate {

namespace {

bool in_item_screen = false;  // see __wrap_JE_itemScreen

// The virtual joystick has the axes and buttons that the default assignments
// (reset_joystick_assignments) use: a stick, and fire, change fire, left and
// right sidekick, menu and pause.
constexpr int kJoystickAxes = 2, kJoystickButtons = 6;

class InputDriver {
 public:
  explicit InputDriver(long seed)
      : seed_(static_cast<uint64_t>(seed)),
        press_commands_(env("TYRX_INPUT_COMMANDS") != nullptr),
        press_cheats_(env("TYRX_INPUT_CHEATS") != nullptr),
        move_mouse_(env("TYRX_INPUT_MOUSE") != nullptr),
        midtick_(env("TYRX_INPUT_MIDTICK") != nullptr),
        press_screens_(env("TYRX_INPUT_SCREENS") != nullptr),
        wants_joystick_(env("TYRX_INPUT_JOYSTICK") != nullptr) {
    // The keys that close the screens go out as the virtual clock advances,
    // which a round trip doesn't restore.
    if (press_screens_ && round_trip_enabled()) {
      std::fprintf(stderr, "tyrx: TYRX_INPUT_SCREENS is ignored with TYRX_ROUNDTRIP\n");
      press_screens_ = false;
    }
  }

  void advance(uint32_t from, uint32_t to) {
    if (!focused_) {
      SDL_Event ev{};
      ev.type = SDL_WINDOWEVENT;
      ev.window.event = SDL_WINDOWEVENT_FOCUS_GAINED;
      SDL_PushEvent(&ev);
      focused_ = true;
    }

    bool const in_level = tick_ms_valid_ && to - last_tick_ms_ < 200;
    if (screen_open_) {
      // Escape closes the pause screen (as any key) and the in-game menu.
      for (uint32_t t = from / 350 + 1; t * 350 <= to; ++t) {
        key(SDL_SCANCODE_ESCAPE, true);
        key(SDL_SCANCODE_ESCAPE, false);
      }
      return;
    }
    if (!in_level) {
      release_all();
      // Only after a level has ended: a screen that a level opens stops its
      // ticks too.
      if (press_cheats_ && level_ended_) {
        next_settings();
      }
      level_ended_ = false;
      between_levels_ = true;
      for (uint32_t t = from / 350 + 1; t * 350 <= to; ++t) {
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

  void on_level_ended() { level_ended_ = true; }

  void set_screen_open(bool open) { screen_open_ = open && press_screens_; }

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
  void push_input(uint64_t next_tick) {
    if (next_tick == pushed_tick_) {
      return;
    }
    pushed_tick_ = next_tick;
    for (size_t i = 0; i < std::size(kKeys); ++i) {
      set_key(kKeys[i], key_down(i, next_tick));
    }
    if (press_commands_) {
      // Backspace+Minus cycles the level filter (JE_mainKeyboardInput): a
      // command that the game handles in the middle of a tick.
      bool const down = next_tick % 300 == 150;
      set_key(kCommandModifier, down);
      set_key(SDL_SCANCODE_MINUS, down);
    }
    if (press_cheats_) {
      push_cheats(next_tick);
    }
    if (press_screens_) {
      // P opens the pause screen, F1 the in-game help and Escape the in-game
      // menu, in turn every 1000 ticks.  The help closes at the next key, and
      // advance closes the others.
      uint64_t const t = next_tick % 1000;
      set_key(SDL_SCANCODE_P, t == 400);
      set_key(SDL_SCANCODE_F1, t == 600);
      set_key(SDL_SCANCODE_ESCAPE, t == 800);
    }
    if (move_mouse_) {
      push_mouse(next_tick);
    }
    if (joystick_) {
      push_joystick(next_tick);
    }
  }

  // With TYRX_INPUT_CHEATS, the game's own key commands (JE_mainKeyboardInput)
  // show more of it: in each level, the debug display is on from its tick 30
  // to 230, a random smoothie is toggled every 100 ticks, and from tick 500 the
  // level is skipped.  In about one level in four, the player gets the Nort
  // ship (with its weapons and the Astral Zone special) at tick 20, for the
  // rest of the game.  Between levels, the detail level goes to the next one,
  // as the setup menu would set it, and so does the difficulty (Wimp to
  // Nortaneous).
  void push_cheats(uint64_t next_tick) {
    uint64_t const     t = next_tick - level_first_tick_;
    bool const         debug = t == 30 || t == 230;
    bool const         smoothie = t < 500 && t % 100 == 60;
    bool const         skip = t >= 500 && t < 503;
    bool const         nort = t == 20 && mix(seed_ ^ mix(level_first_tick_)) % 4 == 0;
    SDL_Scancode const digit =
        kSmoothieKeys[mix(seed_ ^ mix(level_first_tick_ + t)) % std::size(kSmoothieKeys)];
    bool const command = press_commands_ && next_tick % 300 == 150;  // as above
    set_key(kCommandModifier, debug || smoothie || command);
    set_key(SDL_SCANCODE_F10, debug);
    set_key(SDL_SCANCODE_F12, smoothie);
    set_key(SDL_SCANCODE_SCROLLLOCK, smoothie);
    for (SDL_Scancode k : kSmoothieKeys) {
      set_key(k, smoothie && k == digit);
    }
    set_key(SDL_SCANCODE_F2, skip || nort);
    set_key(SDL_SCANCODE_F6, skip || nort);
    set_key(SDL_SCANCODE_F7, skip || nort);
    for (SDL_Scancode k : {SDL_SCANCODE_F4, SDL_SCANCODE_F9, SDL_SCANCODE_BACKSLASH, SDL_SCANCODE_SLASH}) {
      set_key(k, nort);
    }
  }

  static void next_settings() {
    processorType = processorType % 6 + 1;
    JE_initProcessorType();
    difficultyLevel = (difficultyLevel + 1) % DIFFICULTY_10;
  }

  // Backspace+F12+Scroll Lock with 2-9 toggles smoothies[0-7], with 0
  // smoothies[8].
  static constexpr SDL_Scancode kSmoothieKeys[] = {
      SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4, SDL_SCANCODE_5, SDL_SCANCODE_6,
      SDL_SCANCODE_7, SDL_SCANCODE_8, SDL_SCANCODE_9, SDL_SCANCODE_0,
  };
  static constexpr SDL_Scancode kCheatKeys[] = {
      SDL_SCANCODE_F10, SDL_SCANCODE_F12, SDL_SCANCODE_SCROLLLOCK, SDL_SCANCODE_F2,        SDL_SCANCODE_F6,
      SDL_SCANCODE_F7,  SDL_SCANCODE_F4,  SDL_SCANCODE_F9,         SDL_SCANCODE_BACKSLASH, SDL_SCANCODE_SLASH,
  };

  // The virtual joystick's stick and its fire and sidekick buttons follow runs
  // like the keys' (with seeds of their own), and the change-fire button is
  // down for one tick in every 200.  Menu and pause are never pressed.  The
  // game sees the new state at its next poll_joystick().
  void push_joystick(uint64_t tick) {
    auto const down = [tick, this](size_t i) { return key_down(std::size(kKeys) + i, tick); };
    SDL_JoystickSetVirtualAxis(joystick_, 0, axis_value(down(0), down(1)));  // left, right
    SDL_JoystickSetVirtualAxis(joystick_, 1, axis_value(down(2), down(3)));  // up, down
    SDL_JoystickSetVirtualButton(joystick_, 0, down(4));                     // fire
    SDL_JoystickSetVirtualButton(joystick_, 1, tick % 200 == 100);           // change fire
    SDL_JoystickSetVirtualButton(joystick_, 2, down(5));                     // left sidekick
    SDL_JoystickSetVirtualButton(joystick_, 3, down(6));                     // right sidekick
  }

  // With TYRX_INPUT_MOUSE, the mouse moves left, right, up and down, and its
  // left and right buttons are held, in runs like the keys' (with seeds of
  // their own).  Motion goes out once per tick, as relative motion.
  void push_mouse(uint64_t tick) {
    auto const down = [tick, this](size_t i) {
      return key_down(std::size(kKeys) + kJoystickInputs + i, tick);
    };
    int const xrel = (down(1) - down(0)) * kMouseStep, yrel = (down(3) - down(2)) * kMouseStep;
    if (xrel != 0 || yrel != 0) {
      SDL_Event ev{};
      ev.type = SDL_MOUSEMOTION;
      ev.motion.x = 160;
      ev.motion.y = 100;
      ev.motion.xrel = xrel;
      ev.motion.yrel = yrel;
      SDL_PushEvent(&ev);
    }
    set_mouse_button(SDL_BUTTON_LEFT, down(4));
    set_mouse_button(SDL_BUTTON_RIGHT, down(5));
  }

  static void set_mouse_button(uint8_t button, bool down) {
    if (down == ((mouseButtonsDown & SDL_BUTTON(button)) != 0)) {
      return;
    }

    SDL_Event ev{};
    ev.type = down ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
    ev.button.button = button;
    ev.button.state = down ? SDL_PRESSED : SDL_RELEASED;
    ev.button.x = 160;
    ev.button.y = 100;
    SDL_PushEvent(&ev);
  }

  static constexpr size_t kJoystickInputs = 7;  // the stick's four directions and three buttons
  static constexpr int    kMouseStep = 8;

  static int16_t axis_value(bool negative, bool positive) {
    return static_cast<int16_t>((positive - negative) * SDL_JOYSTICK_AXIS_MAX);
  }

  static void set_key(SDL_Scancode scancode, bool down) {
    if (down != keysactive[scancode]) {
      key(scancode, down);
    }
  }

  static constexpr SDL_Scancode kCommandModifier = SDL_SCANCODE_BACKSPACE;

  static uint64_t mix(uint64_t x) {  // splitmix64 finalizer
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
    return x ^ (x >> 31);
  }

  // Each key keeps its state for runs of 2 to 9 ticks (fixed per key) and is
  // down in about a third of the runs.
  bool key_down(size_t i, uint64_t tick) const {
    uint64_t const key_seed = mix(seed_ + 0x9e3779b97f4a7c15ull * (i + 1));
    uint64_t const run = tick / (2 + key_seed % 8);
    return mix(key_seed ^ run) % 3 == 0;
  }

  static constexpr SDL_Scancode kKeys[] = {
      SDL_SCANCODE_UP,    SDL_SCANCODE_DOWN,   SDL_SCANCODE_LEFT,  SDL_SCANCODE_RIGHT,
      SDL_SCANCODE_SPACE, SDL_SCANCODE_RETURN, SDL_SCANCODE_LCTRL, SDL_SCANCODE_LALT,
  };

  // In the item screen, go to "Play Next Level" and pick the first planet (in
  // arcade modes, play the next level); elsewhere (title screen, episode and
  // difficulty selection, ...), Enter.
  static SDL_Scancode menu_key() {
    if (!in_item_screen) {
      return SDL_SCANCODE_RETURN;
    }
    int16_t const     menu = curMenu;
    uint8_t const     sel = curSel[menu];
    constexpr int16_t kFullGame = 0, kPlayNextLevel = 3, k2PlayerArcade = 9,
                      k1PlayerArcade = 10;  // game_menu.c
    if (menu == kFullGame) {
      return sel == 6 ? SDL_SCANCODE_RETURN : SDL_SCANCODE_DOWN;
    }
    if (menu == kPlayNextLevel) {
      return SDL_SCANCODE_RETURN;
    }
    // "Play Next Level" is item 2
    if (menu == k2PlayerArcade || menu == k1PlayerArcade) {
      return sel == 2 ? SDL_SCANCODE_RETURN : SDL_SCANCODE_UP;
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
    for (SDL_Scancode k : {kCommandModifier, SDL_SCANCODE_MINUS}) {
      if (keysactive[k]) {
        key(k, false);
      }
    }
    if (press_cheats_) {
      for (auto const& keys :
           {std::span<SDL_Scancode const>(kCheatKeys), std::span<SDL_Scancode const>(kSmoothieKeys)})
      {
        for (SDL_Scancode k : keys) {
          if (keysactive[k]) {
            key(k, false);
          }
        }
      }
    }
    if (joystick_) {
      release_joystick();
    }
    if (move_mouse_) {
      set_mouse_button(SDL_BUTTON_LEFT, false);
      set_mouse_button(SDL_BUTTON_RIGHT, false);
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

  uint64_t      seed_;
  bool          press_commands_;
  bool          press_cheats_;
  bool          move_mouse_;
  bool          midtick_;
  bool          press_screens_;
  bool          wants_joystick_;
  SDL_Joystick *joystick_ = nullptr;
  bool          joystick_stale_ = false;
  bool          between_levels_ = true;
  bool          level_ended_ = false;
  bool          screen_open_ = false;  // the pause screen or the in-game menu
  uint64_t      level_first_tick_ = 0;
  uint64_t      pushed_tick_ = UINT64_MAX;
  bool          focused_ = false;
  bool          tick_ms_valid_ = false;
  uint32_t      last_tick_ms_ = 0;
};

InputDriver *input_driver() {
  static InputDriver *driver = []() -> InputDriver * {
    long const seed = env_long("TYRX_INPUT_SEED", -1);
    return vclock_enabled() && seed >= 0 ? new InputDriver(seed) : nullptr;
  }();
  return driver;
}

}  // namespace

void input_driver_level_ended() {
  if (InputDriver *d = input_driver()) {
    d->on_level_ended();
  }
}

void input_driver_screen(bool open) {
  if (InputDriver *d = input_driver()) {
    d->set_screen_open(open);
  }
}

void input_driver_advance(uint32_t from, uint32_t to) {
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

}  // namespace tyrx::validate

// Linked with -Wl,--wrap=JE_itemScreen, so the driver knows when the shop menu
// is up.
extern "C" void __real_JE_itemScreen(void);

extern "C" void __wrap_JE_itemScreen(void) {
  tyrx::validate::in_item_screen = true;
  __real_JE_itemScreen();
  tyrx::validate::in_item_screen = false;
}

// Linked with -Wl,--wrap=gameplaySelect.  With TYRX_INPUT_TWO_PLAYER or
// TYRX_INPUT_ARCADE (and synthetic input), the game-mode menu makes its
// "2 Player Arcade" or "1 Player Arcade" choice right away, as it does when
// that is picked: keys sent while the menu fades in can be lost, so the driver
// can't reliably move to it.
extern "C" bool __real_gameplaySelect(void);

extern "C" bool __wrap_gameplaySelect(void) {
  bool const two_player = tyrx::env("TYRX_INPUT_TWO_PLAYER") != nullptr,
             arcade = tyrx::env("TYRX_INPUT_ARCADE") != nullptr;
  if (tyrx::validate::input_driver() == nullptr || (!two_player && !arcade)) {
    return __real_gameplaySelect();
  }
  fade_black(10);
  onePlayerAction = !two_player;
  twoPlayerMode = two_player;
  return true;
}

// Linked with -Wl,--wrap=init_joysticks, to register the joystick array, which
// holds each joystick's previous state and repeat timer, as input state.  With
// TYRX_INPUT_JOYSTICK, the game gets one virtual joystick with the default
// assignments in place of any real ones, even with --no-joystick.
// deinit_joysticks closes it as usual.
extern "C" void __real_init_joysticks(void);

extern "C" void __wrap_init_joysticks(void) {
  tyrx::validate::InputDriver *const d = tyrx::validate::input_driver();
  if (d == nullptr || !d->wants_joystick()) {
    __real_init_joysticks();
    if (joysticks > 0) {
      tyrx::validate::register_heap_block("joystick[]", tyrx::validate::StateClass::input, joystick,
                                          joysticks * sizeof *joystick);
    }
    return;
  }

  SDL_Joystick *handle = nullptr;
  if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) == 0) {
    SDL_JoystickEventState(SDL_IGNORE);
    int const index = SDL_JoystickAttachVirtual(
        SDL_JOYSTICK_TYPE_GAMECONTROLLER, tyrx::validate::kJoystickAxes, tyrx::validate::kJoystickButtons, 0);
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
  tyrx::validate::register_heap_block("joystick[]", tyrx::validate::StateClass::input, joystick,
                                      sizeof *joystick);
  d->set_joystick(handle);
}
