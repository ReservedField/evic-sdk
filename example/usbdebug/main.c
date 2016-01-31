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

#include <M451Series.h>
#include <Display.h>
#include <Font.h>
#include <USB_VirtualCOM.h>
#include <TimerUtils.h>

int main() {
	// The virtual COM port is not initialized by default.
	// To initialize it, follow those steps:
	// 1) Unlock system control registers.
	SYS_UnlockReg();
	// 2) Initialize the virtual COM port.
	USB_VirtualCOM_Init();
	// 3) Lock system control registers.
	SYS_LockReg();

	// Display some text, just to know it's working
	Display_PutText(0, 0, "USB OK.", FONT_DEJAVU_8PT);
	Display_Update();

	while(1) {
		// Send string over the virtual COM port
		USB_VirtualCOM_SendString("Hello, USB!\r\n");

		// Wait 3 seconds
		Timer_DelayMs(3000);
	}
}
