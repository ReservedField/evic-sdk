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
#include <Timer.h>
#include <Battery.h>

int main() {
	int i;
	char buf[100];
	uint16_t battVolt;

	while(1) {
		// Check if the battery is present
		if(Battery_IsPresent()) {
			// Read voltage
			battVolt = Battery_GetVoltage();
			sprintf(buf, "Voltage:\n%d.%03d V", battVolt / 1000, battVolt % 1000);
		}
		else {
			sprintf(buf, "No batt");
		}

		// Clear and blit text
		Display_Clear();
		Display_PutText(0, 0, buf, FONT_DEJAVU_8PT);
		Display_Update();

		// Update every second
		for(i = 0; i < 10; i++) {
			Timer_DelayUs(100000);
		}
	}
}
