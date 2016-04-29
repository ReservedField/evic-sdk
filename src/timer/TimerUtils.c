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

#include <M451Series.h>
#include <TimerUtils.h>

/**
 * Structure for holding timeout status.
 */
typedef struct {
	/**< Number of ticks so far. */
	uint16_t tickCounter;
	/**< Number of ticks to be reached. */
	uint16_t tickTarget;
} Timer_TimeoutData_t;

/**
 * Timer callback pointers.
 * NULL when the timer is unused.
 */
static volatile Timer_Callback_t Timer_callbackPtr[4];

/**
 * Timer callback user-defined data.
 */
static volatile uint32_t Timer_callbackData[4];

/**
 * Timeout status.
 */
static volatile Timer_TimeoutData_t Timer_timeoutData[4];

/**
 * Timer information flags.
 * Bits 0-3 are assigned to timers 0-3.
 * If a bit is 1, the corresponding timer is a timeout.
 * Bits 4-7 are unused.
 */
static volatile uint8_t Timer_info;

/**
 * Lookup table for timer pointers.
 */
static TIMER_T * const Timer_TimerPtr[] = {TIMER0, TIMER1, TIMER2, TIMER3};

/**
 * Lookup table for timer IRQ numbers.
 */
static const IRQn_Type Timer_IrqNum[] = {TMR0_IRQn, TMR1_IRQn, TMR2_IRQn, TMR3_IRQn};

/**
 * Convenience macro to define timer IRQ handlers.
 */
#define TIMER_DEFINE_IRQ_HANDLER(n) void TMR ## n ## _IRQHandler() { \
	if(TIMER_GetIntFlag(TIMER ## n) == 1) { \
		TIMER_ClearIntFlag(TIMER ## n); \
		if(Timer_info & (1 << n)) { \
			Timer_HandleTimeoutTick(n); \
		} \
		else { \
			Timer_callbackPtr[n](Timer_callbackData[n]); \
		} \
	} \
}

/**
 * Handles a timeout tick.
 * This is an internal function.
 *
 * @param timerIndex Timer index.
 */
static void Timer_HandleTimeoutTick(uint8_t timerIndex) {
	Timer_timeoutData[timerIndex].tickCounter++;

	if(Timer_timeoutData[timerIndex].tickCounter >= Timer_timeoutData[timerIndex].tickTarget) {
		Timer_timeoutData[timerIndex].tickCounter = 0;
		Timer_callbackPtr[timerIndex](Timer_callbackData[timerIndex]);
	}
}

TIMER_DEFINE_IRQ_HANDLER(0);
TIMER_DEFINE_IRQ_HANDLER(1);
TIMER_DEFINE_IRQ_HANDLER(2);
TIMER_DEFINE_IRQ_HANDLER(3);

/**
 * Finds a slot for a new timer and sets it up.
 * The timer will not be started yet.
 * For best accuracy, the frequency should be a divisor of 12MHz.
 * This is an internal function.
 *
 * @param freq         Timer frequency, in Hz.
 * @param isPeriodic   True if the timer is periodic, false if one-shot.
 *
 * @return A positive index for the newly created timer, or a negative
 *         value if there are no timer slots available.
 */
static int8_t Timer_AssignSlot(uint32_t freq, uint8_t isPeriodic) {
	uint8_t i;

	// Find an unused timer
	for(i = 0; i < 4 && Timer_callbackPtr[i] != NULL; i++);

	if(i == 4) {
		// All timers are in use
		return -1;
	}

	// Set up timer
	TIMER_Open(Timer_TimerPtr[i], isPeriodic ? TIMER_PERIODIC_MODE : TIMER_ONESHOT_MODE, freq);
	TIMER_EnableInt(Timer_TimerPtr[i]);
	NVIC_EnableIRQ(Timer_IrqNum[i]);

	return i;
}

int8_t Timer_CreateTimer(uint32_t freq, uint8_t isPeriodic, Timer_Callback_t callback, uint32_t callbackData) {
	uint8_t timerIndex;

	if(callback == NULL) {
		return -1;
	}

	// Get a timer slot
	timerIndex = Timer_AssignSlot(freq, isPeriodic);
	if(timerIndex < 0) {
		return timerIndex;
	}

	// Setup callback and info
	Timer_callbackPtr[timerIndex] = callback;
	Timer_callbackData[timerIndex] = callbackData;
	Timer_info &= ~(1 << timerIndex);

	TIMER_Start(Timer_TimerPtr[timerIndex]);

	return timerIndex;
}

int8_t Timer_CreateTimeout(uint16_t timeout, uint8_t isPeriodic, Timer_Callback_t callback, uint32_t callbackData) {
	int8_t i, timerIndex;
	uint16_t tickCount;
	uint32_t timerFreq;

	if(callback == NULL) {
		return -1;
	}

	// In case someone tries to do a 0ms timeout,
	// raise to 1ms to avoid it lasting 1000ms.
	if(timeout == 0) {
		timeout = 1;
	}

	// We want to minimize the timer frequency.
	// Frequency must also evenly divide 12MHz for maximum precision.
	// Since timeout is in ms, the minimum frequency is 1KHz.
	// If N divides both timeout and 1Khz, we can use 1KHz / N
	// as the frequency and timeout / N as the counter target.
	// If N divides 1KHz, it also divides 12MHz, so the precision
	// requirement is met.
	// 1KHz = 2^3 * 5^3 Hz. Let's scan for divisors.
	timerFreq = 1000;
	tickCount = timeout;
	// 2^3
	for(i = 0; i < 3 && tickCount % 2 == 0; i++) {
		timerFreq /= 2;
		tickCount /= 2;
	}
	// 5^3
	for(i = 0; i < 3 && tickCount % 5 == 0; i++) {
		timerFreq /= 5;
		tickCount /= 5;
	}

	// Get a timer slot
	timerIndex = Timer_AssignSlot(timerFreq, isPeriodic);
	if(timerIndex < 0) {
		return timerIndex;
	}

	// Setup callback, info and timeout status
	Timer_callbackPtr[timerIndex] = callback;
	Timer_callbackData[timerIndex] = callbackData;
	Timer_info |= 1 << timerIndex;
	Timer_timeoutData[timerIndex].tickCounter = 0;
	Timer_timeoutData[timerIndex].tickTarget = tickCount;

	TIMER_Start(Timer_TimerPtr[timerIndex]);

	return timerIndex;
}

void Timer_DeleteTimer(int8_t index) {
	if(index < 0 || index > 3) {
		// Invalid index
		return;
	}

	TIMER_Close(Timer_TimerPtr[index]);
	NVIC_DisableIRQ(Timer_IrqNum[index]);
	TIMER_DisableInt(Timer_TimerPtr[index]);
	Timer_callbackPtr[index] = NULL;
}

void Timer_DelayUs(uint32_t delay) {
	CLK_SysTickDelay(delay);
}

void Timer_DelayMs(uint32_t delay) {
	uint8_t delayRem;

	// We do 233000us delays, which is nearly the maximum
	// Maximizing single delay time increases precision
	delayRem = delay % 233;
	delay = delay / 233;

	while(delay--) {
		CLK_SysTickDelay(233000);
	}

	CLK_SysTickDelay(delayRem * 1000);
}
