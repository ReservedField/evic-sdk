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

#include <M451Series.h>
#include <System.h>

void Sys_Sleep() {
	// In power down mode the only active clocks are LIRC and LXT.
	// All our pheriperials are clocked from HXT or PLL, so we
	// don't have to worry about them.
	// Button debounce is clocked from LIRC so it will work.
	SYS_UnlockReg();
	CLK_PowerDown();
	SYS_LockReg();
}
