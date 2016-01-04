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

#ifndef EVICSDK_DISPLAY_H
#define EVICSDK_DISPLAY_H

#include <Font.h>

/**
 * Framebuffer page size.
 */
#define DISPLAY_FRAMEBUFFER_PAGE_SIZE 0x40

/**
 * Number of framebuffer pages.
 */
#define DISPLAY_FRAMEBUFFER_PAGES 0x10

/**
 * Framebuffer size.
 */
#define DISPLAY_FRAMEBUFFER_SIZE (DISPLAY_FRAMEBUFFER_PAGE_SIZE * DISPLAY_FRAMEBUFFER_PAGES)

/**
 * Display width, in pixels.
 */
#define DISPLAY_WIDTH 64

/**
 * Display height, in pixels.
 */
#define DISPLAY_HEIGHT 128

/**
 * LCD type.
 * 0 -> SSD1327
 * 1 -> SSD1306
 */
#define LCD_TYPE 1

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
 * Sends the framebuffer to the controller and updates the display.
 */
void Display_Update();

/**
 * Copies a bitmap in the framebuffer.
 * Each byte in the pixel buffer encodes a 8-pixel column.
 * Each bit is 1 if the pixel is on, 0 if it is off.
 * Top to bottom is MSB to LSB.
 * The bitmap is made of rows, from top-left to bottom-right.
 * The top-left corner is at (0, 0).
 * TODO: this does not support non-8-aligned Y.
 *
 * @param x      X coordinate to place the bitmap at.
 * @param y      Y coordinate to place the bitmap at.
 * @param pixels Pixel buffer.
 * @param w      Width of the bitmap.
 * @param h      Height of the bitmap.
 */
void Display_PutPixels(int x, int y, const uint8_t *pixels, int w, int h);

/**
 * Blits text in the framebuffer.
 * TODO: this does not support non-8-aligned Y.
 *
 * @param x    X coordinate to place the text at.
 * @param y    Y coordinato to place the text at.
 * @param txt  Text string.
 * @param font Font to use.
 */
void Display_PutText(int x, int y, const char *txt, const Font_Info_t *font);

#endif
