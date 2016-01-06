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

/**
 * \file
 * Display library header file.
 * The bitmap format is column-major.
 * Each byte encodes 8 vertical pixels.
 * A bit value of 1 means pixel on, 0 means pixel off.
 * The topmost pixel is the least significant bit.
 * The bitmap direction is top-to-bottom, left-to-right.
 * The origin (0, 0) is at the top-left corner.
 */

#ifndef EVICSDK_DISPLAY_H
#define EVICSDK_DISPLAY_H

#include <Font.h>

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
 * Initializes the SPI interface for the display controller.
 * System control registers must be unlocked.
 */
void Display_SetupSPI();

/**
 * Initializes the display controller.
 */
void Display_Init();

/**
 * Turns the display on or off.
 *
 * @param isOn True to turn the display on, false to turn it off.
 */
void Display_SetOn(uint8_t isOn);

/**
 * Flips the display.
 */
void Display_Flip();

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
 * TODO: this does not support non-8-aligned Y.
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
 * TODO: this does not support non-8-aligned Y.
 *
 * @param x    X coordinate to place the text at.
 * @param y    Y coordinato to place the text at.
 * @param txt  Text string.
 * @param font Font to use.
 */
void Display_PutText(int x, int y, const char *txt, const Font_Info_t *font);

#endif
