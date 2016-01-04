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
#include <M451Series.h>
#include <Display.h>
#include <Display_SSD.h>
#include <Display_SSD1306.h>
#include <Display_SSD1327.h>
#include <Timer.h>
#include <Font.h>
#include <Dataflash.h>

/**
 * Global framebuffer.
 */
static uint8_t Display_framebuf[DISPLAY_FRAMEBUFFER_SIZE];

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
	SYS->GPB_MFPL &= ~(SYS_GPE_MFPH_PE11MFP_Msk | SYS_GPE_MFPH_PE12MFP_Msk | SYS_GPE_MFPH_PE13MFP_Msk);
	SYS->GPE_MFPH |= SYS_GPE_MFPH_PE11MFP_SPI0_MOSI0 | SYS_GPE_MFPH_PE12MFP_SPI0_SS | SYS_GPE_MFPH_PE13MFP_SPI0_CLK;

	// SPI0 master, MSB first, 8bit transaction, SPI Mode-0 timing, 4MHz clock
	SPI_Open(SPI0, SPI_MASTER, SPI_MODE_0, 8, 4000000);

	// Low level active
	SPI_EnableAutoSS(SPI0, SPI_SS, SPI_SS_ACTIVE_LOW);

	// Start SPI
	SPI_ENABLE(SPI0);
}

void Display_Init() {
	// Clear framebuffer
	memset(Display_framebuf, 0x00, DISPLAY_FRAMEBUFFER_SIZE);

	// Initialize display controller
	if(Dataflash_info.displayType == DISPLAY_SSD1327) {
		Display_SSD1327_Init();
	}
	else {
		Display_SSD1306_Init();
	}
}

void Display_Update() {
	if(Dataflash_info.displayType == DISPLAY_SSD1327) {
		Display_SSD1327_Update(Display_framebuf);
	}
	else {
		Display_SSD1306_Update(Display_framebuf);
	}
}

void Display_PutPixels(int x, int y, const uint8_t *pixels, int w, int h) {
	int startPage, endPage, pixelX, page;
	uint8_t pixelMask;

	// Sanity check
	if(x < 0 || y < 0 ||
		w <= 0 || h <= 0 ||
		x + w > DISPLAY_WIDTH ||
		y + h > DISPLAY_HEIGHT) {
		return;
	}

	// Calculate start & end for full pages
	startPage = y / 8;
	endPage = startPage + (h / 8);

	// Copy the full pages
	for(page = startPage; page < endPage; page++) {
		memcpy(&Display_framebuf[page * DISPLAY_FRAMEBUFFER_PAGE_SIZE + x], &pixels[(page - startPage) * w], w);
	}

	if(h % 8 != 0) {
		// Last page is not a full column
		// Lowest (h % 8) bits set
		pixelMask = (1 << (h % 8)) - 1;

		for(pixelX = 0; pixelX < w; pixelX++) {
			// Copy column by masking bits
			Display_framebuf[endPage * DISPLAY_FRAMEBUFFER_PAGE_SIZE + x + pixelX] &= ~pixelMask;
			Display_framebuf[endPage * DISPLAY_FRAMEBUFFER_PAGE_SIZE + x + pixelX] |= pixels[(endPage - startPage) * w + pixelX] & pixelMask;
		}
	}
}

void Display_PutText(int x, int y, const char *txt, const Font_Info_t *font) {
	int i, curX, charIdx;
 	const uint8_t *charPtr;

 	curX = x;

 	for(i = 0; i < strlen(txt); i++) {
 		// Handle newlines
 		if(txt[i] == '\n') {
 			// TODO: support non-8-aligned Y
 			Display_PutText(x, (y + font->height + 7) & ~7, &txt[i + 1], font);
 			break;
 		}

 		// Handle spaces
 		if(txt[i] == ' ') {
 			curX += font->spacePixels;
 			continue;
 		}

 		// Sanity check
 		if(txt[i] < font->startChar || txt[i] > font->endChar) {
 			continue;
 		}

 		// Calculate character index and pointer to bitmap
 		charIdx = txt[i] - font->startChar;
 		charPtr = font->data + font->charInfo[charIdx].offset;

 		// Blit character
		Display_PutPixels(curX, y, charPtr, font->charInfo[charIdx].width, font->height);
		curX += font->charInfo[charIdx].width;
 	}
}