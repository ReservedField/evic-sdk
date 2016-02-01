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
 * Timer callback pointers.
 * NULL when the timer is unused.
 */
static volatile Timer_Callback_t Timer_CallbackPtr[4] = {NULL};

/**
 * Timer callback user-defined data.
 */
static volatile uint32_t Timer_CallbackData[4] = {0};

/**
 * Lookup table for timer pointers.
 */
static TIMER_T *Timer_TimerPtr[] = {TIMER0, TIMER1, TIMER2, TIMER3};

/**
 * Lookup table for timer IRQ numbers.
 */
static IRQn_Type Timer_IrqNum[] = {TMR0_IRQn, TMR1_IRQn, TMR2_IRQn, TMR3_IRQn};

/**
 * Convenience macro to define timer IRQ handlers.
 */
#define TIMER_DEFINE_IRQ_HANDLER(n) void TMR ## n ## _IRQHandler() { \
	if(TIMER_GetIntFlag(TIMER ## n) == 1) { \
		TIMER_ClearIntFlag(TIMER ## n); \
		Timer_CallbackPtr[n](Timer_CallbackData[n]); \
	} \
}

TIMER_DEFINE_IRQ_HANDLER(0);
TIMER_DEFINE_IRQ_HANDLER(1);
TIMER_DEFINE_IRQ_HANDLER(2);
TIMER_DEFINE_IRQ_HANDLER(3);

int8_t Timer_CreateTimer(uint32_t freq, uint8_t isPeriodic, Timer_Callback_t callback, uint32_t callbackData) {
	uint8_t i;

	// Find an unused timer
	for(i = 0; i < 4 && Timer_CallbackPtr[i] != NULL; i++);

	if(i == 4) {
		// All timers are in use
		return -1;
	}

	Timer_CallbackPtr[i] = callback;
	Timer_CallbackData[i] = callbackData;

	// Set up and start timer
	TIMER_Open(Timer_TimerPtr[i], isPeriodic ? TIMER_PERIODIC_MODE : TIMER_ONESHOT_MODE, freq);
	TIMER_EnableInt(Timer_TimerPtr[i]);
	NVIC_EnableIRQ(Timer_IrqNum[i]);
	TIMER_Start(Timer_TimerPtr[i]);

	return i;
}

void Timer_DeleteTimer(int8_t index) {
	if(index < 0 || index > 3) {
		// Invalid index
		return;
	}

	TIMER_Close(Timer_TimerPtr[index]);
	NVIC_DisableIRQ(Timer_IrqNum[index]);
	TIMER_DisableInt(Timer_TimerPtr[index]);
	Timer_CallbackPtr[index] = NULL;
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
