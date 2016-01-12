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
 * Copyright (C) 2015 ReservedField
 * Copyright (C) 2016 Jussi Timperi
 */

#ifndef EVICSDK_DATAFLASH_H
#define EVICSDK_DATAFLASH_H


/**
 * Status flag.
 */
#define DATAFLASH_STATUS_FLIP 4 // Display flipped.

/**
 * This structure contains the dataflash information.
 */
typedef struct {
	/**
	 * Hardware version.
	 */
	uint8_t hwVersion;
	/**
	 * Status flag.
	 */
	uint32_t status;
} Dataflash_Info_t;

/**
 * Global dataflash information.
 */
extern Dataflash_Info_t Dataflash_info;

/**
 * Reads the dataflash and initializes the info structure.
 * System control registers must be unlocked.
 */
void Dataflash_Init();

#endif
