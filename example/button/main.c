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
#include <Button.h>

int main() {
	uint8_t state;
	char buf[100];

	while(1) {
		// Build state report
		state = Button_GetState();
		sprintf(buf, "Fire: %c\nRight: %c\nLeft: %c",
			(state & BUTTON_MASK_FIRE) ? '1' : '0',
			(state & BUTTON_MASK_RIGHT) ? '1' : '0',
			(state & BUTTON_MASK_LEFT) ? '1' : '0');

		// Clear and blit text
		Display_Clear();
		Display_PutText(0, 0, buf, FONT_DEJAVU_8PT);
		Display_Update();
	}
}
