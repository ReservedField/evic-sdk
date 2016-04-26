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
 * Copyright (C) 2016 Jussi Timperi
 */

#ifndef EVICSDK_SYSINFO_H
#define EVICSDK_SYSINFO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Boot flag: boot from APROM.
 */
#define SYSINFO_BOOTFLAG_APROM 0x00
/**
 * Boot flag: boot from LDROM.
 */
#define SYSINFO_BOOTFLAG_LDROM 0x01

/**
 * Structure to hold system information.
 */
typedef struct {
	/**< Hardware version. */
	uint8_t hwVersion;
	/**< True if the display is flipped. */
	uint8_t displayFlip;
	/**< Boot flag. One of SYSINFO_BOOTFLAG_*. */
	uint8_t bootFlag;
} SysInfo_Info_t;

/**
 * Global system information.
 */
extern SysInfo_Info_t gSysInfo;

/**
 * Initializes the system information.
 * Additionally, it defaults boot to APROM.
 * System control registers must be unlocked.
 */
void SysInfo_Init();

#ifdef __cplusplus
}
#endif

#endif