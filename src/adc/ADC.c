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
 * 0x01: voltage?
 * 0x02: resistance?
 * 0x0E: ?
 * 0x12: battery voltage (through 1/2 voltage divider).
 */

void ADC_Init() {
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
}

uint16_t ADC_Read(uint32_t moduleNum) {
	// Configure ADC
	EADC_Open(EADC, EADC_CTL_DIFFEN_SINGLE_END);
	EADC_SetInternalSampleTime(EADC, 6);
	EADC_ConfigSampleModule(EADC, moduleNum, EADC_SOFTWARE_TRIGGER, moduleNum);

	// Start conversion and wait for completion
	EADC_START_CONV(EADC, 1 << moduleNum);
	while(EADC_IS_BUSY(EADC));

	return EADC_GET_CONV_DATA(EADC, moduleNum);
}
