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

#include "input_record.hpp"

#include "../c/api.hpp"
#include "state.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>

namespace tyrx::validate {

namespace {

struct JoystickPoll {
  std::vector<unsigned char> joysticks;  // the joystick array after the poll

  bool joydown;
};

struct TickInput {
  Snapshot boundary;  // the input class at the top of the tick

  bool keyboard = false;      // keyboardHasInput()
  bool mouse = false;         // mouseHasInput(INPUT_NO_MOTION)
  bool mouse_motion = false;  // mouseHasInput(INPUT_ANY)

  std::vector<std::pair<Sint32, Sint32>> mouse_reads;  // mouseGetRelativePosition
  std::vector<JoystickPoll>              joystick_polls;
};

bool is_input(StateClass c) { return c == StateClass::input; }

class InputRecord {
 public:
  ~InputRecord() {
    if (misses_ > 0) {
      std::fprintf(stderr, "tyrx input record: %" PRIu64 " replayed reads beyond the recording\n", misses_);
    }
  }

  bool     recording() const { return mode_ == Mode::recording; }
  bool     replaying() const { return mode_ == Mode::replaying; }
  uint64_t misses() const { return misses_; }

  void start() {
    stop();
    ticks_.clear();
    mode_ = Mode::recording;
  }

  void tick() {
    if (recording()) {
      TickInput& t = ticks_.emplace_back();
      t.boundary =
          Snapshot::capture(&is_input, ticks_.size() > 1 ? &ticks_[ticks_.size() - 2].boundary : nullptr);
      t.keyboard = keyboardHasInput();
      t.mouse = mouseHasInput(INPUT_NO_MOTION);
      t.mouse_motion = mouseHasInput(INPUT_ANY);
      return;
    }
    if (!replaying()) {
      return;
    }
    if (next_tick_ >= ticks_.size()) {
      ++misses_;
      current_ = nullptr;
      return;
    }
    current_ = &ticks_[next_tick_++];
    current_->boundary.restore();
    keyboard_ = current_->keyboard;
    mouse_ = current_->mouse;
    mouse_motion_ = current_->mouse_motion;
    next_mouse_read_ = next_joystick_poll_ = 0;
  }

  void replay() {
    live_ = Snapshot::capture(&is_input);
    mode_ = Mode::replaying;
    next_tick_ = 0;
    current_ = nullptr;
  }

  void stop() {
    if (replaying()) {
      live_.restore();
    }
    mode_ = Mode::off;
    current_ = nullptr;
  }

  // The wrapped calls, while recording or replaying.  The queue queries and
  // clears only go to the recorded queue state while replaying: recording
  // doesn't need them.

  bool keyboard_has_input() const { return keyboard_; }

  bool has_input(InputFlags flags) const {
    return keyboard_ || ((flags & INPUT_NO_MOTION) != 0 ? mouse_ : mouse_motion_);
  }

  void keyboard_clear_input() { keyboard_ = false; }

  void mouse_clear_input() { mouse_ = mouse_motion_ = false; }

  void mouse_relative_position(Sint32 *out_x, Sint32 *out_y, void (*real)(Sint32 *, Sint32 *)) {
    if (recording()) {
      real(out_x, out_y);
      ticks_.back().mouse_reads.emplace_back(*out_x, *out_y);
      return;
    }
    if (current_ == nullptr || next_mouse_read_ >= current_->mouse_reads.size()) {
      ++misses_;
      *out_x = *out_y = 0;
      return;
    }
    std::tie(*out_x, *out_y) = current_->mouse_reads[next_mouse_read_++];
  }

  void joystick_poll(std::function<void()> const& poll) {
    if (recording()) {
      poll();
      JoystickPoll& p = ticks_.back().joystick_polls.emplace_back();
      auto const   *bytes = reinterpret_cast<unsigned char const *>(joystick);
      p.joysticks.assign(bytes, bytes + (size_t)joysticks * sizeof *joystick);
      p.joydown = joydown;
      return;
    }
    if (current_ == nullptr || next_joystick_poll_ >= current_->joystick_polls.size()) {
      ++misses_;
      return;
    }
    JoystickPoll const& p = current_->joystick_polls[next_joystick_poll_++];
    if (!p.joysticks.empty()) {
      std::memcpy(joystick, p.joysticks.data(), p.joysticks.size());
    }
    joydown = p.joydown;
  }

 private:
  enum class Mode { off, recording, replaying };

  Mode                   mode_ = Mode::off;
  std::vector<TickInput> ticks_;
  Snapshot               live_;  // the input class when the replay started

  // The tick being replayed, and what it has left of its recorded input.
  TickInput *current_ = nullptr;
  size_t     next_tick_ = 0;
  bool       keyboard_ = false, mouse_ = false, mouse_motion_ = false;
  size_t     next_mouse_read_ = 0, next_joystick_poll_ = 0;

  uint64_t misses_ = 0;
};

InputRecord& input_record() {
  static InputRecord r;
  return r;
}

}  // namespace

void input_record_start() { input_record().start(); }

void input_record_tick() { input_record().tick(); }

void input_record_replay() { input_record().replay(); }

void input_record_stop() { input_record().stop(); }

bool input_record_replaying() { return input_record().replaying(); }

void input_record_joystick_poll(std::function<void()> const& poll) {
  InputRecord& r = input_record();
  if (!r.recording() && !r.replaying()) {
    poll();
    return;
  }
  r.joystick_poll(poll);
}

uint64_t input_record_misses() { return input_record().misses(); }

}  // namespace tyrx::validate

//------------------------------------------------------------------------------
// The input queries that the record answers while replaying, the mouse reads
// and joystick polls that it records, and event polling, which replays
// suppress.  Linked with -Wl,--wrap for each: calls from other object files
// (the level loop, the C++ tick) come here, and those within keyboard.c and
// joystick.c stay as they are.

extern "C" {

bool __real_keyboardHasInput(void);
bool __real_hasInput(InputFlags);
void __real_keyboardClearInput(void);
void __real_mouseClearInput(void);
void __real_mouseGetRelativePosition(Sint32 *, Sint32 *);
void __real_push_joysticks_as_keyboard(void);
void __real_poll_joystick(int);
void __real_handleSdlEvents(void);

bool __wrap_keyboardHasInput(void) {
  if (tyrx::validate::input_record_replaying()) {
    return tyrx::validate::input_record().keyboard_has_input();
  }
  return __real_keyboardHasInput();
}

bool __wrap_hasInput(InputFlags flags) {
  if (tyrx::validate::input_record_replaying()) {
    return tyrx::validate::input_record().has_input(flags);
  }
  return __real_hasInput(flags);
}

void __wrap_keyboardClearInput(void) {
  if (tyrx::validate::input_record_replaying()) {
    tyrx::validate::input_record().keyboard_clear_input();
    return;
  }
  __real_keyboardClearInput();
}

void __wrap_mouseClearInput(void) {
  if (tyrx::validate::input_record_replaying()) {
    tyrx::validate::input_record().mouse_clear_input();
    return;
  }
  __real_mouseClearInput();
}

void __wrap_mouseGetRelativePosition(Sint32 *out_x, Sint32 *out_y) {
  tyrx::validate::InputRecord& r = tyrx::validate::input_record();
  if (!r.recording() && !r.replaying()) {
    __real_mouseGetRelativePosition(out_x, out_y);
    return;
  }
  r.mouse_relative_position(out_x, out_y, __real_mouseGetRelativePosition);
}

// Replays also drop the key events that push_joysticks_as_keyboard sends, as
// they suppress event polling.
void __wrap_push_joysticks_as_keyboard(void) {
  tyrx::validate::input_record_joystick_poll([] { __real_push_joysticks_as_keyboard(); });
}

void __wrap_poll_joystick(int j) {
  tyrx::validate::input_record_joystick_poll([j] { __real_poll_joystick(j); });
}

// Replays play recorded input, and leave the events for after the replay.
void __wrap_handleSdlEvents(void) {
  if (tyrx::validate::input_record_replaying()) {
    return;
  }
  __real_handleSdlEvents();
}

}  // extern "C"
