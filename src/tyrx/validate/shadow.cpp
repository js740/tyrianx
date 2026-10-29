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

#include "shadow.hpp"

#include "../c/api.hpp"
#include "../c/state.hpp"
#include "../env.hpp"
#include "../level.hpp"
#include "bindings.hpp"
#include "draw/record.hpp"
#include "memory.hpp"
#include "runtime.hpp"
#include "state.hpp"
#include "vclock.hpp"

#include <algorithm>
#include <cinttypes>
#include <csetjmp>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace tyrx::validate {

namespace {

using render::DrawCall;
using render::DrawList;
using render::DrawOp;

std::jmp_buf abort_jump;

// The three surfaces the game draws on, and copies of them for the shadow.
SDL_Surface **const kSurfaces[3] = {&game_screen, &VGAScreen2, &VGAScreenSeg};
char const *const   kSurfaceNames[3] = {"game_screen", "VGAScreen2", "VGAScreenSeg"};

bool same_calls(DrawCall const& a, DrawList const& la, DrawCall const& b, DrawList const& lb) {
  return a.op == b.op && a.dst == b.dst && a.src == b.src && a.sheet.data == b.sheet.data &&
         a.sheet.size == b.sheet.size && a.map == b.map && std::ranges::equal(a.args, b.args) &&
         a.blob_size == b.blob_size &&
         std::memcmp(la.blob.data() + a.blob, lb.blob.data() + b.blob, a.blob_size) == 0;
}

// The calls of a list that touch a surface, drawing on it or reading it, in
// their order.  Two lists draw the same pixels if each surface's calls are
// the same: calls touching different surfaces commute.
std::vector<size_t> surface_calls(DrawList const& list, SDL_Surface const *surface) {
  std::vector<size_t> calls;
  for (size_t i = 0; i < list.calls.size(); ++i) {
    if (list.calls[i].dst == surface || list.calls[i].src == surface) {
      calls.push_back(i);
    }
  }
  return calls;
}

// "name count" lines for the kinds of calls counted.
std::string summary(uint64_t const (&counts)[render::kDrawOps]) {
  std::string s;
  for (size_t op = 0; op < std::size(counts); ++op) {
    if (counts[op] != 0) {
      s += render::op_name(static_cast<DrawOp>(op));
      s += ' ';
      s += std::to_string(counts[op]);
      s += '\n';
    }
  }
  return s;
}

class Shadow final : public Runner {
 public:
  Shadow() {
    if (env("TYRX_SHADOW") == nullptr) {
      return;
    }
    if (!vclock_enabled()) {
      std::fprintf(stderr, "tyrx: TYRX_SHADOW needs TYRX_VCLOCK\n");
      return;
    }
    if (env("TYRX_ROUNDTRIP") || env("TYRX_REWIND")) {
      std::fprintf(stderr, "tyrx: TYRX_SHADOW can't be combined with round trips or rewind\n");
      return;
    }
    if (!CMemory::available()) {
      std::fprintf(stderr, "tyrx: TYRX_SHADOW: the C data isn't where it is expected\n");
      return;
    }
    enabled_ = true;
  }

  ~Shadow() {
    if (!enabled_) {
      return;
    }
    std::fprintf(stderr,
                 "tyrx shadow: %" PRIu64 " ticks checked, %" PRIu64 " differed (%" PRIu64
                 " in state, %" PRIu64 " in draw calls, %" PRIu64 " in pixels)\n",
                 checked_, mismatched_, state_mismatches_, list_mismatches_, pixel_mismatches_);
    for (auto const& [why, n] : skipped_) {
      std::fprintf(stderr, "tyrx shadow: %" PRIu64 " ticks skipped: %s\n", n, why.c_str());
    }
    for (auto const& [why, n] : aborted_) {
      std::fprintf(stderr, "tyrx shadow: %" PRIu64 " passes abandoned: %s\n", n, why.c_str());
    }
    if (env("TYRX_SHADOW_CALLS") != nullptr) {
      auto const print = [](char const *what, std::string const& calls) {
        for (size_t b = 0, e; b < calls.size(); b = e + 1) {
          e = calls.find('\n', b);
          std::fprintf(stderr, "tyrx shadow: %s: %.*s\n", what, (int)(e - b), calls.data() + b);
        }
      };
      print("C++ calls", summary(emitted_));
      print("drawn during the simulation", summary(during_simulation_));
    }
    for (SDL_Surface *s : shadow_surfaces_) {
      if (s != nullptr) {
        SDL_FreeSurface(s);
      }
    }
  }

  // What the pass needs from what runs it.  It presents and polls nothing, as
  // the C tick did that, and the C loops abort it (see the wrappers below).
  void drawing_begins() override { simulated_calls_ = pass_list_.calls.size(); }
  void present() override {}
  void poll_events() override {}
  void in_game_help_ahead() override { abort("in-game help"); }
  void c_loop(void (*loop)()) override { loop(); }

  bool     enabled() const { return enabled_; }
  bool     active() const { return active_; }
  uint64_t mismatches() const { return mismatched_; }

  void level_start() {
    have_pre_ = false;
    hud_pending_ = false;
    old_weapon_bar_[0] = old_weapon_bar_[1] = 0;
  }
  void level_ended() { have_pre_ = false; }

  void skip_tick(char const *why) {
    if (!active_ && skip_reason_ == nullptr) {
      skip_reason_ = why;
    }
  }

  // File reads in a C tick (demo input) are recorded, and the shadow pass gets
  // the same data instead of reading the file again.
  template <typename Read>
  size_t file_read(File *file, void *data, size_t size, Read read) {
    if (active_) {
      if (next_read_ >= reads_.size() || reads_[next_read_].requested != size) {
        abort("unexpected file read");
      }
      FileRead const& r = reads_[next_read_++];
      std::memcpy(data, r.data.data(), r.data.size());
      *file = r.file;
      return r.data.size();
    }
    size_t const n = read();
    if (have_pre_) {
      auto const *b = static_cast<unsigned char const *>(data);
      reads_.push_back({size, std::vector<unsigned char>(b, b + n), *file});
    }
    return n;
  }

  // Joystick polls in a C tick (poll_joystick, and push_joysticks_as_keyboard)
  // read the joysticks as they were then.  The shadow pass runs after the next
  // tick's input has arrived, so it gets the state each poll left instead.
  template <typename Poll>
  void joystick_poll(Poll poll) {
    if (active_) {
      if (next_poll_ >= polls_.size()) {
        abort("unexpected joystick poll");
      }
      JoystickPoll const& p = polls_[next_poll_++];
      restore_joysticks(p.joysticks);
      joydown = p.joydown;
      return;
    }
    poll();
    if (have_pre_) {
      JoystickPoll& p = polls_.emplace_back();
      copy_joysticks(p.joysticks);
      p.joydown = joydown;
    }
  }

  [[noreturn]] void abort(char const *why) {
    abort_reason_ = why;
    std::longjmp(abort_jump, 1);
  }

  void level_tick() {
    if (have_pre_) {
      if (skip_reason_ != nullptr) {
        ++skipped_[skip_reason_];
      }
      else if (isNetworkGame) {
        ++skipped_["network game"];
      }
      else {
        run();
      }
    }
    track_old_weapon_bar();
    capture_pre();
    reads_.clear();
    polls_.clear();
  }

  // The C code's old_weapon_bar, a local variable of JE_main: the weapon power
  // that the HUD saw at the top of the previous tick, if it was drawn then.
  void track_old_weapon_bar() {
    if (hud_pending_) {
      old_weapon_bar_[0] = weapon_power_[0];
      old_weapon_bar_[1] = weapon_power_[1];
    }
    hud_pending_ = !endLevel;
    for (unsigned i = 0; i < 2; ++i) {
      weapon_power_[i] = player[twoPlayerMode ? i : 0].items.weapon[i].power;
    }
  }

 private:
  void capture_pre() {
    pre_memory_.capture();
    copy_joysticks(pre_joysticks_);
    for (int i = 0; i < 3; ++i) {
      SDL_Surface const *s = *kSurfaces[i];
      uint8_t const     *p = static_cast<uint8_t const *>(s->pixels);
      pre_pixels_[i].assign(p, p + (size_t)s->pitch * s->h);
    }
    pre_clock_ = vclock_now();
    pre_old_weapon_bar_[0] = old_weapon_bar_[0];
    pre_old_weapon_bar_[1] = old_weapon_bar_[1];
    have_pre_ = true;
    skip_reason_ = nullptr;
  }

  static void copy_joysticks(std::vector<unsigned char>& to) {
    auto const *p = reinterpret_cast<unsigned char const *>(joystick);
    to.assign(p, p + (size_t)joysticks * sizeof *joystick);
  }

  static void restore_joysticks(std::vector<unsigned char> const& from) {
    if (!from.empty()) {
      std::memcpy(joystick, from.data(), from.size());
    }
  }

  void run() {
    // The state after the C tick.
    CMemory post_memory;
    post_memory.capture();
    std::vector<unsigned char> post_joysticks;
    copy_joysticks(post_joysticks);
    std::vector<uint64_t> const post_hashes = block_hashes();

    uint32_t const post_clock = vclock_now();
    SDL_Surface   *real[3];
    for (int i = 0; i < 3; ++i) {
      real[i] = *kSurfaces[i];
    }

    // The state before it, drawing on copies of the surfaces.
    pre_memory_.restore();
    restore_joysticks(pre_joysticks_);
    vclock_set(pre_clock_);
    SDL_Surface *const vga_screen = VGAScreen;
    for (int i = 0; i < 3; ++i) {
      SDL_Surface *& s = shadow_surfaces_[i];
      if (s == nullptr) {
        s = SDL_CreateRGBSurfaceWithFormat(0, real[i]->w, real[i]->h, 8, SDL_PIXELFORMAT_INDEX8);
        if (s == nullptr || s->pitch != real[i]->pitch) {
          std::fprintf(stderr, "tyrx shadow: cannot create a surface: %s\n", SDL_GetError());
          std::exit(EXIT_FAILURE);
        }
      }
      std::memcpy(s->pixels, pre_pixels_[i].data(), pre_pixels_[i].size());
      *kSurfaces[i] = s;
      if (vga_screen == real[i]) {
        VGAScreen = s;
      }
    }

    locals_.old_weapon_bar[0] = pre_old_weapon_bar_[0];
    locals_.old_weapon_bar[1] = pre_old_weapon_bar_[1];

    // The pass works on the C++ State, and the C globals it holds the state of
    // are scrambled until it is written back.
    c::read_state(sim_state_);
    check_bound_pointers(sim_state_);
    scramble_bound_globals();

    next_read_ = 0;
    next_poll_ = 0;
    vclock_set_quiet(true);
    pass_list_ = {};
    simulated_calls_ = SIZE_MAX;
    active_ = true;
    bool const completed = setjmp(abort_jump) == 0;
    if (completed) {
      level_pass(sim_state_, locals_, pass_list_, *this);
    }
    active_ = false;
    c::write_state(sim_state_);
    DrawList shadow_list = std::move(pass_list_);
    vclock_set_quiet(false);

    if (!completed) {
      ++aborted_[abort_reason_];
    }
    else {
      for (size_t i = 0; i < shadow_list.calls.size(); ++i) {
        size_t const op = static_cast<size_t>(shadow_list.calls[i].op);
        ++emitted_[op];
        if (i < simulated_calls_) {
          ++during_simulation_[op];
        }
      }
      for (DrawCall& c : shadow_list.calls) {
        for (int i = 0; i < 3; ++i) {
          if (c.dst == shadow_surfaces_[i]) {
            c.dst = real[i];
          }
          if (c.src == shadow_surfaces_[i]) {
            c.src = real[i];
          }
        }
      }
      compare(post_hashes, shadow_list, real);
    }

    // Back to the state after the C tick.
    post_memory.restore();
    restore_joysticks(post_joysticks);
    vclock_set(post_clock);
  }

  void compare(std::vector<uint64_t> const& post_hashes, DrawList const& shadow_list,
               SDL_Surface *const *real) {
    ++checked_;
    uint64_t const tick = draw::drawlist_current().tick;
    bool           differed = false;

    std::vector<uint64_t> const hashes = block_hashes();

    std::string blocks;
    for (size_t i = 0; i < hashes.size(); ++i) {
      if (hashes[i] != post_hashes[i]) {
        blocks += ' ';
        blocks += hashed_blocks()[i]->name;
      }
    }
    if (!blocks.empty()) {
      differed = true;
      ++state_mismatches_;
      report("tick %" PRIu64 ": state differs:%s", tick, blocks.c_str());
    }

    DrawList const& c_list = draw::drawlist_current();
    bool            list_differed = false;
    for (int r = 0; r < 3 && !list_differed; ++r) {
      std::vector<size_t> const c_calls = surface_calls(c_list, real[r]),
                                shadow_calls = surface_calls(shadow_list, real[r]);

      size_t const n = std::min(c_calls.size(), shadow_calls.size());
      size_t       i = 0;
      while (i < n &&
             same_calls(c_list.calls[c_calls[i]], c_list, shadow_list.calls[shadow_calls[i]], shadow_list))
      {
        ++i;
      }
      if (i >= n && c_calls.size() == shadow_calls.size()) {
        continue;
      }

      list_differed = true;
      report("tick %" PRIu64 ": call %zu of %zu on %s (C++: %zu) differs:\n  C:   %s\n  C++: %s", tick, i,
             c_calls.size(), kSurfaceNames[r], shadow_calls.size(),
             i < c_calls.size() ? draw::describe(c_list.calls[c_calls[i]], c_list).c_str() : "(none)",
             i < shadow_calls.size() ? draw::describe(shadow_list.calls[shadow_calls[i]], shadow_list).c_str()
                                     : "(none)");
    }
    if (!list_differed && c_list.calls.size() != shadow_list.calls.size()) {
      list_differed = true;
      report("tick %" PRIu64 ": %zu calls (C++: %zu), some on other surfaces", tick, c_list.calls.size(),
             shadow_list.calls.size());
    }
    if (list_differed) {
      differed = true;
      ++list_mismatches_;
    }

    // The frame, drawn from the list by the C++ ports.
    for (int s = 0; s < 3; ++s) {
      std::memcpy(shadow_surfaces_[s]->pixels, pre_pixels_[s].data(), pre_pixels_[s].size());
    }
    auto const canvas = [&](SDL_Surface const *r) -> render::Canvas {
      for (int s = 0; s < 3; ++s) {
        if (r == real[s]) {
          SDL_Surface *const c = shadow_surfaces_[s];
          return {static_cast<uint8_t *>(c->pixels), c->pitch, c->w, c->h};
        }
      }
      return {nullptr, 0, 0, 0};
    };
    for (DrawCall const& c : shadow_list.calls) {
      render::execute(c, shadow_list.blob.data() + c.blob, canvas(c.dst), canvas(c.src));
    }

    uint64_t differing = 0;
    for (int s = 0; s < 3; ++s) {
      SDL_Surface const *a = real[s], *b = shadow_surfaces_[s];
      for (int y = 0; y < a->h; ++y) {
        uint8_t const *ra = static_cast<uint8_t const *>(a->pixels) + y * a->pitch;
        uint8_t const *rb = static_cast<uint8_t const *>(b->pixels) + y * b->pitch;
        for (int x = 0; x < a->w; ++x) {
          differing += ra[x] != rb[x];
        }
      }
    }
    if (differing != 0) {
      differed = true;
      ++pixel_mismatches_;
      report("tick %" PRIu64 ": %" PRIu64 " pixels differ", tick, differing);
    }
    mismatched_ += differed;
  }

  void report(char const *format, ...) __attribute__((format(printf, 2, 3))) {
    if (++reported_ > 20) {
      return;
    }
    std::fputs("tyrx shadow: ", stderr);
    va_list args;
    va_start(args, format);
    std::vfprintf(stderr, format, args);
    va_end(args);
    std::fputc('\n', stderr);
  }

  bool             enabled_ = false;
  bool             active_ = false;
  sim::State       sim_state_;
  sim::LevelLocals locals_;
  // The pass's draw list, which the C++ code draws into (render/tick/draw.hpp), and
  // how many of its calls the simulation made (all, until drawing begins).
  DrawList pass_list_;
  size_t   simulated_calls_ = SIZE_MAX;
  // The calls of each kind in shadow passes, and those made during the
  // simulation, for TYRX_SHADOW_CALLS.
  uint64_t    emitted_[render::kDrawOps] = {}, during_simulation_[render::kDrawOps] = {};
  bool        have_pre_ = false;
  char const *skip_reason_ = nullptr;
  char const *abort_reason_ = nullptr;

  CMemory pre_memory_;

  std::vector<unsigned char> pre_joysticks_;
  std::vector<uint8_t>       pre_pixels_[3];

  uint32_t     pre_clock_ = 0;
  SDL_Surface *shadow_surfaces_[3] = {};
  struct FileRead {
    size_t requested;

    std::vector<unsigned char> data;

    File file;  // after the read
  };
  std::vector<FileRead> reads_;

  size_t next_read_ = 0;
  struct JoystickPoll {
    std::vector<unsigned char> joysticks;

    bool joydown;
  };
  std::vector<JoystickPoll> polls_;

  size_t next_poll_ = 0;

  bool     hud_pending_ = false;
  unsigned weapon_power_[2] = {}, old_weapon_bar_[2] = {}, pre_old_weapon_bar_[2] = {};

  uint64_t checked_ = 0, mismatched_ = 0, reported_ = 0;
  uint64_t state_mismatches_ = 0, list_mismatches_ = 0, pixel_mismatches_ = 0;

  std::map<std::string, uint64_t> skipped_, aborted_;
};

Shadow& shadow() {
  static Shadow s;
  return s;
}

}  // namespace

bool shadow_enabled() { return shadow().enabled(); }

size_t shadow_file_read(File *file, void *data, size_t size, size_t (*read)(File *, void *, size_t)) {
  Shadow& s = shadow();
  if (!s.enabled()) {
    return read(file, data, size);
  }
  return s.file_read(file, data, size, [&] { return read(file, data, size); });
}

bool shadow_active() { return shadow().active(); }

void shadow_joystick_poll(std::function<void()> const& poll) {
  Shadow& s = shadow();
  if (!s.enabled()) {
    poll();
    return;
  }
  s.joystick_poll(poll);
}

void shadow_abort(char const *why) { shadow().abort(why); }

void shadow_skip_tick(char const *why) {
  if (Shadow& s = shadow(); s.enabled()) {
    s.skip_tick(why);
  }
}

void shadow_level_start() {
  if (Shadow& s = shadow(); s.enabled()) {
    s.level_start();
  }
}

void shadow_level_tick() {
  if (Shadow& s = shadow(); s.enabled()) {
    s.level_tick();
  }
}

void shadow_level_ended() {
  if (Shadow& s = shadow(); s.enabled()) {
    s.level_ended();
  }
}

uint64_t shadow_mismatches() { return shadow().mismatches(); }

}  // namespace tyrx::validate

//------------------------------------------------------------------------------
// Side effects to suppress in the shadow pass, and C code that makes a tick
// impossible to check.
//------------------------------------------------------------------------------

extern "C" {

void   __real_multiSamplePlay(int16_t const *, size_t, uint8_t, uint8_t);
void   __real_play_song(unsigned int);
void   __real_set_volume(uint8_t, uint8_t);
void   __real_push_joysticks_as_keyboard(void);
void   __real_poll_joystick(int);
void   __real_JE_loadCompShapes(Sprite2_array *, char);
void   __real_free_sprite2s(Sprite2_array *);
void   __real_JE_pauseGame(void);
void   __real_JE_doInGameSetup(void);
void   __real_JE_showVGA(void);
void   __real_handleSdlEvents(void);
size_t __real_fileReadAtMost(File *, void *, size_t);
void   __real_fileReadExactly(File *, void *, size_t);

size_t __wrap_fileReadAtMost(File *file, void *data, size_t size) {
  return tyrx::validate::shadow_file_read(file, data, size, __real_fileReadAtMost);
}

void __wrap_fileReadExactly(File *file, void *data, size_t size) {
  tyrx::validate::shadow_file_read(file, data, size, [](File *f, void *d, size_t n) {
    __real_fileReadExactly(f, d, n);
    return n;
  });
}

void __wrap_multiSamplePlay(int16_t const *samples, size_t count, uint8_t chan, uint8_t vol) {
  if (!tyrx::validate::shadow_active()) {
    __real_multiSamplePlay(samples, count, chan, vol);
  }
}

void __wrap_play_song(unsigned int song) {
  if (!tyrx::validate::shadow_active()) {
    __real_play_song(song);
  }
}

void __wrap_set_volume(uint8_t music, uint8_t sample) {
  if (!tyrx::validate::shadow_active()) {
    __real_set_volume(music, sample);
  }
}

void __wrap_push_joysticks_as_keyboard(void) {
  tyrx::validate::shadow_joystick_poll([] { __real_push_joysticks_as_keyboard(); });
}

void __wrap_poll_joystick(int j) {
  tyrx::validate::shadow_joystick_poll([j] { __real_poll_joystick(j); });
}

void __wrap_JE_loadCompShapes(Sprite2_array *sheet, char s) {
  if (tyrx::validate::shadow_active()) {
    tyrx::validate::shadow_abort("sprites loaded");
  }
  tyrx::validate::shadow_skip_tick("sprites loaded");
  __real_JE_loadCompShapes(sheet, s);
}

void __wrap_free_sprite2s(Sprite2_array *sheet) {
  if (tyrx::validate::shadow_active()) {
    tyrx::validate::shadow_abort("sprites loaded");
  }
  tyrx::validate::shadow_skip_tick("sprites loaded");
  __real_free_sprite2s(sheet);
}

void __wrap_JE_pauseGame(void) {
  if (tyrx::validate::shadow_active()) {
    tyrx::validate::shadow_abort("pause screen");
  }
  tyrx::validate::shadow_skip_tick("pause screen");
  __real_JE_pauseGame();
}

void __wrap_JE_doInGameSetup(void) {
  if (tyrx::validate::shadow_active()) {
    tyrx::validate::shadow_abort("in-game menu");
  }
  tyrx::validate::shadow_skip_tick("in-game menu");
  __real_JE_doInGameSetup();
}

// The C++ level pass presents and polls by itself, so a call from C code
// means a screen of its own (pause, menu, help).
void __wrap_JE_showVGA(void) {
  if (tyrx::validate::shadow_active()) {
    tyrx::validate::shadow_abort("another screen presented");
  }
  __real_JE_showVGA();
}

void __wrap_handleSdlEvents(void) {
  if (tyrx::validate::shadow_active()) {
    tyrx::validate::shadow_abort("another screen polled");
  }
  __real_handleSdlEvents();
}

}  // extern "C"
