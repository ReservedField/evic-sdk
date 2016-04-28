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

#ifndef EVICSDK_DISPLAY_SSD1306_H
#define EVICSDK_DISPLAY_SSD1306_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Display controller commands.
 */
#define SSD1306_SET_NOREMAP        0xA0
#define SSD1306_SET_REMAP          0xA1
#define SSD1306_OUTPUT_GDDRAM      0xA4
#define SSD1306_NORMAL_DISPLAY     0xA6
#define SSD1306_INVERTED_DISPLAY   0xA7
#define SSD1306_PAGE_START_ADDRESS 0xB0
#define SSD1306_SET_COM_NORMAL     0xC0
#define SSD1306_SET_COM_REMAP      0xC8
#define SSD1306_SET_OFFSET         0xD3
#define SSD1306_SET_CLOCK_DIV      0xD5
#define SSD1306_SET_PRECHARGE      0xD9
#define SSD1306_SET_VCOMH          0xDB

/**
 * Number of pages in SSD1306 GDDRAM.
 */
#define SSD1306_NUM_PAGES 0x10

/**
 * Performs the controller power-on sequence.
 */
void Display_SSD1306_PowerOn();

/**
 * Performs the controller power-off sequence.
 */
void Display_SSD1306_PowerOff();

/**
 * Sends the initialization commands to the controller.
 */
void Display_SSD1306_SendInitCmds();

/**
 * Sends the framebuffer to the controller and updates the display.
 *
 * @param framebuf Framebuffer.
 */
void Display_SSD1306_Update(const uint8_t *framebuf);

/**
 * Flips the display according to the display orientation value in data flash.
 * An update must be issued afterwards.
 */
void Display_SSD1306_Flip();

/**
 * Sets whether the display colors are inverted.
 *
 * @param invert True for inverted display, false for normal display.
 */
void Display_SSD1306_SetInverted(bool invert);

#ifdef __cplusplus
}
#endif

#endif
