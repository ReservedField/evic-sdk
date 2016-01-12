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
#include <Timer.h>
#include <Display.h>
#include <Display_SSD.h>
#include <Display_SSD1306.h>
#include <Display_SSD1327.h>
#include <Dataflash.h>

void Display_SSD_Write(uint8_t isData, const uint8_t *buf, uint32_t len) {
	int i;

	// Set D/C#
	DISPLAY_SSD_DC = isData ? 1 : 0;

	for(i = 0; i < len; i++) {
		// Send byte, wait until it is transmitted
		SPI_WRITE_TX(SPI0, buf[i]);
		while(SPI_IS_BUSY(SPI0));
	}
}

void Display_SSD_SendCommand(uint8_t cmd) {
	Display_SSD_Write(0, &cmd, 1);
}

void Display_SSD_Flip() {
	if(Display_type == DISPLAY_SSD1327) {
		Display_SSD1327_Flip();
	}
	else {
		Display_SSD1306_Flip();
	}
}

void Display_SSD_Update(const uint8_t *framebuf) {
	if(Display_type == DISPLAY_SSD1327) {
		Display_SSD1327_Update(framebuf);
	}
	else {
		Display_SSD1306_Update(framebuf);
	}
}

void Display_SSD_Init() {
	int i;
	uint8_t *initCmds;
	uint8_t initCmds_size;

	if(Display_type == DISPLAY_SSD1327) {
		initCmds = Display_SSD1327_initCmds;
		initCmds_size = sizeof(Display_SSD1327_initCmds);
	}
	else {
		initCmds = Display_SSD1306_initCmds;
		initCmds_size = sizeof(Display_SSD1306_initCmds);
	}

	// Reset display controller
	// TODO: figure out PA.1 and PC.4
	PA1 = 1;
	PC4 = 1;
	Timer_DelayUs(1000);
	DISPLAY_SSD_RESET = 0;
	Timer_DelayUs(1000);
	DISPLAY_SSD_RESET = 1;
	Timer_DelayUs(1000);

	// Send initialization commands
	for(i = 0; i < initCmds_size; i++) {
		Display_SSD_SendCommand(*(initCmds + i));
	}

	if(Dataflash_info.flipDisplay) {
		Display_SSD_Flip();
	}

	// Update GDDRAM
	Display_Update();

	// Display ON
	Display_SSD_SetOn(1);

	// Delay 20ms
	Timer_DelayUs(20000);
}

void Display_SSD_SetOn(uint8_t isOn) {
	Display_SSD_SendCommand(isOn ? SSD_DISPLAY_ON : SSD_DISPLAY_OFF);
}
