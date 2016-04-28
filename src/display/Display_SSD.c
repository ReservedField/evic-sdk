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
 * Copyright (C) 2016 Jussi Timperi
 */

#include <M451Series.h>
#include <TimerUtils.h>
#include <Display.h>
#include <Display_SSD.h>
#include <Display_SSD1306.h>
#include <Display_SSD1327.h>

/**
 * True if the display is powered.
 */
static uint8_t Display_SSD_isPowerOn;

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
	if(Display_GetType() == DISPLAY_SSD1327) {
		Display_SSD1327_Flip();
	}
	else {
		Display_SSD1306_Flip();
	}
}

void Display_SSD_SetInverted(bool invert) {
	if(Display_GetType() == DISPLAY_SSD1327) {
		Display_SSD1327_SetInverted(invert);
	}
	else {
		Display_SSD1306_SetInverted(invert);
	}
}

void Display_SSD_Update(const uint8_t *framebuf) {
	if(Display_GetType() == DISPLAY_SSD1327) {
		Display_SSD1327_Update(framebuf);
	}
	else {
		Display_SSD1306_Update(framebuf);
	}
}

void Display_SSD_Init() {
	// Power on and initialize controller
	Display_SSD_isPowerOn = 1;
	if(Display_GetType() == DISPLAY_SSD1327) {
		Display_SSD1327_PowerOn();
		Display_SSD1327_SendInitCmds();
	}
	else {
		Display_SSD1306_PowerOn();
		Display_SSD1306_SendInitCmds();
	}

	if(Display_IsFlipped()) {
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

void Display_SSD_SetPowerOn(uint8_t isPowerOn) {
	if(!isPowerOn == !Display_SSD_isPowerOn) {
		return;
	}

	if(isPowerOn) {
		Display_SSD_Init();
		return;
	}

	Display_SSD_isPowerOn = 0;
	if(Display_GetType() == DISPLAY_SSD1327) {
		Display_SSD1327_PowerOff();
	}
	else {
		Display_SSD1306_PowerOff();
	}
}
