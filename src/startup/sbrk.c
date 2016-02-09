/*
 * This file is part of eVic SDK.
 *
 * eVic SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * eVic SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eVic SDK.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2016 ReservedField
 */

#include <errno.h>

void *_sbrk(int incr) {
	// Defined by linker script
	extern char Heap_Start;
	extern char Heap_Limit;
	// Current heap end pointer
	static char *heapEnd = &Heap_Start;
	char *prevHeapEnd;

	if(heapEnd + incr > &Heap_Limit) {
		errno = ENOMEM;
		return (void *) -1;
	}

	prevHeapEnd = heapEnd;
	heapEnd += incr;

	return prevHeapEnd;
}
