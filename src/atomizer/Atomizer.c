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
#include <Atomizer.h>
#include <ADC.h>
#include <TimerUtils.h>

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
 * Target voltage, in 100ths of a Volt.
 */
static volatile uint16_t Atomizer_targetVolts = 0;

/**
 * Current duty cycle.
 */
static volatile uint16_t Atomizer_curCmr = 10;

/**
 * Current converters state.
 */
static volatile Atomizer_ConverterState_t Atomizer_curState = POWEROFF;

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
 * Takes parameters an an ADC callback.
 * This is an internal function.
 */
static void Atomizer_NegativeFeedback(uint16_t adcValue, uint32_t unused) {
	uint16_t curVolts;

	if(Atomizer_curState == POWEROFF) {
		// Powered off, nothing to do
		return;
	}

	// Calculate current voltage
	curVolts = (adcValue * 1109) >> 12;
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

extern uint8_t ADC_ReadAsyncEx(uint32_t moduleNum, ADC_Callback_t callback, uint32_t callbackData, uint8_t useReserved);

/**
 * Timer callback for DC/DC negative feedback loop.
 * This is an internal function.
 */
static void Atomizer_TimerCallback(uint32_t unused) {
	if(Atomizer_curState == POWEROFF) {
		// Powered off, nothing to do
		return;
	}

	// Schedule conversion (on reserved slot, if needed)
	// We don't care if it fails, we'll call it again
	ADC_ReadAsyncEx(ADC_MODULE_VATM, Atomizer_NegativeFeedback, 0, 1);
}

void Atomizer_Init() {
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
	Atomizer_curCmr = 10;
	Atomizer_curState = POWEROFF;

	// Setup 5kHz timer for negative feedback cycle
	// This function should run during system init, so
	// the user hasn't had time to create timers yet.
	Timer_CreateTimer(5000, 1, Atomizer_TimerCallback, 0);
}

void Atomizer_SetOutputVoltage(uint16_t volts) {
	if(volts > ATOMIZER_MAX_VOLTS) {
		volts = ATOMIZER_MAX_VOLTS;
	}

	Atomizer_targetVolts = volts;
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