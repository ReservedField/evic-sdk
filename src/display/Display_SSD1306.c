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
#include <Display_SSD.h>
#include <Display_SSD1306.h>
#include <Display.h>
#include <Timer.h>
#include <Dataflash.h>

static uint8_t Display_SSD1306_initCmds1[] = {
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
	SSD1306_ENTIRE_DISPLAY_ON,
	SSD1306_NORMAL_DISPLAY,
	0xAD,
	0x8A,
	SSD1306_SET_PRECHARGE,   0x22,
	SSD1306_SET_VCOMH,       0x35
};

static uint8_t Display_SSD1306_initCmds2[] = {
	SSD1306_SET_COM_NORMAL,
	SSD1306_SET_OFFSET, 0x60,
	0xDC,
	0x20,
	SSD1306_SET_NOREMAP
};

void Display_SSD1306_Init() {
	int i;

	// Reset controller
	// TODO: figure out PA.1 and PC.4
	PA1 = 1;
	PC4 = 1;
	Timer_DelayUs(1000);
	DISPLAY_SSD_RESET = 0;
	Timer_DelayUs(1000);
	DISPLAY_SSD_RESET = 1;
	Timer_DelayUs(1000);

	// Send initialization commands (1)
	for(i = 0; i < sizeof(Display_SSD1306_initCmds1); i++) {
		Display_SSD_SendCommand(Display_SSD1306_initCmds1[i]);
	}

	if(Dataflash_info.flipDisplay) {
		// Send initialization commands (2)
		for(i = 0; i < sizeof(Display_SSD1306_initCmds2); i++) {
			Display_SSD_SendCommand(Display_SSD1306_initCmds2[i]);
		}
	}

	// Update GDDRAM
	Display_Update();

	// Display ON
	Display_SSD_SendCommand(SSD_DISPLAY_ON);

	// Delay 20ms
	Timer_DelayUs(20000);
}

void Display_SSD1306_Update(const uint8_t *framebuf) {
	int page;

	for(page = 0; page < DISPLAY_FRAMEBUFFER_PAGES; page++) {
		// Set page start address (start 0x00, end 0x12/0x10)
		Display_SSD_SendCommand(SSD1306_PAGE_START_ADDRESS | page);
		Display_SSD_SendCommand(0x00);
		Display_SSD_SendCommand(Dataflash_info.flipDisplay ? 0x12 : 0x10);

		// Write page to GDDRAM
		Display_SSD_Write(1, framebuf, DISPLAY_FRAMEBUFFER_PAGE_SIZE);
		framebuf += DISPLAY_FRAMEBUFFER_PAGE_SIZE;
	}
}
