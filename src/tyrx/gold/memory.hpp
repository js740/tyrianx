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

// A copy of all of the C code's writable data: its .data and .bss, which the
// linker places before the C++ objects' (between __data_start and
// tyrx_c_data_end, and __bss_start and tyrx_c_bss_end).  Restoring it puts
// every C global back, including the ones that the state registry doesn't
// track, which a whole tick of the game may change.  Heap memory isn't
// included.

#pragma once

#include <cstddef>
#include <vector>

namespace tyrx::gold {

class CMemory {
 public:
  // Whether the C data is where it is expected: every registered state block
  // must lie within it.
  static bool available();

  void capture();
  void restore() const;

 private:
  std::vector<unsigned char> data_, bss_;
};

}  // namespace tyrx::gold
