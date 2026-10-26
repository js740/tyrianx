#!/bin/bash
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

# run_headless.sh — run OpenTyrian headless and deterministically, writing a
# hash log (TYRX_HASHLOG).
#
#   tools/run_headless.sh BINARY DATA_DIR HASHLOG demos
#       Play the five Tyrian 2.1 demos (~20 s).
#   tools/run_headless.sh BINARY DATA_DIR HASHLOG input SEED TICKS
#       Start a new game through the menus and play it with pseudo-random
#       keyboard input from SEED, for TICKS level ticks.
#
# Further VAR=VALUE arguments are passed in the environment (e.g.
# TYRX_DUMP_TICK=1234).  BINARY must be built with `make WITH_TYRX=true`.
# Runs with a throwaway config/save directory, no sound, no joystick, no
# Christmas mode, and a virtual clock (see src/tyrx/vclock.cpp).
set -euo pipefail

usage() { sed -n '24,28p' "$0" | sed 's/^# //' >&2; exit 2; }
[ $# -ge 4 ] || usage
bin=$(realpath "$1"); data=$(realpath "$2"); log=$(realpath -m "$3"); mode=$4; shift 4

case "$mode" in
    demos) vars=(TYRX_EXIT_AFTER_DEMOS=5) ;;
    input) [ $# -ge 2 ] || usage
           vars=(TYRX_INPUT_SEED="$1" TYRX_EXIT_AFTER_TICKS="$2"); shift 2 ;;
    *) usage ;;
esac

tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

cd "$tmp"
status=0
HOME="$tmp" XDG_CONFIG_HOME="$tmp" \
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy \
TYRX_VCLOCK=1 TYRX_HASHLOG="$log" \
env "${vars[@]}" "$@" timeout 600 "$bin" --no-sound --no-joystick --no-xmas --data="$data" \
    > "$tmp/out.txt" 2>&1 || status=$?
if [ $status -ne 0 ]; then
    cat "$tmp/out.txt" >&2
    echo "run_headless.sh: exited with status $status" >&2
    exit $status
fi
