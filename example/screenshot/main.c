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
#include <Button.h>
#include <USB_VirtualCOM.h>

// Reuse bitmap from 'bitmap' example
#include "../bitmap/Bitmap_eVicSDK.h"

// To get a screenshot:
// 1) Open up COM terminal, then press fire on the device
// 2) Copypaste/pipe hexdump to a file (e.g screen.hex)
// 3) xxd -r -p screen.hex screen.bin
// 4) $EVICSDK/tools/img-decode 64 128 screen.bin screen.bmp

int main() {
	char buf[3];
	uint8_t *framebuf, firePressed, btnState;
	uint16_t i;

	// Init USB virtual COM port (synchronous)
	SYS_UnlockReg();
	USB_VirtualCOM_Init();
	SYS_LockReg();
	USB_VirtualCOM_SetAsyncMode(0);

	// Get framebuffer address
	// I'm putting it before drawing to show it doesn't change
	framebuf = Display_GetFramebuffer();

	// Show some stuff on the display
	Display_PutText(0, 0, "Draw  me\nlike one\nof  your\n french \n girls. ", FONT_DEJAVU_8PT);
	Display_PutPixels(0, 64, Bitmap_evicSdk, Bitmap_evicSdk_width, Bitmap_evicSdk_height);
	Display_Update();

	firePressed = 0;
	while(1) {
		// Take screenshot when fire is pressed
		btnState = Button_GetState();
		if(!firePressed && (btnState & BUTTON_MASK_FIRE)) {
			firePressed = 1;

			// Send framebuffer as hexdump over USB
			for(i = 0; i < DISPLAY_FRAMEBUFFER_SIZE; i++) {
				siprintf(buf, "%02x", framebuf[i]);
				USB_VirtualCOM_SendString(buf);
			}
		}
		else if(firePressed && !(btnState & BUTTON_MASK_FIRE)) {
			firePressed = 0;
		}
	}
}
