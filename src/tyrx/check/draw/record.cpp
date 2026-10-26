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

#include "record.hpp"

#include "../../runtime.hpp"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace tyrx::check::draw {

namespace {

using render::surface_name;

constexpr std::size_t kOps = render::kDrawOps;

// Watches the 8-bit surfaces for pixel changes made outside recorded calls.
class PixelAudit {
 public:
  // Reports pixels that changed since the last sync.
  void check(char const *where, std::uint64_t tick) {
    for (Watched& w : watched_) {
      SDL_Surface const *s = *w.surface;
      if (s == nullptr || w.copy.size() != (std::size_t)s->pitch * s->h) {
        continue;
      }
      Uint8 const *p = static_cast<Uint8 const *>(s->pixels);
      if (std::memcmp(p, w.copy.data(), w.copy.size()) == 0) {
        continue;
      }
      std::uint64_t differing = 0;
      for (int y = 0; y < s->h; ++y) {
        for (int x = 0; x < s->w; ++x) {
          differing += p[y * s->pitch + x] != w.copy[y * s->pitch + x];
        }
      }
      if (differing == 0) {
        continue;  // only the padding at the end of rows changed
      }
      ++unrecorded_writes_;
      if (tick != last_reported_tick_) {
        last_reported_tick_ = tick;
        ++unrecorded_ticks_;
      }
      if (unrecorded_writes_ <= 20) {
        std::fprintf(stderr,
                     "tyrx drawlist: %" PRIu64
                     " pixels of %s changed outside recorded calls, %s (tick %" PRIu64 ")\n",
                     differing, surface_name(s), where, tick);
      }
    }
  }

  // Takes copies of all surfaces, or only of `drawn` if it is watched: a
  // recorded call that wrote to another surface shows up at the next check.
  void sync(SDL_Surface const *drawn = nullptr) {
    for (Watched& w : watched_) {
      SDL_Surface const *s = *w.surface;
      if (drawn != nullptr && s != drawn) {
        continue;
      }
      if (s == nullptr) {
        w.copy.clear();
        continue;
      }
      Uint8 const *p = static_cast<Uint8 const *>(s->pixels);
      w.copy.assign(p, p + (std::size_t)s->pitch * s->h);
    }
  }

  std::uint64_t unrecorded_writes() const { return unrecorded_writes_; }
  std::uint64_t unrecorded_ticks() const { return unrecorded_ticks_; }

 private:
  struct Watched {
    SDL_Surface *const *surface;
    std::vector<Uint8> copy;
  };
  Watched watched_[3] = {{&game_screen, {}}, {&VGAScreen2, {}}, {&VGAScreenSeg, {}}};
  std::uint64_t unrecorded_writes_ = 0, unrecorded_ticks_ = 0;
  std::uint64_t last_reported_tick_ = UINT64_MAX;
};

// Copies of the watched surfaces at the start of a tick, on which the tick's
// draw list is replayed at its end.
class ReplayCheck {
 public:
  ~ReplayCheck() {
    for (SDL_Surface *s : scratch_) {
      if (s != nullptr) {
        SDL_FreeSurface(s);
      }
    }
  }

  void begin_tick() {
    for (int i = 0; i < 3; ++i) {
      SDL_Surface *real = *kReal[i];
      if (scratch_[i] != nullptr &&
          (real == nullptr || scratch_[i]->w != real->w || scratch_[i]->h != real->h))
      {
        SDL_FreeSurface(scratch_[i]);
        scratch_[i] = nullptr;
      }
      if (real == nullptr) {
        continue;
      }
      if (scratch_[i] == nullptr) {
        scratch_[i] = SDL_CreateRGBSurfaceWithFormat(0, real->w, real->h, 8, SDL_PIXELFORMAT_INDEX8);
        if (scratch_[i] == nullptr) {
          std::fprintf(stderr, "tyrx drawlist: cannot create a surface to replay on: %s\n", SDL_GetError());
          std::exit(EXIT_FAILURE);
        }
      }
      for (int y = 0; y < real->h; ++y) {
        std::memcpy(row(scratch_[i], y), row(real, y), real->w);
      }
    }
  }

  void replay(DrawList const& list) {
    for (DrawCall const& c : list.calls) {
      SDL_Surface *dst = map(c.dst), *src = map(c.src);
      if ((c.dst != nullptr && dst == nullptr) || (c.src != nullptr && src == nullptr)) {
        ++unreplayable_;
        continue;
      }
      drawlist_replay(c, list.blob.data() + c.blob, dst, src);
    }
    bool differed = false;
    for (int i = 0; i < 3; ++i) {
      SDL_Surface const *real = *kReal[i];
      if (real == nullptr || scratch_[i] == nullptr) {
        continue;
      }
      std::uint64_t differing = 0;
      int first_x = 0, first_y = 0;
      for (int y = 0; y < real->h; ++y) {
        Uint8 const *r = row(real, y), *s = row(scratch_[i], y);
        for (int x = 0; x < real->w; ++x) {
          if (r[x] != s[x] && differing++ == 0) {
            first_x = x;
            first_y = y;
          }
        }
      }
      if (differing == 0) {
        continue;
      }

      differed = true;
      if (++reported_ > 20) {
        continue;
      }

      std::fprintf(stderr,
                   "tyrx drawlist: replaying tick %" PRIu64 " gave %" PRIu64
                   " different pixels on %s, first at "
                   "(%d, %d)\n",
                   list.tick, differing, surface_name(real), first_x, first_y);
    }
    ++ticks_;
    mismatched_ticks_ += differed;
  }

  std::uint64_t ticks() const { return ticks_; }
  std::uint64_t mismatched_ticks() const { return mismatched_ticks_; }
  std::uint64_t unreplayable() const { return unreplayable_; }

 private:
  static constexpr SDL_Surface *const *kReal[3] = {&game_screen, &VGAScreen2, &VGAScreenSeg};

  static Uint8 *row(SDL_Surface *s, int y) { return static_cast<Uint8 *>(s->pixels) + y * s->pitch; }
  static Uint8 const *row(SDL_Surface const *s, int y) {
    return static_cast<Uint8 const *>(s->pixels) + y * s->pitch;
  }

  SDL_Surface *map(SDL_Surface const *real) const {
    for (int i = 0; i < 3; ++i) {
      if (real != nullptr && real == *kReal[i]) {
        return scratch_[i];
      }
    }
    return nullptr;
  }

  SDL_Surface *scratch_[3] = {};
  std::uint64_t ticks_ = 0, mismatched_ticks_ = 0, unreplayable_ = 0, reported_ = 0;
};

class Recorder {
 public:
  Recorder() {
    char const *path = env("TYRX_DRAWLIST");
    check_ = env("TYRX_DRAWLIST_CHECK") != nullptr;
    if (path != nullptr) {
      out_ = std::fopen(path, "w");
      if (out_ == nullptr) {
        std::fprintf(stderr, "tyrx: cannot write %s\n", path);
      }
    }
    enabled_ = out_ != nullptr || check_;
  }

  ~Recorder() {
    if (!enabled_) {
      return;
    }
    finish_tick();
    if (out_ != nullptr) {
      std::fclose(out_);
    }
    std::fprintf(stderr,
                 "tyrx drawlist: %" PRIu64 " ticks, %" PRIu64 " calls (%" PRIu64
                 " nested calls not recorded)\n",
                 ticks_, calls_, nested_);
    std::string line = "tyrx drawlist:";
    for (std::size_t op = 0; op < kOps; ++op) {
      if (op_calls_[op] != 0) {
        line += ' ';
        line += render::op_name(static_cast<DrawOp>(op));
        line += '=';
        line += std::to_string(op_calls_[op]);
      }
    }
    std::fprintf(stderr, "%s\n", line.c_str());
    if (check_) {
      std::fprintf(stderr, "tyrx drawlist: %" PRIu64 " unrecorded pixel changes in %" PRIu64 " ticks\n",
                   audit_.unrecorded_writes(), audit_.unrecorded_ticks());
      std::fprintf(stderr,
                   "tyrx drawlist: %" PRIu64 " ticks replayed, %" PRIu64 " differed (%" PRIu64
                   " calls on other surfaces skipped)\n",
                   replay_.ticks(), replay_.mismatched_ticks(), replay_.unreplayable());
    }
  }

  bool enabled() const { return enabled_; }

  std::uint64_t check_failures() const { return audit_.unrecorded_writes() + replay_.mismatched_ticks(); }

  void level_tick() {
    finish_tick();
    if (check_) {
      if (in_level_) {
        audit_.check("between ticks", list_.tick);
      }
      audit_.sync();
      replay_.begin_tick();
    }
    in_level_ = true;
    in_tick_ = true;
    list_.tick = tick_++;
  }

  void level_ended() {
    finish_tick();
    in_level_ = false;
  }

  bool begin(DrawCall const& call, void const *blob, std::size_t blob_size) {
    if (!in_tick_) {
      return false;
    }
    if (depth_ > 0) {
      ++nested_;
      return false;
    }
    if (check_) {
      char where[64];
      std::snprintf(where, sizeof where, "before call %zu (%s)", list_.calls.size(),
                    render::op_name(call.op));
      audit_.check(where, list_.tick);
    }
    DrawCall& c = list_.calls.emplace_back(call);
    if (blob != nullptr) {
      c.blob = list_.blob.size();
      c.blob_size = blob_size;
      auto const *b = static_cast<unsigned char const *>(blob);
      list_.blob.insert(list_.blob.end(), b, b + blob_size);
    }
    ++op_calls_[static_cast<std::size_t>(call.op)];
    ++calls_;
    depth_ = 1;
    return true;
  }

  void end() {
    depth_ = 0;
    if (check_) {
      SDL_Surface const *dst = list_.calls.back().dst;
      bool const watched = dst == game_screen || dst == VGAScreen2 || dst == VGAScreenSeg;
      audit_.sync(watched ? dst : nullptr);
    }
  }

 private:
  void finish_tick() {
    if (!in_tick_) {
      return;
    }
    in_tick_ = false;  // so that the replay isn't recorded
    if (out_ != nullptr) {
      write(list_);
    }
    if (check_) {
      replay_.replay(list_);
    }
    ++ticks_;
    list_.calls.clear();
    list_.blob.clear();
  }

  void write(DrawList const& list) {
    std::fprintf(out_, "T %" PRIu64 " %zu\n", list.tick, list.calls.size());
    for (DrawCall const& c : list.calls) {
      std::fprintf(out_, "%s\n", render::describe(c, list).c_str());
    }
  }

  bool enabled_ = false;
  bool check_ = false;
  std::FILE *out_ = nullptr;
  bool in_level_ = false;
  bool in_tick_ = false;
  int depth_ = 0;
  std::uint64_t tick_ = 0;
  DrawList list_;
  PixelAudit audit_;
  ReplayCheck replay_;
  std::uint64_t ticks_ = 0, calls_ = 0, nested_ = 0;
  std::uint64_t op_calls_[kOps] = {};
};

Recorder& recorder() {
  static Recorder r;
  return r;
}

}  // namespace

bool drawlist_enabled() { return recorder().enabled(); }

std::uint64_t drawlist_check_failures() { return recorder().check_failures(); }

void drawlist_level_tick() {
  if (Recorder& r = recorder(); r.enabled()) {
    r.level_tick();
  }
}

void drawlist_level_ended() {
  if (Recorder& r = recorder(); r.enabled()) {
    r.level_ended();
  }
}

namespace detail {

bool drawlist_begin(DrawCall const& call, void const *blob, std::size_t blob_size) {
  return recorder().begin(call, blob, blob_size);
}

void drawlist_end() { recorder().end(); }

}  // namespace detail

}  // namespace tyrx::check::draw
