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

#include <stdio.h>
#include <M451Series.h>
#include <Display.h>
#include <Font.h>
#include <Dataflash.h>

int main() {
	char hwVerMajor, hwVerMinor1, hwVerMinor2, buffer[100];

	// Build version chars
	hwVerMajor = '0' + (Dataflash_info.hwVersion / 100);
	hwVerMinor1 = '0' + ((Dataflash_info.hwVersion % 100) / 10);
	hwVerMinor2 = '0' + (Dataflash_info.hwVersion % 10);

	// Build and blit text
	sprintf(buffer, "Hw: %c.%c%c\n%s\nFlip: %c",
		hwVerMajor, hwVerMinor1, hwVerMinor2,
		Dataflash_info.displayType == DISPLAY_SSD1327 ? "SSD1327" : "SSD1306",
		Dataflash_info.flipDisplay ? '1' : '0');
	Display_PutText(0, 0, buffer, FONT_DEJAVU_8PT);

	// Update display
	Display_Update();
}
