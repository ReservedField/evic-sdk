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
 * Atomizer voltage module.
 */
#define ADC_MODULE_VATM 0x01

/**
 * Atomizer current shunt module.
 */
#define ADC_MODULE_CURS  0x02

/**
 * Battery voltage module.
 * The battery voltage is read through a 1/2 voltage divider.
 */
#define ADC_MODULE_VBAT 0x12

/**
 * Function pointer type for ADC callbacks.
 * The first argument is the ADC result.
 * The second argument is user-defined. If more than 4 bytes
 * are needed, a pointer can be stored and then casted.
 * Callbacks will be invoked from an interrupt handler,
 * so they should be as fast as possible. You'll typically
 * want to just set a flag and return, and then act on that
 * flag from you main application loop.
 */
typedef void (*ADC_Callback_t)(uint16_t, uint32_t);

/**
 * Initializes the ADC.
 * System control registers must be unlocked.
 */
void ADC_Init();

/**
 * Reads a value from the ADC.
 *
 * @param moduleNum One of ADC_MODULE_*.
 *
 * @return ADC conversion result.
 */
uint16_t ADC_Read(uint32_t moduleNum);

/**
 * Performs an asynchronous read on the ADC, calling the callback on
 * completion.
 * There are three conversion slots available to users.
 *
 * @param moduleNum One of ADC_MODULE_*.
 * @param callback  ADC callback function.
 * @param callbackData Optional argument to pass to the callback function.
 *
 * @return True if the conversion was successfully scheduled, false if no
 *         conversion slots were available.
 */
uint8_t ADC_ReadAsync(uint32_t moduleNum, ADC_Callback_t callback, uint32_t callbackData);

#endif
