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

#include <stdio.h>
#include <M451Series.h>
#include <Display.h>
#include <Font.h>
#include <Button.h>

/**
 * Counters, just to make our callbacks do something.
 */
volatile uint8_t counter[2] = {0};

void fireCallback(uint8_t state) {
	counter[0]++;
}

void rightLeftCallback(uint8_t state) {
	counter[1]++;
}

int main() {
	uint8_t state;
	char buf[100];

	// Create some button callbacks
	// Both with a single button and with multiple buttons
	Button_CreateCallback(fireCallback, BUTTON_MASK_FIRE);
	Button_CreateCallback(rightLeftCallback, BUTTON_MASK_RIGHT | BUTTON_MASK_LEFT);

	while(1) {
		// Build state report
		state = Button_GetState();
		siprintf(buf, "Mask: %02X\n\nFire: %d\nRight: %d\nLeft: %d\n\nC0: %d\nC1: %d",
			state,
			(state & BUTTON_MASK_FIRE) ? 1 : 0,
			(state & BUTTON_MASK_RIGHT) ? 1 : 0,
			(state & BUTTON_MASK_LEFT) ? 1 : 0,
			counter[0], counter[1]);

		// Clear and blit text
		Display_Clear();
		Display_PutText(0, 0, buf, FONT_DEJAVU_8PT);
		Display_Update();
	}
}
