/*
 * OpenTyrian: A modern cross-platform port of Tyrian
 * Copyright (C) The OpenTyrian Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#ifndef HOOKS_TYRX_H
#define HOOKS_TYRX_H

/* A callback for Tyrian X (src/tyrx) that only its fork carries: it runs the
 * C++ level tick in place of the C code's.  It is NULL unless Tyrian X sets
 * it, and then the game runs as without it. */

typedef enum
{
	TYRX_PASS_C,            // run the C code's pass of level_loop
	TYRX_PASS_LEVEL_LOOP,   // the pass ran: go on to the next tick
	TYRX_PASS_START_LEVEL,  // the pass ran and ended the level
} TyrxPass;

typedef struct
{
	TyrxPass (*level_pass)(void);  // top of level_loop, after game_hooks.level_tick
} TyrxHooks;

extern TyrxHooks tyrx_hooks;

#endif /* HOOKS_TYRX_H */
