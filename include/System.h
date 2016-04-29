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

/**
 * Wakeup source: fire button.
 */
#define SYS_WAKEUP_FIRE    0x01
/**
 * Wakeup source: right button.
 */
#define SYS_WAKEUP_RIGHT   0x02
/**
 * Wakeup source: left button.
 */
#define SYS_WAKEUP_LEFT    0x04
/**
 * Wakeup source: battery presence change.
 * This includes battery insertion/removal and
 * USB charging connect/disconnect.
 */
#define SYS_WAKEUP_BATTERY 0x08

/**
 * Puts the system into power down mode.
 * Will return once the system wakes up.
 */
void Sys_Sleep();

/**
 * Sets the desired wakeup sources.
 *
 * @param source OR combination of SYS_WAKEUP_*.
 */
void Sys_SetWakeupSource(uint8_t source);

/**
 * Gets the source of last wakeup.
 *
 * @return Last wakeup source (SYS_WAKEUP_*), or
 *         zero if no wakeup has ever happened.
 */
uint8_t Sys_GetLastWakeupSource();
