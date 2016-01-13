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

#include <M451Series.h>
#include <Battery.h>
#include <ADC.h>

/**
 * \file
 * Battery library.
 * Voltage reading is done through the ADC.
 * PD.7 is low when the battery is present.
 */

void Battery_Init() {
	// Setup presence pin
	GPIO_SetMode(PD, BIT7, GPIO_MODE_INPUT);
}

uint8_t Battery_IsPresent() {
	return !PD7;
}

uint16_t Battery_GetVoltage() {
	// Double the voltage to compensate for the divider
	uint16_t adcValue = ADC_Read(ADC_MODULE_VBAT);
	return adcValue * 2 * ADC_VREF / ADC_DENOMINATOR;
}
