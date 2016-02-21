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
 * Copyright (C) 2016 kfazz
 */

#include <M451Series.h>
#include <Atomizer.h>
#include <ADC.h>
#include <TimerUtils.h>
#include <Dataflash.h>

/**
 * \file
 * Atomizer library.
 * The DC/DC converter uses PWM channels 0 and 2.
 * PWM channel 0 (PC.0) is a buck converter.
 * PWM channel 2 (PC.2) is a boost converter.
 * Increasing duty cycle for the buck converter increases
 * voltage, while it decreases voltage for the boost channel.
 * PC.1 and PC.3 are control outputs. They should be both high
 * when one of the converters is powered, both low otherwise.
 */

/* DC/DC converters PWM channels */
#define ATOMIZER_PWMCH_BUCK  0
#define ATOMIZER_PWMCH_BOOST 2

/* Macros to convert ADC values */
// Read voltage is x * ADC_VREF / ADC_DENOMINATOR.
// This is 3/13 of actual voltage, so we multiply by 13/3.
// Result is in 10mV units.
// Maximum result size: 11 bits.
#define ATOMIZER_ADC_VOLTAGE(x) (13L * (x) * ADC_VREF / 30L / ADC_DENOMINATOR)
// Voltage drop on shunt (100x gain) is x * ADC_VREF / ADC_DENOMINATOR.
// Current is Vdrop / R, units are in mV/mOhm, R has 100x gain too (100ths of mOhm).
// So current will be in A. We multiply by 1000 to get mA.
// Simplified expression: I = 1000 * x * ADC_VREF / Atomizer_shuntRes / ADC_DENOMINATOR
// To avoid overflows, get better precision and save a division, ADC_VREF and ADC_DENOMINATOR are hardcoded.
// Maximum result size: 15 bits.
#define ATOMIZER_ADC_CURRENT(x) (625L * (x) / Atomizer_shuntRes)
// Resistance is V / I. Since it will be in Ohms, we multiply by 1000 to get mOhms.
// This macro is provided to get better precision when taking multiple V and I samples.
// If the number of samples is the same, it will simplify, giving better precision than averaging.
// 1000 * ATOMIZER_ADC_VOLTAGE(voltsX) / ATOMIZER_ADC_CURRENT(currX) simplifies to this expression.
// Current is forced to 1 if zero to avoid division by zero.
// Maximum result size: 22 bits.
#define ATOMIZER_ADC_RESISTANCE(voltsX, currX) (13L * (voltsX) * Atomizer_shuntRes / 3L / ((currX) == 0 ? 1 : (currX)))
// The thermistor is read through a voltage divider supplied by 3.3V.
// The thermistor is on the low side, a 20K resistor is on the high side.
// R = V * 20000 / (3.3 - V)
// V = ADC_VREF * x / ADC_DENOMINATOR
// Simplified expression: R = 20000 * ADC_VREF * x / (3300 * ADC_DENOMINATOR - ADC_VREF * x)
// The nominator overflows, leaving us with at most 100ohms accuracy when breaking into 2000 * 100.
// To get 1ohm accuracy and save a multiplication, ADC_VREF and ADC_DENOMINATOR are hardcoded.
// Maximum result size: 17 bits.
#define ATOMIZER_ADC_THERMRES(x) (20000L * (x) / (5280L - (x)))

/**
 * Type for storing converters state.
 */
typedef enum {
	/**
	 * Converters are powered off.
	 */
	POWEROFF,
	/**
	 * Buck converter is active.
	 */
	POWERON_BUCK,
	/**
	 * Boost converter is active.
	 */
	POWERON_BOOST
} Atomizer_ConverterState_t;

/**
 * Target voltage, in 10mV units.
 */
static volatile uint16_t Atomizer_targetVolts = 0;

/**
 * Current duty cycle.
 */
static volatile uint16_t Atomizer_curCmr = 0;

/**
 * Current converters state.
 */
static volatile Atomizer_ConverterState_t Atomizer_curState = POWEROFF;

/**
 * Shunt resistor value, in 100ths of a mOhm.
 * Depends on hardware version.
 */
static uint8_t Atomizer_shuntRes;

/**
 * Error code.
 */
static volatile Atomizer_Error_t Atomizer_error;

/**
 * Initial atomizer resistance, in mOhm.
 * If an atomizer error occurs, this is set to zero.
 */
static volatile uint16_t Atomizer_baseRes;

/**
 * Counter for timing.
 */
static volatile uint16_t Atomizer_timerCount;

/**
 * Thermistor resistance to board temperature lookup table.
 * boardTempTable[i] maps the 5°C range starting at 5*i °C.
 * Linear interpolation is done inside the range.
 */
static const uint16_t Atomizer_boardTempTable[21] = {
	34800, 26670, 20620, 16070, 12630, 10000, 7976,
	 6407,  5182,  4218,  3455,  2847,  2360, 1967,
	 1648,  1388,  1175,   999,   853,   732,  630
};

/**
 * Configures a PWM channel.
 * This is an internal function.
 *
 * @param channel PWM channel to configure.
 * @param enable  True to enable the channel, false to disable it.
 */
static void Atomizer_ConfigurePWMChannel(uint8_t channel, uint8_t enable) {
	// If the channel is enabled, we set the MFP register to PWM
	// If it is disabled, we set it as a GPIO output
	switch(channel) {
		case ATOMIZER_PWMCH_BUCK:
			SYS->GPC_MFPL &= ~SYS_GPC_MFPL_PC0MFP_Msk;
			if(enable) {
				SYS->GPC_MFPL |= SYS_GPC_MFPL_PC0MFP_PWM0_CH0;
			}
			else {
				PC0 = 1;
				GPIO_SetMode(PC, BIT0, GPIO_MODE_OUTPUT);
			}
			break;
		case ATOMIZER_PWMCH_BOOST:
			SYS->GPC_MFPL &= ~SYS_GPC_MFPL_PC2MFP_Msk;
			if(enable) {
				SYS->GPC_MFPL |= SYS_GPC_MFPL_PC2MFP_PWM0_CH2;
			}
			else {
				PC2 = 1;
				GPIO_SetMode(PC, BIT2, GPIO_MODE_OUTPUT);
			}
			break;
	}
}

/**
 * Configures the DC/DC converters.
 * This is an internal function.
 *
 * @param enableBuck  True to enable the buck converter, false to disable it.
 * @param enableBoost True to enable the boost converter, false to disable it.
 */
static void Atomizer_ConfigureConverters(uint8_t enableBuck, uint8_t enableBoost) {
	// FIRST power off
	if(!enableBuck) {
		Atomizer_ConfigurePWMChannel(ATOMIZER_PWMCH_BUCK, 0);
	}
	if(!enableBoost) {
		Atomizer_ConfigurePWMChannel(ATOMIZER_PWMCH_BOOST, 0);
	}

	// THEN power on
	if(enableBuck) {
		Atomizer_ConfigurePWMChannel(ATOMIZER_PWMCH_BUCK, 1);
	}
	if(enableBoost) {
		Atomizer_ConfigurePWMChannel(ATOMIZER_PWMCH_BOOST, 1);
	}

	// If a converter is enabled, PC.1 and PC.3 are set high
	// Otherwise, they are set low
	PC1 = PC3 = (enableBuck || enableBoost) ? 1 : 0;
}

/**
 * Negative feedback iteration to keep the DC/DC converters stable.
 * Takes parameters as a timer callback.
 * This is an internal function.
 */
static void Atomizer_NegativeFeedback(uint32_t unused) {
	uint16_t adcVoltage, adcCurrent, curVolts;
	uint32_t resistance;

	// Update ADC cache without blocking.
	// This loop always runs (until this point), even when
	// the atomizer is not powered on. Let's exploit it to
	// keep good values in the cache for when we power it up.
	ADC_UpdateCache((uint8_t []) {ADC_MODULE_VATM, ADC_MODULE_CURS}, 2, 0);

	Atomizer_timerCount++;

	if(Atomizer_curState == POWEROFF) {
		// Powered off, nothing to do
		return;
	}

	// Get ADC readings
	adcVoltage = ADC_GetCachedResult(ADC_MODULE_VATM);
	adcCurrent = ADC_GetCachedResult(ADC_MODULE_CURS);

	// Check resistance
	resistance = ATOMIZER_ADC_RESISTANCE(adcVoltage, adcCurrent);
	if(resistance >= 5 && resistance < 50) {
		Atomizer_error = SHORT;
	}
	else if(resistance > 5000) {
		Atomizer_error = OPEN;
	}
	else {
		Atomizer_error = OK;
	}
	if(Atomizer_error != OK) {
		Atomizer_baseRes = 0;
		Atomizer_Control(0);
		return;
	}

	curVolts = ATOMIZER_ADC_VOLTAGE(adcVoltage);
	if(curVolts == Atomizer_targetVolts) {
		// Target reached, nothing to do
		return;
	}

	if(curVolts < Atomizer_targetVolts) {
		if(Atomizer_curState == POWERON_BUCK) {
			if(Atomizer_curCmr == 479) {
				// Reached maximum for buck, switch to boost
				Atomizer_curState = POWERON_BOOST;
				Atomizer_ConfigureConverters(0, 1);
			}
			else {
				// In buck mode, increased duty cycle = increased voltage
				Atomizer_curCmr++;
			}
		}
		else if(Atomizer_curCmr > 80) {
			// Boost duty cycle must be greater than 80
			// In boost mode, decreased duty cycle = increased voltage
			Atomizer_curCmr--;
		}
	}
	else {
		if(Atomizer_curState == POWERON_BOOST) {
			if(Atomizer_curCmr == 479) {
				// Reached minimum for boost, switch to buck
				Atomizer_curState = POWERON_BUCK;
				Atomizer_ConfigureConverters(1, 0);
			}
			else {
				// In boost mode, increased duty cycle = decreased voltage
				Atomizer_curCmr++;
			}
		}
		else {
			if(Atomizer_curCmr <= 10) {
				// Buck duty cycles below 10 are forced to zero
				Atomizer_curCmr = 0;
			}
			else {
				// In buck mode, decreased duty cycle = decreased voltage
				Atomizer_curCmr--;
			}
		}
	}

	// Set new duty cycle
	PWM_SET_CMR(PWM0, Atomizer_curState == POWERON_BUCK ? ATOMIZER_PWMCH_BUCK : ATOMIZER_PWMCH_BOOST, Atomizer_curCmr);
}

void Atomizer_Init() {
	// Select shunt value based on hardware version
	switch(Dataflash_info.hwVersion) {
		case 101:
		case 108:
			Atomizer_shuntRes = 125;
			break;
		case 103:
		case 104:
		case 105:
		case 106:
			Atomizer_shuntRes = 110;
			break;
		case 107:
		case 109:
			Atomizer_shuntRes = 120;
			break;
		case 110:
		case 111:
			Atomizer_shuntRes = 105;
			break;
		case 100:
		case 102:
		default:
			Atomizer_shuntRes = 115;
			break;
	}

	// Setup control pins
	PC1 = 0;
	GPIO_SetMode(PC, BIT1, GPIO_MODE_OUTPUT);
	PC3 = 0;
	GPIO_SetMode(PC, BIT3, GPIO_MODE_OUTPUT);

	// Both channels powered down
	Atomizer_ConfigureConverters(0, 0);

	// Configure 150kHz PWM
	PWM_ConfigOutputChannel(PWM0, ATOMIZER_PWMCH_BUCK, 150000, 0);
	PWM_ConfigOutputChannel(PWM0, ATOMIZER_PWMCH_BOOST, 150000, 0);

	// Start PWM
	PWM_EnableOutput(PWM0, PWM_CH_0_MASK);
	PWM_EnableOutput(PWM0, PWM_CH_2_MASK);
	PWM_Start(PWM0, PWM_CH_0_MASK);
	PWM_Start(PWM0, PWM_CH_2_MASK);

	// Set duty cycle to zero
	PWM_SET_CMR(PWM0, ATOMIZER_PWMCH_BUCK, 0);
	PWM_SET_CMR(PWM0, ATOMIZER_PWMCH_BOOST, 0);

	Atomizer_targetVolts = 0;
	Atomizer_curCmr = 0;
	Atomizer_curState = POWEROFF;
	Atomizer_error = OK;
	Atomizer_baseRes = 0;
	Atomizer_timerCount = 0;

	// Setup 5kHz timer for negative feedback cycle
	// This function should run during system init, so
	// the user hasn't had time to create timers yet.
	Timer_CreateTimer(5000, 1, Atomizer_NegativeFeedback, 0);
}

void Atomizer_SetOutputVoltage(uint16_t volts) {
	if(volts > ATOMIZER_MAX_VOLTS) {
		volts = ATOMIZER_MAX_VOLTS;
	}

	Atomizer_targetVolts = (volts + 5) / 10;
}

void Atomizer_Control(uint8_t powerOn) {
	if((!powerOn && Atomizer_curState == POWEROFF) || (powerOn && Atomizer_curState != POWEROFF)) {
		// Nothing to do
		return;
	}

	if(powerOn) {
		// Start from buck with duty cycle 10
		Atomizer_curCmr = 10;
		Atomizer_ConfigureConverters(1, 0);
		PWM_SET_CMR(PWM0, ATOMIZER_PWMCH_BUCK, Atomizer_curCmr);
		Atomizer_curState = POWERON_BUCK;
	}
	else {
		Atomizer_curState = POWEROFF;
		Atomizer_ConfigureConverters(0, 0);
	}
}

uint8_t Atomizer_IsOn() {
	return Atomizer_curState != POWEROFF;
}

Atomizer_Error_t Atomizer_GetError() {
	return Atomizer_error;
}

/**
 * Samples the atomizer info.
 * This is an internal function.
 *
 * @param targetVolts Target volts for sampling (if not firing).
 * @param voltage     Pointer to store voltage (0 on error).
 * @param current     Pointer to store current (0 on error).
 * @param resistance  Pointer to store resistance (0 on error).
 */
static void Atomizer_Sample(uint16_t targetVolts, uint16_t *voltage, uint16_t *current, uint16_t *resistance) {
	uint32_t vSum, iSum, adcRes;
	uint16_t savedTargetVolts;
	uint8_t i;

	if(Atomizer_curState == POWEROFF) {
		// Power on atomizer for measurement
		savedTargetVolts = Atomizer_targetVolts;
		Atomizer_targetVolts = targetVolts;
		Atomizer_Control(1);
		Timer_DelayMs(2);

		// Sample and average V and I
		vSum = iSum = 0;
		for(i = 0; i < 50; i++) {
			Timer_DelayUs(10);
			vSum += ADC_Read(ADC_MODULE_VATM);
			Timer_DelayUs(10);
			iSum += ADC_Read(ADC_MODULE_CURS);
		}

		// Power off and restore target voltage
		Atomizer_Control(0);
		Atomizer_targetVolts = savedTargetVolts;

		*voltage = 0;
		*current = 0;
	}
	else {
		// Use cached V and I
		vSum = ADC_GetCachedResult(ADC_MODULE_VATM);
		iSum = ADC_GetCachedResult(ADC_MODULE_CURS);

		*voltage = ATOMIZER_ADC_VOLTAGE(vSum);
		*current = ATOMIZER_ADC_CURRENT(iSum);
	}

	if(Atomizer_error != OK) {
		*resistance = 0;
		return;
	}

	// The feedback cycle has more relaxed limits,
	// so we re-check the resistance.
	adcRes = ATOMIZER_ADC_RESISTANCE(vSum, iSum);
	if(adcRes < 50) {
		Atomizer_error = SHORT;
	}
	else if(adcRes > 3500) {
		Atomizer_error = OPEN;
	}
	else {
		*resistance = adcRes;
		if(Atomizer_curState == POWEROFF) {
			Atomizer_baseRes = adcRes;
		}
		return;
	}

	Atomizer_Control(0);
	Atomizer_baseRes = 0;
	*voltage = 0;
	*current = 0;
	*resistance = 0;

	return;
}

void Atomizer_ReadInfo(Atomizer_Info_t *info) {
	uint16_t targetVolts, savedTargetVolts;

	// Try to use saved resistance
	if(Atomizer_curState == POWEROFF && Atomizer_baseRes != 0) {
		if(Atomizer_timerCount >= 1000) {
			// Every 200ms, let's check if the atomizer is still OK
			Atomizer_timerCount = 0;
			savedTargetVolts = Atomizer_targetVolts;
			Atomizer_targetVolts = 30;
			Atomizer_Control(1);
			Timer_DelayMs(2);
			Atomizer_Control(0);
			Atomizer_targetVolts = savedTargetVolts;
		}

		info->voltage = 0;
		info->current = 0;
		info->resistance = Atomizer_baseRes;
		return;
	}

	// Sample info (@ 1.00V if not firing)
	Atomizer_Sample(100, &info->voltage, &info->current, &info->resistance);
	if(Atomizer_error != OK) {
		return;
	}

	if(Atomizer_curState == POWEROFF) {
		// Calculate test voltage for 1.5% target error
		targetVolts = (info->resistance * 49L / 30L + 744L) / 10L;
		if(targetVolts <= 100) {
			return;
		}

		Atomizer_Sample(targetVolts, &info->voltage, &info->current, &info->resistance);
	}
}

uint8_t Atomizer_ReadBoardTemp() {
	uint8_t i;
	uint16_t thermAdcSum, lowerBound, higherBound;
	uint32_t thermRes;

	// Sample and average thermistor resistance
	// A power-of-2 sample count is better for
	// optimization. 16 is also the biggest number
	// of samples that will fit in a uint16_t.
	thermAdcSum = 0;
	for(i = 0; i < 16; i++) {
		thermAdcSum += ADC_Read(ADC_MODULE_TEMP);
	}
	thermRes = ATOMIZER_ADC_THERMRES(thermAdcSum / 16);

	// Handle corner cases
	if(thermRes >= Atomizer_boardTempTable[0]) {
		return 0;
	}
	else if(thermRes <= Atomizer_boardTempTable[20]) {
		return 99;
	}

	// Look up lower resistance bound (higher temperature bound)
	for(i = 1; i < 20 && thermRes < Atomizer_boardTempTable[i]; i++);

	// Interpolate
	lowerBound = Atomizer_boardTempTable[i];
	higherBound = Atomizer_boardTempTable[i - 1];
	return 5 * (i - 1) + (thermRes - lowerBound) * 5 / (higherBound - lowerBound);
}
