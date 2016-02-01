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
#include <Atomizer.h>
#include <Button.h>
#include <TimerUtils.h>
#include <Battery.h>

int main() {
	char buf[100];
	uint16_t volts, battVolts;
	uint8_t btnState;

	// Let's start with 3.00V as the initial value
	volts = 300;
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
			if(volts != ATOMIZER_MAX_VOLTS) {
				volts++;
			}

			// Set voltage
			Atomizer_SetOutputVoltage(volts);
			// Slow down increment
			Timer_DelayMs(50);
		}
		if(btnState & BUTTON_MASK_LEFT) {
			if(volts != 0) {
				volts--;
			}

			// Set voltage
			Atomizer_SetOutputVoltage(volts);
			// Slow down decrement
			Timer_DelayMs(50);
		}

		// Get battery voltage
		if(Battery_IsPresent()) {
			battVolts = Battery_GetVoltage() / 10;
		}
		else {
			battVolts = 0;
		}

		// Display info
		sprintf(buf, "Voltage:\n%d.%02dV\n%s\n\nBattery:\n%d.%02dV\n%s",
			volts / 100, volts % 100,
			Atomizer_IsOn() ? "FIRING" : "",
			battVolts / 100, battVolts % 100,
			Battery_IsCharging() ? "CHARGING" : "");
		Display_Clear();
		Display_PutText(0, 0, buf, FONT_DEJAVU_8PT);
		Display_Update();
	}
}
