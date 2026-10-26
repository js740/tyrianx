# render: drawing

`render.hpp` holds C++ ports of the C drawing functions (`background.cpp`),
which draw from explicit state onto an 8-bit `Canvas`. They are the start of a
renderer that can draw frames independently of the C code, which changes game
state while it draws. Every port must produce exactly the C function's pixels.

`draw_list.hpp` describes a level tick's drawing as a *draw list*: calls to
those primitives, each with its arguments and, for primitives that draw from
game state (backgrounds, superpixels, the starfield), that state as it was at
the call.

`draw_background_1` is checked call by call (`TYRX_RENDER_DIFF`,
`../check/README.md`). Most of the C drawing is called from inside `tyrian2.c`,
where it can't be wrapped, so it will need checking by whole frame instead,
comparing a frame drawn in C++ with what the C code presents.
