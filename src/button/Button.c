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
 * Button library.
 * The GPIOs are pulled low when the
 * button is pressed.
 * The following pins are used:
 * - PE.0 (Fire button)
 * - PD.2 (Right button)
 * - PD.3 (Left button)
 */

#include <M451Series.h>
#include <Button.h>

void Button_Init() {
	// Setup GPIOs
	GPIO_SetMode(PE, BIT0, GPIO_MODE_INPUT);
	GPIO_SetMode(PD, BIT2, GPIO_MODE_INPUT);
	GPIO_SetMode(PD, BIT3, GPIO_MODE_INPUT);
}

uint8_t Button_GetState() {
	return (PE0 ? 0 : BUTTON_MASK_FIRE) |
		(PD2 ? 0 : BUTTON_MASK_RIGHT) |
		(PD3 ? 0 : BUTTON_MASK_LEFT);
}