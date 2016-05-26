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
 * Copyright (C) 2015-2016 ReservedField
 */

#ifndef EVICSDK_TIMERUTILS_H
#define EVICSDK_TIMERUTILS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Function pointer type for timer callbacks.
 * It accepts a user-defined argument. If more than 4 bytes
 * are needed, a pointer can be stored and then casted.
 * Callbacks will be invoked from an interrupt handler,
 * so they should be as fast as possible. You'll typically
 * want to just set a flag and return, and then act on that
 * flag from you main application loop.
 */
typedef void (*Timer_Callback_t)(uint32_t);

/**
 * Creates and starts a timer with a specified frequency.
 * There are three timer slots available to users.
 * For best accuracy, the frequency should be a divisor of 12MHz.
 * If you need a frequency lower than 1Hz or more precise than 1Hz,
 * look at Timer_CreateTimeout.
 * Even if the timer is one-shot, the slot will only be freed when
 * Timer_DeleteTimer is called.
 *
 * @param freq         Timer frequency, in Hz.
 * @param isPeriodic   True if the timer is periodic, false if one-shot.
 * @param callback     Timeout callback function.
 * @param callbackData Optional argument to pass to the callback function.
 *
 * @return A positive index for the newly created timer, or a negative
 *         value if there are no timer slots available or if callback is NULL.
 */
int8_t Timer_CreateTimer(uint32_t freq, uint8_t isPeriodic, Timer_Callback_t callback, uint32_t callbackData);

/**
 * Creates and starts a timer with a specified period.
 * There are three timer slots available to users.
 * Even if the timer is one-shot, the slot will only be freed when
 * Timer_DeleteTimer is called.
 *
 * @param timeout      Timer period, in milliseconds.
 * @param isPeriodic   True if the timer is periodic, false if one-shot.
 * @param callback     Timeout callback function.
 * @param callbackData Optional argument to pass to the callback function.
 *
 * @return A positive index for the newly created timer, or a negative
 *         value if there are no timer slots available or if callback is NULL.
 */
int8_t Timer_CreateTimeout(uint16_t timeout, uint8_t isPeriodic, Timer_Callback_t callback, uint32_t callbackData);

/**
 * Stops and deletes a timer.
 *
 * @param index Timer index.
 */
void Timer_DeleteTimer(int8_t index);

/**
 * Delays for the specified time.
 * Do not call from interrupt/callback context.
 *
 * @param delay Delay in milliseconds.
 */
void Timer_DelayMs(uint32_t delay);

/* Always inline, no extern version. */
#define TIMERUTILS_INLINE __attribute__((always_inline)) static inline

/**
 * Delays for the specified time.
 * Busy loops wasting CPU cycles, so it's only
 * appropriate for small delays (e.g. < 1ms).
 *
 * @param delay Delay in microseconds.
 */
TIMERUTILS_INLINE void Timer_DelayUs(uint16_t delay) {
	// Clock is 72MHz and every subs + bne iteration takes 3 cycles.
	// The last iteration would take only 2 cycles due to the bne not
	// being taken, so a nop is inserted to bring it up to 3.
	// We multiply delay by 72 / 3 = 24 to get the number of iterations.
	// Multiplication is broken into:
	//  - delay += delay << 1 (i.e. delay = delay * 3)
	//  - delay  = delay << 3 (i.e. delay = delay * 8)
	// This takes the same cycles and space as mov + mul, but it doesn't
	// use an extra register to store the constant multiplier.
	// We subtract 4 cycles to the total for the checks and calculations
	// (non-taken cbz, movs, muls, subs).
	asm volatile("@ Timer_DelayUs\n\t"
		"cbz  %0, 2f\n\t"             // 2 cycles taken, 1 cycle not taken
		"add  %0, %0, %0, lsl #1\n\t" // 1 cycle
		"lsl  %0, %0, #3\n\t"         // 1 cycle
		"subs %0, %0, #4\n"           // 1 cycle
	"1:\n\t"
		"subs %0, %0, #1\n\t"         // 1 cycle
		"bne  1b\n\t"                 // 2 cycles taken, 1 cycle not taken
		"nop\n"                       // 1 cycle
	"2:"
	: "+&r" (delay)
	: : "cc");
}

#undef TIMERUTILS_INLINE

#ifdef __cplusplus
}
#endif

#endif