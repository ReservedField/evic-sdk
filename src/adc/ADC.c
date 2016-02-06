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
#include <ADC.h>

/**
 * \file
 * ADC library.
 * There are 4 modules used in the eVic:
 * 0x01: atomizer voltage
 * 0x02: atomizer current
 * 0x0E: temperature
 * 0x12: battery voltage (through 1/2 voltage divider).
 * Interrupt 0 is reserved for SDK use.
 */

/**
 * ADC callback pointers.
 * NULL when the interrupt is unused.
 */
static volatile ADC_Callback_t ADC_callbackPtr[4] = {NULL};

/**
 * ADC callback user-defined data.
 */
static volatile uint32_t ADC_callbackData[4];

/**
 * ADC sample module numbers.
 * In Nuvoton SDK those are 32 bit ints, but 8 bits
 * is enough for us and saves memory.
 */
static volatile uint8_t ADC_moduleNum[4];

/**
 * Convenience macro to define ADC IRQ handlers.
 * We disable interrupts and clear flag before callback,
 * in case the user schedules another conversion.
 */
#define ADC_DEFINE_IRQ_HANDLER(n) void ADC0 ## n ## _IRQHandler() { \
	EADC_DISABLE_SAMPLE_MODULE_INT(EADC, n, 1 << ADC_moduleNum[n]); \
	EADC_CLR_INT_FLAG(EADC, 1 << n); \
	ADC_callbackPtr[n](EADC_GET_CONV_DATA(EADC, ADC_moduleNum[n]), ADC_callbackData[n]); \
	ADC_callbackPtr[n] = NULL; \
}

ADC_DEFINE_IRQ_HANDLER(0);
ADC_DEFINE_IRQ_HANDLER(1);
ADC_DEFINE_IRQ_HANDLER(2);
ADC_DEFINE_IRQ_HANDLER(3);

/**
 * Default callback for synchronous ADC reads.
 * This is an internal function.
 *
 * @param result    Coversion result.
 * @param resultPtr Pointer to a int32_t variable where the result
 *                  will be stored.
 */
static void ADC_SyncCallback(uint16_t result, uint32_t resultPtr) {
	*((volatile int32_t *) resultPtr) = result;
}

/**
 * Extended version of ADC_ReadAsync.
 * This function is exported but reserved for SDK use, so
 * its prototype is not declared in the header file.
 *
 * @param moduleNum    ADC module number.
 * @param callback     ADC callback function.
 * @param callbackData Optional argument to pass to the callback function.
 * @param useReserved  True to use the reserved interrupt if no interrupts are available.
 *
 * @return True on success, false if no interrupts were available.
 */
uint8_t ADC_ReadAsyncEx(uint32_t moduleNum, ADC_Callback_t callback, uint32_t callbackData, uint8_t useReserved) {
	uint8_t i;

	// Find an unused interrupt
	// Exclude interrupt 0, as it is reserved
	for(i = 1; i < 4 && ADC_callbackPtr[i] != NULL; i++);

	if(i == 4) {
		// All interrupts are in use
		if(useReserved && ADC_callbackPtr[0] == NULL) {
			// Use reserved interrupt
			i = 0;
		}
		else {
			return 0;
		}
	}

	ADC_callbackPtr[i] = callback;
	ADC_callbackData[i] = callbackData;
	ADC_moduleNum[i] = moduleNum;

	// Configure module
	EADC_ConfigSampleModule(EADC, moduleNum, EADC_SOFTWARE_TRIGGER, moduleNum);

	// Enable interrupt
	EADC_CLR_INT_FLAG(EADC, 1 << i);
	EADC_ENABLE_SAMPLE_MODULE_INT(EADC, i, 1 << moduleNum);

	// Start conversion
	EADC_START_CONV(EADC, 1 << moduleNum);

	return 1;
}

void ADC_Init() {
	uint8_t i;
	IRQn_Type irqNum[4] = {ADC00_IRQn, ADC01_IRQn, ADC02_IRQn, ADC03_IRQn};

	// Enable VBAT unity gain buffer
	SYS->IVSCTL |= SYS_IVSCTL_VBATUGEN_Msk;

	// Set internal 2.56V voltage reference
	SYS->VREFCTL = SYS_VREFCTL_VREF_2_56V;

	// Configure PB.0 - PB.6 analog input pins
	SYS->GPB_MFPL &= ~(SYS_GPB_MFPL_PB0MFP_Msk | SYS_GPB_MFPL_PB1MFP_Msk |
		SYS_GPB_MFPL_PB2MFP_Msk | SYS_GPB_MFPL_PB3MFP_Msk |
		SYS_GPB_MFPL_PB4MFP_Msk | SYS_GPB_MFPL_PB5MFP_Msk |
		SYS_GPB_MFPL_PB6MFP_Msk);
	SYS->GPB_MFPL |= (SYS_GPB_MFPL_PB0MFP_EADC_CH0 | SYS_GPB_MFPL_PB1MFP_EADC_CH1 |
		SYS_GPB_MFPL_PB2MFP_EADC_CH2 | SYS_GPB_MFPL_PB3MFP_EADC_CH3 |
		SYS_GPB_MFPL_PB4MFP_EADC_CH4 | SYS_GPB_MFPL_PB5MFP_EADC_CH13 |
		SYS_GPB_MFPL_PB6MFP_EADC_CH14);

	// Disable PB.0 - PB.6 digital input paths to avoid leakage currents
	GPIO_DISABLE_DIGITAL_PATH(PB, 0x7F);

	// Configure ADC (single-ended)
	EADC_Open(EADC, EADC_CTL_DIFFEN_SINGLE_END);
	EADC_SetInternalSampleTime(EADC, 6);

	// Enable interrupts
	for(i = 0; i < 4; i++) {
		EADC_ENABLE_INT(EADC, 1 << i);
		NVIC_EnableIRQ(irqNum[i]);
	}
}

uint16_t ADC_Read(uint32_t moduleNum) {
	volatile int32_t result = -1;

	// Keep trying until we get a slot
	while(!ADC_ReadAsyncEx(moduleNum, ADC_SyncCallback, (uint32_t) &result, 0));

	// Wait until conversion is complete
	while(result == -1);

	return result;
}

uint8_t ADC_ReadAsync(uint32_t moduleNum, ADC_Callback_t callback, uint32_t callbackData) {
	return ADC_ReadAsyncEx(moduleNum, callback, callbackData, 0);
}