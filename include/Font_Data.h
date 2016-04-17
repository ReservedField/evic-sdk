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
 */

#ifndef EVICSDK_FONT_DATA_H
#define EVICSDK_FONT_DATA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This structure describes a single character's display information.
 */
typedef struct {
	/**
	 * Width (in pixels).
	 */
	const uint8_t width;
	/**
	 * Offset in the bitmap.
	 */
	const uint16_t offset;
} Font_CharInfo_t;	

/**
 * This structure describes a single font.
 */
typedef struct {
	/**
	 * Character height (in pixels).
	 */
	const uint8_t height;
	/**
	 * First character in the font.
	 */
	const uint8_t startChar;
	/**
	 * Last character in the font.
	 */
	const uint8_t endChar;
	/**
	 * Size of a space (in pixels).
	 */
	const uint8_t spacePixels;
	/**
	 * Array of character information.
	 */
	const Font_CharInfo_t *charInfo;
	/**
	 * Font bitmap.
	 */
	const uint8_t *data;
	/**
	 * Font char kerning
	 */
	const int8_t kerning;
} Font_Info_t;

#ifdef __cplusplus
}
#endif

#endif
