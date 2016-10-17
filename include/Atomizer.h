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

#ifndef EVICSDK_ATOMIZER_H
#define EVICSDK_ATOMIZER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Minimum output voltage, in mV.
 */
#define ATOMIZER_VOLTAGE_MIN 500
/**
 * Maximum output voltage, in mV.
 */
#define ATOMIZER_VOLTAGE_MAX 9000
/**
 * Maximum output current, in mA.
 */
#define ATOMIZER_CURRENT_MAX 25000
/**
 * Minimum output power, in mW.
 */
#define ATOMIZER_POWER_MIN 1000
/**
 * Maximum output power, in mW.
 */
#define ATOMIZER_POWER_MAX 75000
/**
 * Minimum resistance, in mOhm.
 */
#define ATOMIZER_RESISTANCE_MIN 50
/**
 * Maximum resistance, in mOhm.
 */
#define ATOMIZER_RESISTANCE_MAX 3500

/**
 * Maximum output voltage, in millivolts.
 * Deprecated: use ATOMIZER_VOLTAGE_MAX.
 */
#define ATOMIZER_MAX_VOLTS ATOMIZER_VOLTAGE_MAX

/**
 * Structure to hold atomizer info.
 */
typedef struct {
	/**
	 * Output voltage in mV, measured at the atomizer.
	 */
	uint16_t voltage;
	/**
	 * Atomizer resistance, in mOhm.
	 * If this is 0, an error occurred.
	 * Check the atomizer error code.
	 */
	uint16_t resistance;
	/**
	 * Output current in mA, measured at the atomizer.
	 */
	uint16_t current;
	/**
	 * Atomizer resistance when cold, in mOhm.
	 * If this is 0, an error occurred.
	 * Check the atomizer error code.
	 */
	uint16_t baseResistance;
	/**
	 * Temperature at which baseResistance is
	 * measured, in °C. Only valid if baseResistance
	 * is not zero.
	 */
	uint8_t baseTemperature;
} Atomizer_Info_t;

/**
 * Atomizer error states.
 */
typedef enum {
	/**
	 * No error.
	 */
	OK,
	/**
	 * Atomizer is shorted.
	 */
	SHORT,
	/**
	 * Atomizer not present or large resistance.
	 */
	OPEN,
	/**
	 * Weak battery.
	 */
	WEAK_BATT,
	/**
	 * Board is too hot.
	 */
	OVER_TEMP
} Atomizer_Error_t;

/**
 * Function pointer type for atomizer base update callbacks.
 * This callback will be invoked when base resistance and/or
 * temperature are going to be updated. The callback is executed
 * in user context, so it doesn't have to be fast and it can
 * block. In that case, it will block inside your most recent
 * call to Atomizer_ReadInfo().
 *
 * @param oldRes  Old base resistance. Only valid if not zero.
 *                If zero, newRes is the first stable measure for this
 *                atomizer. Ignoring a first measure causes the measure
 *                to be repeated until it is accepted.
 * @param oldTemp Old base temperature. Only valid if oldRes is not zero.
 * @param newRes  Pointer to proposed new resistance. You can modify
 *                its value to change the updated base resistance.
 * @param newTemp Pointer to proposed new temperature. You can modify
 *                its value to change the updated base temperature.
 *
 * @return True to update, false to ignore the new values.
 */
typedef uint8_t (*Atomizer_BaseUpdateCallback_t)(uint16_t oldRes, uint8_t oldTemp, uint16_t *newRes, uint8_t *newTemp);

/**
 * Function pointer type for atomizer error callbacks.
 * This callback will be invoked both from normal context and
 * interrupt context, so it should be as fast as possible.
 * You'll typically want to just set a flag and return, and
 * then act on that flag from you main application loop.
 *
 * @param error Latest atomizer error.
 */
typedef void (*Atomizer_ErrorCallback_t)(Atomizer_Error_t error);

/**
 * Initializes the atomizer library.
 * System control registers must be unlocked.
 */
void Atomizer_Init();

/**
 * Sets the atomizer output voltage.
 * This can always be called, the atomizer doesn't need
 * to be powered on.
 * While the voltage is expressed in mV, the actual precision
 * is 10mV. Millivolt units are used for uniformity between SDK
 * functions. Voltage will be rounded to the nearest 10mV.
 *
 * @param volts Output voltage, in millivolts.
 */
void Atomizer_SetOutputVoltage(uint16_t volts);

/**
 * Powers the atomizer on or off.
 *
 * @param powerOn True to power the atomizer on, false to power it off.
 */
void Atomizer_Control(uint8_t powerOn);

/**
 * Checks whether the atomizer is powered on.
 *
 * @return True if the atomizer is powered on, false otherwise.
 */
uint8_t Atomizer_IsOn();

/**
 * Gets the latest atomizer error.
 *
 * @return Atomizer error.
 */
Atomizer_Error_t Atomizer_GetError();

/**
 * Reads the atomizer info.
 * This may power up the atomizer for resistance measuring,
 * depending on the situation. Refresh rate is internally
 * limited, so you can call this as often as you like.
 *
 * @param info Info structure to fill.
 */
void Atomizer_ReadInfo(Atomizer_Info_t *info);

/**
 * Sets a callback that will be invoked when base resistance
 * and/or temperature are going to be updated.
 * If a callback was previously set, it will be replaced.
 *
 * @param callbackPtr Callback function pointer, or NULL to disable.
 */
void Atomizer_SetBaseUpdateCallback(Atomizer_BaseUpdateCallback_t callbackPtr);

/**
 * Schedules a forced atomizer measurement.
 * The current resistance measurement will be kept, but successive
 * calls to Atomizer_ReadInfo will perform the measurement again.
 * The previous measurement will be replaced only once the new one
 * is complete. If a base update callback is set, it will be called
 * once the new measuremet is complete.
 * NOTE: use this *very* sparingly and carefully. Fast, consecutive
 * measurements can heat up the atomizer considerably.
 */
void Atomizer_ForceMeasure();

/**
 * Enable or disable atomizer locking on error.
 * The lock affects Atomizer_Control and Atomizer_ReadInfo.
 *
 * @param enable True to enable, false to disable.
 */
void Atomizer_SetErrorLock(uint8_t enable);

/**
 * If error locking is enabled, unlocks the atomizer after an error
 * locked it.
 */
void Atomizer_Unlock();

/**
 * Sets a callback that will be invoked when the atomizer encounters
 * an error.
 *
 * @param callbackPtr Callback function pointer, or NULL to disable.
 */
void Atomizer_SetErrorCallback(Atomizer_ErrorCallback_t callbackPtr);

/**
 * Reads the DC/DC converter temperature.
 *
 * @return DC/DC converter temperature, in °C.
 *         Range is 0 - 99 °C.
 */
uint8_t Atomizer_ReadBoardTemp();

#ifdef __cplusplus
}
#endif

#endif
