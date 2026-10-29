# c: the C game as seen from C++

The C code runs everything outside the levels, on its globals, and the C++
game takes the state it needs from them when a level starts. This directory
is how C++ code sees the C game:

- `api.hpp`: the C game's declarations, for C++.
- `state.hpp`: the C global that holds each member of each of the C++ game's
  `sim::State` subsystems (`for_each_binding`), and copying between the two
  (`read_state`, `write_state`).
- `mirror.hpp`: which member of a C struct each member of the C++ structs in
  `sim::State` maps to, and copying between them.

The C++ game (`../game.hpp`) reads its `State` from the C globals when a level
starts and after C code that works on them has run (the pause screen, the
in-game menu and help), and writes it back after each pass, for the C code
that runs between ticks and after the level.

The C++ structs (`sim::Event` for `JE_EventRecType`, and so on) have their own
names, types and order, and are copied member by member by the field lists in
`mirror.hpp`. A member that doesn't copy as it is has a conversion: a
player's lives, which the C struct points to in its items, are the index of
the item weapon whose power they are. None of the `State`'s pointers point
into C globals that it holds, so a copy of the state never has to be
relocated.

With `TYRX_VALIDATE=on`, the validation checks (`../validate/bindings.hpp`):
- at compile time, that each field list has every member of both structs;
- at startup, that each member of the `State` has exactly one C global;
- on each pass of the C++ game, that no pointer in the `State` points into a
  C global that it holds.
