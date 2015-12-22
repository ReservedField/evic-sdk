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
 * Copyright (C) 2015 Jussi Timperi
 */

#ifndef EVICSDK_DISPLAY_SSD_H
#define EVICSDK_DISPLAY_SSD_H

/**
 * Commands shared by the SSDxxxx display controllers.
 */
#define SSD_SET_CONTRAST_LEVEL  0x81
#define SSD_SET_MULTIPLEX_RATIO 0xA8
#define SSD_DISPLAY_OFF         0xAE
#define SSD_DISPLAY_ON          0xAF

/**
 * Reset is at PA.0
 */
#define DISPLAY_SSD_RESET PA0

/**
 * D/C# is at PE10
 */
#define DISPLAY_SSD_DC    PE10

#endif
