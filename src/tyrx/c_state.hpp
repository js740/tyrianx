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

// Registry of the C code's global state, and reflection-based hashing and
// dumping of it.
//
// Every writable global/static of the C objects has a StateClass: the globals
// registered as blocks are listed in c_state_registry.inc, and the rest in
// c_state_untracked.txt (tools/check_c_state_registry.py checks that together
// they are complete).  Blocks of the "hashed" classes make up the game state
// that is hashed every tick and that snapshots capture.

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <meta>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace tyrx {

enum class StateClass {
  sim,      // gameplay state that changes during play
  present,  // state that changes during play but only affects what is shown or heard
  data,     // level/episode data loaded from files (a few events modify it)
  scratch,  // temporaries that are always written before being read within a tick
  render,   // video, palettes, sprite sheets
  audio,    // music and sound effects
  input,    // raw device input, before it is turned into gameplay input
  ui,       // menus, shop, in-game text entry
  config,   // user preferences
  io,       // files and paths
  net,      // network play
  timing,   // frame pacing and wall-clock timers
  other,
};

constexpr bool is_hashed(StateClass c) {
  return c == StateClass::sim || c == StateClass::present || c == StateClass::data;
}

std::string_view to_string(StateClass c);

//------------------------------------------------------------------------------
// Pointer description
//------------------------------------------------------------------------------

// Describes where a pointer points in a way that does not depend on ASLR or
// on the build: as an offset into a registered block (any class, not only
// hashed ones).  A pointer one past the end of a block is unresolved: blocks
// are often adjacent in memory, so it is indistinguishable from a pointer to
// the start of whatever follows, which may be an unregistered global.
struct PointerRef {
  enum class Kind : std::uint8_t { null, block, unresolved } kind = Kind::null;
  std::string_view block;  // for Kind::block
  std::uintptr_t offset = 0;
};

PointerRef describe_pointer(void const *p);

// Number of non-null pointers that Hasher::pointer could not attribute to a
// registered block.  Such pointers are hashed only as "unresolved", so a
// nonzero count means the hash may miss changes; register their targets.
std::uint64_t unresolved_pointers();

//------------------------------------------------------------------------------
// Hashing
//------------------------------------------------------------------------------

class Hasher {
 public:
  void bytes(void const *data, std::size_t size) {
    // FNV-1a, 64-bit.
    auto *p = static_cast<unsigned char const *>(data);
    for (std::size_t i = 0; i < size; ++i) {
      h_ = (h_ ^ p[i]) * 0x100000001b3ull;
    }
  }
  void u64(std::uint64_t v) { bytes(&v, sizeof v); }
  void string(std::string_view s) {
    u64(s.size());
    bytes(s.data(), s.size());
  }
  void pointer(void const *p);

  std::uint64_t value() const { return h_; }

 private:
  std::uint64_t h_ = 0xcbf29ce484222325ull;
};

namespace detail {

template <typename T>
consteval auto data_members() {
  return std::define_static_array(
      std::meta::nonstatic_data_members_of(^^T, std::meta::access_context::unchecked()));
}

template <typename T>
constexpr bool is_bytewise = std::is_arithmetic_v<T> || std::is_enum_v<T>;

}  // namespace detail

template <typename T>
void hash_value(Hasher& h, T const& v) {
  using U = std::remove_cv_t<T>;
  if constexpr (detail::is_bytewise<U>) {
    h.bytes(&v, sizeof v);
  }
  else if constexpr (std::is_pointer_v<U>) {
    h.pointer(reinterpret_cast<void const *>(v));
  }
  else if constexpr (std::is_array_v<U>) {
    if constexpr (detail::is_bytewise<std::remove_all_extents_t<U>>) {
      h.bytes(&v, sizeof v);  // no padding between elements of scalar arrays
    }
    else {
      for (auto const& e : v) {
        hash_value(h, e);
      }
    }
  }
  else if constexpr (std::is_class_v<U>) {
    static_assert(!std::is_union_v<U>, "unions need explicit handling");
    template for (constexpr auto m : detail::data_members<U>()) { hash_value(h, v.[:m:]); }
  }
  else {
    static_assert(false, "unsupported type");
  }
}

//------------------------------------------------------------------------------
// Dumping (one "path = value" line per scalar), for diffing two states
//------------------------------------------------------------------------------

class Dumper {
 public:
  explicit Dumper(std::FILE *out) : out_(out) {}

  void push(std::string_view name) {
    stack_.push_back(path_.size());
    if (!path_.empty()) {
      path_ += '.';
    }
    path_ += name;
  }
  void push_index(std::size_t i) {
    stack_.push_back(path_.size());
    path_ += '[';
    path_ += std::to_string(i);
    path_ += ']';
  }
  void pop() {
    path_.resize(stack_.back());
    stack_.pop_back();
  }

  void integer(long long v) { std::fprintf(out_, "%s = %lld\n", path_.c_str(), v); }
  void unsigned_integer(unsigned long long v) { std::fprintf(out_, "%s = %llu\n", path_.c_str(), v); }
  void floating(double v) { std::fprintf(out_, "%s = %a\n", path_.c_str(), v); }
  void pointer(void const *p);

 private:
  std::FILE *out_;
  std::string path_;
  std::vector<std::size_t> stack_;
};

template <typename T>
void dump_value(Dumper& d, T const& v) {
  using U = std::remove_cv_t<T>;
  if constexpr (std::is_same_v<U, bool>) {
    d.unsigned_integer(v);
  }
  else if constexpr (std::is_enum_v<U>) {
    d.integer(static_cast<long long>(v));
  }
  else if constexpr (std::is_floating_point_v<U>) {
    d.floating(v);
  }
  else if constexpr (std::is_signed_v<U>) {
    d.integer(v);
  }
  else if constexpr (std::is_unsigned_v<U>) {
    d.unsigned_integer(v);
  }
  else if constexpr (std::is_pointer_v<U>) {
    d.pointer(reinterpret_cast<void const *>(v));
  }
  else if constexpr (std::is_array_v<U>) {
    for (std::size_t i = 0; i < std::extent_v<U>; ++i) {
      d.push_index(i);
      dump_value(d, v[i]);
      d.pop();
    }
  }
  else if constexpr (std::is_class_v<U>) {
    template for (constexpr auto m : detail::data_members<U>()) {
      d.push(std::meta::identifier_of(m));
      dump_value(d, v.[:m:]);
      d.pop();
    }
  }
  else {
    static_assert(false, "unsupported type");
  }
}

//------------------------------------------------------------------------------
// Registry
//------------------------------------------------------------------------------

struct StateBlock {
  std::string_view name;
  StateClass cls;
  void *addr = nullptr;
  std::size_t size = 0;
  void (*hash)(Hasher&, void const *) = nullptr;  // hashed classes only
  void (*dump)(Dumper&, void const *) = nullptr;  // hashed classes only
};

// All registered blocks, by class (as in c_state_registry.inc), then by name.
std::span<StateBlock const> state_blocks();

// Blocks of hashed classes, in state_blocks() order.
std::span<StateBlock const *const> hashed_blocks();

void dump_state(std::FILE *out);

//------------------------------------------------------------------------------
// Snapshots
//------------------------------------------------------------------------------

// A copy of the registered blocks of some classes, taken at a tick boundary,
// that can be written back.  Restoring is only valid within the level the
// snapshot was taken in: pointers are copied as they are, and some point into
// data that is loaded per level (e.g. sprite sheets).
class Snapshot {
 public:
  static Snapshot capture(bool (*include)(StateClass));
  void restore() const;
  std::size_t size() const { return bytes_.size(); }

 private:
  std::vector<StateBlock const *> blocks_;
  std::vector<std::byte> bytes_;
};

}  // namespace tyrx
