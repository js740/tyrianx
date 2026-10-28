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

// One pass of the level loop (from the level_loop label in JE_main, tyrian2.c),
// in C++: the tick's simulation (../sim/level.cpp), which keeps a record for
// the drawing; then the drawing, from the record; then the frame.  See
// level.hpp.

#include "level.hpp"

#include "../gold/api.hpp"
#include "port.hpp"

namespace tyrx::port {

namespace {

// The end of JE_starShowVGA, after the tick's drawing: waits for the frame
// deadline, presents the frame and polls events.  (Its drawing, the copy of
// the playing area to the screen, ends the tick's drawing.)
void star_show_vga_present() {
  if (!state().ending.player_end_level && !skipStarShowVGA) {
    if (smoothScroll != 0) {
      delayUntilElapsed();
      setFrameCount(state().game.frame_count_max);
    }
    present();
  }
  poll_events();
  quitRequested = false;
  skipStarShowVGA = false;
}

// What the level tick needs from what runs the passes (the Runner).
struct RunnerHost final : sim::Host {
  void in_game_help_ahead() override { port::in_game_help_ahead(); }
  void c_loop(void (*loop)()) override { port::c_loop(loop); }
  int pitch(Surface s) override { return surface(s)->pitch; }
};

}  // namespace

void draw_so_far() {
  DrawRecord& r = draw_record();
  draw_tick(r);
  Surface const screen = r.screen;
  r = DrawRecord{};
  r.screen = screen;
}

void c_loop(void (*loop)()) {
  DrawRecord& r = draw_record();
  VGAScreen = surface(r.screen);
  runner_target()->c_loop(loop);
  r.screen = surface_name(VGAScreen);
}

LevelLocals& level_locals() {
  static LevelLocals locals;
  return locals;
}

LevelPass level_pass() {
  DrawRecord& r = draw_record();
  r = DrawRecord{};
  RunnerHost host;
  bool frame = false;
  LevelPass const next = sim::level_tick(state(), level_locals(), r, host, frame);
  VGAScreen = surface(r.screen);
  if (next != LevelPass::unsupported) {
    drawing_begins();
    draw_tick(r);
    if (frame) {
      star_show_vga_present();
    }
  }
  return next;
}

}  // namespace tyrx::port
