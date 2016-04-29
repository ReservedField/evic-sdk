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
#include <TimerUtils.h>
#include <Button.h>
#include <System.h>

volatile uint32_t timerCountdown;

void timerCallback(uint32_t unused) {
	if(timerCountdown != 0) {
		timerCountdown--;
	}
}

int main() {
	char buf[100];
	const char *lastWakeup;
	uint8_t buttonState, buttonPress = 0;

	// Wake up from:
	//  - Fire button
	//  - Battery presence change
	// Check System.h for a list of sources.
	Sys_SetWakeupSource(SYS_WAKEUP_FIRE | SYS_WAKEUP_BATTERY);

	// Setup countdown with 1Hz timer
	timerCountdown = 10;
	Timer_CreateTimer(1, 1, timerCallback, 0);

	while(1) {
		buttonState = Button_GetState();
		if(buttonState && !buttonPress) {
			// If any button is pressed, reset countdown
			timerCountdown = 10;
			buttonPress = 1;
		}
		else if(!buttonState && buttonPress) {
			// Button has been released
			buttonPress = 0;
		}

		if(timerCountdown == 0) {
			// You could actually sleep with the screen on.
			// Screen on/off is left to the programmer because
			// you might not always want to turn it on on wakeup.
			// For example, the usual 5-click power off is actually
			// just sleep, so you don't want to turn on the screen
			// until you counted five clicks.
			
			// Go to bed
			Display_SetPowerOn(0);
			Sys_Sleep();

			// Zzz...
			
			// Woke up, turn screen on and reset countdown
			Display_SetPowerOn(1);
			timerCountdown = 10;
		}

		// Display the last wakeup source nicely
		switch(Sys_GetLastWakeupSource()) {
			case SYS_WAKEUP_FIRE:
				lastWakeup = "FIRE";
				break;
			case SYS_WAKEUP_RIGHT:
				// Won't happen with our sources
				lastWakeup = "RIGHT";
				break;
			case SYS_WAKEUP_LEFT:
				// Won't happen with our sources
				lastWakeup = "LEFT";
				break;
			case SYS_WAKEUP_BATTERY:
				lastWakeup = "BATTERY";
				break;
			default:
				// No wakeup has happened yet
				lastWakeup = "---";
				break;
		}

		siprintf(buf, "Sleeping\nin: %lus\n\nSource:\n%s", timerCountdown, lastWakeup);
		Display_Clear();
		Display_PutText(0, 0, buf, FONT_DEJAVU_8PT);
		Display_Update();
	}
}
