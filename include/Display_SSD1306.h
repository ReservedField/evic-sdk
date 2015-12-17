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

#ifndef EVICSDK_DISPLAY_SSD1306_H
#define EVICSDK_DISPLAY_SSD1306_H

/**
 * Writes data to the display controller.
 *
 * @param isData True if writing GDDRAM data (D/C# high).
 * @param buf    Data buffer.
 * @param len    Size in bytes of the data buffer.
 */
void Display_SSD1306_Write(uint8_t isData, const uint8_t *buf, uint32_t len);

/**
 * Sends a command to the display controller.
 *
 * @param cmd Command.
 */
void Display_SSD1306_SendCommand(uint8_t cmd);

/**
 * Initializes the display controller.
 */
void Display_SSD1306_Init();

/**
 * Sends the framebuffer to the controller and updates the display.
 *
 * @param framebuf Framebuffer.
 */
void Display_SSD1306_Update(const uint8_t *framebuf);

#endif