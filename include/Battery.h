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

#ifndef EVICSDK_BATTERY_H
#define EVICSDK_BATTERY_H

/**
 * Initializes the battery I/O.
 * System control registers must be unlocked.
 */
void Battery_Init();

/**
 * Checks if the battery is present.
 *
 * @return True if the battery is present, false otherwise.
 */
uint8_t Battery_IsPresent();

/**
 * Reads the battery voltage.
 * If the battery is not present, this will return wrong values.
 * Always check for battery before trying to read the voltage.
 *
 * @return Battery voltage, in millivolts.
 */
uint16_t Battery_GetVoltage();

#endif
