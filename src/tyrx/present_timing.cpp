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

#include "present_timing.hpp"

#include "c_api.hpp"
#include "pacing.hpp"
#include "runtime.hpp"
#include "vclock.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace tyrx {

namespace {

struct Stats {
  std::vector<double> v;

  void print(char const *name) {
    if (v.empty()) {
      return;
    }
    std::sort(v.begin(), v.end());
    double sum = 0, sum2 = 0;
    for (double x : v) {
      sum += x;
      sum2 += x * x;
    }
    double const n = (double)v.size(), mean = sum / n;
    auto const pct = [&](double p) { return v[std::min(v.size() - 1, (std::size_t)(p / 100 * n))]; };
    std::fprintf(stderr,
                 "tyrx present: %-24s mean %7.3f  sd %6.3f  min %7.3f  p1 %7.3f  p50 %7.3f  p99 %7.3f  max "
                 "%7.3f ms\n",
                 name, mean, std::sqrt(std::max(0.0, sum2 / n - mean * mean)), v.front(), pct(1), pct(50),
                 pct(99), v.back());
  }
};

class PresentTiming {
 public:
  PresentTiming() {
    char const *path = env("TYRX_PRESENT_LOG");
    if (path == nullptr) {
      return;
    }
    if (vclock_enabled()) {
      std::fprintf(stderr, "tyrx: TYRX_PRESENT_LOG needs real time, so it is ignored with TYRX_VCLOCK\n");
      return;
    }
    log_ = std::fopen(path, "w");
    if (log_ == nullptr) {
      std::fprintf(stderr, "tyrx: cannot write %s\n", path);
      return;
    }
    std::fprintf(log_,
                 "# tick frames deadline_ms woke start end sampled flags  (woke, start and end are "
                 "relative to the deadline; sampled is when input was read, relative to start)\n");
  }

  ~PresentTiming() {
    if (log_ == nullptr) {
      return;
    }
    finish_pass();
    std::fclose(log_);
    std::fprintf(stderr,
                 "tyrx present: %zu ticks measured, %u with the deadline moved, %u with other screens, "
                 "%u unpaced; %u deadline mismatches, %u clock recalibrations\n",
                 woke_.v.size(), moved_ticks_, modal_ticks_, unpaced_ticks_, deadline_mismatches_,
                 recalibrations_);
    woke_.print("wake - deadline");
    start_.print("present start - deadline");
    duration_.print("present duration");
    interval_.print("interval - nominal");
    sampled_.print("present start - input");
  }

  bool enabled() const { return log_ != nullptr; }

  void level_tick() {
    finish_pass();
    in_level_ = true;
    pass_ = {};
    pass_.tick = tick_++;
  }

  void level_ended() {
    finish_pass();
    in_level_ = false;
    have_previous_ = false;
    have_poll_ = false;
  }

  void before_wait() {
    if (!in_level_) {
      return;
    }
    pass_.waited = true;
    pass_.presents_before_wait += pass_.presents;
    pass_.presents = 0;
    pass_.deadline = frame_deadline().ms();
    if (getFrameCountTicks() != frame_deadline().remaining_ticks(last_sdl_ticks())) {
      ++deadline_mismatches_;
    }
  }

  void after_wait() {
    if (in_level_) {
      pass_.woke = pacing_clock().now();
      frame_deadline().moved = false;
    }
  }

  void after_set_count(Uint16 count) {
    if (in_level_ && pass_.waited && pass_.presents == 0) {
      pass_.count = count;
      pass_.moved = frame_deadline().moved;
    }
  }

  void presented(double start, double end) {
    if (!in_level_) {
      return;
    }
    ++pass_.presents;
    pass_.start = start;
    pass_.end = end;
    if (!pacing_clock().check()) {
      ++recalibrations_;
    }
  }

  // The poll that follows the tick's present reads the next tick's input.
  void polled() {
    if (in_level_ && pass_.waited && pass_.presents > 0) {
      pass_.poll = pacing_clock().now();
    }
  }

 private:
  struct Pass {
    std::uint64_t tick = 0;
    bool waited = false;
    unsigned presents_before_wait = 0;  // pause, menu and help screens
    unsigned presents = 0;
    Uint16 count = 0;
    bool moved = false;
    double deadline = 0, woke = 0, start = 0, end = 0, poll = 0;
  };

  void finish_pass() {
    if (!in_level_ || pass_.presents == 0) {
      return;
    }
    if (!pass_.waited) {
      ++unpaced_ticks_;
      have_previous_ = have_poll_ = false;
      return;
    }
    bool const sampled = have_poll_ && poll_tick_ + 1 == pass_.tick;
    have_poll_ = pass_.poll != 0;
    poll_tick_ = pass_.tick;
    double const input = sampled ? poll_ - pass_.start : 0;
    poll_ = pass_.poll;

    bool const modal = pass_.presents_before_wait > 0;
    std::fprintf(log_, "%llu %u %.3f %.3f %.3f %.3f ", (unsigned long long)pass_.tick, pass_.count,
                 pass_.deadline, pass_.woke - pass_.deadline, pass_.start - pass_.deadline,
                 pass_.end - pass_.deadline);
    if (sampled) {
      std::fprintf(log_, "%.3f", input);
    }
    else {
      std::fputc('-', log_);
    }
    std::fprintf(log_, " %s%s\n", pass_.moved ? "m" : "", modal ? "s" : "");
    if (pass_.moved || modal) {
      moved_ticks_ += pass_.moved;
      modal_ticks_ += modal;
      have_previous_ = false;
      return;
    }
    woke_.v.push_back(pass_.woke - pass_.deadline);
    start_.v.push_back(pass_.start - pass_.deadline);
    duration_.v.push_back(pass_.end - pass_.start);
    if (sampled) {
      sampled_.v.push_back(-input);
    }
    if (have_previous_ && previous_.tick + 1 == pass_.tick) {
      interval_.v.push_back((pass_.start - previous_.start) - (pass_.deadline - previous_.deadline));
    }
    previous_ = pass_;
    have_previous_ = true;
  }

  std::FILE *log_ = nullptr;
  bool in_level_ = false;
  std::uint64_t tick_ = 0;
  Pass pass_, previous_;
  bool have_previous_ = false;
  bool have_poll_ = false;  // the previous tick's poll_
  std::uint64_t poll_tick_ = 0;
  double poll_ = 0;
  Stats woke_, start_, duration_, interval_, sampled_;
  unsigned moved_ticks_ = 0, modal_ticks_ = 0, unpaced_ticks_ = 0;
  unsigned deadline_mismatches_ = 0, recalibrations_ = 0;
};

PresentTiming& present_timing() {
  static PresentTiming pt;
  return pt;
}

}  // namespace

void present_timing_level_tick() {
  if (PresentTiming& pt = present_timing(); pt.enabled()) {
    pt.level_tick();
  }
}

void present_timing_level_ended() {
  if (PresentTiming& pt = present_timing(); pt.enabled()) {
    pt.level_ended();
  }
}

bool present_timing_enabled() { return present_timing().enabled(); }

void present_timing_before_wait() {
  if (PresentTiming& pt = present_timing(); pt.enabled()) {
    pt.before_wait();
  }
}

void present_timing_after_wait() {
  if (PresentTiming& pt = present_timing(); pt.enabled()) {
    pt.after_wait();
  }
}

void present_timing_after_set_count(Uint16 count) {
  if (PresentTiming& pt = present_timing(); pt.enabled()) {
    pt.after_set_count(count);
  }
}

void present_timing_presented(double start, double end) {
  if (PresentTiming& pt = present_timing(); pt.enabled()) {
    pt.presented(start, end);
  }
}

void present_timing_polled() {
  if (PresentTiming& pt = present_timing(); pt.enabled()) {
    pt.polled();
  }
}

}  // namespace tyrx
