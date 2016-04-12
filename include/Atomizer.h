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

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Maximum output voltage, in millivolts.
 */
#define ATOMIZER_MAX_VOLTS 9000

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
