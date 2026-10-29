# render: drawing

`render.hpp` holds C++ ports of the C drawing primitives (`sprite.cpp`,
`vga256d.cpp`, `background.cpp`), which draw from explicit state onto an 8-bit
`Canvas`, independently of the C code, which changes game state while it
draws. Every port must produce exactly the C function's pixels.

`draw_list.hpp` describes a level tick's drawing as a *draw list*: calls to
those primitives, each with its arguments and, for primitives that draw from
game state (backgrounds, superpixels, the starfield), that state as it was at
the call. `execute.cpp` draws a call with the ports.

The validation checks the ports by replaying the C code's recorded draw lists
with them (`TYRX_DRAWLIST_CHECK`, `../validate/draw/README.md`).
