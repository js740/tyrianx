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

#include "draw_list.hpp"

namespace tyrx::render {

namespace {

constexpr char const *kOpNames[] = {
#define TYRX_DRAW_OP_NAME(name) #name,
    TYRX_DRAW_OPS(TYRX_DRAW_OP_NAME)
#undef TYRX_DRAW_OP_NAME
};

}  // namespace

char const *op_name(DrawOp op) { return kOpNames[static_cast<size_t>(op)]; }

void DrawList::append(DrawCall const& call, void const *blob_data, size_t blob_size) {
  DrawCall& c = calls.emplace_back(call);
  if (blob_data != nullptr) {
    c.blob = blob.size();
    c.blob_size = blob_size;
    auto const *b = static_cast<unsigned char const *>(blob_data);
    blob.insert(blob.end(), b, b + blob_size);
  }
}

}  // namespace tyrx::render
