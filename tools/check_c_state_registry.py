#!/usr/bin/env python3
#-------------------------------------------------------------------------------
# OpenTyrian: A modern cross-platform port of Tyrian
# Copyright (C) The OpenTyrian Development Team
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#-------------------------------------------------------------------------------

"""Check that every writable symbol of the C objects is classified.

    tools/check_c_state_registry.py [--by-object] REGISTRY UNTRACKED OBJECT...

REGISTRY (src/tyrx/gold/registry.inc) lists the globals Tyrian X registers,
as X macros by state class.  UNTRACKED (src/tyrx/gold/untracked.txt) lists
the symbols it neither hashes nor snapshots, by object.  Using `nm` on each C
object file, this fails if a data/bss symbol is classified by neither, if a
registered symbol is static, missing or defined more than once, if an entry
is duplicated, if an untracked symbol is no longer in the C sources (the .c
and .h files next to the objects' sources, obj/X.o being built from
src/X.c), or if a registry list is not sorted.

--by-object prints the classification of every listed symbol, by object.

The registry's lists are expanded with the C++ preprocessor ($CXX, default
g++); `nm` is $NM (default nm).
"""
import os
import re
import subprocess
import sys

# Registry list -> class of its symbols.
LISTS = {
    "TYRX_SIM": "sim",
    "TYRX_PRESENT": "present",
    "TYRX_DATA": "data",
    "TYRX_INPUT": "input",
    "TYRX_POINTEES": "render",
}
HASHED = {"sim", "present", "data"}
CLASSES = HASHED | {"scratch", "render", "audio", "input", "ui", "config", "io", "net", "timing", "other"}


def sort_key(sym):
    """Case-insensitive, with digit runs compared as numbers (spriteSheet9 < spriteSheet10)."""
    parts = re.split(r"(\d+)", sym.lower())
    return ([int(p) if i % 2 else p for i, p in enumerate(parts)], sym)


def read_registry(path, errors):
    """Returns {symbol: class} for the registry's lists."""
    with open(path) as f:
        names = re.findall(r"^#define (\w+)\(X\)", f.read(), flags=re.M)
    for n in names:
        if n not in LISTS:
            errors.append(f"{path}: unknown list {n}")
    names = [n for n in names if n in LISTS]
    src = f'#include "{os.path.abspath(path)}"\n#define X(sym) sym\n'
    src += "".join(f"@{n} {n}(X)\n" for n in names)
    cxx = os.environ.get("CXX", "g++").split()
    out = subprocess.run(cxx + ["-E", "-P", "-x", "c++", "-"], input=src, check=True, capture_output=True,
                         text=True).stdout
    registered = {}
    for line in out.splitlines():
        if not line.startswith("@"):
            continue
        name, *syms = line[1:].split()
        if syms != sorted(syms, key=sort_key):
            errors.append(f"{path}: {name} is not sorted")
        for s in syms:
            if s in registered:
                errors.append(f"{path}: {s} is listed more than once")
            registered[s] = LISTS[name]
    return registered


def read_untracked(path, errors):
    """Returns ({(object, symbol): class}, {object: default class})."""
    entries, defaults = {}, {}
    with open(path) as f:
        lines = f.read().split("\n")
    obj = cls = None
    for lineno, raw in enumerate(lines, 1):
        line = raw.split("#", 1)[0].rstrip()
        if not line:
            continue
        where = f"{path}:{lineno}"
        words = line.split()
        if not raw[0].isspace():
            if len(words) < 3:
                errors.append(f"{where}: expected OBJECT CLASS SYMBOL...")
                continue
            obj, cls, words = words[0], words[1], words[2:]
            if cls not in CLASSES or cls in HASHED:
                errors.append(f"{where}: {cls} is not an unhashed class")
        elif obj is None:
            errors.append(f"{where}: continuation line without an object")
            continue
        for sym in words:
            if sym == "*":
                if obj in defaults:
                    errors.append(f"{where}: second default for {obj}")
                defaults[obj] = cls
            elif (obj, sym) in entries:
                errors.append(f"{where}: {obj} {sym} is listed more than once")
            else:
                entries[(obj, sym)] = cls
    return entries, defaults


def object_symbols(nm, path):
    """Returns {symbol: nm type} of the data/bss symbols of an object file."""
    out = subprocess.run([nm, path], check=True, capture_output=True, text=True).stdout
    syms = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[1] in "bBdDcCgGsS":
            syms[parts[2]] = parts[1]
    return syms


def c_sources_text(objects):
    """The text of the C sources (.c and .h) next to the objects' sources."""
    dirs = set()
    for p in objects:
        parts = os.path.normpath(p).split(os.sep)
        dirs.add(os.path.join(*(["src"] + parts[1:-1])) if parts[0] == "obj" else os.path.dirname(p))
    text = []
    for d in sorted(dirs):
        for name in sorted(os.listdir(d)):
            if name.endswith((".c", ".h")):
                with open(os.path.join(d, name), errors="replace") as f:
                    text.append(f.read())
    return "\n".join(text)


def main():
    args = sys.argv[1:]
    by_object = "--by-object" in args
    args = [a for a in args if a != "--by-object"]
    if len(args) < 3:
        print(__doc__, file=sys.stderr)
        return 2
    registry, untracked, objects = args[0], args[1], args[2:]
    nm = os.environ.get("NM", "nm")
    errors = []
    registered = read_registry(registry, errors)
    entries, defaults = read_untracked(untracked, errors)

    symbols = {os.path.splitext(os.path.basename(p))[0]: object_symbols(nm, p) for p in objects}
    for obj in sorted(set(defaults) | {o for o, _ in entries}):
        if obj not in symbols:
            errors.append(f"{untracked}: no object file {obj}.o")

    # Registered symbols: external, and defined in exactly one object.
    defined_in = {}
    for obj, syms in symbols.items():
        for sym, kind in syms.items():
            if sym in registered and kind.isupper():
                defined_in.setdefault(sym, []).append(obj)
    for sym in sorted(registered, key=sort_key):
        objs = defined_in.get(sym, [])
        if not objs:
            static = [o for o, syms in symbols.items() if sym in syms]
            if static:
                errors.append(f"{registry}: {sym} is static in {static[0]}.o; registered state needs "
                              f"external linkage")
            else:
                errors.append(f"{registry}: {sym} is not defined by any object")
        elif len(objs) > 1:
            errors.append(f"{registry}: {sym} is defined by several objects ({', '.join(objs)})")

    # Every symbol of every object is classified exactly once.
    for obj, syms in sorted(symbols.items()):
        for sym, kind in sorted(syms.items()):
            is_registered = sym in registered and kind.isupper()
            listed = (obj, sym) in entries
            if is_registered and listed:
                errors.append(f"{obj}.{sym} is both registered and listed as untracked")
            elif not is_registered and not listed and obj not in defaults:
                errors.append(f"unclassified symbol {obj}.{sym} ('{kind}')")

    # Untracked entries whose symbol is missing from its object are not errors
    # as long as the C sources still have it: the optimizer drops statics that
    # are never read.  Those whose symbol the sources no longer mention are
    # stale.
    source_text = c_sources_text(objects)
    for obj, sym in sorted(entries):
        if obj in symbols and sym not in symbols[obj] and not re.search(rf"\b{re.escape(sym)}\b", source_text):
            errors.append(f"{untracked}: {obj} {sym} is not in the C sources any more")

    if by_object:
        for obj, syms in sorted(symbols.items()):
            rows = []
            for sym in sorted(syms, key=sort_key):
                if sym in registered and syms[sym].isupper():
                    rows.append(f"  {registered[sym]:8} {sym}")
                elif (obj, sym) in entries:
                    rows.append(f"  {entries[(obj, sym)]:8} {sym}  (untracked)")
            if obj in defaults:
                rows.append(f"  {defaults[obj]:8} *  (untracked)")
            if rows:
                print(f"{obj}.o")
                print("\n".join(rows))

    for e in errors:
        print(f"state registry: {e}", file=sys.stderr)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
