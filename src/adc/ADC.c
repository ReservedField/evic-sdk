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
#include <Thread.h>

/**
 * \file
 * ADC library.
 * There are 4 modules used in the eVic:
 * 0x01: atomizer voltage
 * 0x02: atomizer current
 * 0x0E: temperature
 * 0x12: battery voltage
 * Interrupts 0-3 are assigned in that order.
 */

/**
 * ADC sample module numbers for interrupts 0-3.
 * In Nuvoton SDK those are 32 bit ints, but 8 bits
 * is enough for us and saves memory.
 */
static const uint8_t ADC_moduleNum[4] = {
	ADC_MODULE_VATM, ADC_MODULE_CURS,
	ADC_MODULE_TEMP, ADC_MODULE_VBAT
};

/**
 * Cached ADC conversion results for interrupts 0-3.
 */
static volatile uint16_t ADC_convResult[4] = {0};

/**
 * Convenience macro to define ADC IRQ handlers.
 */
#define ADC_DEFINE_IRQ_HANDLER(n) void ADC0 ## n ## _IRQHandler() { \
	ADC_convResult[n] = EADC_GET_CONV_DATA(EADC, ADC_moduleNum[n]); \
	EADC_DISABLE_SAMPLE_MODULE_INT(EADC, n, 1 << ADC_moduleNum[n]); \
	EADC_CLR_INT_FLAG(EADC, 1 << n); \
}

ADC_DEFINE_IRQ_HANDLER(0);
ADC_DEFINE_IRQ_HANDLER(1);
ADC_DEFINE_IRQ_HANDLER(2);
ADC_DEFINE_IRQ_HANDLER(3);

void ADC_UpdateCache(const uint8_t moduleNum[], uint8_t len, uint8_t isBlocking) {
	uint8_t i, j, finishFlag;
	uint32_t primask;

	for(i = 0; i < len; i++) {
		// Find interrupt number for module number
		for(j = 0; j < 4 && moduleNum[i] != ADC_moduleNum[j]; j++);

		primask = Thread_IrqDisable();
		if(!(EADC_GET_PENDING_CONV(EADC) & (1 << moduleNum[i]))) {
			// Configure module
			EADC_ConfigSampleModule(EADC, moduleNum[i], EADC_SOFTWARE_TRIGGER, moduleNum[i]);

			// Enable interrupt
			EADC_CLR_INT_FLAG(EADC, 1 << j);
			EADC_ENABLE_SAMPLE_MODULE_INT(EADC, j, 1 << moduleNum[i]);

			// Start conversion
			EADC_START_CONV(EADC, 1 << moduleNum[i]);
		}
		Thread_IrqRestore(primask);
	}

	if(isBlocking) {
		// Wait for modules to finish
		// Keep in mind they could be restarted by another concurrent
		// call to ADC_UpdateCache while we're busy waiting.
		finishFlag = 0;
		while(finishFlag != (1 << len) - 1) {
			for(i = 0; i < len; i++) {
				if(!(EADC_GET_PENDING_CONV(EADC) & (1 << moduleNum[i]))) {
					finishFlag |= 1 << i;
				}
			}
		}
	}
}

uint16_t ADC_GetCachedResult(uint8_t moduleNum) {
	uint8_t i;

	// Find interrupt number for module number
	for(i = 0; i < 4 && moduleNum != ADC_moduleNum[i]; i++);

	// Atomic
	return ADC_convResult[i];
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

uint16_t ADC_Read(uint8_t moduleNum) {
	ADC_UpdateCache((uint8_t []) {moduleNum}, 1, 1);
	return ADC_GetCachedResult(moduleNum);
}