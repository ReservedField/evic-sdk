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

#include <stdio.h>
#include <M451Series.h>
#include <Display.h>
#include <Font.h>
#include <TimerUtils.h>

// All globals used by timer callback should be volatile
volatile uint32_t timerCounter[3] = {0};

void timerCallback(uint32_t counterIndex) {
	// We use the optional parameter as an index
	// counterIndex is the index in timerCounter
	timerCounter[counterIndex]++;
}

int main() {
	char buf[100];

	// Setup three periodic timers: 0.2Hz (5s period), 1Hz, 100Hz
	// All of them use the same callback
	// They are distinguished by the optional parameter
	Timer_CreateTimeout(5000, 1, timerCallback, 0);
	Timer_CreateTimer(1, 1, timerCallback, 1);
	Timer_CreateTimer(100, 1, timerCallback, 2);

	while(1) {
		siprintf(buf, "Count 1:\n%lu\nCount 2:\n%lu\nCount 3:\n%lu",
			timerCounter[0], timerCounter[1], timerCounter[2]);

		Display_Clear();
		Display_PutText(0, 0, buf, FONT_DEJAVU_8PT);
		Display_Update();
	}
}
