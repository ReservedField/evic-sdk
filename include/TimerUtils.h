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
 *
 * @param delay Delay in microseconds. Valid range is 0 - 233016.
 */
void Timer_DelayUs(uint32_t delay);

/**
 * Delays for the specified time.
 *
 * @param delay Delay in milliseconds.
 */
void Timer_DelayMs(uint32_t delay);

#ifdef __cplusplus
}
#endif

#endif