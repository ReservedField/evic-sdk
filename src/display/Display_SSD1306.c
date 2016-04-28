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
#include <Display_SSD1306.h>
#include <Display.h>
#include <TimerUtils.h>

static const uint8_t Display_SSD1306_initCmds[] = {
	SSD_DISPLAY_OFF,
	SSD_SET_MULTIPLEX_RATIO, 0x3F,
	SSD1306_SET_CLOCK_DIV,   0xF1,
	SSD1306_SET_COM_REMAP,
	SSD1306_SET_OFFSET,      0x20,
	0xDC,
	0x00,
	0x20,
	SSD_SET_CONTRAST_LEVEL,  0x2F,
	SSD1306_SET_REMAP,
	SSD1306_OUTPUT_GDDRAM,
	SSD1306_NORMAL_DISPLAY,
	0xAD,
	0x8A,
	SSD1306_SET_PRECHARGE,   0x22,
	SSD1306_SET_VCOMH,       0x35
};

void Display_SSD1306_PowerOn() {
	DISPLAY_SSD_VDD = 1;
	DISPLAY_SSD_VCC = 1;
	Timer_DelayUs(1000);
	DISPLAY_SSD_RESET = 0;
	Timer_DelayUs(1000);
	DISPLAY_SSD_RESET = 1;
	Timer_DelayUs(1000);
}

void Display_SSD1306_PowerOff() {
	Display_SSD_SendCommand(SSD_DISPLAY_OFF);
	DISPLAY_SSD_VCC = 0;
	Timer_DelayUs(100000);
	DISPLAY_SSD_VDD = 0;
	Timer_DelayUs(100000);
	DISPLAY_SSD_RESET = 0;
	Timer_DelayUs(100000);
}

void Display_SSD1306_SendInitCmds() {
	Display_SSD_Write(0, Display_SSD1306_initCmds, sizeof(Display_SSD1306_initCmds));
}

void Display_SSD1306_Update(const uint8_t *framebuf) {
	int page, x;

	for(page = 0; page < SSD1306_NUM_PAGES; page++) {
		// Set page start address (start 0x00, end 0x12/0x10)
		Display_SSD_SendCommand(SSD1306_PAGE_START_ADDRESS | page);
		Display_SSD_SendCommand(0x00);
		Display_SSD_SendCommand(Display_IsFlipped() ? 0x12 : 0x10);

		// Write page to GDDRAM
		for(x = 0; x < DISPLAY_WIDTH; x++) {
			Display_SSD_Write(1, &framebuf[x * (DISPLAY_HEIGHT / 8) + page], 1);
		}
	}
}

void Display_SSD1306_Flip() {
	bool flipped;

	flipped = Display_IsFlipped();

	Display_SSD_SendCommand(flipped ? SSD1306_SET_COM_NORMAL : SSD1306_SET_COM_REMAP);
	Display_SSD_SendCommand(SSD1306_SET_OFFSET);
	Display_SSD_SendCommand(flipped ? 0x60 : 0x20);
	Display_SSD_SendCommand(0xDC);
	Display_SSD_SendCommand(flipped ? 0x20 : 0x00);
	Display_SSD_SendCommand(flipped ? SSD1306_SET_NOREMAP : SSD1306_SET_REMAP);
}

void Display_SSD1306_SetInverted(bool invert) {
	Display_SSD_SendCommand(invert ? SSD1306_INVERTED_DISPLAY : SSD1306_NORMAL_DISPLAY);
}
