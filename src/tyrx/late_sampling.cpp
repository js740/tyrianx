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

#include "late_sampling.hpp"

#include "pacing.hpp"
#include "runtime.hpp"
#include "vclock.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace tyrx {

namespace {

constexpr double kSpinMs = 1.0;           // spin for the last part of the wait before a present
constexpr double kMarginMs = 0.5;         // added to the measured budget
constexpr double kInitialBudgetMs = 4.0;  // until there are measurements
constexpr double kMinBudgetMs = 1.0;

class LateSampling {
 public:
  LateSampling() {
    char const *v = env("TYRX_LATE_SAMPLING");
    if (v == nullptr) {
      return;
    }
    if (vclock_enabled()) {
      std::fprintf(stderr, "tyrx: TYRX_LATE_SAMPLING needs real time, so it is ignored with TYRX_VCLOCK\n");
      return;
    }
    if (std::strcmp(v, "precise") == 0) {
      mode_ = Mode::precise;
    }
    else if (std::strcmp(v, "auto") == 0) {
      mode_ = Mode::adaptive;
    }
    else if (double const ms = std::atof(v); ms > 0) {
      mode_ = Mode::fixed;
      fixed_budget_ = ms;
    }
    else {
      std::fprintf(stderr, "tyrx: bad TYRX_LATE_SAMPLING '%s'\n", v);
    }
  }

  ~LateSampling() {
    if (mode_ == Mode::off) {
      return;
    }
    std::fprintf(stderr, "tyrx late sampling: %llu ticks waited for, %llu ran past their deadline",
                 (unsigned long long)waits_, (unsigned long long)late_);
    if (mode_ != Mode::precise && early_waits_ > 0) {
      std::fprintf(stderr, "; %llu early waits, budget mean %.3f max %.3f ms",
                   (unsigned long long)early_waits_, budget_sum_ / early_waits_, budget_max_);
    }
    std::fputc('\n', stderr);
  }

  bool active() const { return mode_ != Mode::off; }

  void level_tick() {
    in_level_ = true;
    ++pass_;
    waited_ = false;
    presents_ = 0;
  }

  void level_ended() {
    in_level_ = false;
    poll_pending_ = false;
    measuring_ = false;
  }

  bool wait() {
    if (!in_level_) {
      return false;
    }
    PacingClock const& clock = pacing_clock();
    double const now = clock.now();
    double const deadline = frame_deadline().ms();
    // The budget had to cover everything from the early wait's target to
    // here, in the tick after it, unless another screen ran in between.
    if (measuring_ && measured_pass_ + 1 == pass_ && presents_ == 0) {
      samples_[next_sample_++ % samples_.size()] = now - early_target_;
    }
    measuring_ = false;
    ++waits_;
    if (now > deadline) {
      ++late_;
    }
    clock.sleep_until(deadline - kSpinMs);
    clock.spin_until(deadline);
    waited_ = true;
    return true;
  }

  void presented() {
    if (!in_level_) {
      return;
    }
    if (waited_) {
      poll_pending_ = true;
    }
    else {
      ++presents_;
    }
  }

  // Before the poll that follows the tick's present, waits until the budget
  // before the next deadline, which setFrameCount has just set.
  void before_poll() {
    if (!poll_pending_) {
      return;
    }
    poll_pending_ = false;
    if (mode_ == Mode::precise) {
      return;
    }
    double const b = budget();
    early_target_ = frame_deadline().ms() - b;
    pacing_clock().sleep_until(early_target_);
    measuring_ = true;
    measured_pass_ = pass_;
    ++early_waits_;
    budget_sum_ += b;
    budget_max_ = std::max(budget_max_, b);
  }

 private:
  enum class Mode { off, precise, adaptive, fixed };

  double budget() const {
    if (mode_ == Mode::fixed) {
      return fixed_budget_;
    }
    if (next_sample_ == 0) {
      return kInitialBudgetMs;
    }
    std::size_t const n = std::min<std::size_t>(next_sample_, samples_.size());
    double const slowest = *std::max_element(samples_.begin(), samples_.begin() + n);
    return std::max(kMinBudgetMs, slowest + kMarginMs);
  }

  Mode mode_ = Mode::off;
  double fixed_budget_ = 0;

  bool in_level_ = false;
  std::uint64_t pass_ = 0;
  bool waited_ = false;    // this tick's wait before its present has run
  unsigned presents_ = 0;  // other screens' presents in this tick
  bool poll_pending_ = false;

  bool measuring_ = false;  // an early wait ended in pass measured_pass_
  std::uint64_t measured_pass_ = 0;
  double early_target_ = 0;
  std::array<double, 64> samples_{};
  std::size_t next_sample_ = 0;

  std::uint64_t waits_ = 0, late_ = 0, early_waits_ = 0;
  double budget_sum_ = 0, budget_max_ = 0;
};

LateSampling& late_sampling() {
  static LateSampling ls;
  return ls;
}

}  // namespace

void late_sampling_level_tick() {
  if (LateSampling& ls = late_sampling(); ls.active()) {
    ls.level_tick();
  }
}

void late_sampling_level_ended() {
  if (LateSampling& ls = late_sampling(); ls.active()) {
    ls.level_ended();
  }
}

bool late_sampling_wait() {
  LateSampling& ls = late_sampling();
  return ls.active() && ls.wait();
}

void late_sampling_presented() {
  if (LateSampling& ls = late_sampling(); ls.active()) {
    ls.presented();
  }
}

void late_sampling_before_poll() {
  if (LateSampling& ls = late_sampling(); ls.active()) {
    ls.before_poll();
  }
}

}  // namespace tyrx
