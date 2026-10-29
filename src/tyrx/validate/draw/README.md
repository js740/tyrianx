# validate/draw: the C code's draw lists

The C code draws each level tick through a set of primitives: the sprite
blitters in `sprite.c`, the rectangles and gauges in `vga256d.c`, the
backgrounds, filters and screen copies in `backgrnd.c`, `JE_drawSP` in `varz.c`
and `JE_clr256` in `video.c`. The level code calls all of them across object
files, so `wraps.cpp` wraps each one at link time and `record.cpp` records each
tick's calls, in order, with their arguments: the tick's *draw list*
(`../../render/draw_list.hpp`). Text is recorded as the sprite calls it is made
of. A call made from inside another recorded one is part of it and not recorded
again. Primitives that draw from game state record that state as it is at the
call: the backgrounds their scroll position (the map row as a pointer into
`megaData1`-`3`, the column offset and the x and y positions), `JE_drawSP` the
superpixels, the starfield its stars, the water filter its hue, and
`JE_filterScreen` the two settings that decide whether it filters.

The C code's draw lists are the reference that the C++ game's drawing has to
reproduce, call for call. `TYRX_DRAWLIST=FILE` writes them as text, a
`T tick calls` line followed by one line per call:

```
T 0 40
fill_rectangle_xy VGAScreenSeg 289 17 310 19 0 0 0 0
draw_background_1 game_screen &megaData1+32696 0 0 0 0 0 0 0 0
blit_sprite2x2 game_screen spriteSheet10 84 162 49 0 0 0 0 0
JE_drawSP game_screen 0 0 0 0 0 0 0 0 blob=2424:1c76ae387d01de85
...
```

`TYRX_DRAWLIST_CHECK=1` checks two things:
1. That the list is complete. Before each recorded call and at the top of each
   tick, it compares `game_screen`, `VGAScreen2` and `VGAScreenSeg` with their
   contents after the previous recorded call, so any pixel written by
   unrecorded code is reported.
2. That the list is enough to draw the tick. At the end of each tick, it
   replays the list on copies of the three surfaces as they were at the start
   of the tick, and compares the result with the surfaces. It replays it twice:
   with the C primitives, setting game state that a primitive reads from the
   record for the call and restoring the state it changes (background
   scrolling, superpixels, stars, the brightness fade) afterwards; and with
   their C++ ports (`../../render/execute.cpp`), which take everything from the
   record.

Exits that the validation initiates return status 5 if either fails, and when the C++
ports alone fail, the call after which they first differ is reported. The five
demos (2.2 million calls), 80,000 ticks of synthetic play and 60,000 ticks of
it with `TYRX_INPUT_CHEATS` (8 million calls) pass both. The replay runs within
the tick, so it doesn't show that the list records inputs that stay the same
for the whole tick, such as sprite data or settings. The in-game menu
(`JE_inGameSetup`) and help (`JE_helpSystem`) save and restore the screen with
`memcpy`, which the first check would report; synthetic play doesn't open them.
