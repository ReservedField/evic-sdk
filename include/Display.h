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

/**
 * \file
 * Display library header file.
 * The bitmap format is column-major.
 * Each byte encodes 8 vertical pixels.
 * A bit value of 1 means pixel on, 0 means pixel off.
 * The topmost pixel is the least significant bit.
 * The bitmap direction is top-to-bottom, left-to-right.
 * The origin (0, 0) is at the top-left corner.
 * Each column has to be padded to a multiple of 8 bits
 * (i.e. each column begins at a byte boundary). The value
 * of padding bits is ignored.
 */

#ifndef EVICSDK_DISPLAY_H
#define EVICSDK_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <Font.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Display width, in pixels.
 */
#define DISPLAY_WIDTH 64

/**
 * Display height, in pixels.
 */
#define DISPLAY_HEIGHT 128

/**
 * Framebuffer size.
 */
#define DISPLAY_FRAMEBUFFER_SIZE (DISPLAY_HEIGHT / 8 * DISPLAY_WIDTH)

/**
 * Display type enum.
 */
typedef enum {
	/**
	 * SSD1306 display.
	 */
	DISPLAY_SSD1306,
	/**
	 * SSD1327 display.
	 */
	DISPLAY_SSD1327
} Display_Type_t;

/**
 * Initializes the SPI interface for the display controller.
 * System control registers must be unlocked.
 */
void Display_SetupSPI();

/**
 * Initializes the display controller.
 */
void Display_Init();

/**
 * Returns the display controller type.
 *
 * @return Returns the display type.
 */
Display_Type_t Display_GetType();

/**
 * Turns the display on or off.
 * This only acts on the pixels, the display will
 * still be powered. If you want to control the supply
 * rails, use Display_SSD_SetPowerOn().
 *
 * @param isOn True to turn the display on, false to turn it off.
 */
void Display_SetOn(uint8_t isOn);

/**
 * Powers the display on or off.
 * This turns the actual supply rails on/off, cutting
 * off all current draw from the display when off. It is
 * slower than Display_SSD_SetOn().
 *
 * @param isPowerOn True to power on the display, false to power it off.
 */
void Display_SetPowerOn(uint8_t isPowerOn);

/**
 * Returns if the display is flipped or not.
 *
 * @return True if the display is flipped, false if not.
 */
bool Display_IsFlipped();

/**
 * Flips the display.
 */
void Display_Flip();

/**
 * Sets whether the display colors are inverted.
 *
 * @param invert True for inverted display, false for normal display.
 */
void Display_SetInverted(bool invert);

/**
 * Sends the framebuffer to the controller and updates the display.
 */
void Display_Update();

/**
 * Clears the framebuffer.
 */
void Display_Clear();

/**
 * Copies a bitmap into the framebuffer.
 *
 * @param x      X coordinate to place the bitmap at.
 * @param y      Y coordinate to place the bitmap at.
 * @param bitmap Bitmap buffer.
 * @param w      Width of the bitmap.
 * @param h      Height of the bitmap.
 */
void Display_PutPixels(int x, int y, const uint8_t *bitmap, int w, int h);

/**
 * Blits text into the framebuffer.
 *
 * @param x    X coordinate to place the text at.
 * @param y    Y coordinato to place the text at.
 * @param txt  Text string.
 * @param font Font to use.
 */
void Display_PutText(int x, int y, const char *txt, const Font_Info_t *font);

/**
 * Gets the framebuffer address.
 * Framebuffer size is DISPLAY_FRAMEBUFFER_SIZE.
 * Dimensions are DISPLAY_WIDTH and DISPLAY_HEIGHT.
 * It's stored in bitmap format.
 *
 * @return Pointer to global framebuffer.
 */
uint8_t *Display_GetFramebuffer();

/*
 * Sets the display contrast.
 *
 * @param contrast Contrast (0 - 255).
 */
void Display_SetContrast(uint8_t contrast);

#ifdef __cplusplus
}
#endif

#endif
