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

#ifndef EVICSDK_ATOMICOPS_H
#define EVICSDK_ATOMICOPS_H

#include <stdint.h>

// extern "C" is omitted because none
// of this is going to be exported.

/* Always inline, no extern version. */
#define ATOMICOPS_INLINE __attribute__((always_inline)) static inline

/**
 * Atomically stores a 32-bit value to memory, giving
 * back the old value.
 *
 * @param ptr    Memory to store the value to.
 * @param newVal Value to store.
 *
 * @return Old value.
 */
ATOMICOPS_INLINE uint32_t AtomicOps_Swap(volatile uint32_t *ptr, uint32_t newVal) {
	uint32_t oldVal, strexRet;

	// TODO: too many memory barriers?
	asm volatile("@ AtomicOps_Swap\n\t"
		"dmb\n"
	"1:\n\t"
		"ldrex %0, [%2]\n\t"
		"strex %1, %3, [%2]\n\t"
		"teq   %1, #0\n\t"
		"bne   1b\n\t"
		"dmb"
	: "=&r" (oldVal), "=&r" (strexRet)
	: "r" (ptr), "r" (newVal)
	: "memory", "cc");

	return oldVal;
}

/**
 * Atomically stores a 32-bit value to memory, only if
 * the value currently in memory is equal to the expected
 * value, giving back the old value.
 *
 * @param ptr    Memory to store the value to.
 * @param expVal Expected value.
 * @param newVal Value to store.
 *
 * @return Old value.
 */
ATOMICOPS_INLINE uint32_t AtomicOps_CmpSwap(volatile uint32_t *ptr, uint32_t expVal, uint32_t newVal) {
	uint32_t loadVal, strexRet;

	// TODO: too many memory barriers?
	asm volatile("@ AtomicOps_CmpSwap\n\t"
		"dmb\n"
	"1:\n\t"
		"ldrex   %0, [%2]\n\t"
		"teq     %0, %3\n\t"
		"itt     eq\n\t"
		"strexeq %1, %4, [%2]\n\t"
		"teqeq   %1, #1\n\t"
		"beq     1b\n\t"
		"dmb"
	: "=&r" (loadVal), "=&r" (strexRet)
	: "r" (ptr), "r" (expVal), "r" (newVal)
	: "memory", "cc");

	return loadVal;
}

/**
 * Atomically adds to a 32-bit value in memory.
 *
 * @param ptr Memory where the first addend is and
 *            where the result will be stored.
 * @param n   Second addend.
 *
 * @return New value after addition.
 */
ATOMICOPS_INLINE uint32_t AtomicOps_Add(volatile uint32_t *ptr, uint32_t n) {
	uint32_t result, strexRet;

	// TODO: too many memory barriers?
	asm volatile("@ AtomicOps_Add:\n\t"
		"dmb\n"
	"1:\n\t"
		"ldrex %0, [%2]\n\t"
		"add   %0, %0, %3\n\t"
		"strex %1, %0, [%2]\n\t"
		"teq   %1, #0\n\t"
		"bne   1b\n\t"
		"dmb"
	: "=&r" (result), "=&r" (strexRet)
	: "r" (ptr), "r" (n)
	: "memory", "cc");

	return result;
}

#undef ATOMICOPS_INLINE

#endif
