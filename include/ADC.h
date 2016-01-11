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

#ifndef EVICSDK_ADC_H
#define EVICSDK_ADC_H

/**
 * ADC reference voltage, in millivolts.
 */
#define ADC_VREF 2560

/**
 * Denominator for scale calculations.
 * The ADC is 12 bits, range is 0 - 4095.
 */
#define ADC_DENOMINATOR 4096

/**
 * Battery voltage module.
 * The battery voltage is read through a 1/2 voltage divider.
 */
#define ADC_MODULE_VBAT 0x12

/**
 * Initializes the ADC.
 * System control registers must be unlocked.
 */
void ADC_Init();

/**
 * Reads a value from the ADC.
 *
 * @param moduleNum One of AVC_MODULE_*.
 *
 * @return ADC conversion result.
 */
uint16_t ADC_Read(uint32_t moduleNum);

#endif
