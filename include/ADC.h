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

#ifndef EVICSDK_ADC_H
#define EVICSDK_ADC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * ADC reference voltage, in millivolts.
 */
#define ADC_VREF 2560L

/**
 * Denominator for scale calculations.
 * The ADC is 12 bits, range is 0 - 4095.
 */
#define ADC_DENOMINATOR 4096L

/**
 * Atomizer voltage module.
 * This is 3/13 of actual voltage.
 */
#define ADC_MODULE_VATM 0x01

/**
 * Atomizer current shunt module.
 * This is read from the shunt through a INA199A2 current
 * shunt monitor (100V/V gain).
 */
#define ADC_MODULE_CURS 0x02

/**
 * Board temperature sensor.
 * This is probably the DC/DC converter temperature.
 * The sensor is a 10Kohm (@ 25°C) NTC thermistor, beta = 4066.
 * Input circuit as follows:
 * 3.3V---|20Kohm|---o---|Thermistor|---GND
 *                   |---|ADC input|
 */
#define ADC_MODULE_TEMP 0x0E

/**
 * Battery voltage module.
 * The battery voltage is read through a 1/2 voltage divider.
 */
#define ADC_MODULE_VBAT 0x12

/**
 * Function pointer type for ADC filters.
 * Invoked from an interrupt handler, keep it as fast as possible.
 *
 * @param value      Read ADC value.
 * @param filterData Optional filter data passed to ADC_SetFilter().
 *
 * @return Filtered ADC value.
 */
typedef uint16_t (*ADC_Filter_t)(uint16_t value, uint32_t filterData);

/**
 * Initializes the ADC.
 * System control registers must be unlocked.
 */
void ADC_Init();

/**
 * Updates the ADC cache for the specified modules.
 *
 * @param moduleNum  Array of module numbers (ADC_MODULE_*) to be updated.
 * @param len        Length of the module numbers array.
 * @param isBlocking True to wait until all conversions finish, false otherwise.
 */
void ADC_UpdateCache(const uint8_t moduleNum[], uint8_t len, uint8_t isBlocking);

/**
 * Gets a result from ADC cache.
 *
 * @param moduleNum One of ADC_MODULE_*.
 *
 * @return Cached ADC result.
 */
uint16_t ADC_GetCachedResult(uint8_t moduleNum);

/**
 * Reads a value from the ADC (blocking).
 *
 * @param moduleNum One of ADC_MODULE_*.
 *
 * @return ADC conversion result.
 */
uint16_t ADC_Read(uint8_t moduleNum);

/**
 * Sets a filter for an ADC module.
 *
 * @param moduleNum  One of ADC_MODULE_*.
 * @param filter     ADC filter function, or NULL to disable.
 * @param filterData Optional data to pass to the filter function.
 *                   Ignored when disabling.
 */
void ADC_SetFilter(uint8_t moduleNum, ADC_Filter_t filter, uint32_t filterData);

#ifdef __cplusplus
}
#endif

#endif
