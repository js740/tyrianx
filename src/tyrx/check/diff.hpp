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

// Differential checking of C++ ports of C game functions.
//
// A C function F that is called across object files can be wrapped at link
// time (-Wl,--wrap=F).  With TYRX_DIFF=1, the wrapper copies the state F
// owns, runs the C function, runs the C++ port on the copy, and reports any
// difference.  The C result always stands, so the game is unaffected.
//
// See diff.cpp for the wrapped functions.

#pragma once

#include "../gold/state.hpp"

#include <stdint.h>
#include <string>
#include <string_view>

namespace tyrx::check {

bool diff_enabled();

// Number of mismatches found so far (all slices).
uint64_t diff_mismatches();

namespace detail {
std::string dump_to_string(void (*dump)(gold::Dumper&, void const *), void const *value);
void diff_record(std::string_view slice, bool match, std::string const& c_dump, std::string const& cxx_dump);
}  // namespace detail

// Compares the C and C++ results of one call of `slice`.
template <typename T>
bool diff_check(std::string_view slice, T const& c_result, T const& cxx_result) {
  gold::Hasher hc, hx;
  gold::hash_value(hc, c_result);
  gold::hash_value(hx, cxx_result);
  bool const match = hc.value() == hx.value();
  if (match) {
    detail::diff_record(slice, true, {}, {});
  }
  else {
    auto dump = [](gold::Dumper& d, void const *p) { gold::dump_value(d, *static_cast<T const *>(p)); };
    detail::diff_record(slice, false, detail::dump_to_string(dump, &c_result),
                        detail::dump_to_string(dump, &cxx_result));
  }
  return match;
}

}  // namespace tyrx::check
