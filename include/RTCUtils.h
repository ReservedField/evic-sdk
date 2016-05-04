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

#ifndef EVICSDK_RTCUTILS_H
#define EVICSDK_RTCUTILS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Structure to hold date/time information.
 */
typedef struct {
	/**< Number of years since 2000. */
	uint8_t year;
	/**< Month (1-12). */
	uint8_t month;
	/**< Day (1-31). */
	uint8_t day;
	/**< Day of week (0-6, beginning on Sunday). */
	uint8_t dayOfWeek;
	/**< Hour (0-23). */
	uint8_t hour;
	/**< Minute (0-59). */
	uint8_t minute;
	/**< Second (0-59). */
	uint8_t second;
} RTCUtils_DateTime_t;

/**
 * Updates the RTC date and time and starts it (if needed).
 * The RTC will keep running even in sleep mode.
 *
 * @param dateTime Pointer to date/time info.
 */
void RTCUtils_SetDateTime(const RTCUtils_DateTime_t *dateTime);

/**
 * Reads date and time from the RTC.
 * Make sure you have called RTCUtils_SetDateTime before or
 * the date/time info won't be updated.
 *
 * @param dateTime Pointer to receive date/time info.
 */
void RTCUtils_GetDateTime(RTCUtils_DateTime_t *dateTime);

#ifdef __cplusplus
}
#endif

#endif
