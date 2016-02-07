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

/**
 * Battery mV voltage to percent lookup table.
 * percentTable[i] maps the 10% range starting at 10*i%.
 * Linear interpolation is done inside the range.
 */
static const uint16_t Battery_percentTable[11] = {
	3100, 3300, 3420, 3500, 3580, 3630, 3680, 3790, 3890, 4000, 4100
};

void Battery_Init() {
	// Setup presence pin
	// Debounce: 1024 LIRC clocks
	GPIO_SetMode(PD, BIT7, GPIO_MODE_INPUT);
	GPIO_SET_DEBOUNCE_TIME(GPIO_DBCTL_DBCLKSRC_LIRC, GPIO_DBCTL_DBCLKSEL_1024);
	GPIO_ENABLE_DEBOUNCE(PD, BIT7);
}

uint8_t Battery_IsPresent() {
	return !PD7;
}

uint8_t Battery_IsCharging() {
	return Battery_IsPresent() && USBD_IS_ATTACHED();
}

uint16_t Battery_GetVoltage() {
	// Double the voltage to compensate for the divider
	uint16_t adcValue = ADC_Read(ADC_MODULE_VBAT);
	return adcValue * 2L * ADC_VREF / ADC_DENOMINATOR;
}

uint8_t Battery_VoltageToPercent(uint16_t volts) {
	uint8_t i;
	uint16_t lowerBound, higherBound;

	// Handle corner cases
	if(volts <= Battery_percentTable[0]) {
		return 0;
	}
	else if(volts >= Battery_percentTable[10]) {
		return 100;
	}

	// Look up higher bound
	for(i = 1; i < 10 && volts > Battery_percentTable[i]; i++);

	// Interpolate
	lowerBound = Battery_percentTable[i - 1];
	higherBound = Battery_percentTable[i];
	return 10 * (i - 1) + (volts - lowerBound) * 10 / (higherBound - lowerBound);
}
