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

#ifndef HOOKS_H
#define HOOKS_H

/* Optional callbacks for code linked into the game, such as testing and
 * debugging tools.  They are all NULL unless such code sets them; the game
 * itself never does. */

typedef struct
{
	void (*level_initialized)(void);  // level loaded, before its first tick
	void (*level_tick)(void);         // top of level_loop, before each tick
	void (*level_ended)(void);        // level_loop exited, before the level is torn down
} GameHooks;

extern GameHooks game_hooks;

#endif /* HOOKS_H */
