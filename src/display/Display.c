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
 * Display controller library.
 * The controller communicates by SPI on SPI0.
 * The following pins are used:
 * - PA.0 (RESET)
 * - PA.1 (?)
 * - PC.4 (?)
 * - PE.10 (D/C#)
 * - PE.11 (MOSI)
 * - PE.12 (SS)
 * - PE.13 (CLK)
 */

#include <string.h>
#include <stdlib.h>
#include <M451Series.h>
#include <Display.h>
#include <Display_SSD.h>
#include <Font.h>
#include <SysInfo.h>

/**
 * Global framebuffer.
 */
static uint8_t Display_framebuf[DISPLAY_FRAMEBUFFER_SIZE];

/**
 * Display type. Depends on hardware version.
 */
static Display_Type_t Display_type;

void Display_SetupSPI() {
	// Setup output pins
	PA0 = 0;
	GPIO_SetMode(PA, BIT0, GPIO_MODE_OUTPUT);
	PA1 = 0;
	GPIO_SetMode(PA, BIT1, GPIO_MODE_OUTPUT);
	PC4 = 0;
	GPIO_SetMode(PC, BIT4, GPIO_MODE_OUTPUT);
	PE10 = 0;
	GPIO_SetMode(PE, BIT10, GPIO_MODE_OUTPUT);
	PE12 = 0;
	GPIO_SetMode(PE, BIT12, GPIO_MODE_OUTPUT);

	// Setup MFP
	SYS->GPE_MFPL &= ~(SYS_GPE_MFPH_PE11MFP_Msk | SYS_GPE_MFPH_PE12MFP_Msk | SYS_GPE_MFPH_PE13MFP_Msk);
	SYS->GPE_MFPH |= SYS_GPE_MFPH_PE11MFP_SPI0_MOSI0 | SYS_GPE_MFPH_PE12MFP_SPI0_SS | SYS_GPE_MFPH_PE13MFP_SPI0_CLK;

	// SPI0 master, MSB first, 8bit transaction, SPI Mode-0 timing, 4MHz clock
	SPI_Open(SPI0, SPI_MASTER, SPI_MODE_0, 8, 4000000);

	// Low level active
	SPI_EnableAutoSS(SPI0, SPI_SS, SPI_SS_ACTIVE_LOW);

	// Start SPI
	SPI_ENABLE(SPI0);
}

void Display_Init() {
	switch(gSysInfo.hwVersion) {
		case 102:
		case 103:
		case 106:
		case 108:
		case 109:
		case 111:
			Display_type = DISPLAY_SSD1327;
			break;
		default:
			Display_type = DISPLAY_SSD1306;
			break;
	}
	Display_Clear();
	Display_SSD_Init();
}

void Display_SetOn(uint8_t isOn) {
	Display_SSD_SetOn(isOn);
}

void Display_SetPowerOn(uint8_t isPowerOn) {
	Display_SSD_SetPowerOn(isPowerOn);
}

Display_Type_t Display_GetType() {
	return Display_type;
}

bool Display_IsFlipped() {
	return gSysInfo.displayFlip;
}

void Display_Flip() {
	gSysInfo.displayFlip ^= 1;
	Display_SSD_SetOn(0);
	Display_SSD_Flip();
	Display_SSD_Update(Display_framebuf);
	Display_SSD_SetOn(1);
}

void Display_SetInverted(bool invert) {
	Display_SSD_SetInverted(invert);
}

void Display_Update() {
	Display_SSD_Update(Display_framebuf);
}

void Display_Clear() {
	memset(Display_framebuf, 0x00, DISPLAY_FRAMEBUFFER_SIZE);
}

/**
 * Copies bits across byte boundaries.
 * Copy is done LSB to MSB.
 * This is an internal function.
 *
 * @param dst       Destination buffer.
 * @param src       Source buffer.
 * @param dstOffset Offset from destination, in bits.
 * @param size      Size to copy, in bits.
 */
static void Display_BitCopy(uint8_t *dst, const uint8_t *src, uint32_t dstOffset, uint32_t size) {
	uint32_t numFullBytes;
	uint8_t bitMask1, bitMask2, remSize, dstOffsetRem;

	// Handle bigger-than-7 dstOffset
	dst += dstOffset / 8;
	dstOffset = dstOffset % 8;

	// If copying whole bytes, fall back to faster memcpy
	if(dstOffset == 0 && size % 8 == 0) {
		memcpy(dst, src, size / 8);
		return;
	}

	numFullBytes = size / 8;
	dstOffsetRem = 8 - dstOffset;

	if(dstOffset == 0) {
		// Destination is byte-aligned
		// Copy full bytes with faster memcpy
		memcpy(dst, src, numFullBytes);
		src += numFullBytes;
		dst += numFullBytes;
	}
	else {
		// Lowest dstOffsetRem bits set
		bitMask1 = (1 << dstOffsetRem) - 1;
		// Highest dstOffset bits set
		bitMask2 = ~bitMask1;

		while(numFullBytes--) {
			// dstOffsetRem bits, from src[0] low to dst[0] high
			dst[0] &= ~(bitMask1 << dstOffset);
			dst[0] |= (src[0] & bitMask1) << dstOffset;

			// dstOffset bits, from src[0] high to dst[1] low
			dst[1] &= ~(bitMask2 >> dstOffsetRem);
			dst[1] |= (src[0] & bitMask2) >> dstOffsetRem;

			src++;
			dst++;
		}
	}

	remSize = size % 8;
	if(remSize == 0) {
		// No trailing bits to copy
		return;
	}

	if(remSize > dstOffsetRem) {
		// The last source byte will span across two destination bytes
		remSize -= dstOffsetRem;

		// remSize bits, from src[0] (trucated at size) high to dst[1] low
		bitMask2 = ((1 << remSize) - 1) << dstOffsetRem;
		dst[1] &= ~(bitMask2 >> dstOffsetRem);
		dst[1] |= (src[0] & bitMask2) >> dstOffsetRem;

		// Setup final dst[0] copy
		remSize = dstOffsetRem;
	}

	// remSize bits, from src[0] low to dst[0] high
	bitMask1 = (1 << remSize) - 1;
	dst[0] &= ~(bitMask1 << dstOffset);
	dst[0] |= (src[0] & bitMask1) << dstOffset;
}

void Display_PutPixels(int x, int y, const uint8_t *bitmap, int w, int h) {
	int colSize, startRow, curX;

	// Sanity check
	if(x < 0 || y < 0 ||
		w <= 0 || h <= 0 ||
		x + w > DISPLAY_WIDTH ||
		y + h > DISPLAY_HEIGHT) {
		return;
	}

	// Size (in bytes) of a column in the bitmap
	colSize = (h + 7) / 8;
	// Row containing the first point of the bitmap
	startRow = y / 8;

	for(curX = 0; curX < w; curX++) {
		// Copy column
		Display_BitCopy(&Display_framebuf[(x + curX) * (DISPLAY_HEIGHT / 8) + startRow],
			&bitmap[curX * colSize], y % 8, h);
	}
}

void Display_PutLine(int x0, int y0, int x1, int y1) {
	int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
	int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
	int err = (dx>dy ? dx : -dy)/2, e2;

	uint8_t buff[] = { 0xFF };

	for(;;){
		Display_PutPixels(x0, y0, buff, 1, 1);
		if (x0==x1 && y0==y1) break;
		e2 = err;
		if (e2 >-dx) { err -= dy; x0 += sx; }
		if (e2 < dy) { err += dx; y0 += sy; }
	}
}

void Display_PutText(int x, int y, const char *txt, const Font_Info_t *font) {
	int i, curX, charIdx;
	const uint8_t *charPtr;

	curX = x;

	for(i = 0; i < strlen(txt); i++) {
		// Handle newlines
		if(txt[i] == '\n') {
			curX = x;
			y += font->height;
			continue;
		}

		// Handle spaces
		if(txt[i] == ' ') {
			curX += font->spacePixels;
			continue;
		}

		// Skip unknown characters
		if(txt[i] < font->startChar || txt[i] > font->endChar) {
			continue;
		}

		// Handle "kerning" (skip on first character in line)
		if (x != curX && font->kerning != 0) {
			curX += font->kerning;
		}

		// Calculate character index and pointer to bitmap
		charIdx = txt[i] - font->startChar;
		charPtr = font->data + font->charInfo[charIdx].offset;

		// Blit character
		Display_PutPixels(curX, y, charPtr, font->charInfo[charIdx].width, font->height);
		curX += font->charInfo[charIdx].width;
	}
}

uint8_t *Display_GetFramebuffer() {
	return Display_framebuf;
}

void Display_SetContrast(uint8_t contrast) {
	Display_SSD_SetContrast(contrast);
}
