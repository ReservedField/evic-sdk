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

/*
 * Display controller commands.
 */
#define SSD1306_SET_NOREMAP        0xA0
#define SSD1306_SET_REMAP          0xA1
#define SSD1306_ENTIRE_DISPLAY_ON  0xA4
#define SSD1306_NORMAL_DISPLAY     0xA6
#define SSD1306_PAGE_START_ADDRESS 0xB0
#define SSD1306_SET_COM_NORMAL     0xC0
#define SSD1306_SET_COM_REMAP      0xC8
#define SSD1306_SET_OFFSET         0xD3
#define SSD1306_SET_CLOCK_DIV      0xD5
#define SSD1306_SET_PRECHARGE      0xD9
#define SSD1306_SET_VCOMH          0xDB


/*
 * Initialization commands
 */
extern uint8_t Display_SSD1306_initCmds[22];

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

/**
 * Flips the display according to the display orientation value in data flash.
 * An update must be issued afterwards.
 */
void Display_SSD1306_Flip();

#endif
