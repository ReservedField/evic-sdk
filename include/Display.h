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
 *
 * @param framebuf Framebuffer.
 */
void Display_Update(const uint8_t *framebuf);

/**
 * Copies a bitmap in the framebuffer.
 * Each byte in the pixel buffer encodes a 8-pixel column.
 * Each bit is 1 if the pixel is on, 0 if it is off.
 * Top to bottom is MSB to LSB.
 * The bitmap is made of rows, from bottom-right to top-left.
 * The bottom-right corner is at (0, 0).
 *
 * @param framebuf Framebuffer.
 * @param x        X coordinate to place the bitmap at.
 * @param y        Y coordinate to place the bitmap at.
 * @param pixels   Pixel buffer.
 * @param w        Width of the bitmap.
 * @param h        Height of the bitmap.
 */
void Display_PutPixels(uint8_t *framebuf, int x, int y, const uint8_t *pixels, int w, int h);

#endif