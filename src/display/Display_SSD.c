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