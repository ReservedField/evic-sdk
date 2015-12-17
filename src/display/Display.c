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

#include <M451Series.h>
#include <Display.h>
#include <Display_SSD1306.h>
#include <string.h>

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
	Display_SSD1306_Init();
}

void Display_Update(const uint8_t *framebuf) {
	Display_SSD1306_Update(framebuf);
}

void Display_PutPixels(uint8_t *framebuf, int x, int y, const uint8_t *pixels, int w, int h) {
	int startPage, endPage, firstPassEndPage, pixelX, page;
	uint8_t pixelMask;

	// Sanity check
	if(x < 0 || y < 0 ||
		w <= 0 || h <= 0 ||
		x + w > DISPLAY_WIDTH ||
		y + h > DISPLAY_HEIGHT) {
		return;
	}
	
	// Calculate start & end pages
	startPage = (y + 7) / 8;
	endPage = startPage + (h + 7) / 8;

	// If the last page is a full column, simply copy it
	// If it isn't, bitmasking is needed
	firstPassEndPage = (h % 8) == 0 ? endPage : endPage - 1;

	// Copy the full pages
	for(page = startPage; page < firstPassEndPage; page++) {
		memcpy(&framebuf[page * DISPLAY_FRAMEBUFFER_PAGE_SIZE + x], &pixels[(page - startPage) * w], w);
	}

	if(h % 8 != 0) {
		// Last page is not a full column
		// Lowest (h % 8) bits set
		pixelMask = (1 << (h % 8)) - 1;

		for(pixelX = 0; pixelX < w; pixelX++) {
			// Copy column by masking bits
			framebuf[(endPage - 1) * DISPLAY_FRAMEBUFFER_PAGE_SIZE + x + pixelX] &= ~pixelMask;
			framebuf[(endPage - 1) * DISPLAY_FRAMEBUFFER_PAGE_SIZE + x + pixelX] |= pixels[(endPage - 1 - startPage) * w + pixelX] & pixelMask;
		}
	}
}