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
#include <math.h>
#include <M451Series.h>
#include <Display.h>
#include <Font.h>
#include <Atomizer.h>
#include <Button.h>
#include <TimerUtils.h>
#include <Battery.h>

uint16_t wattsToVolts(uint32_t watts, uint16_t res) {
	// Units: mV, mW, mOhm
	// V = sqrt(P * R)
	// Round to nearest multiple of 10
	uint16_t volts = (sqrt(watts * res) + 5) / 10;
	return volts * 10;
}

int main() {
	char buf[100];
	uint16_t volts, newVolts, battVolts;
	uint32_t watts;
	uint8_t btnState, battPerc;
	Atomizer_Info_t atomInfo;

	// Initial measure
	Atomizer_ReadInfo(&atomInfo);

	// Let's start with 10.0W as the initial value
	// We keep watts as mW
	watts = 10000;
	volts = wattsToVolts(watts, atomInfo.resistance);
	Atomizer_SetOutputVoltage(volts);

	while(1) {
		btnState = Button_GetState();

		// Handle fire button
		if(!Atomizer_IsOn() && (btnState & BUTTON_MASK_FIRE)) {
			// Power on
			Atomizer_Control(1);
		}
		else if(Atomizer_IsOn() && !(btnState & BUTTON_MASK_FIRE)) {
			// Power off
			Atomizer_Control(0);
		}

		// Handle plus/minus keys
		if(btnState & BUTTON_MASK_RIGHT) {
			newVolts = wattsToVolts(watts + 100, atomInfo.resistance);

			if(newVolts <= ATOMIZER_MAX_VOLTS) {
				watts += 100;
				volts = newVolts;

				// Set voltage
				Atomizer_SetOutputVoltage(volts);
				// Slow down increment
				Timer_DelayMs(50);
			}
		}
		if(btnState & BUTTON_MASK_LEFT && watts >= 100) {
			watts -= 100;
			volts = wattsToVolts(watts, atomInfo.resistance);

			// Set voltage
			Atomizer_SetOutputVoltage(volts);
			// Slow down decrement
			Timer_DelayMs(50);
		}

		if(Atomizer_IsOn()) {
			// Update info while firing
			Atomizer_ReadInfo(&atomInfo);

			// Update output voltage to correct res variations
			// We only do 10mV steps, otherwise a flake res reading
			// can make it plummet to zero and stop.
			newVolts = wattsToVolts(watts, atomInfo.resistance);
			if(newVolts != volts) {
				if(newVolts < volts && volts >= 10) {
					volts -= 10;
				}
				else if(newVolts > volts && volts + 10 <= ATOMIZER_MAX_VOLTS) {
					volts += 10;
				}

				Atomizer_SetOutputVoltage(volts);
			}
		}

		// Get battery voltage
		if(Battery_IsPresent()) {
			battVolts = Battery_GetVoltage();
		}
		else {
			battVolts = 0;
		}

		// Calculate battery charge
		battPerc = Battery_VoltageToPercent(battVolts);

		// Display info
		sprintf(buf, "Power:\n%lu.%01luW\nV: %d.%02dV\nR: %d.%02do\nI: %lu.%02luA\n%s\n\nBattery:\n%d%%\n%s",
			watts / 1000, watts % 1000 / 100,
			atomInfo.voltage / 1000, atomInfo.voltage % 1000 / 10,
			atomInfo.resistance / 1000, atomInfo.resistance % 1000 / 10,
			atomInfo.current / 1000, atomInfo.current % 1000 / 10,
			Atomizer_IsOn() ? "FIRING" : "",
			battPerc,
			Battery_IsCharging() ? "CHARGING" : "");
		Display_Clear();
		Display_PutText(0, 0, buf, FONT_DEJAVU_8PT);
		Display_Update();
	}
}
