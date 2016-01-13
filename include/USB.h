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

#ifndef EVICSDK_USB_H
#define EVICSDK_USB_H

#include <M451Series.h>

/**
 * USB setup packet structure.
 */
typedef struct {
	/** Characteristics of request. */
	uint8_t bmRequestType;
	/** Specific request. */
	uint8_t bRequest;
	/** Request-specific value. */
	uint16_t wValue;
	/** Request-specific index/offset. */
	uint16_t wIndex;
	/** Number of bytes to transfer. */
	uint16_t wLength;
} USB_SetupPacket_t;

#endif