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
 * Copyright (C) 2015 Jussi Timperi
 */

#include <M451Series.h>
#include <Display_SSD.h>
#include <Display_SSD1327.h>
#include <Display.h>
#include <Timer.h>

// Swapped to get (0,0) at top-left
#define TODO_DATAFLAG 1

// Value for pixel ON
#define GRAYHIGH 0xF0
#define GRAYLOW  0x0F

/* Changes to init commands compared to the official image:
 * Removed:
 *  0xB4 Not found in datasheet.
 *  0xB5 Set GPIO, Requires a second byte which was not there.
 * Added:
 *  SSD1327_SET_COMMAND_LOCK for unlocking the controller.
 * Modified:
 *  Set vertical re-map for dealing with the framebuffer format.
*/
static uint8_t Display_SSD1327_initCmds1[] = {
	SSD1327_SET_COMMAND_LOCK,     0x12,
	SSD_DISPLAY_OFF,
	SSD1327_SET_REMAP,            0x40,
	SSD1327_SET_START_LINE,       0x00,
	SSD1327_SET_OFFSET,           0x00,
	SSD1327_NORMAL_DISPLAY,
	SSD_SET_MULTIPLEX_RATIO,      0x7F,
	SSD1327_FUNC_SELECT_A,        0x01,
	SSD_SET_CONTRAST_LEVEL,       0xA6,
	SSD1327_SET_CLOCK_DIV,        0xB1,
	SSD1327_SET_SECOND_PRECHARGE, 0x0D,
	SSD1327_SET_PRECHARGE,        0x07,
	SSD1327_SET_VCOMH,            0x07,
	SSD1327_FUNC_SELECT_B,        0x02
};

static uint8_t Display_SSD1327_initCmds2[] = {
	SSD1327_SET_OFFSET, 0x80,
	SSD1327_SET_REMAP,  0x57
};

void Display_SSD1327_Init() {
	int i;

/*	Power on sequence from the datasheet:
		1. Power ON VCI.
		2. After VCI becomes stable, set wait time at least 1ms
		for internal VDD become stable. Then set RES# pin LOW (logic low)
		for at least 100us and then HIGH (logic high).
		3. After set RES# pin LOW (logic low), wait for at least 100us.
		Then Power ON VCC.
		4. After VCC become stable, send command AFh for display ON.
		SEG/COM will be ON after 200ms
	Reset controller
	TODO: figure out PA.1 and PC.4
	*/
	PA1 = 1;
	PC4 = 1;
	Timer_DelayUs(1000);
	DISPLAY_SSD_RESET = 0;
	Timer_DelayUs(1000);
	DISPLAY_SSD_RESET = 1;
	Timer_DelayUs(1000);

	// Send initialization commands (1)
	for(i = 0; i < sizeof(Display_SSD1327_initCmds1); i++) {
		Display_SSD_SendCommand(Display_SSD1327_initCmds1[i]);
	}

	// Swapped flag to get (0,0) at top-left
	if(!TODO_DATAFLAG) {
		// Send initialization commands (2)
		for(i = 0; i < sizeof(Display_SSD1327_initCmds2); i++) {
			Display_SSD_SendCommand(Display_SSD1327_initCmds2[i]);
		}
	}

	// Update GDDRAM
	Display_Update();

	// Display ON
	Display_SSD_SendCommand(SSD_DISPLAY_ON);

	// Delay 20ms
	Timer_DelayUs(20000);
}

void Display_SSD1327_Update(const uint8_t *framebuf) {
	int col, row, bit;
	uint8_t value, pixelOne, pixelTwo;

	Display_SSD_SendCommand(SSD1327_SET_ROW_ADDRESS);
	Display_SSD_SendCommand(0x00); // Start
	Display_SSD_SendCommand(0x7F); // End

	Display_SSD_SendCommand(SSD1327_SET_COL_ADDRESS);
	Display_SSD_SendCommand(0x10); // Start
	Display_SSD_SendCommand(0x2F); // End

	// SSD1327 uses 4 bits per pixel but framebuffer is 1 bit per pixel.
	for(col = 0; col < DISPLAY_WIDTH; col += 2) {
		for(row = 0; row < 16; row++) {
			for(bit = 0; bit < 8; bit++) {
				value = 0x0;

				pixelOne = framebuf[col + (64 * row)] >> bit & 0x01;
				pixelTwo = framebuf[col + (64 * row) + 1] >> bit & 0x01;

				value |= (pixelOne) ? GRAYLOW   : 0x00;
				value |= (pixelTwo) ? GRAYHIGH  : 0x00;

				Display_SSD_Write(1, &value, 1);
			}
		}
	}
}
