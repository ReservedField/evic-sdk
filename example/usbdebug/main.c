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

#include <string.h>
#include <ctype.h>
#include <M451Series.h>
#include <Display.h>
#include <Font.h>
#include <USB_VirtualCOM.h>
#include <TimerUtils.h>
#include <Button.h>

// This example has two modes:
// 1) When first started, it will send the
//    "Hello, USB!" greeting every 3 seconds.
// 2) When fire is kept pressed for 3 seconds
//    it switches to receive mode: it will display
//    the last 64 received characters.

int main() {
	char tmpBuf[9];
	uint8_t recvBuf[64], bufPos, i, tmp;
	uint16_t readSize;

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

	// Send mode
	while(!(Button_GetState() & BUTTON_MASK_FIRE)) {
		// Send string over the virtual COM port
		USB_VirtualCOM_SendString("Hello, USB!\r\n");

		// Wait 3 seconds
		Timer_DelayMs(3000);
	}

	// Receive mode
	memset(recvBuf, 0x00, sizeof(recvBuf));
	bufPos = 0;
	while(1) {
		if(USB_VirtualCOM_GetAvailableSize() != 0) {
			// Data is available
			// We read 1 byte at a time because I'm too lazy
			// to write a real ring buffer for this example
			readSize = USB_VirtualCOM_Read(&tmp, 1);
			// Filter out control characters for display
			if(readSize && isprint(tmp)) {
				recvBuf[bufPos] = tmp;
				bufPos = (bufPos + readSize) % 64;
			}
		}

		// Display the received data
		Display_Clear();
		for(i = 0; i < 8; i++) {
			// We use an auxiliary buffer to keep 8 chars + terminator
			memcpy(tmpBuf, recvBuf + 8 * i, 8);
			tmpBuf[8] = '\0';
			// Blit one 8-char line of text
			Display_PutText(0, 16 * i, tmpBuf, FONT_DEJAVU_8PT);
		}
		Display_Update();
	}
}
