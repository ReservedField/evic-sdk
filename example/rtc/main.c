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

#include <stdio.h>
#include <M451Series.h>
#include <Display.h>
#include <Font.h>
#include <RTCUtils.h>

// Names of weekdays
const char const *weekDays[] = {
	"Sun", "Mon", "Tue", "Wed",
	"Thu", "Fri", "Sat"
};

// Names of months
const char const *months[] = {
	"Jan", "Feb", "Mar", "Apr",
	"May", "Jun", "Jul", "Aug",
	"Sep", "Nov", "Dec"
};

int main() {
	char buf[100];
	RTCUtils_DateTime_t dateTime;

	// Initialize RTC date/time
	// Wednesday, 4th May 2016, 23:59:00
	dateTime.year      = 16;
	dateTime.month     = 5;
	dateTime.day       = 4;
	dateTime.dayOfWeek = 3;
	dateTime.hour      = 23;
	dateTime.minute    = 59;
	dateTime.second    = 0;
	RTCUtils_SetDateTime(&dateTime);

	while(1) {
		// Get RTC date/time
		RTCUtils_GetDateTime(&dateTime);

		// Display date/time
		siprintf(buf, "%s %02d\n%s %d\n\n%02d:%02d:%02d",
			weekDays[dateTime.dayOfWeek], dateTime.day,
			months[dateTime.month - 1], 2000 + dateTime.year,
			dateTime.hour, dateTime.minute, dateTime.second);
		Display_Clear();
		Display_PutText(0, 0, buf, FONT_DEJAVU_8PT);
		Display_Update();
	}
}
