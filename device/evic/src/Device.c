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

#include <Device.h>
#include <Display.h>
#include <SysInfo.h>

Display_Type_t Device_GetDisplayType() {
	switch(gSysInfo.hwVersion) {
		case 102:
		case 103:
		case 106:
		case 108:
		case 109:
		case 111:
			return DISPLAY_SSD1327;
		default:
			return DISPLAY_SSD1306;
	}
}

uint8_t Device_GetAtomizerShunt() {
	switch(gSysInfo.hwVersion) {
		case 101:
		case 108:
			return 125;
		case 103:
		case 104:
		case 105:
		case 106:
			return 110;
		case 107:
		case 109:
			return 120;
		case 110:
		case 111:
			return 105;
		default:
			return 115;
	}
}
