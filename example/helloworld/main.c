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
 * Copyright (C) 2015 ReservedField
 */

#include <M451Series.h>
#include <Display.h>

#include "hello_image.h"

uint8_t framebuf[DISPLAY_FRAMEBUFFER_SIZE];

int main() {
	// Initialize I/O
	SYS_UnlockReg();
	Display_SetupSPI();
	SYS_LockReg();
	
	// Initialize display
	Display_Init();

	// Build framebuffer
	Display_PutPixels(framebuf, 0, 0, bitmap, bitmapWidth, bitmapHeight);

	// Update display
	Display_Update(framebuf);
}