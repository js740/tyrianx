/*
 * OpenTyrian: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
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
#ifndef MTRAND_H
#define MTRAND_H

#define MT_RAND_MAX 0xffffffffUL

void mt_srand(unsigned long s);
unsigned long mt_rand(void);
float mt_rand_1(void);
float mt_rand_lt1(void);

/* New reentrant API using explicit state for deterministic gameplay checks. */

#define MT_STATE_N 624

typedef struct {
	unsigned long state[MT_STATE_N];
	int p0, p1, pm;  /* indices into state[] */
} MtState;

void mt_srand_r(MtState *mt, unsigned long s);
unsigned long mt_rand_r(MtState *mt);
float mt_rand_1_r(MtState *mt);
float mt_rand_lt1_r(MtState *mt);

void mt_save_state(MtState *out);
void mt_restore_state(const MtState *in);

#endif /* MTRAND_H */
