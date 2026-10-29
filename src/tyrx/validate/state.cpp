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

#include "state.hpp"

#include "../c/api.hpp"
#include "registry.inc"

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstring>
#include <deque>

namespace tyrx::validate {

namespace {

template <typename T>
void hash_thunk(Hasher& h, void const *p) {
  hash_value(h, *static_cast<T const *>(p));
}

template <typename T>
void dump_thunk(Dumper& d, void const *p) {
  dump_value(d, *static_cast<T const *>(p));
}

// Hashed blocks get hash and dump functions; other blocks are registered only
// as pointer targets, so their types never need to be reflected on.
template <typename T, StateClass Cls>
StateBlock make_block(std::string_view name, void *addr) {
  if constexpr (is_hashed(Cls)) {
    return {name, Cls, addr, sizeof(T), &hash_thunk<T>, &dump_thunk<T>};
  }
  else {
    return {name, Cls, addr, sizeof(T)};
  }
}

std::vector<StateBlock> resolve_blocks() {
  using enum StateClass;
  std::vector<StateBlock> blocks;

#define TYRX_REGISTER(sym, cls) \
  blocks.push_back(make_block<std::remove_cvref_t<decltype(::sym)>, cls>(#sym, (void *)&::sym));
#define TYRX_REGISTER_SIM(sym) TYRX_REGISTER(sym, sim)
#define TYRX_REGISTER_PRESENT(sym) TYRX_REGISTER(sym, present)
#define TYRX_REGISTER_DATA(sym) TYRX_REGISTER(sym, data)
#define TYRX_REGISTER_INPUT(sym) TYRX_REGISTER(sym, input)
#define TYRX_REGISTER_POINTEE(sym) TYRX_REGISTER(sym, render)

  TYRX_SIM(TYRX_REGISTER_SIM)
  TYRX_PRESENT(TYRX_REGISTER_PRESENT)
  TYRX_DATA(TYRX_REGISTER_DATA)
  TYRX_INPUT(TYRX_REGISTER_INPUT)
  TYRX_POINTEES(TYRX_REGISTER_POINTEE)

#undef TYRX_REGISTER
#undef TYRX_REGISTER_SIM
#undef TYRX_REGISTER_PRESENT
#undef TYRX_REGISTER_DATA
#undef TYRX_REGISTER_INPUT
#undef TYRX_REGISTER_POINTEE

  return blocks;
}

// All blocks, sorted by address, for describe_pointer().
std::vector<StateBlock const *> const& blocks_by_address() {
  static std::vector<StateBlock const *> const blocks = [] {
    std::vector<StateBlock const *> v;
    for (auto const& b : state_blocks()) {
      v.push_back(&b);
    }
    std::ranges::sort(v, {}, [](StateBlock const *b) { return reinterpret_cast<uintptr_t>(b->addr); });
    return v;
  }();
  return blocks;
}

uint64_t unresolved_count = 0;

}  // namespace

std::string_view to_string(StateClass c) {
  switch (c) {
    using enum StateClass;
    case sim:
      return "sim";
    case present:
      return "present";
    case data:
      return "data";
    case scratch:
      return "scratch";
    case render:
      return "render";
    case audio:
      return "audio";
    case input:
      return "input";
    case ui:
      return "ui";
    case config:
      return "config";
    case io:
      return "io";
    case net:
      return "net";
    case timing:
      return "timing";
    case other:
      return "other";
  }
  return "?";
}

PointerRef describe_pointer(void const *p) {
  if (p == nullptr) {
    return {};
  }
  auto const  addr = reinterpret_cast<uintptr_t>(p);
  auto const& blocks = blocks_by_address();
  // The last block starting at or before addr; blocks never overlap, so it is
  // the only one that can contain addr.
  auto it = std::ranges::upper_bound(
      blocks, addr, {}, [](StateBlock const *b) { return reinterpret_cast<uintptr_t>(b->addr); });
  if (it != blocks.begin()) {
    StateBlock const *b = *--it;
    uintptr_t const   offset = addr - reinterpret_cast<uintptr_t>(b->addr);
    if (offset < b->size) {
      return {PointerRef::Kind::block, b->name, offset};
    }
  }
  return {PointerRef::Kind::unresolved};
}

uint64_t unresolved_pointers() { return unresolved_count; }

void Hasher::pointer(void const *p) {
  PointerRef const ref = describe_pointer(p);
  u64(static_cast<uint64_t>(ref.kind));
  if (ref.kind == PointerRef::Kind::block) {
    string(ref.block);
    u64(ref.offset);
  }
  else if (ref.kind == PointerRef::Kind::unresolved) {
    ++unresolved_count;
  }
}

void Dumper::pointer(void const *p) {
  PointerRef const ref = describe_pointer(p);
  switch (ref.kind) {
    case PointerRef::Kind::null:
      std::fprintf(out_, "%s = null\n", path_.c_str());
      break;
    case PointerRef::Kind::block:
      std::fprintf(out_, "%s = &%.*s+%" PRIuPTR "\n", path_.c_str(), (int)ref.block.size(), ref.block.data(),
                   ref.offset);
      break;
    case PointerRef::Kind::unresolved:
      std::fprintf(out_, "%s = <unresolved>\n", path_.c_str());
      break;
  }
}

namespace {

std::deque<StateBlock>& heap_blocks() {  // a deque, so blocks never move
  static std::deque<StateBlock> blocks;
  return blocks;
}

}  // namespace

std::span<StateBlock const> state_blocks() {
  static std::vector<StateBlock> const blocks = resolve_blocks();
  return blocks;
}

std::span<StateBlock const *const> hashed_blocks() {
  static std::vector<StateBlock const *> const blocks = [] {
    std::vector<StateBlock const *> v;
    for (auto const& b : state_blocks()) {
      if (is_hashed(b.cls)) {
        v.push_back(&b);
      }
    }
    return v;
  }();
  return blocks;
}

void register_heap_block(std::string_view name, StateClass cls, void *addr, size_t size) {
  assert(!is_hashed(cls));
  heap_blocks().push_back({name, cls, addr, size});
}

void dump_state(std::FILE *out) {
  Dumper d(out);
  for (StateBlock const *b : hashed_blocks()) {
    d.push(b->name);
    b->dump(d, b->addr);
    d.pop();
  }
}

Snapshot Snapshot::capture(bool (*include)(StateClass), Snapshot const *previous) {
  Snapshot   s;
  size_t     i = 0;  // index into previous->copies_
  auto const copy = [&](StateBlock const& b) {
    if (!include(b.cls)) {
      return;
    }
    if (previous && i < previous->copies_.size() && previous->copies_[i].block == &b &&
        std::memcmp(previous->copies_[i].bytes.get(), b.addr, b.size) == 0)
    {
      s.copies_.push_back(previous->copies_[i]);
    }
    else {
      auto bytes = std::make_shared_for_overwrite<std::byte[]>(b.size);
      std::memcpy(bytes.get(), b.addr, b.size);
      s.copies_.push_back({&b, std::move(bytes)});
      s.unshared_size_ += b.size;
    }
    ++i;
  };
  std::ranges::for_each(state_blocks(), copy);
  std::ranges::for_each(heap_blocks(), copy);
  return s;
}

void Snapshot::restore() const {
  for (Copy const& c : copies_) {
    std::memcpy(c.block->addr, c.bytes.get(), c.block->size);
  }
}

std::vector<uint64_t> block_hashes() {
  std::vector<uint64_t> hashes;
  for (StateBlock const *b : hashed_blocks()) {
    Hasher h;
    b->hash(h, b->addr);
    hashes.push_back(h.value());
  }
  return hashes;
}

}  // namespace tyrx::validate
