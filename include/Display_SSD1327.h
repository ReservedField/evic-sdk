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

#ifndef EVICSDK_DISPLAY_SSD1327_H
#define EVICSDK_DISPLAY_SSD1327_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Display controller commands.
 */
#define SSD1327_SET_COL_ADDRESS      0x15
#define SSD1327_SET_ROW_ADDRESS      0x75
#define SSD1327_SET_REMAP            0xA0
#define SSD1327_SET_START_LINE       0xA1
#define SSD1327_SET_OFFSET           0xA2
#define SSD1327_NORMAL_DISPLAY       0xA4
#define SSD1327_INVERTED_DISPLAY     0xA7
#define SSD1327_FUNC_SELECT_A        0xAB
#define SSD1327_SET_PHASE_LENGTH     0xB1
#define SSD1327_SET_CLOCK_DIV        0xB3
#define SSD1327_SET_SECOND_PRECHARGE 0xB6
#define SSD1327_SET_PRECHARGE        0xBC
#define SSD1327_SET_VCOMH            0xBE
#define SSD1327_FUNC_SELECT_B        0xD5
#define SSD1327_SET_COMMAND_LOCK     0xFD

/**
 * Performs the controller power-on sequence.
 */
void Display_SSD1327_PowerOn();

/**
 * Performs the controller power-off sequence.
 */
void Display_SSD1327_PowerOff();

/**
 * Sends the initialization commands to the controller.
 */
void Display_SSD1327_SendInitCmds();

/**
 * Sends the framebuffer to the controller and updates the display.
 *
 * @param framebuf Framebuffer.
 */
void Display_SSD1327_Update(const uint8_t *framebuf);

/**
 * Flips the display according to the display orientation value in data flash.
 */
void Display_SSD1327_Flip();

/**
 * Sets whether the display colors are inverted.
 *
 * @param invert True for inverted display, false for normal display.
 */
void Display_SSD1327_SetInverted(bool invert);

#ifdef __cplusplus
}
#endif

#endif
