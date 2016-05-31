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

#ifndef EVICSDK_BUTTON_H
#define EVICSDK_BUTTON_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * No button mask
 */
#define BUTTON_MASK_NONE  0x00
/**
 * No button mask.
 */
#define BUTTON_MASK_NONE  0x00
/**
 * Fire button mask.
 */
#define BUTTON_MASK_FIRE  0x01
/**
 * Right button mask.
 */
#define BUTTON_MASK_RIGHT 0x02
/**
 * Left button mask.
 */
#define BUTTON_MASK_LEFT  0x04

/**
 * Function pointer type for button callbacks.
 * The current button state as returned by Button_GetState()
 * is passed as the first argument.
 * Callbacks will be invoked from an interrupt handler,
 * so they should be as fast as possible. You'll typically
 * want to just set a flag and return, and then act on that
 * flag from you main application loop.
 */
typedef void (*Button_Callback_t)(uint8_t);

/**
 * Initializes the buttons I/O.
 * System control registers must be unlocked.
 */
void Button_Init();

/**
 * Gets the buttons state.
 * The return value can be tested using the
 * button masks.
 *
 * @return Button state.
 */
uint8_t Button_GetState();

/**
 * Creates a button callback. It will be called every time the specified
 * buttons change state. There are three callback slots available to users.
 *
 * @param callback Callback function.
 * @param buttonMask Bitwise OR of BUTTON_MASK_* values to specify which
 *                   buttons the callback should be notified of.
 *
 * @return A positive index if the callback was successfully created, or a
 *         negative value if no callback slots are available or if callback
 *         is NULL.
 */
int8_t Button_CreateCallback(Button_Callback_t callback, uint8_t buttonMask);

/**
 * Deletes a callback.
 *
 * @param index Callback inde as returned by Button_CreateCallback().
 */
void Button_DeleteCallback(int8_t index);

#ifdef __cplusplus
}
#endif

#endif
