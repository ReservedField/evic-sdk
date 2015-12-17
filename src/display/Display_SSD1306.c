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
#include <Display_SSD1306.h>
#include <Display.h>
#include <Timer.h>

#define DISPLAY_SSD1306_RESET PA0
#define DISPLAY_SSD1306_DC    PE10

#define TODO_DATAFLAG 1

static uint8_t Display_SSD1306_initCmds1[] = {
	0xAE, 0xA8, 0x3F, 0xD5, 0xF1, 0xC8, 0xD3, 0x20, 0xDC, 0x00, 0x20,
	0x81, 0x2F, 0xA1, 0xA4, 0xA6, 0xAD, 0x8A, 0xD9, 0x22, 0xDB, 0x35
};

static uint8_t Display_SSD1306_initCmds2[] = {
	0xC0, 0xD3, 0x60, 0xDC, 0x20, 0xA0
};

void Display_SSD1306_Write(uint8_t isData, const uint8_t *buf, uint32_t len) {
	int i;
	
	// Set D/C#
	DISPLAY_SSD1306_DC = isData ? 1 : 0;
	
	for(i = 0; i < len; i++) {
		// Send byte, wait until it is transmitted
		SPI_WRITE_TX(SPI0, buf[i]);
		while(SPI_IS_BUSY(SPI0));
	}
}

void Display_SSD1306_SendCommand(uint8_t cmd) {
	Display_SSD1306_Write(0, &cmd, 1);
}

void Display_SSD1306_Init() {
	int i, page;
	uint8_t buf;
	
	// Reset controller
	// TODO: figure out PA.1 and PC.4
	PA1 = 1;
	PC4 = 1;
	Timer_DelayUs(1000);
	DISPLAY_SSD1306_RESET = 0;
	Timer_DelayUs(1000);
	DISPLAY_SSD1306_RESET = 1;
	Timer_DelayUs(1000);
	
	// Send initialization commands (1)
	for(i = 0; i < sizeof(Display_SSD1306_initCmds1); i++) {
		Display_SSD1306_SendCommand(Display_SSD1306_initCmds1[i]);
	}
	
	if(TODO_DATAFLAG) {
		// Send initialization commands (2)
		for(i = 0; i < sizeof(Display_SSD1306_initCmds2); i++) {
			Display_SSD1306_SendCommand(Display_SSD1306_initCmds2[i]);
		}
	}

	// Clear GDDRAM
	for(page = 0; page < DISPLAY_FRAMEBUFFER_PAGES; page++) {
		// Set page start address (start 0x00, end 0x12/0x10)
		Display_SSD1306_SendCommand(0xB0 | page);
		Display_SSD1306_SendCommand(0x00);
		Display_SSD1306_SendCommand(TODO_DATAFLAG ? 0x12 : 0x10);
		
		// Clear page in GDDRAM
		buf = 0x00;
		for(i = 0; i < DISPLAY_FRAMEBUFFER_PAGE_SIZE; i++) {
			Display_SSD1306_Write(1, &buf, 1);
		}
	}
	
	// Display ON
	Display_SSD1306_SendCommand(0xAF);
	
	// Delay 20ms
	Timer_DelayUs(20000);
}

void Display_SSD1306_Update(const uint8_t *framebuf) {
	int page;
	
	for(page = 0; page < DISPLAY_FRAMEBUFFER_PAGES; page++) {
		// Set page start address (start 0x00, end 0x12/0x10)
		Display_SSD1306_SendCommand(0xB0 | page);
		Display_SSD1306_SendCommand(0x00);
		Display_SSD1306_SendCommand(TODO_DATAFLAG ? 0x12 : 0x10);
		
		// Write page to GDDRAM
		Display_SSD1306_Write(1, framebuf, DISPLAY_FRAMEBUFFER_PAGE_SIZE);
		framebuf += DISPLAY_FRAMEBUFFER_PAGE_SIZE;
	}
}
