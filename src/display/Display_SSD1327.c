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
 * Copyright (C) 2015-2016 Jussi Timperi
 */

#include <stdbool.h>
#include <Display_SSD.h>
#include <Display_SSD1327.h>
#include <Display.h>
#include <TimerUtils.h>

// Value for pixel ON
#define GRAYHIGH 0xF0
#define GRAYLOW  0x0F

/* Changes to init commands compared to the official image:
 * Modified:
 *     Set vertical re-map for dealing with the framebuffer format.
*/
static const uint8_t Display_SSD1327_initCmds[] = {
	SSD_DISPLAY_OFF,
	SSD1327_SET_REMAP,            0x44,
	SSD1327_SET_START_LINE,       0x00,
	SSD1327_SET_OFFSET,           0x00,
	SSD1327_NORMAL_DISPLAY,
	SSD_SET_MULTIPLEX_RATIO,      0x7F,
	SSD1327_FUNC_SELECT_A,        0x01,
	SSD_SET_CONTRAST_LEVEL,       0xA6,
	SSD1327_SET_PHASE_LENGTH,     0x31,
	SSD1327_SET_CLOCK_DIV,        0xB1,
	0xB4,
	0xB5,
	SSD1327_SET_SECOND_PRECHARGE, 0x0D,
	SSD1327_SET_PRECHARGE,        0x07,
	SSD1327_SET_VCOMH,            0x07,
	SSD1327_FUNC_SELECT_B,        0x02
};

void Display_SSD1327_PowerOn() {
	DISPLAY_SSD_VDD = 1;
	Timer_DelayUs(1000);
	DISPLAY_SSD_RESET = 0;
	Timer_DelayUs(10000);
	DISPLAY_SSD_RESET = 1;
	Timer_DelayUs(1000);
	DISPLAY_SSD_VCC = 1;
	Timer_DelayUs(1000);
}

void Display_SSD1327_PowerOff() {
	Display_SSD_SendCommand(SSD_DISPLAY_OFF);
	DISPLAY_SSD_VCC = 0;
	Timer_DelayUs(100000);
	DISPLAY_SSD_VDD = 0;
	Timer_DelayUs(100000);
	DISPLAY_SSD_RESET = 0;
	Timer_DelayUs(100000);
}

void Display_SSD1327_SendInitCmds() {
	Display_SSD_Write(0, Display_SSD1327_initCmds, sizeof(Display_SSD1327_initCmds));
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
		for(row = 0; row < (DISPLAY_HEIGHT / 8); row++) {
			for(bit = 0; bit < 8; bit++) {
				value = 0x0;

				pixelOne = framebuf[row + ((DISPLAY_HEIGHT / 8) * col)] >> bit & 0x01;
				pixelTwo = framebuf[row + ((DISPLAY_HEIGHT / 8) * (col + 1))] >> bit & 0x01;

				value |= (pixelOne) ? GRAYLOW   : 0x00;
				value |= (pixelTwo) ? GRAYHIGH  : 0x00;

				Display_SSD_Write(1, &value, 1);
			}
		}
	}
}

void Display_SSD1327_Flip() {
	bool flipped;

	flipped = Display_IsFlipped();

	Display_SSD_SendCommand(SSD1327_SET_OFFSET);
	Display_SSD_SendCommand(flipped ? 0x80 : 0x00);
	Display_SSD_SendCommand(SSD1327_SET_REMAP);
	Display_SSD_SendCommand(flipped ? 0x57 : 0x44);
}

void Display_SSD1327_SetInverted(bool invert) {
	Display_SSD_SendCommand(invert ? SSD1327_INVERTED_DISPLAY : SSD1327_NORMAL_DISPLAY);
}
