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
 * Copyright (C) 2016 Jussi Timperi
 */

#include <stdio.h>
#include <M451Series.h>
#include <Display.h>
#include <Font.h>
#include <Dataflash.h>

int main() {
	int hwVerMajor, hwVerMinor;
	char buffer[100];

	// Separate major/minor version
	hwVerMajor = Dataflash_info.hwVersion / 100;
	hwVerMinor = Dataflash_info.hwVersion % 100;

	// Build and blit text
	sprintf(buffer, "Hw: %d.%02d\n%s\nFlip: %d\nBoot: %s",
		hwVerMajor, hwVerMinor,
		Display_GetType() == DISPLAY_SSD1327 ? "SSD1327" : "SSD1306",
		Display_IsFlipped(),
		Dataflash_info.bootFlag == DATAFLASH_BOOTFLAG_LDROM ? "LD" : "AP");
	Display_PutText(0, 0, buffer, FONT_DEJAVU_8PT);

	// Update display
	Display_Update();
}
