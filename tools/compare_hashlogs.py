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

"""Compare two hash logs (see TYRX_HASHLOG) and report the first divergence.

    tools/compare_hashlogs.py A.log B.log

Exit status is 0 if the logs agree on every tick, 1 otherwise.  To see the
differing fields, rerun both builds with TYRX_DUMP_TICK=<tick> and diff the
dumps.
"""
import sys


def read(path):
    blocks, events = None, []
    with open(path) as f:
        for line in f:
            parts = line.split()
            if not parts or parts[0].startswith("#"):
                continue
            if parts[0] == "blocks":
                blocks = parts[1:]
            elif parts[0] == "L":
                events.append(("L", int(parts[2]), " ".join(parts[3:])))
            elif parts[0] == "T":
                events.append(("T", int(parts[1]), parts[2], parts[3:]))
    return blocks or [], events


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    (blocks_a, a), (blocks_b, b) = read(sys.argv[1]), read(sys.argv[2])

    if blocks_a != blocks_b:
        # Different registries: compare only the blocks both logs have.
        common = [n for n in blocks_a if n in set(blocks_b)]
        print(f"note: block lists differ; comparing {len(common)} common blocks")
        print(f"  only in A: {sorted(set(blocks_a) - set(blocks_b))}")
        print(f"  only in B: {sorted(set(blocks_b) - set(blocks_a))}")
    else:
        common = blocks_a
    ia = {n: i for i, n in enumerate(blocks_a)}
    ib = {n: i for i, n in enumerate(blocks_b)}
    same_blocks = blocks_a == blocks_b

    level = "(before first level)"
    ticks = 0
    for ea, eb in zip(a, b):
        if ea[0] != eb[0] or ea[1] != eb[1]:
            print(f"DIVERGED: event sequence differs: A={ea[:3]} B={eb[:3]}")
            return 1
        if ea[0] == "L":
            if ea[2] != eb[2]:
                print(f"DIVERGED at level start, tick {ea[1]}: A={ea[2]!r} B={eb[2]!r}")
                return 1
            level = ea[2]
            continue
        ticks += 1
        if same_blocks and ea[2] == eb[2]:
            continue
        diff = [n for n in common if ea[3][ia[n]] != eb[3][ib[n]]]
        if same_blocks or diff:
            print(f"DIVERGED at tick {ea[1]} ({level})")
            print(f"  differing blocks: {' '.join(diff) or '(none; total hash only)'}")
            return 1

    if len(a) != len(b):
        longer = "A" if len(a) > len(b) else "B"
        print(f"DIVERGED: {longer} is longer ({len(a)} vs {len(b)} events) after {ticks} matching ticks")
        return 1
    print(f"identical: {ticks} ticks")
    return 0


if __name__ == "__main__":
    sys.exit(main())
