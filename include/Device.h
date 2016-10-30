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

#ifndef EVICSDK_DEVICE_H
#define EVICSDK_DEVICE_H

#include <Display.h>

/**
 * Defines:
 * DEVICE_xxx: check concrete header for name.
 * DEVICE_ADC_MODULE_VBAT: battery voltage ADC module.
 */

/**
 * Gets the display controller type for this device.
 *
 * @return Display type.
 */
Display_Type_t Device_GetDisplayType();

/**
 * Gets the atomizer shunt resistance.
 *
 * @return Shunt resistance, in 100ths of a mOhm.
 */
uint8_t Device_GetAtomizerShunt();

// Include concrete header
#include_next <Device.h>

#endif
