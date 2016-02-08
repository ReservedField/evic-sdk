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

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif
