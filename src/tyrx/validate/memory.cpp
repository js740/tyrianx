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

#include "memory.hpp"

#include "state.hpp"

#include <cstring>

extern "C" char __data_start[], __bss_start[];
extern "C" char tyrx_c_data_end, tyrx_c_bss_end;

namespace tyrx::validate {

namespace {

struct Range {
  unsigned char *begin, *end;
  bool           contains(void const *p, size_t size) const {
    auto const *b = static_cast<unsigned char const *>(p);
    return b >= begin && b + size <= end;
  }
};

Range data_range() {
  return {reinterpret_cast<unsigned char *>(__data_start),
          reinterpret_cast<unsigned char *>(&tyrx_c_data_end)};
}

Range bss_range() {
  return {reinterpret_cast<unsigned char *>(__bss_start), reinterpret_cast<unsigned char *>(&tyrx_c_bss_end)};
}

}  // namespace

bool CMemory::available() {
  static bool const ok = [] {
    Range const data = data_range(), bss = bss_range();
    if (data.begin >= data.end || bss.begin >= bss.end) {
      return false;
    }
    for (StateBlock const& b : state_blocks()) {
      if (!data.contains(b.addr, b.size) && !bss.contains(b.addr, b.size)) {
        return false;
      }
    }
    return true;
  }();
  return ok;
}

void CMemory::capture() {
  Range const data = data_range(), bss = bss_range();
  data_.assign(data.begin, data.end);
  bss_.assign(bss.begin, bss.end);
}

void CMemory::restore() const {
  std::memcpy(data_range().begin, data_.data(), data_.size());
  std::memcpy(bss_range().begin, bss_.data(), bss_.size());
}

}  // namespace tyrx::validate
