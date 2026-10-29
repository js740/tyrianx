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

#include "../state.hpp"

#include <cinttypes>
#include <cstdarg>
#include <cstdio>

namespace tyrx::validate::draw {

namespace {

std::string sheet_name(Sprite2_array const& a) {
  struct Named {
    Sprite2_array const *sheet;
    char const          *name;
  };
  static Named const kSheets[] = {
      {&explosionSpriteSheet, "explosionSpriteSheet"},
      {&enemySpriteSheets[0], "enemySpriteSheets[0]"},
      {&enemySpriteSheets[1], "enemySpriteSheets[1]"},
      {&enemySpriteSheets[2], "enemySpriteSheets[2]"},
      {&enemySpriteSheets[3], "enemySpriteSheets[3]"},
      {&spriteSheet8, "spriteSheet8"},
      {&spriteSheet9, "spriteSheet9"},
      {&spriteSheet10, "spriteSheet10"},
      {&spriteSheet11, "spriteSheet11"},
      {&spriteSheet12, "spriteSheet12"},
      {&shopSpriteSheet, "shopSpriteSheet"},
      {&destructSpriteSheet, "destructSpriteSheet"},
  };
  for (Named const& n : kSheets) {
    if (a.data != nullptr && a.data == n.sheet->data) {
      return n.name;
    }
  }
  char buf[32];
  std::snprintf(buf, sizeof buf, "?%p", static_cast<void const *>(a.data));
  return buf;
}

void append_format(std::string& s, char const *format, ...) __attribute__((format(printf, 2, 3)));

void append_format(std::string& s, char const *format, ...) {
  char    buf[256];
  va_list args;
  va_start(args, format);
  std::vsnprintf(buf, sizeof buf, format, args);
  va_end(args);
  s += buf;
}

}  // namespace

char const *surface_name(SDL_Surface const *s) {
  if (s == nullptr) {
    return "null";
  }
  if (s == game_screen) {
    return "game_screen";
  }
  if (s == VGAScreen2) {
    return "VGAScreen2";
  }
  if (s == VGAScreenSeg) {
    return "VGAScreenSeg";
  }
  return "?";
}

std::string describe(DrawCall const& c, DrawList const& list) {
  std::string line;
  append_format(line, "%s %s", op_name(c.op), surface_name(c.dst));
  if (c.src != nullptr) {
    append_format(line, " <- %s", surface_name(c.src));
  }
  if (c.sheet.data != nullptr) {
    append_format(line, " %s", sheet_name(c.sheet).c_str());
  }
  if (c.map != nullptr) {
    PointerRef const ref = describe_pointer(c.map);
    if (ref.kind == PointerRef::Kind::block) {
      append_format(line, " &%.*s+%" PRIuPTR, (int)ref.block.size(), ref.block.data(), ref.offset);
    }
    else {
      line += " <unresolved>";
    }
  }
  for (intptr_t a : c.args) {
    append_format(line, " %" PRIdPTR, a);
  }
  if (c.blob_size != 0) {
    Hasher h;
    h.bytes(list.blob.data() + c.blob, c.blob_size);
    append_format(line, " blob=%u:%016" PRIx64, c.blob_size, h.value());
  }
  return line;
}

}  // namespace tyrx::validate::draw
