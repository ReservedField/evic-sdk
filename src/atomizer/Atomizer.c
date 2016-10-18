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

#include <string.h>
#include <M451Series.h>
#include <Atomizer.h>
#include <ADC.h>
#include <TimerUtils.h>
#include <SysInfo.h>
#include <Battery.h>
#include <Thread.h>

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

/* Feedback loop frequency (Hz) */
#define ATOMIZER_LOOP_FREQ 10000

/* Warmup timer: 10 feedback iterations */
#define ATOMIZER_TMRCNT_WARMUP  10
/* Refresh timer: 200ms */
#define ATOMIZER_TMRCNT_REFRESH (200 * ATOMIZER_LOOP_FREQ / 1000)

/* Median filter window size (must be odd) */
#define ATOMIZER_MEDIANFILTER_WINDOW 5

/* Macros to convert ADC readings to absolute values */
// Read voltage is x * ADC_VREF / ADC_DENOMINATOR.
// This is 3/13 of actual voltage, so we multiply by 13/3.
// Result is in 10mV units.
// Maximum result size: 11 bits.
#define ATOMIZER_ADC_VOLTAGE(x) ((x) * (13L * ADC_VREF) / (30L * ADC_DENOMINATOR))
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
// 1000 * ATOMIZER_ADC_VOLTAGE(voltsX) * 10 / ATOMIZER_ADC_CURRENT(currX) simplifies to this expression.
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
// Battery is weak when < 2.8V under load, i.e. ADC value < 2240.
#define ATOMIZER_ADC_WEAKBATT(x) ((x) < 2240)
// Board temperature limit is 70°C.
// Simplified from: ATOMIZER_ADC_THERMRES(x) <= Atomizer_boardTempTable[14]
#define ATOMIZER_ADC_OVERTEMP(x) (41L * (x) <= 16480L)

/* Macros to convert absolute values to ADC readings */
// Simplified expression: x = I * Atomizer_shuntRes * ADC_DENOMINATOR / ADC_VREF / 1000
// See ATOMIZER_ADC_CURRENT for more details on the expression.
// To avoid overflows in the nominator, ADC_VREF and ADC_DENOMINATOR are hardcoded.
// Maximum result size (for 16-bit input): 14 bits.
#define ATOMIZER_ADCINV_CURRENT(I) ((I) * 8L * Atomizer_shuntRes / 5000L)

// This assumes a battery internal resistance of 10mOhm (which is pretty low).
// It takes the target output voltage in 10mV units, the atomizer resistance in mOhm and
// the battery voltage in mV. The battery is weak if it's under 3.1V, or if it's expected
// to sag below 2.8V under load (only checked if res != 0).
#define ATOMIZER_PREDICT_WEAKBATT(targetVolts, res, battVolts) ((battVolts) < 3100 || \
	((res) != 0 && (battVolts) - (targetVolts) * 100L / (res) < 2800))

// Timer flags
#define ATOMIZER_TMRFLAG_WARMUP (1 << 0)
#define ATOMIZER_TMRFLAG_REFRESH (1 << 1)
#define ATOMIZER_TIMER_WARMUP_RESET() do { \
	uint32_t primask = Thread_IrqDisable(); \
	Atomizer_timerCountWarmup = ATOMIZER_TMRCNT_WARMUP + 1; \
	Atomizer_timerFlag &= ~ATOMIZER_TMRFLAG_WARMUP; \
	Thread_IrqRestore(primask); } while(0)
#define ATOMIZER_TIMER_REFRESH_RESET() do { \
	uint32_t primask = Thread_IrqDisable(); \
	Atomizer_timerCountRefresh = ATOMIZER_TMRCNT_REFRESH + 1; \
	Atomizer_timerFlag &= ~ATOMIZER_TMRFLAG_REFRESH; \
	Thread_IrqRestore(primask); } while(0)

// Busy wait for warmup or error
#define ATOMIZER_WAIT_WARMUP() do {} while(!(Atomizer_timerFlag & ATOMIZER_TMRFLAG_WARMUP) && Atomizer_error == OK)

// Updates the ADC cache, blocking if block is true
#define ATOMIZER_ADC_UPDATECACHE(block) do { \
	ADC_UpdateCache((uint8_t []) { \
		ADC_MODULE_VATM, ADC_MODULE_CURS, \
		ADC_MODULE_VBAT, ADC_MODULE_TEMP \
	}, 4, block); } while(0)

// True when abs(a - b) > bound
// Works for unsigned types
#define ATOMIZER_DIFF_NOT_BOUND(a, b, bound) (((a) < (b) && (b) - (a) > (bound)) || ((a) > (b) && (a) - (b) > (bound)))

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
 * Struct to accumulate ADC data.
 */
typedef struct {
	/**
	 * Voltage accumulator (ADC).
	 */
	uint32_t voltage;
	/**
	 * Current accumulator (ADC).
	 */
	uint32_t current;
	/**
	 * Resistance accumulator (mOhm).
	 */
	uint32_t resistance;
	/**
	 * Accumulator counter. Counts down to zero.
	 */
	uint8_t count;
} Atomizer_ADCAccumulator_t;

/**
 * Struct to hold context for a median filter.
 */
typedef struct {
	/**
	 * Sample buffer.
	 */
	uint16_t buf[ATOMIZER_MEDIANFILTER_WINDOW];
	/**
	 * Index of the oldest sample in the buffer.
	 */
	uint8_t idx;

} Atomizer_MedianFilterCtx_t;

/**
 * Target voltage, in 10mV units.
 */
static volatile uint16_t Atomizer_targetVolts;

/**
 * Current duty cycle.
 */
static volatile uint16_t Atomizer_curCmr;

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
 * Will be reset to zero on atomizer error.
 */
static volatile uint16_t Atomizer_baseRes;

/**
 * Temporary resistance reading, in mOhm.
 * Zero if not waiting for resistance stabilization.
 * Will be reset to zero on atomizer error.
 */
static volatile uint16_t Atomizer_tempRes;

/**
 * Temperature at which Atomizer_baseRes is measured,
 * in °C. Only valid if Atomizer_baseRes != 0.
 */
static volatile uint8_t Atomizer_baseTemp;

/**
 * True if a measure is in progress.
 */
static volatile uint8_t Atomizer_isMeasuring;

/**
 * True if a measure has been forced.
 * Will be reset to false on atomizer error.
 */
static volatile uint8_t Atomizer_forceMeasure;

/**
 * Target voltage for resistance stabilization, in 10mV units.
 */
static volatile uint16_t Atomizer_tempTargetVolts;

/**
 * Warmup timer counter. One tick per feedback operation.
 * Counts down to zero and sets ATOMIZER_TMRFLAG_WARMUP.
 */
static volatile uint8_t Atomizer_timerCountWarmup;

/**
 * Refresh timer counter. One tick per feedback iteration.
 * Counts down to zero and sets ATOMIZER_TMRFLAG_REFRESH.
 */
static volatile uint16_t Atomizer_timerCountRefresh;

/**
 * Bitwise combination of ATOMIZER_TMRFLAG_*.
 */
static volatile uint8_t Atomizer_timerFlag;

/**
 * Pointer to base update callback.
 * NULL when not used.
 */
static volatile Atomizer_BaseUpdateCallback_t Atomizer_baseUpdateCallbackPtr;

/**
 * True if the atomizer should be locked on any error.
 */
static volatile uint8_t Atomizer_errorLock;

/**
 * True if the atomizer is locked.
 */
static volatile uint8_t Atomizer_isLocked;

/**
 * Pointer to error callback.
 * NULL when not used.
 */
static volatile Atomizer_ErrorCallback_t Atomizer_errorCallbackPtr;

/**
 * Overcurrent threshold (ADC value).
 */
static uint16_t Atomizer_adcOverCurrent;

/**
 * Atomizer mutex.
 */
static Thread_Mutex_t Atomizer_mutex;

/**
 * ADC data.
 */
static volatile Atomizer_ADCAccumulator_t Atomizer_adcAcc;

/**
 * Median filter contexts.
 */
static Atomizer_MedianFilterCtx_t Atomizer_medianFilterCtx[3];
#define ATOMIZER_MEDIANFILTER_VOLTAGE    Atomizer_medianFilterCtx[0]
#define ATOMIZER_MEDIANFILTER_CURRENT    Atomizer_medianFilterCtx[1]
#define ATOMIZER_MEDIANFILTER_RESISTANCE Atomizer_medianFilterCtx[2]

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
 * Performs median filtering.
 * Can be casted to ADC_Filter_t.
 *
 * @param value  New sample.
 * @param ctx    Context to work on.
 *
 * @return New filtered sample.
 */
static uint16_t Atomizer_MedianFilter(uint16_t value, Atomizer_MedianFilterCtx_t *ctx) {
	uint8_t i, j, minIdx;
	uint16_t sortBuf[ATOMIZER_MEDIANFILTER_WINDOW], min;

	// Replace oldest sample with the new one
	ctx->buf[ctx->idx] = value;
	ctx->idx = (ctx->idx + 1) % ATOMIZER_MEDIANFILTER_WINDOW;

	// Selection sort. We only need to sort the first half.
	memcpy(sortBuf, ctx->buf, sizeof(sortBuf));
	for(i = 0; i < (ATOMIZER_MEDIANFILTER_WINDOW + 1) / 2; i++) {
		minIdx = i;
		for(j = i + 1; j < ATOMIZER_MEDIANFILTER_WINDOW; j++) {
			if(sortBuf[j] < sortBuf[minIdx]) {
				minIdx = j;
			}
		}
		if(i != minIdx) {
			min = sortBuf[minIdx];
			sortBuf[minIdx] = sortBuf[i];
			sortBuf[i] = min;
		}
	}

	return sortBuf[ATOMIZER_MEDIANFILTER_WINDOW / 2];
}

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
 * Set the PWM duty cycle BEFORE calling this.
 * This is an internal function.
 *
 * @param enableBuck  True to enable the buck converter, false to disable it.
 * @param enableBoost True to enable the boost converter, false to disable it.
 */
static void Atomizer_ConfigureConverters(uint8_t enableBuck, uint8_t enableBoost) {
	// Set PC.1 and PC.3 low (if off) BEFORE PWM configuration
	if(!enableBuck && !enableBoost) {
		PC1 = 0;
		PC3 = 0;
	}

	// FIRST power off
	if(!enableBuck) {
		Atomizer_ConfigurePWMChannel(ATOMIZER_PWMCH_BUCK, 0);
	}
	if(!enableBoost) {
		Atomizer_ConfigurePWMChannel(ATOMIZER_PWMCH_BOOST, 0);
	}

	// THEN power on
	if(enableBuck) {
		PC3 = 1;
		Atomizer_ConfigurePWMChannel(ATOMIZER_PWMCH_BUCK, 1);
		PC1 = 1;
	}
	if(enableBoost) {
		PC1 = 1;
		Atomizer_ConfigurePWMChannel(ATOMIZER_PWMCH_BOOST, 1);
		PC3 = 1;
	}
}

static void Atomizer_SetError(Atomizer_Error_t);

/**
 * Powers the atomizer on or off.
 * This is an internal function.
 *
 * @param powerOn True to power the atomizer on, false to power it off.
 */
static void Atomizer_ControlUnlocked(uint8_t powerOn) {
	uint8_t i;
	uint16_t battVolts, resSeed;

	if(powerOn && (Atomizer_isLocked || Atomizer_error == SHORT)) {
		// Lock atomizer after short or if locked by error
		return;
	}

	if((!powerOn && Atomizer_curState == POWEROFF) || (powerOn && Atomizer_curState != POWEROFF)) {
		// Nothing to do
		return;
	}

	if(powerOn) {
		// Don't even bother firing if the battery is weak
		battVolts = Battery_GetVoltage();
		if(ATOMIZER_PREDICT_WEAKBATT(Atomizer_targetVolts, Atomizer_baseRes, battVolts)) {
			Atomizer_SetError(WEAK_BATT);
			return;
		}

		// Reset filters used by the feedback loop
		memset(Atomizer_medianFilterCtx, 0, sizeof(Atomizer_medianFilterCtx));
		// Seed resistance filter with ATOMIZER_RESISTANCE_MIN or base resistance
		resSeed = Atomizer_baseRes == 0 ? ATOMIZER_RESISTANCE_MIN : Atomizer_baseRes;
		for(i = 0; i < ATOMIZER_MEDIANFILTER_WINDOW; i++) {
			ATOMIZER_MEDIANFILTER_RESISTANCE.buf[i] = resSeed;
		}

		// Update ADC cache for the first feedback iteration, blocking
		ATOMIZER_ADC_UPDATECACHE(1);

		// Start from buck with duty cycle 20
		Atomizer_error = OK;
		Atomizer_curCmr = 20;
		PWM_SET_CMR(PWM0, ATOMIZER_PWMCH_BUCK, Atomizer_curCmr);
		Atomizer_ConfigureConverters(1, 0);
		ATOMIZER_TIMER_WARMUP_RESET();
		Atomizer_curState = POWERON_BUCK;
	}
	else {
		Atomizer_curState = POWEROFF;
		Atomizer_ConfigureConverters(0, 0);
		ATOMIZER_TIMER_REFRESH_RESET();
	}
}

/**
 * Sets the atomizer error, resetting the approriate
 * atomizer state if needed. If the error is not OK,
 * powers off the atomizer.
 *
 * @param error New error.
 */
static void Atomizer_SetError(Atomizer_Error_t error) {
	if(error != OK) {
		// Shutdown and reset measurement state
		Atomizer_ControlUnlocked(0);
		Atomizer_tempRes = 0;
		Atomizer_forceMeasure = 0;
		Atomizer_isMeasuring = 0;

		if(Atomizer_errorLock) {
			// Lock atomizer
			Atomizer_isLocked = 1;
		}
	}

	if(error == SHORT || error == OPEN) {
		// Reset base resistance
		Atomizer_baseRes = 0;
	}

	Atomizer_error = error;

	if(Atomizer_errorCallbackPtr != NULL) {
		// Invoke error callback
		Atomizer_errorCallbackPtr(error);
	}
}

/**
 * Negative feedback iteration to keep the DC/DC converters stable.
 * Takes parameters as a timer callback.
 * This is an internal function.
 */
static void Atomizer_NegativeFeedback(uint32_t unused) {
	uint16_t adcVoltage, adcCurrent, adcBattery, adcBoardTemp, curVolts;
	uint32_t resistance;
	Atomizer_ConverterState_t nextState;

	if(Atomizer_timerCountRefresh > 0) {
		Atomizer_timerCountRefresh--;
	}
	else {
		Atomizer_timerFlag |= ATOMIZER_TMRFLAG_REFRESH;
	}

	if(Atomizer_curState == POWEROFF) {
		return;
	}

	if(Atomizer_timerCountWarmup > 0) {
		Atomizer_timerCountWarmup--;
	}
	else {
		Atomizer_timerFlag |= ATOMIZER_TMRFLAG_WARMUP;
	}

	// Update ADC cache for next iteration without blocking
	ATOMIZER_ADC_UPDATECACHE(0);

	// Get ADC readings
	adcVoltage = ADC_GetCachedResult(ADC_MODULE_VATM);
	adcCurrent = ADC_GetCachedResult(ADC_MODULE_CURS);
	adcBattery = ADC_GetCachedResult(ADC_MODULE_VBAT);
	adcBoardTemp = ADC_GetCachedResult(ADC_MODULE_TEMP);

	// Critical checks
	if(adcCurrent >= Atomizer_adcOverCurrent) {
		Atomizer_SetError(SHORT);
		return;
	}
	if(ATOMIZER_ADC_WEAKBATT(adcBattery)) {
		Atomizer_SetError(WEAK_BATT);
		return;
	}
	if(ATOMIZER_ADC_OVERTEMP(adcBoardTemp)) {
		Atomizer_SetError(OVER_TEMP);
		return;
	}

	// Calculate resistance (16-bit clamped)
	resistance = ATOMIZER_ADC_RESISTANCE(adcVoltage, adcCurrent);
	if(resistance > 0xFFFF) {
		resistance = 0xFFFF;
	}

	// Don't check resistance unless there's some precision
	if(adcVoltage >= 5 && adcCurrent >= 5) {
		// Filter resistance (filter is pre-seeded)
		resistance = Atomizer_MedianFilter(resistance, &ATOMIZER_MEDIANFILTER_RESISTANCE);

		// Check resistance
		if(resistance < ATOMIZER_RESISTANCE_MIN) {
			Atomizer_SetError(SHORT);
			return;
		}
		if(resistance > ATOMIZER_RESISTANCE_MAX) {
			Atomizer_SetError(OPEN);
			return;
		}
	}

	Atomizer_error = OK;

	// Accumulate ADC data after warmup
	if((Atomizer_timerFlag & ATOMIZER_TMRFLAG_WARMUP) && Atomizer_adcAcc.count > 0) {
		Atomizer_adcAcc.voltage += adcVoltage;
		Atomizer_adcAcc.current += adcCurrent;
		Atomizer_adcAcc.resistance += resistance;
		Atomizer_adcAcc.count--;
	}

	curVolts = ATOMIZER_ADC_VOLTAGE(adcVoltage);
	if(curVolts == Atomizer_targetVolts) {
		// Target reached, nothing to do
		return;
	}

	nextState = Atomizer_curState;

	if(curVolts < Atomizer_targetVolts) {
		if(Atomizer_curState == POWERON_BUCK) {
			if(Atomizer_curCmr == 959) {
				// Reached maximum for buck, switch to boost
				nextState = POWERON_BOOST;
			}
			else {
				// In buck mode, increased duty cycle = increased voltage
				Atomizer_curCmr++;
			}
		}
		else if(Atomizer_curCmr > 160) {
			// Boost duty cycle must be greater than 160
			// In boost mode, decreased duty cycle = increased voltage
			Atomizer_curCmr--;
		}
	}
	else {
		if(Atomizer_curState == POWERON_BOOST) {
			if(Atomizer_curCmr == 959) {
				// Reached minimum for boost, switch to buck
				nextState = POWERON_BUCK;
			}
			else {
				// In boost mode, increased duty cycle = decreased voltage
				Atomizer_curCmr++;
			}
		}
		else {
			if(Atomizer_curCmr <= 20) {
				// Buck duty cycles below 20 are forced to zero
				Atomizer_curCmr = 0;
			}
			else {
				// In buck mode, decreased duty cycle = decreased voltage
				Atomizer_curCmr--;
			}
		}
	}

	// Set new duty cycle
	// nextState is used here because:
	// If state didn't change, it'll be == Atomizer_curState
	// If state changed, duty cycle on the new channel must be set before reconfiguration
	PWM_SET_CMR(PWM0, nextState == POWERON_BUCK ? ATOMIZER_PWMCH_BUCK : ATOMIZER_PWMCH_BOOST, Atomizer_curCmr);

	// If needed, update state and reconfigure converters
	if(nextState != Atomizer_curState) {
		Atomizer_curState = nextState;
		Atomizer_ConfigureConverters(nextState == POWERON_BUCK, nextState == POWERON_BOOST);
	}
}

void Atomizer_Init() {
	// Select shunt value based on hardware version
	switch(gSysInfo.hwVersion) {
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

	// Calculate overcurrent threshold
	Atomizer_adcOverCurrent = ATOMIZER_ADCINV_CURRENT(ATOMIZER_CURRENT_MAX);
	if(Atomizer_adcOverCurrent > ADC_DENOMINATOR - 1) {
		Atomizer_adcOverCurrent = ADC_DENOMINATOR - 1;
	}

	// Setup ADC median filtering
	ADC_SetFilter(ADC_MODULE_VATM, (ADC_Filter_t) Atomizer_MedianFilter,
		(uint32_t) &ATOMIZER_MEDIANFILTER_VOLTAGE);
	ADC_SetFilter(ADC_MODULE_CURS, (ADC_Filter_t) Atomizer_MedianFilter,
		(uint32_t) &ATOMIZER_MEDIANFILTER_CURRENT);

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

	// Create our Big Atomizer Lock
	if(Thread_MutexCreate(&Atomizer_mutex) != SUCCESS) {
		// No user code has run yet, the heap is messed up
		asm volatile ("udf");
	}

	// Setup timer for the feedback loop.
	// This function runs during system init, so
	// the user hasn't had time to create timers yet.
	Timer_CreateTimer(ATOMIZER_LOOP_FREQ, 1, Atomizer_NegativeFeedback, 0);
}

void Atomizer_SetOutputVoltage(uint16_t volts) {
	if(volts < ATOMIZER_VOLTAGE_MIN) {
		volts = ATOMIZER_VOLTAGE_MIN;
	}
	else if(volts > ATOMIZER_VOLTAGE_MAX) {
		volts = ATOMIZER_VOLTAGE_MAX;
	}

	Atomizer_targetVolts = (volts + 5) / 10;
}

void Atomizer_Control(uint8_t powerOn) {
	// This is ISR safe.
	// User ISRs won't preempt the feedback loop.
	Thread_CriticalEnter();
	Atomizer_ControlUnlocked(powerOn);
	Thread_CriticalExit();
}

uint8_t Atomizer_IsOn() {
	return Atomizer_curState != POWEROFF;
}

Atomizer_Error_t Atomizer_GetError() {
	// Mask OK code while resistance is stabilizing (on first measure)
	return Atomizer_isMeasuring ? OPEN : Atomizer_error;
}

/**
 * Updates base resistance and temperature, invoking the
 * callback if registered.
 * This is an internal function.
 *
 * @param newRes  New base resistance.
 * @param newTemp New base temperature.
 */
static void Atomizer_BaseUpdate(uint16_t newRes, uint8_t newTemp) {
	if(Atomizer_baseUpdateCallbackPtr != NULL &&
	   !Atomizer_baseUpdateCallbackPtr(Atomizer_baseRes, Atomizer_baseTemp, &newRes, &newTemp)) {
		// Callback refused the update
		return;
	}

	// Update base values (callback can modify them)
	Atomizer_baseRes = newRes;
	Atomizer_baseTemp = newTemp;
}

/**
 * Samples the atomizer info.
 * This is an internal function.
 *
 * @param targetVolts Target volts for sampling (if not firing), in 10mV units.
 *                    If this is zero the atomizer will be assumed to be firing.
 * @param voltage     Pointer to store voltage in mV (optional, can be NULL).
 * @param current     Pointer to store current in mA (optional, can be NULL).
 * @param resistance  Pointer to store resistance in mOhm (optional, can be NULL).
 *
 * @return True on success, false if an atomizer error occurs.
 */
static uint8_t Atomizer_Sample(uint16_t targetVolts, uint16_t *voltage, uint16_t *current, uint16_t *resistance) {
	uint32_t vSum, iSum, res;
	uint16_t savedTargetVolts;
	uint8_t fromPowerOff, count, newTemp;

	// OFF -> ON transistions are assumed to be locked.
	// ON -> OFF transistions are assumed to be locked from userspace
	// and will only happen if an atomizer error occurs.

	fromPowerOff = (targetVolts && Atomizer_curState == POWEROFF);
	count = fromPowerOff ? 50 : 1;

	// Reset accumulators
	Atomizer_adcAcc.count = 0;
	Atomizer_adcAcc.voltage = 0;
	Atomizer_adcAcc.current = 0;
	Atomizer_adcAcc.resistance = 0;
	Atomizer_adcAcc.count = count;

	if(fromPowerOff) {
		// Power on atomizer for measurement
		savedTargetVolts = Atomizer_targetVolts;
		Atomizer_targetVolts = targetVolts;
		Atomizer_ControlUnlocked(1);
	}

	// Wait for accumulation to complete
	while(Atomizer_adcAcc.count > 0 && Atomizer_error == OK);

	if(fromPowerOff) {
		// Power off and restore target voltage
		Atomizer_ControlUnlocked(0);
		Atomizer_targetVolts = savedTargetVolts;
	}

	if(Atomizer_error != OK) {
		goto error;
	}

	// Avoid useless volatile accesses
	vSum = Atomizer_adcAcc.voltage;
	iSum = Atomizer_adcAcc.current;

	// If we take more than one sample, calculate resistance from
	// accumulated voltage and current because it's more precise.
	// If we take only one sample, use the accumulated resistance
	// because it has been filtered.
	res = count > 1 ? ATOMIZER_ADC_RESISTANCE(vSum, iSum) : Atomizer_adcAcc.resistance;

	// The feedback cycle has more relaxed limits,
	// so we re-check the resistance.
	if(res < ATOMIZER_RESISTANCE_MIN) {
		Atomizer_SetError(SHORT);
		goto error;
	}
	if(res > ATOMIZER_RESISTANCE_MAX) {
		Atomizer_SetError(OPEN);
		goto error;
	}

	if(voltage != NULL) {
		*voltage = ATOMIZER_ADC_VOLTAGE(vSum / count) * 10;
	}
	if(current != NULL) {
		*current = ATOMIZER_ADC_CURRENT(iSum / count);
	}
	if(resistance != NULL) {
		// No need to clamp to 16-bit (see checks above)
		*resistance = res;
	}

	// Since TCR is always positive, res < baseRes
	// implies a better resistance reading has been acquired.
	if(res >= 5 && res < Atomizer_baseRes) {
		// Also update base temperature if lower
		newTemp = Atomizer_ReadBoardTemp();
		if(newTemp > Atomizer_baseTemp) {
			newTemp = Atomizer_baseTemp;
		}

		Atomizer_BaseUpdate(res, newTemp);
	}

	return 1;

error:
	Atomizer_adcAcc.count = 0;
	return 0;
}

/**
 * Refreshes the atomizer, taking care to update Atomizer_baseRes.
 * Do NOT call this while firing.
 * This is an internal function.
 */
static void Atomizer_Refresh() {
	uint16_t resistance, targetVolts;

	if(Atomizer_tempRes == 0) {
		Atomizer_isMeasuring = Atomizer_forceMeasure || !Atomizer_baseRes;

		// Use a 300mV test voltage for refresh
		targetVolts = Atomizer_targetVolts;
		Atomizer_targetVolts = 30;
		Atomizer_ControlUnlocked(1);
		ATOMIZER_WAIT_WARMUP();
		Atomizer_ControlUnlocked(0);
		Atomizer_targetVolts = targetVolts;

		if(!Atomizer_isMeasuring) {
			return;
		}

		// The atomizer has just been connected or a measure has just
		// been forced. Start sampling at 1.00V.
		targetVolts = 100;
	}
	else {
		// Calculate test voltage for 1.5% target error.
		targetVolts = (Atomizer_tempRes * 49L / 30L + 744L) / 10L;
	}

	if(!Atomizer_Sample(targetVolts, NULL, NULL, &resistance)) {
		return;
	}

	// Only update baseRes when resistance has stabilized to +/- 5mOhm.
	// This is needed because resistance fluctuates while screwing
	// in the 510 connector.
	if(Atomizer_tempRes == 0 || ATOMIZER_DIFF_NOT_BOUND(resistance, Atomizer_tempRes, 5)) {
		Atomizer_tempRes = resistance;
	}
	else {
		Atomizer_tempRes = 0;
		Atomizer_forceMeasure = 0;
		// If this is the first measure and the update is
		// refused both tempRes and baseRes will be zero
		// and the measure will be repeated.
		Atomizer_BaseUpdate(resistance, Atomizer_ReadBoardTemp());
		if(Atomizer_baseRes == 0) {
			Atomizer_SetError(OPEN);
		}
		Atomizer_isMeasuring = 0;
	}
}

void Atomizer_ReadInfo(Atomizer_Info_t *info) {
	Thread_MutexLock(Atomizer_mutex);

	if(Atomizer_curState == POWEROFF) {
		// Lock atomizer after short or if locked by error
		if(!Atomizer_isLocked && Atomizer_error != SHORT &&
		   (Atomizer_timerFlag & ATOMIZER_TMRFLAG_REFRESH)) {
			ATOMIZER_TIMER_REFRESH_RESET();
			Atomizer_Refresh();
		}

		info->voltage = 0;
		info->current = 0;
		info->resistance = Atomizer_baseRes;
	}
	else {
		if(Atomizer_forceMeasure) {
			// Abort forced measurement on fire
			Atomizer_tempRes = 0;
			Atomizer_forceMeasure = 0;
		}

		if(!Atomizer_Sample(0, &info->voltage, &info->current, &info->resistance)) {
			// Either an atomizer error happened while sampling
			// or an error raced our ON/OFF if.
			info->voltage = info->current = info->resistance = 0;
		}
	}

	info->baseResistance = Atomizer_baseRes;
	info->baseTemperature = Atomizer_baseTemp;

	Thread_MutexUnlock(Atomizer_mutex);
}

void Atomizer_SetBaseUpdateCallback(Atomizer_BaseUpdateCallback_t callbackPtr) {
	Atomizer_baseUpdateCallbackPtr = callbackPtr;
}

void Atomizer_ForceMeasure() {
	Atomizer_forceMeasure = 1;
}

void Atomizer_SetErrorLock(uint8_t enable) {
	Atomizer_errorLock = enable;
}

void Atomizer_Unlock() {
	Atomizer_isLocked = 0;
}

void Atomizer_SetErrorCallback(Atomizer_ErrorCallback_t callbackPtr) {
	Atomizer_errorCallbackPtr = callbackPtr;
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
