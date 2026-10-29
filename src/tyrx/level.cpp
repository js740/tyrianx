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

#include "level.hpp"

#include "render/tick/tick.hpp"

namespace tyrx {

namespace {

using sim::DrawRecord;
using sim::Surface;

// What the level tick needs from what runs it.  The C loops draw on the C
// code's VGAScreen, and save and restore it: it is set from the record's
// surface before one, and the record's surface from it after.
class Host final : public sim::Host {
 public:
  Host(sim::State const& state, DrawRecord& record, render::DrawList& list, Runner& runner)
      : state_(state), record_(record), list_(list), runner_(runner) {}

  void in_game_help_ahead() override { runner_.in_game_help_ahead(); }

  // The C code runs the C loops in the middle of the tick, when it has drawn
  // the playing area up to there, and they draw on top of it: the drawing
  // recorded so far goes into the list first, and a new record starts.
  void c_loop(void (*loop)()) override {
    render::draw_tick(list_, state_, record_);
    Surface const screen = record_.screen;
    record_ = DrawRecord{};
    record_.screen = screen;

    VGAScreen = render::surface(record_.screen);
    runner_.c_loop(loop);
    record_.screen = render::record_surface(VGAScreen);
  }

  int pitch(Surface s) override { return render::surface(s)->pitch; }

 private:
  sim::State const& state_;
  DrawRecord&       record_;
  render::DrawList& list_;
  Runner&           runner_;
};

// The end of JE_starShowVGA, after the tick's drawing: waits for the frame
// deadline, presents the frame and polls events.  (Its drawing, the copy of
// the playing area to the screen, ends the tick's drawing.)
void star_show_vga_present(sim::State const& state, Runner& runner) {
  if (!state.ending.player_end_level && !skipStarShowVGA) {
    if (smoothScroll != 0) {
      delayUntilElapsed();
      setFrameCount(state.game.frame_count_max);
    }
    runner.present();
  }
  runner.poll_events();
  quitRequested = false;
  skipStarShowVGA = false;
}

}  // namespace

sim::LevelPass level_pass(sim::State& state, sim::LevelLocals& locals, render::DrawList& list,
                          Runner& runner) {
  DrawRecord           record;
  Host                 host(state, record, list, runner);
  bool                 frame = false;
  sim::LevelPass const next = sim::level_tick(state, locals, record, host, frame);
  VGAScreen = render::surface(record.screen);
  runner.drawing_begins();
  render::draw_tick(list, state, record);
  if (frame) {
    star_show_vga_present(state, runner);
  }
  return next;
}

}  // namespace tyrx
